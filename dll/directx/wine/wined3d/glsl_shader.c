/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2007-2009, 2013 Stefan Dösinger for CodeWeavers
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

/*
 * D3D shader asm has swizzles on source parameters, and write masks for
 * destination parameters. GLSL uses swizzles for both. The result of this is
 * that for example "mov dst.xw, src.zyxw" becomes "dst.xw = src.zw" in GLSL.
 * Ie, to generate a proper GLSL source swizzle, we need to take the D3D write
 * mask for the destination parameter into account.
 */

#include "config.h"
#include "wine/port.h"

#include <limits.h>
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINED3D_GLSL_SAMPLE_PROJECTED   0x01
#define WINED3D_GLSL_SAMPLE_LOD         0x02
#define WINED3D_GLSL_SAMPLE_GRAD        0x04
#define WINED3D_GLSL_SAMPLE_LOAD        0x08
#define WINED3D_GLSL_SAMPLE_OFFSET      0x10

static const struct
{
    unsigned int coord_size;
    unsigned int resinfo_size;
    const char *type_part;
}
resource_type_info[] =
{
    {0, 0, ""},        /* WINED3D_SHADER_RESOURCE_NONE */
    {1, 1, "Buffer"},  /* WINED3D_SHADER_RESOURCE_BUFFER */
    {1, 1, "1D"},      /* WINED3D_SHADER_RESOURCE_TEXTURE_1D */
    {2, 2, "2D"},      /* WINED3D_SHADER_RESOURCE_TEXTURE_2D */
    {2, 2, ""},        /* WINED3D_SHADER_RESOURCE_TEXTURE_2DMS */
    {3, 3, "3D"},      /* WINED3D_SHADER_RESOURCE_TEXTURE_3D */
    {3, 2, "Cube"},    /* WINED3D_SHADER_RESOURCE_TEXTURE_CUBE */
    {2, 2, ""},        /* WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY */
    {3, 3, "2DArray"}, /* WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY */
    {3, 3, ""},        /* WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY */
    {4, 3, ""},        /* WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY */
};

static const struct
{
    enum wined3d_data_type data_type;
    const char *glsl_scalar_type;
    const char *glsl_vector_type;
}
component_type_info[] =
{
    {WINED3D_DATA_FLOAT, "float", "vec"},  /* WINED3D_TYPE_UNKNOWN */
    {WINED3D_DATA_UINT,  "uint",  "uvec"}, /* WINED3D_TYPE_UINT */
    {WINED3D_DATA_INT,   "int",   "ivec"}, /* WINED3D_TYPE_INT */
    {WINED3D_DATA_FLOAT, "float", "vec"},  /* WINED3D_TYPE_FLOAT */
};

struct glsl_dst_param
{
    char reg_name[150];
    char mask_str[6];
};

struct glsl_src_param
{
    char param_str[200];
};

struct glsl_sample_function
{
    struct wined3d_string_buffer *name;
    unsigned int coord_mask;
    unsigned int deriv_mask;
    enum wined3d_data_type data_type;
    BOOL output_single_component;
    unsigned int offset_size;
};

enum heap_node_op
{
    HEAP_NODE_TRAVERSE_LEFT,
    HEAP_NODE_TRAVERSE_RIGHT,
    HEAP_NODE_POP,
};

struct constant_entry
{
    unsigned int idx;
    unsigned int version;
};

struct constant_heap
{
    struct constant_entry *entries;
    BOOL *contained;
    unsigned int *positions;
    unsigned int size;
};

/* GLSL shader private data */
struct shader_glsl_priv
{
    struct wined3d_string_buffer shader_buffer;
    struct wined3d_string_buffer_list string_buffers;
    struct wine_rb_tree program_lookup;
    struct constant_heap vconst_heap;
    struct constant_heap pconst_heap;
    unsigned char *stack;
    UINT next_constant_version;

    BOOL consts_ubo;
    GLuint ubo_vs_c;
    BOOL prev_device_swvp;
    struct wined3d_vec4 vs_c_buffer[WINED3D_MAX_VS_CONSTS_F_SWVP];
    unsigned int max_vs_consts_f;

    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    struct wine_rb_tree ffp_vertex_shaders;
    struct wine_rb_tree ffp_fragment_shaders;
    BOOL ffp_proj_control;
    BOOL legacy_lighting;

    GLuint ubo_modelview;
    struct wined3d_matrix *modelview_buffer;
};

struct glsl_vs_program
{
    struct list shader_entry;
    GLuint id;
    GLenum vertex_color_clamp;
    GLint uniform_f_locations[WINED3D_MAX_VS_CONSTS_F_SWVP];
    GLint uniform_i_locations[WINED3D_MAX_CONSTS_I];
    GLint uniform_b_locations[WINED3D_MAX_CONSTS_B];
    GLint pos_fixup_location;
    GLint base_vertex_id_location;

    GLint modelview_matrix_location[MAX_VERTEX_BLENDS];
    GLint modelview_block_index;
    GLint projection_matrix_location;
    GLint normal_matrix_location;
    GLint texture_matrix_location[WINED3D_MAX_TEXTURES];
    GLint material_ambient_location;
    GLint material_diffuse_location;
    GLint material_specular_location;
    GLint material_emissive_location;
    GLint material_shininess_location;
    GLint light_ambient_location;
    struct
    {
        GLint diffuse;
        GLint specular;
        GLint ambient;
        GLint position;
        GLint direction;
        GLint range;
        GLint falloff;
        GLint c_att;
        GLint l_att;
        GLint q_att;
        GLint cos_htheta;
        GLint cos_hphi;
    } light_location[WINED3D_MAX_ACTIVE_LIGHTS];
    GLint pointsize_location;
    GLint pointsize_min_location;
    GLint pointsize_max_location;
    GLint pointsize_c_att_location;
    GLint pointsize_l_att_location;
    GLint pointsize_q_att_location;
    GLint clip_planes_location;
    GLint vs_c_block_index;
};

struct glsl_hs_program
{
    struct list shader_entry;
    GLuint id;
};

struct glsl_ds_program
{
    struct list shader_entry;
    GLuint id;

    GLint pos_fixup_location;
};

struct glsl_gs_program
{
    struct list shader_entry;
    GLuint id;

    GLint pos_fixup_location;
};

struct glsl_ps_program
{
    struct list shader_entry;
    GLuint id;
    GLint uniform_f_locations[WINED3D_MAX_PS_CONSTS_F];
    GLint uniform_i_locations[WINED3D_MAX_CONSTS_I];
    GLint uniform_b_locations[WINED3D_MAX_CONSTS_B];
    GLint bumpenv_mat_location[WINED3D_MAX_TEXTURES];
    GLint bumpenv_lum_scale_location[WINED3D_MAX_TEXTURES];
    GLint bumpenv_lum_offset_location[WINED3D_MAX_TEXTURES];
    GLint tss_constant_location[WINED3D_MAX_TEXTURES];
    GLint tex_factor_location;
    GLint specular_enable_location;
    GLint fog_color_location;
    GLint fog_density_location;
    GLint fog_end_location;
    GLint fog_scale_location;
    GLint alpha_test_ref_location;
    GLint ycorrection_location;
    GLint np2_fixup_location;
    GLint color_key_location;
    const struct ps_np2fixup_info *np2_fixup_info;
};

struct glsl_cs_program
{
    struct list shader_entry;
    GLuint id;
};

/* Struct to maintain data about a linked GLSL program */
struct glsl_shader_prog_link
{
    struct wine_rb_entry program_lookup_entry;
    struct glsl_vs_program vs;
    struct glsl_hs_program hs;
    struct glsl_ds_program ds;
    struct glsl_gs_program gs;
    struct glsl_ps_program ps;
    struct glsl_cs_program cs;
    GLuint id;
    DWORD constant_update_mask;
    unsigned int constant_version;
    DWORD shader_controlled_clip_distances : 1;
    DWORD clip_distance_mask : 8; /* WINED3D_MAX_CLIP_DISTANCES, 8 */
    DWORD padding : 23;
};

struct glsl_program_key
{
    GLuint vs_id;
    GLuint hs_id;
    GLuint ds_id;
    GLuint gs_id;
    GLuint ps_id;
    GLuint cs_id;
};

struct shader_glsl_ctx_priv {
    const struct vs_compile_args    *cur_vs_args;
    const struct ds_compile_args    *cur_ds_args;
    const struct ps_compile_args    *cur_ps_args;
    struct ps_np2fixup_info         *cur_np2fixup_info;
    struct wined3d_string_buffer_list *string_buffers;
};

struct glsl_context_data
{
    struct glsl_shader_prog_link *glsl_program;
    GLenum vertex_color_clamp;
    BOOL rasterization_disabled;
    BOOL ubo_bound;
};

struct glsl_ps_compiled_shader
{
    struct ps_compile_args          args;
    struct ps_np2fixup_info         np2fixup;
    GLuint                          id;
};

struct glsl_vs_compiled_shader
{
    struct vs_compile_args          args;
    GLuint                          id;
};

struct glsl_hs_compiled_shader
{
    GLuint id;
};

struct glsl_ds_compiled_shader
{
    struct ds_compile_args args;
    GLuint id;
};

struct glsl_gs_compiled_shader
{
    struct gs_compile_args args;
    GLuint id;
};

struct glsl_cs_compiled_shader
{
    GLuint id;
};

struct glsl_shader_private
{
    union
    {
        struct glsl_vs_compiled_shader *vs;
        struct glsl_hs_compiled_shader *hs;
        struct glsl_ds_compiled_shader *ds;
        struct glsl_gs_compiled_shader *gs;
        struct glsl_ps_compiled_shader *ps;
        struct glsl_cs_compiled_shader *cs;
    } gl_shaders;
    unsigned int num_gl_shaders, shader_array_size;
};

struct glsl_ffp_vertex_shader
{
    struct wined3d_ffp_vs_desc desc;
    GLuint id;
    struct list linked_programs;
};

struct glsl_ffp_fragment_shader
{
    struct ffp_frag_desc entry;
    GLuint id;
    struct list linked_programs;
};

struct glsl_ffp_destroy_ctx
{
    struct shader_glsl_priv *priv;
    const struct wined3d_context_gl *context_gl;
};

static void shader_glsl_generate_shader_epilogue(const struct wined3d_shader_context *ctx);

static const char *debug_gl_shader_type(GLenum type)
{
    switch (type)
    {
#define WINED3D_TO_STR(u) case u: return #u
        WINED3D_TO_STR(GL_VERTEX_SHADER);
        WINED3D_TO_STR(GL_TESS_CONTROL_SHADER);
        WINED3D_TO_STR(GL_TESS_EVALUATION_SHADER);
        WINED3D_TO_STR(GL_GEOMETRY_SHADER);
        WINED3D_TO_STR(GL_FRAGMENT_SHADER);
        WINED3D_TO_STR(GL_COMPUTE_SHADER);
#undef WINED3D_TO_STR
        default:
            return wine_dbg_sprintf("UNKNOWN(%#x)", type);
    }
}

static const char *shader_glsl_get_prefix(enum wined3d_shader_type type)
{
    switch (type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            return "vs";

        case WINED3D_SHADER_TYPE_HULL:
            return "hs";

        case WINED3D_SHADER_TYPE_DOMAIN:
            return "ds";

        case WINED3D_SHADER_TYPE_GEOMETRY:
            return "gs";

        case WINED3D_SHADER_TYPE_PIXEL:
            return "ps";

        case WINED3D_SHADER_TYPE_COMPUTE:
            return "cs";

        default:
            FIXME("Unhandled shader type %#x.\n", type);
            return "unknown";
    }
}

static unsigned int shader_glsl_get_version(const struct wined3d_gl_info *gl_info)
{
    if (gl_info->glsl_version >= MAKEDWORD_VERSION(4, 40))
        return 440;
    else if (gl_info->glsl_version >= MAKEDWORD_VERSION(1, 50))
        return 150;
    else if (gl_info->glsl_version >= MAKEDWORD_VERSION(1, 30))
        return 130;
    else
        return 120;
}

static void shader_glsl_add_version_declaration(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info)
{
    shader_addline(buffer, "#version %u\n", shader_glsl_get_version(gl_info));
}

unsigned int shader_glsl_full_ffp_varyings(const struct wined3d_gl_info *gl_info)
{
    /* On core profile we have to also count diffuse and specular colours and
     * the fog coordinate. */
    return gl_info->limits.glsl_varyings >= (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT]
            ? WINED3D_MAX_TEXTURES * 4 : (WINED3D_MAX_TEXTURES + 2) * 4 + 1);
}

static void shader_glsl_append_imm_vec(struct wined3d_string_buffer *buffer,
        const float *values, unsigned int size, const struct wined3d_gl_info *gl_info)
{
    const int *int_values = (const int *)values;
    unsigned int i;
    char str[17];

    if (!gl_info->supported[ARB_SHADER_BIT_ENCODING])
    {
        if (size > 1)
            shader_addline(buffer, "vec%u(", size);

        for (i = 0; i < size; ++i)
        {
            wined3d_ftoa(values[i], str);
            shader_addline(buffer, i ? ", %s" : "%s", str);
        }

        if (size > 1)
            shader_addline(buffer, ")");

        return;
    }

    shader_addline(buffer, "intBitsToFloat(");
    if (size > 1)
        shader_addline(buffer, "ivec%u(", size);

    for (i = 0; i < size; ++i)
    {
        wined3d_ftoa(values[i], str);
        shader_addline(buffer, i ? ", %#x /* %s */" : "%#x /* %s */", int_values[i], str);
    }

    if (size > 1)
        shader_addline(buffer, ")");
    shader_addline(buffer, ")");
}

static void shader_glsl_append_imm_ivec(struct wined3d_string_buffer *buffer,
        const int *values, unsigned int size)
{
    int i;

    if (!size || size > 4)
    {
        ERR("Invalid vector size %u.\n", size);
        return;
    }

    if (size > 1)
        shader_addline(buffer, "ivec%u(", size);

    for (i = 0; i < size; ++i)
        shader_addline(buffer, i ? ", %#x" : "%#x", values[i]);

    if (size > 1)
        shader_addline(buffer, ")");
}

static const char *get_info_log_line(const char **ptr)
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

/* Context activation is done by the caller. */
void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLuint id, BOOL program)
{
    int length = 0;
    char *log;

    if (!WARN_ON(d3d_shader) && !FIXME_ON(d3d_shader))
        return;

    if (program)
        GL_EXTCALL(glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length));
    else
        GL_EXTCALL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (length > 1)
    {
        const char *ptr, *line;

        log = heap_alloc(length);
        /* The info log is supposed to be zero-terminated, but at least some
         * versions of fglrx don't terminate the string properly. The reported
         * length does include the terminator, so explicitly set it to zero
         * here. */
        log[length - 1] = 0;
        if (program)
            GL_EXTCALL(glGetProgramInfoLog(id, length, NULL, log));
        else
            GL_EXTCALL(glGetShaderInfoLog(id, length, NULL, log));

        ptr = log;
        if (gl_info->quirks & WINED3D_QUIRK_INFO_LOG_SPAM)
        {
            WARN("Info log received from GLSL shader #%u:\n", id);
            while ((line = get_info_log_line(&ptr))) WARN("    %.*s", (int)(ptr - line), line);
        }
        else
        {
            FIXME("Info log received from GLSL shader #%u:\n", id);
            while ((line = get_info_log_line(&ptr))) FIXME("    %.*s", (int)(ptr - line), line);
        }
        heap_free(log);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_compile(const struct wined3d_gl_info *gl_info, GLuint shader, const char *src)
{
    const char *ptr, *line;

    TRACE("Compiling shader object %u.\n", shader);

    if (TRACE_ON(d3d_shader))
    {
        ptr = src;
        while ((line = get_info_log_line(&ptr))) TRACE_(d3d_shader)("    %.*s", (int)(ptr - line), line);
    }

    GL_EXTCALL(glShaderSource(shader, 1, &src, NULL));
    checkGLcall("glShaderSource");
    GL_EXTCALL(glCompileShader(shader));
    checkGLcall("glCompileShader");
    print_glsl_info_log(gl_info, shader, FALSE);
}

/* Context activation is done by the caller. */
static void shader_glsl_dump_program_source(const struct wined3d_gl_info *gl_info, GLuint program)
{
    GLint i, shader_count, source_size = -1;
    GLuint *shaders;
    char *source = NULL;

    GL_EXTCALL(glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count));
    if (!(shaders = heap_calloc(shader_count, sizeof(*shaders))))
    {
        ERR("Failed to allocate shader array memory.\n");
        return;
    }

    GL_EXTCALL(glGetAttachedShaders(program, shader_count, NULL, shaders));
    for (i = 0; i < shader_count; ++i)
    {
        const char *ptr, *line;
        GLint tmp;

        GL_EXTCALL(glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &tmp));

        if (source_size < tmp)
        {
            heap_free(source);

            if (!(source = heap_alloc_zero(tmp)))
            {
                ERR("Failed to allocate %d bytes for shader source.\n", tmp);
                heap_free(shaders);
                return;
            }
            source_size = tmp;
        }

        FIXME("Shader %u:\n", shaders[i]);
        GL_EXTCALL(glGetShaderiv(shaders[i], GL_SHADER_TYPE, &tmp));
        FIXME("    GL_SHADER_TYPE: %s.\n", debug_gl_shader_type(tmp));
        GL_EXTCALL(glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &tmp));
        FIXME("    GL_COMPILE_STATUS: %d.\n", tmp);
        FIXME("\n");

        ptr = source;
        GL_EXTCALL(glGetShaderSource(shaders[i], source_size, NULL, source));
        while ((line = get_info_log_line(&ptr))) FIXME("    %.*s", (int)(ptr - line), line);
        FIXME("\n");
    }

    heap_free(source);
    heap_free(shaders);
}

/* Context activation is done by the caller. */
void shader_glsl_validate_link(const struct wined3d_gl_info *gl_info, GLuint program)
{
    GLint tmp;

    if (!TRACE_ON(d3d_shader) && !FIXME_ON(d3d_shader))
        return;

    GL_EXTCALL(glGetProgramiv(program, GL_LINK_STATUS, &tmp));
    if (!tmp)
    {
        FIXME("Program %u link status invalid.\n", program);
        shader_glsl_dump_program_source(gl_info, program);
    }

    print_glsl_info_log(gl_info, program, TRUE);
}

static BOOL shader_glsl_use_layout_qualifier(const struct wined3d_gl_info *gl_info)
{
    /* Layout qualifiers were introduced in GLSL 1.40. The Nvidia Legacy GPU
     * driver (series 340.xx) doesn't parse layout qualifiers in older GLSL
     * versions. */
    return shader_glsl_get_version(gl_info) >= 140;
}

static BOOL shader_glsl_use_layout_binding_qualifier(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_SHADING_LANGUAGE_420PACK] && shader_glsl_use_layout_qualifier(gl_info);
}

static void shader_glsl_init_uniform_block_bindings(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id,
        const struct wined3d_shader_reg_maps *reg_maps)
{
    const char *prefix = shader_glsl_get_prefix(reg_maps->shader_version.type);
    struct wined3d_string_buffer *name;
    unsigned int i, base, count;
    GLuint block_idx;

    if (shader_glsl_use_layout_binding_qualifier(gl_info))
        return;

    name = string_buffer_get(&priv->string_buffers);
    wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, reg_maps->shader_version.type, &base, &count);
    for (i = 0; i < count; ++i)
    {
        if (!reg_maps->cb_sizes[i])
            continue;

        string_buffer_sprintf(name, "block_%s_cb%u", prefix, i);
        block_idx = GL_EXTCALL(glGetUniformBlockIndex(program_id, name->buffer));
        GL_EXTCALL(glUniformBlockBinding(program_id, block_idx, base + i));
    }
    checkGLcall("glUniformBlockBinding");
    string_buffer_release(&priv->string_buffers, name);
}

/* Context activation is done by the caller. */
static void shader_glsl_load_samplers_range(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, const char *prefix,
        unsigned int base, unsigned int count, const DWORD *tex_unit_map)
{
    struct wined3d_string_buffer *sampler_name = string_buffer_get(&priv->string_buffers);
    unsigned int i, mapped_unit;
    GLint name_loc;

    for (i = 0; i < count; ++i)
    {
        string_buffer_sprintf(sampler_name, "%s_sampler%u", prefix, i);
        name_loc = GL_EXTCALL(glGetUniformLocation(program_id, sampler_name->buffer));
        if (name_loc == -1)
            continue;

        mapped_unit = tex_unit_map ? tex_unit_map[base + i] : base + i;
        if (mapped_unit == WINED3D_UNMAPPED_STAGE || mapped_unit >= gl_info->limits.combined_samplers)
        {
            ERR("Trying to load sampler %s on unsupported unit %u.\n", sampler_name->buffer, mapped_unit);
            continue;
        }

        TRACE("Loading sampler %s on unit %u.\n", sampler_name->buffer, mapped_unit);
        GL_EXTCALL(glUniform1i(name_loc, mapped_unit));
    }
    checkGLcall("Load sampler bindings");
    string_buffer_release(&priv->string_buffers, sampler_name);
}

static unsigned int shader_glsl_map_tex_unit(const struct wined3d_context *context,
        const struct wined3d_shader_version *shader_version, unsigned int sampler_idx)
{
    const struct wined3d_context_gl *context_gl = wined3d_context_gl_const(context);
    const unsigned int *tex_unit_map;
    unsigned int base, count;

    tex_unit_map = wined3d_context_gl_get_tex_unit_mapping(context_gl, shader_version, &base, &count);
    if (sampler_idx >= count)
        return WINED3D_UNMAPPED_STAGE;
    if (!tex_unit_map)
        return base + sampler_idx;
    return tex_unit_map[base + sampler_idx];
}

static void shader_glsl_append_sampler_binding_qualifier(struct wined3d_string_buffer *buffer,
        const struct wined3d_context *context, const struct wined3d_shader_version *shader_version,
        unsigned int sampler_idx)
{
    unsigned int mapped_unit = shader_glsl_map_tex_unit(context, shader_version, sampler_idx);
    if (mapped_unit != WINED3D_UNMAPPED_STAGE)
        shader_addline(buffer, "layout(binding = %u)\n", mapped_unit);
    else
        ERR("Unmapped sampler %u.\n", sampler_idx);
}

/* Context activation is done by the caller. */
static void shader_glsl_load_samplers(const struct wined3d_context *context,
        struct shader_glsl_priv *priv, GLuint program_id, const struct wined3d_shader_reg_maps *reg_maps)
{
    const struct wined3d_context_gl *context_gl = wined3d_context_gl_const(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_shader_version *shader_version;
    const unsigned int *tex_unit_map;
    unsigned int base, count;
    const char *prefix;

    if (shader_glsl_use_layout_binding_qualifier(gl_info))
        return;

    shader_version = reg_maps ? &reg_maps->shader_version : NULL;
    prefix = shader_glsl_get_prefix(shader_version ? shader_version->type : WINED3D_SHADER_TYPE_PIXEL);
    tex_unit_map = wined3d_context_gl_get_tex_unit_mapping(context_gl, shader_version, &base, &count);
    shader_glsl_load_samplers_range(gl_info, priv, program_id, prefix, base, count, tex_unit_map);
}

static void shader_glsl_load_icb(const struct wined3d_gl_info *gl_info, struct shader_glsl_priv *priv,
        GLuint program_id, const struct wined3d_shader_reg_maps *reg_maps)
{
    const struct wined3d_shader_immediate_constant_buffer *icb = reg_maps->icb;

    if (icb)
    {
        struct wined3d_string_buffer *icb_name = string_buffer_get(&priv->string_buffers);
        const char *prefix = shader_glsl_get_prefix(reg_maps->shader_version.type);
        GLint icb_location;

        string_buffer_sprintf(icb_name, "%s_icb", prefix);
        icb_location = GL_EXTCALL(glGetUniformLocation(program_id, icb_name->buffer));
        GL_EXTCALL(glUniform4fv(icb_location, icb->vec4_count, (const GLfloat *)icb->data));
        checkGLcall("Load immediate constant buffer");

        string_buffer_release(&priv->string_buffers, icb_name);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_load_images(const struct wined3d_gl_info *gl_info, struct shader_glsl_priv *priv,
        GLuint program_id, const struct wined3d_shader_reg_maps *reg_maps)
{
    const char *prefix = shader_glsl_get_prefix(reg_maps->shader_version.type);
    struct wined3d_string_buffer *name;
    GLint location;
    unsigned int i;

    if (shader_glsl_use_layout_binding_qualifier(gl_info))
        return;

    name = string_buffer_get(&priv->string_buffers);
    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!reg_maps->uav_resource_info[i].type)
            continue;

        string_buffer_sprintf(name, "%s_image%u", prefix, i);
        location = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        if (location == -1)
            continue;

        TRACE("Loading image %s on unit %u.\n", name->buffer, i);
        GL_EXTCALL(glUniform1i(location, i));
    }
    checkGLcall("Load image bindings");
    string_buffer_release(&priv->string_buffers, name);
}

/* Context activation is done by the caller. */
static void shader_glsl_load_program_resources(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, GLuint program_id, const struct wined3d_shader *shader)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;

    shader_glsl_init_uniform_block_bindings(context_gl->gl_info, priv, program_id, reg_maps);
    shader_glsl_load_icb(context_gl->gl_info, priv, program_id, reg_maps);
    /* Texture unit mapping is set up to be the same each time the shader
     * program is used so we can hardcode the sampler uniform values. */
    shader_glsl_load_samplers(&context_gl->c, priv, program_id, reg_maps);
}

static void append_transform_feedback_varying(const char **varyings, unsigned int *varying_count,
        char **strings, unsigned int *strings_length, struct wined3d_string_buffer *buffer)
{
    if (varyings && *strings)
    {
        char *ptr = *strings;

        varyings[*varying_count] = ptr;

        memcpy(ptr, buffer->buffer, buffer->content_size + 1);
        ptr += buffer->content_size + 1;

        *strings = ptr;
    }

    *strings_length += buffer->content_size + 1;
    ++(*varying_count);
}

static void append_transform_feedback_skip_components(const char **varyings,
        unsigned int *varying_count, char **strings, unsigned int *strings_length,
        struct wined3d_string_buffer *buffer, unsigned int component_count)
{
    unsigned int j;

    for (j = 0; j < component_count / 4; ++j)
    {
        string_buffer_sprintf(buffer, "gl_SkipComponents4");
        append_transform_feedback_varying(varyings, varying_count, strings, strings_length, buffer);
    }
    if (component_count % 4)
    {
        string_buffer_sprintf(buffer, "gl_SkipComponents%u", component_count % 4);
        append_transform_feedback_varying(varyings, varying_count, strings, strings_length, buffer);
    }
}

static BOOL shader_glsl_generate_transform_feedback_varyings(struct wined3d_string_buffer *buffer,
        const char **varyings, unsigned int *varying_count, char *strings, unsigned int *strings_length,
        GLenum buffer_mode, struct wined3d_shader *shader)
{
    const struct wined3d_stream_output_desc *so_desc = &shader->u.gs.so_desc;
    unsigned int buffer_idx, count, length, highest_output_slot, stride;
    unsigned int i, register_idx, component_idx;
    BOOL have_varyings_to_record = FALSE;

    count = length = 0;
    highest_output_slot = 0;
    for (buffer_idx = 0; buffer_idx < WINED3D_MAX_STREAM_OUTPUT_BUFFERS; ++buffer_idx)
    {
        stride = 0;

        for (i = 0; i < so_desc->element_count; ++i)
        {
            const struct wined3d_stream_output_element *e = &so_desc->elements[i];

            highest_output_slot = max(highest_output_slot, e->output_slot);
            if (e->output_slot != buffer_idx)
                continue;

            if (e->stream_idx)
            {
                FIXME("Unhandled stream %u.\n", e->stream_idx);
                continue;
            }

            stride += e->component_count;

            if (!e->semantic_name)
            {
                append_transform_feedback_skip_components(varyings, &count,
                        &strings, &length, buffer, e->component_count);
                continue;
            }

            if (!shader_get_stream_output_register_info(shader, e, &register_idx, &component_idx))
                continue;

            if (component_idx || e->component_count != 4)
            {
                if (so_desc->rasterizer_stream_idx != WINED3D_NO_RASTERIZER_STREAM)
                {
                    FIXME("Unsupported component range %u-%u.\n", component_idx, e->component_count);
                    append_transform_feedback_skip_components(varyings, &count,
                            &strings, &length, buffer, e->component_count);
                    continue;
                }

                string_buffer_sprintf(buffer, "shader_in_out.reg%u_%u_%u",
                        register_idx, component_idx, component_idx + e->component_count - 1);
                append_transform_feedback_varying(varyings, &count, &strings, &length, buffer);
            }
            else
            {
                string_buffer_sprintf(buffer, "shader_in_out.reg%u", register_idx);
                append_transform_feedback_varying(varyings, &count, &strings, &length, buffer);
            }

            have_varyings_to_record = TRUE;
        }

        if (buffer_idx < so_desc->buffer_stride_count
                && stride < so_desc->buffer_strides[buffer_idx] / 4)
        {
            unsigned int component_count = so_desc->buffer_strides[buffer_idx] / 4 - stride;
            append_transform_feedback_skip_components(varyings, &count,
                    &strings, &length, buffer, component_count);
        }

        if (highest_output_slot <= buffer_idx)
            break;

        if (buffer_mode == GL_INTERLEAVED_ATTRIBS)
        {
            string_buffer_sprintf(buffer, "gl_NextBuffer");
            append_transform_feedback_varying(varyings, &count, &strings, &length, buffer);
        }
    }

    if (varying_count)
        *varying_count = count;
    if (strings_length)
        *strings_length = length;

    return have_varyings_to_record;
}

static void shader_glsl_init_transform_feedback(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, GLuint program_id, struct wined3d_shader *shader)
{
    const struct wined3d_stream_output_desc *so_desc = &shader->u.gs.so_desc;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer;
    unsigned int i, count, length;
    const char **varyings;
    char *strings;
    GLenum mode;

    if (!so_desc->element_count)
        return;

    if (gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
    {
        mode = GL_INTERLEAVED_ATTRIBS;
    }
    else
    {
        unsigned int element_count[WINED3D_MAX_STREAM_OUTPUT_BUFFERS] = {0};

        for (i = 0; i < so_desc->element_count; ++i)
        {
            if (!so_desc->elements[i].semantic_name)
            {
                FIXME("ARB_transform_feedback3 is needed for stream output gaps.\n");
                return;
            }
            ++element_count[so_desc->elements[i].output_slot];
        }

        if (element_count[0] == so_desc->element_count)
        {
            mode = GL_INTERLEAVED_ATTRIBS;
        }
        else
        {
            mode = GL_SEPARATE_ATTRIBS;
            for (i = 0; i < ARRAY_SIZE(element_count); ++i)
            {
                if (element_count[i] != 1)
                    break;
            }
            for (; i < ARRAY_SIZE(element_count); ++i)
            {
                if (element_count[i])
                {
                    FIXME("Only single element per buffer is allowed in separate mode.\n");
                    return;
                }
            }
        }
    }

    buffer = string_buffer_get(&priv->string_buffers);

    if (!shader_glsl_generate_transform_feedback_varyings(buffer, NULL, &count, NULL, &length, mode, shader))
    {
        FIXME("No varyings to record, disabling transform feedback.\n");
        shader->u.gs.so_desc.element_count = 0;
        string_buffer_release(&priv->string_buffers, buffer);
        return;
    }

    if (!(varyings = heap_calloc(count, sizeof(*varyings))))
    {
        ERR("Out of memory.\n");
        string_buffer_release(&priv->string_buffers, buffer);
        return;
    }
    if (!(strings = heap_calloc(length, sizeof(*strings))))
    {
        ERR("Out of memory.\n");
        heap_free(varyings);
        string_buffer_release(&priv->string_buffers, buffer);
        return;
    }

    shader_glsl_generate_transform_feedback_varyings(buffer, varyings, NULL, strings, NULL, mode, shader);
    GL_EXTCALL(glTransformFeedbackVaryings(program_id, count, varyings, mode));
    checkGLcall("glTransformFeedbackVaryings");

    heap_free(varyings);
    heap_free(strings);
    string_buffer_release(&priv->string_buffers, buffer);
}

/* Context activation is done by the caller. */
static inline void walk_constant_heap(const struct wined3d_gl_info *gl_info, const struct wined3d_vec4 *constants,
        const GLint *constant_locations, const struct constant_heap *heap, unsigned char *stack, DWORD version)
{
    unsigned int start = ~0U, end = 0;
    int stack_idx = 0;
    unsigned int heap_idx = 1;
    unsigned int idx;

    if (heap->entries[heap_idx].version <= version) return;

    idx = heap->entries[heap_idx].idx;
    if (constant_locations[idx] != -1)
        start = end = idx;
    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;

    while (stack_idx >= 0)
    {
        /* Note that we fall through to the next case statement. */
        switch(stack[stack_idx])
        {
            case HEAP_NODE_TRAVERSE_LEFT:
            {
                unsigned int left_idx = heap_idx << 1;
                if (left_idx < heap->size && heap->entries[left_idx].version > version)
                {
                    heap_idx = left_idx;
                    idx = heap->entries[heap_idx].idx;
                    if (constant_locations[idx] != -1)
                    {
                        if (start > idx)
                            start = idx;
                        if (end < idx)
                            end = idx;
                    }

                    stack[stack_idx++] = HEAP_NODE_TRAVERSE_RIGHT;
                    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;
                    break;
                }
            }

            case HEAP_NODE_TRAVERSE_RIGHT:
            {
                unsigned int right_idx = (heap_idx << 1) + 1;
                if (right_idx < heap->size && heap->entries[right_idx].version > version)
                {
                    heap_idx = right_idx;
                    idx = heap->entries[heap_idx].idx;
                    if (constant_locations[idx] != -1)
                    {
                        if (start > idx)
                            start = idx;
                        if (end < idx)
                            end = idx;
                    }

                    stack[stack_idx++] = HEAP_NODE_POP;
                    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;
                    break;
                }
            }

            case HEAP_NODE_POP:
                heap_idx >>= 1;
                --stack_idx;
                break;
        }
    }
    if (start <= end)
        GL_EXTCALL(glUniform4fv(constant_locations[start], end - start + 1, &constants[start].x));
    checkGLcall("walk_constant_heap()");
}

/* Context activation is done by the caller. */
static inline void apply_clamped_constant(const struct wined3d_gl_info *gl_info,
        GLint location, const struct wined3d_vec4 *data)
{
    GLfloat clamped_constant[4];

    if (location == -1) return;

    clamped_constant[0] = data->x < -1.0f ? -1.0f : data->x > 1.0f ? 1.0f : data->x;
    clamped_constant[1] = data->y < -1.0f ? -1.0f : data->y > 1.0f ? 1.0f : data->y;
    clamped_constant[2] = data->z < -1.0f ? -1.0f : data->z > 1.0f ? 1.0f : data->z;
    clamped_constant[3] = data->w < -1.0f ? -1.0f : data->w > 1.0f ? 1.0f : data->w;

    GL_EXTCALL(glUniform4fv(location, 1, clamped_constant));
}

/* Context activation is done by the caller. */
static inline void walk_constant_heap_clamped(const struct wined3d_gl_info *gl_info,
        const struct wined3d_vec4 *constants, const GLint *constant_locations,
        const struct constant_heap *heap, unsigned char *stack, DWORD version)
{
    int stack_idx = 0;
    unsigned int heap_idx = 1;
    unsigned int idx;

    if (heap->entries[heap_idx].version <= version) return;

    idx = heap->entries[heap_idx].idx;
    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx]);
    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;

    while (stack_idx >= 0)
    {
        /* Note that we fall through to the next case statement. */
        switch(stack[stack_idx])
        {
            case HEAP_NODE_TRAVERSE_LEFT:
            {
                unsigned int left_idx = heap_idx << 1;
                if (left_idx < heap->size && heap->entries[left_idx].version > version)
                {
                    heap_idx = left_idx;
                    idx = heap->entries[heap_idx].idx;
                    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx]);

                    stack[stack_idx++] = HEAP_NODE_TRAVERSE_RIGHT;
                    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;
                    break;
                }
            }

            case HEAP_NODE_TRAVERSE_RIGHT:
            {
                unsigned int right_idx = (heap_idx << 1) + 1;
                if (right_idx < heap->size && heap->entries[right_idx].version > version)
                {
                    heap_idx = right_idx;
                    idx = heap->entries[heap_idx].idx;
                    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx]);

                    stack[stack_idx++] = HEAP_NODE_POP;
                    stack[stack_idx] = HEAP_NODE_TRAVERSE_LEFT;
                    break;
                }
            }

            case HEAP_NODE_POP:
                heap_idx >>= 1;
                --stack_idx;
                break;
        }
    }
    checkGLcall("walk_constant_heap_clamped()");
}

static void bind_and_orphan_consts_ubo(const struct wined3d_gl_info *gl_info, struct shader_glsl_priv *priv)
{
    GL_EXTCALL(glBindBuffer(GL_UNIFORM_BUFFER, priv->ubo_vs_c));
    checkGLcall("glBindBuffer");
    GL_EXTCALL(glBufferData(GL_UNIFORM_BUFFER, priv->max_vs_consts_f * sizeof(struct wined3d_vec4),
            NULL, GL_STREAM_DRAW));
    checkGLcall("glBufferData");
}

/* Context activation is done by the caller. */
static void shader_glsl_load_constants_f(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        const struct wined3d_vec4 *constants, const GLint *constant_locations, const struct constant_heap *heap,
        unsigned char *stack, unsigned int version, struct shader_glsl_priv *priv, BOOL device_swvp)
{
    const struct wined3d_shader_lconst *lconst;
    BOOL is_vertex_shader = shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_VERTEX;

    if (is_vertex_shader && priv->consts_ubo)
    {
        BOOL zero_sw_constants = !device_swvp && priv->prev_device_swvp;
        const struct wined3d_vec4 *data;
        unsigned int const_count;
        unsigned max_const_used;

        if (priv->ubo_vs_c == -1)
        {
            ERR("UBO is not initialized.\n");
            return;
        }

        bind_and_orphan_consts_ubo(gl_info, priv);
        const_count = device_swvp ? priv->max_vs_consts_f : WINED3D_MAX_VS_CONSTS_F;
        max_const_used = shader->reg_maps.usesrelconstF ? const_count : shader->reg_maps.constant_float_count;
        if (shader->load_local_constsF || (zero_sw_constants && shader->reg_maps.usesrelconstF))
        {
            data = priv->vs_c_buffer;
            memcpy(priv->vs_c_buffer, constants, max_const_used * sizeof(*constants));
            if (zero_sw_constants)
            {
                memset(&priv->vs_c_buffer[const_count], 0, (priv->max_vs_consts_f - WINED3D_MAX_VS_CONSTS_F)
                        * sizeof(*constants));
                priv->prev_device_swvp = FALSE;
            }
            if (shader->load_local_constsF)
            {
                LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
                {
                    priv->vs_c_buffer[lconst->idx] = *(const struct wined3d_vec4 *)lconst->value;
                }
            }
        }
        else
        {
            data = constants;
        }
        GL_EXTCALL(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(*constants)
                * (zero_sw_constants ? priv->max_vs_consts_f : max_const_used), data));
        checkGLcall("glBufferSubData");
        return;
    }

    /* 1.X pshaders have the constants clamped to [-1;1] implicitly. */
    if (shader->reg_maps.shader_version.major == 1
            && shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
        walk_constant_heap_clamped(gl_info, constants, constant_locations, heap, stack, version);
    else
        walk_constant_heap(gl_info, constants, constant_locations, heap, stack, version);

    if (!shader->load_local_constsF)
    {
        TRACE("No need to load local float constants for this shader.\n");
        return;
    }

    /* Immediate constants are clamped to [-1;1] at shader creation time if needed */
    LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
    {
        GL_EXTCALL(glUniform4fv(constant_locations[lconst->idx], 1, (const GLfloat *)lconst->value));
    }
    checkGLcall("glUniform4fv()");
}

/* Context activation is done by the caller. */
static void shader_glsl_load_constants_i(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        const struct wined3d_ivec4 *constants, const GLint locations[WINED3D_MAX_CONSTS_I], WORD constants_set)
{
    unsigned int i;
    struct list* ptr;

    for (i = 0; constants_set; constants_set >>= 1, ++i)
    {
        if (!(constants_set & 1)) continue;

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4iv(locations[i], 1, &constants[i].x));
    }

    /* Load immediate constants */
    ptr = list_head(&shader->constantsI);
    while (ptr)
    {
        const struct wined3d_shader_lconst *lconst = LIST_ENTRY(ptr, const struct wined3d_shader_lconst, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4iv(locations[idx], 1, values));
        ptr = list_next(&shader->constantsI, ptr);
    }
    checkGLcall("glUniform4iv()");
}

/* Context activation is done by the caller. */
static void shader_glsl_load_constantsB(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        const GLint locations[WINED3D_MAX_CONSTS_B], const BOOL *constants, WORD constants_set)
{
    unsigned int i;
    struct list* ptr;

    for (i = 0; constants_set; constants_set >>= 1, ++i)
    {
        if (!(constants_set & 1)) continue;

        GL_EXTCALL(glUniform1iv(locations[i], 1, &constants[i]));
    }

    /* Load immediate constants */
    ptr = list_head(&shader->constantsB);
    while (ptr)
    {
        const struct wined3d_shader_lconst *lconst = LIST_ENTRY(ptr, const struct wined3d_shader_lconst, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        GL_EXTCALL(glUniform1iv(locations[idx], 1, values));
        ptr = list_next(&shader->constantsB, ptr);
    }
    checkGLcall("glUniform1iv()");
}

static void reset_program_constant_version(struct wine_rb_entry *entry, void *context)
{
    WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry)->constant_version = 0;
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_np2fixup_constants(const struct glsl_ps_program *ps,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    struct
    {
        float sx, sy;
    }
    np2fixup_constants[WINED3D_MAX_FRAGMENT_SAMPLERS];
    UINT fixup = ps->np2_fixup_info->active;
    UINT i;

    for (i = 0; fixup; fixup >>= 1, ++i)
    {
        const struct wined3d_texture *tex = state->textures[i];
        unsigned char idx = ps->np2_fixup_info->idx[i];

        if (!tex)
        {
            ERR("Nonexistent texture is flagged for NP2 texcoord fixup.\n");
            continue;
        }

        np2fixup_constants[idx].sx = tex->pow2_matrix[0];
        np2fixup_constants[idx].sy = tex->pow2_matrix[5];
    }

    GL_EXTCALL(glUniform4fv(ps->np2_fixup_location, ps->np2_fixup_info->num_consts, &np2fixup_constants[0].sx));
}

void transpose_matrix(struct wined3d_matrix *out, const struct wined3d_matrix *m)
{
    struct wined3d_matrix temp;
    unsigned int i, j;

    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            (&temp._11)[4 * j + i] = (&m->_11)[4 * i + j];

    *out = temp;
}

static void shader_glsl_ffp_vertex_normalmatrix_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_matrix mv;
    float mat[3 * 3];

    if (prog->vs.normal_matrix_location == -1)
        return;

    get_modelview_matrix(&context_gl->c, state, 0, &mv);
    compute_normal_matrix(mat, context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_LEGACY_FFP_LIGHTING, &mv);

    GL_EXTCALL(glUniformMatrix3fv(prog->vs.normal_matrix_location, 1, FALSE, mat));
    checkGLcall("glUniformMatrix3fv");
}

static void shader_glsl_ffp_vertex_texmatrix_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, unsigned int tex, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_matrix mat;

    if (tex >= WINED3D_MAX_TEXTURES)
        return;
    if (prog->vs.texture_matrix_location[tex] == -1)
        return;

    get_texture_matrix(&context_gl->c, state, tex, &mat);
    GL_EXTCALL(glUniformMatrix4fv(prog->vs.texture_matrix_location[tex], 1, FALSE, &mat._11));
    checkGLcall("glUniformMatrix4fv");
}

static void shader_glsl_ffp_vertex_material_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (state->render_states[WINED3D_RS_SPECULARENABLE])
    {
        GL_EXTCALL(glUniform4fv(prog->vs.material_specular_location, 1, &state->material.specular.r));
        GL_EXTCALL(glUniform1f(prog->vs.material_shininess_location, state->material.power));
    }
    else
    {
        static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};

        GL_EXTCALL(glUniform4fv(prog->vs.material_specular_location, 1, black));
    }
    GL_EXTCALL(glUniform4fv(prog->vs.material_ambient_location, 1, &state->material.ambient.r));
    GL_EXTCALL(glUniform4fv(prog->vs.material_diffuse_location, 1, &state->material.diffuse.r));
    GL_EXTCALL(glUniform4fv(prog->vs.material_emissive_location, 1, &state->material.emissive.r));
    checkGLcall("setting FFP material uniforms");
}

static void shader_glsl_ffp_vertex_lightambient_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_color color;

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_AMBIENT]);
    GL_EXTCALL(glUniform3fv(prog->vs.light_ambient_location, 1, &color.r));
    checkGLcall("glUniform3fv");
}

static void shader_glsl_ffp_vertex_light_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, unsigned int light, const struct wined3d_light_info *light_info,
        struct glsl_shader_prog_link *prog)
{
    const struct wined3d_matrix *view = &state->transforms[WINED3D_TS_VIEW];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_vec4 vec4;

    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].diffuse, 1, &light_info->OriginalParms.diffuse.r));
    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].specular, 1, &light_info->OriginalParms.specular.r));
    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].ambient, 1, &light_info->OriginalParms.ambient.r));

    switch (light_info->OriginalParms.type)
    {
        case WINED3D_LIGHT_POINT:
            wined3d_vec4_transform(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].range, light_info->OriginalParms.range));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].c_att, light_info->OriginalParms.attenuation0));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].l_att, light_info->OriginalParms.attenuation1));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].q_att, light_info->OriginalParms.attenuation2));
            break;

        case WINED3D_LIGHT_SPOT:
            wined3d_vec4_transform(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));

            wined3d_vec4_transform(&vec4, &light_info->direction, view);
            GL_EXTCALL(glUniform3fv(prog->vs.light_location[light].direction, 1, &vec4.x));

            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].range, light_info->OriginalParms.range));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].falloff, light_info->OriginalParms.falloff));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].c_att, light_info->OriginalParms.attenuation0));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].l_att, light_info->OriginalParms.attenuation1));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].q_att, light_info->OriginalParms.attenuation2));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].cos_htheta, cosf(light_info->OriginalParms.theta / 2.0f)));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].cos_hphi, cosf(light_info->OriginalParms.phi / 2.0f)));
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            wined3d_vec4_transform(&vec4, &light_info->direction, view);
            GL_EXTCALL(glUniform3fv(prog->vs.light_location[light].direction, 1, &vec4.x));
            break;

        case WINED3D_LIGHT_PARALLELPOINT:
            wined3d_vec4_transform(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));
            break;

        default:
            FIXME("Unrecognized light type %#x.\n", light_info->OriginalParms.type);
    }
    checkGLcall("setting FFP lights uniforms");
}

static void shader_glsl_pointsize_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    float size, att[3];
    float min, max;

    get_pointsize_minmax(&context_gl->c, state, &min, &max);

    GL_EXTCALL(glUniform1f(prog->vs.pointsize_min_location, min));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_max_location, max));
    checkGLcall("glUniform1f");

    get_pointsize(&context_gl->c, state, &size, att);

    GL_EXTCALL(glUniform1f(prog->vs.pointsize_location, size));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_c_att_location, att[0]));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_l_att_location, att[1]));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_q_att_location, att[2]));
    checkGLcall("glUniform1f");
}

static void shader_glsl_load_fog_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_color color;
    float start, end, scale;
    union
    {
        DWORD d;
        float f;
    } tmpvalue;

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_FOGCOLOR]);
    GL_EXTCALL(glUniform4fv(prog->ps.fog_color_location, 1, &color.r));
    tmpvalue.d = state->render_states[WINED3D_RS_FOGDENSITY];
    GL_EXTCALL(glUniform1f(prog->ps.fog_density_location, tmpvalue.f));
    get_fog_start_end(&context_gl->c, state, &start, &end);
    scale = 1.0f / (end - start);
    GL_EXTCALL(glUniform1f(prog->ps.fog_end_location, end));
    GL_EXTCALL(glUniform1f(prog->ps.fog_scale_location, scale));
    checkGLcall("fog emulation uniforms");
}

static void shader_glsl_clip_plane_uniform(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, unsigned int index, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_matrix matrix;
    struct wined3d_vec4 plane;

    plane = state->clip_planes[index];

    /* Clip planes are affected by the view transform in d3d for FFP draws. */
    if (!use_vs(state))
    {
        invert_matrix(&matrix, &state->transforms[WINED3D_TS_VIEW]);
        transpose_matrix(&matrix, &matrix);
        wined3d_vec4_transform(&plane, &plane, &matrix);
    }

    GL_EXTCALL(glUniform4fv(prog->vs.clip_planes_location + index, 1, &plane.x));
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_color_key_constant(const struct glsl_ps_program *ps,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    struct wined3d_color float_key[2];
    const struct wined3d_texture *texture = state->textures[0];

    wined3d_format_get_float_color_key(texture->resource.format, &texture->async.src_blt_color_key, float_key);
    GL_EXTCALL(glUniform4fv(ps->color_key_location, 2, &float_key[0].r));
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_constants(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    const struct wined3d_shader *vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    const struct wined3d_shader *pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct glsl_shader_prog_link *prog = ctx_data->glsl_program;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    float position_fixup[4 * WINED3D_MAX_VIEWPORTS];
    struct shader_glsl_priv *priv = shader_priv;
    unsigned int constant_version;
    DWORD update_mask;
    int i;

    /* No GLSL program set - nothing to do. */
    if (!prog)
        return;

    constant_version = prog->constant_version;
    update_mask = context->constant_update_mask & prog->constant_update_mask;

    if (!ctx_data->ubo_bound)
    {
        unsigned int base, count;

        wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, WINED3D_SHADER_TYPE_VERTEX,
                &base, &count);
        if (priv->consts_ubo)
        {
            if (priv->ubo_vs_c == -1)
            {
                GL_EXTCALL(glGenBuffers(1, &priv->ubo_vs_c));
                GL_EXTCALL(glBindBuffer(GL_UNIFORM_BUFFER, priv->ubo_vs_c));
                checkGLcall("glBindBuffer (UBO)");
                GL_EXTCALL(glBufferData(GL_UNIFORM_BUFFER, priv->max_vs_consts_f * sizeof(struct wined3d_vec4),
                        NULL, GL_STREAM_DRAW));
                checkGLcall("glBufferData");
            }
            GL_EXTCALL(glBindBufferBase(GL_UNIFORM_BUFFER, base, priv->ubo_vs_c));
            checkGLcall("glBindBufferBase");
        }
        if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        {
            if (priv->ubo_modelview == -1)
            {
                GL_EXTCALL(glGenBuffers(1, &priv->ubo_modelview));
                GL_EXTCALL(glBindBuffer(GL_UNIFORM_BUFFER, priv->ubo_modelview));
                checkGLcall("glBindBuffer (UBO)");
                GL_EXTCALL(glBufferData(GL_UNIFORM_BUFFER,
                        sizeof(struct wined3d_matrix) * MAX_VERTEX_BLEND_UBO, NULL, GL_DYNAMIC_DRAW));
                checkGLcall("glBufferData (UBO)");
            }
            GL_EXTCALL(glBindBufferBase(GL_UNIFORM_BUFFER, base + 1, priv->ubo_modelview));
            checkGLcall("glBindBufferBase");
        }
        ctx_data->ubo_bound = TRUE;
    }

    if (update_mask & WINED3D_SHADER_CONST_VS_F)
        shader_glsl_load_constants_f(vshader, gl_info, state->vs_consts_f,
                prog->vs.uniform_f_locations, &priv->vconst_heap, priv->stack,
                constant_version, priv, device_is_swvp(context->device));

    if (update_mask & WINED3D_SHADER_CONST_VS_I)
        shader_glsl_load_constants_i(vshader, gl_info, state->vs_consts_i,
                prog->vs.uniform_i_locations, vshader->reg_maps.integer_constants);

    if (update_mask & WINED3D_SHADER_CONST_VS_B)
        shader_glsl_load_constantsB(vshader, gl_info, prog->vs.uniform_b_locations, state->vs_consts_b,
                vshader->reg_maps.boolean_constants);

    if (update_mask & WINED3D_SHADER_CONST_VS_CLIP_PLANES)
    {
        for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
            shader_glsl_clip_plane_uniform(context_gl, state, i, prog);
    }

    if (update_mask & WINED3D_SHADER_CONST_VS_POINTSIZE)
        shader_glsl_pointsize_uniform(context_gl, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_POS_FIXUP)
    {
        unsigned int fixup_count = state->shader[WINED3D_SHADER_TYPE_GEOMETRY] ?
                max(state->viewport_count, 1) : 1;
        shader_get_position_fixup(context, state, fixup_count, position_fixup);
        if (state->shader[WINED3D_SHADER_TYPE_GEOMETRY])
            GL_EXTCALL(glUniform4fv(prog->gs.pos_fixup_location, fixup_count, position_fixup));
        else if (state->shader[WINED3D_SHADER_TYPE_DOMAIN])
            GL_EXTCALL(glUniform4fv(prog->ds.pos_fixup_location, 1, position_fixup));
        else
            GL_EXTCALL(glUniform4fv(prog->vs.pos_fixup_location, 1, position_fixup));
        checkGLcall("glUniform4fv");
    }

    if (update_mask & WINED3D_SHADER_CONST_BASE_VERTEX_ID)
    {
        GL_EXTCALL(glUniform1i(prog->vs.base_vertex_id_location, state->base_vertex_index));
        checkGLcall("base vertex id");
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_MODELVIEW)
        shader_glsl_ffp_vertex_normalmatrix_uniform(context_gl, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_FFP_VERTEXBLEND)
    {
        struct wined3d_matrix mat;

        if (prog->vs.modelview_block_index != -1)
        {
            if (priv->ubo_modelview == -1)
                FIXME("UBO buffer with vertex blend matrices is not initialized.\n");

            GL_EXTCALL(glBindBuffer(GL_UNIFORM_BUFFER, priv->ubo_modelview));
            checkGLcall("glBindBuffer (UBO)");
            GL_EXTCALL(glBufferData(GL_UNIFORM_BUFFER, sizeof(*priv->modelview_buffer) * MAX_VERTEX_BLEND_UBO,
                    NULL, GL_STREAM_DRAW));
            checkGLcall("glBufferData");

            for (i = 0; i < MAX_VERTEX_BLEND_UBO; ++i)
                get_modelview_matrix(context, state, i, &priv->modelview_buffer[i]);

            GL_EXTCALL(glBufferSubData(GL_UNIFORM_BUFFER, 0,
                    sizeof(*priv->modelview_buffer) * MAX_VERTEX_BLEND_UBO, priv->modelview_buffer));
            checkGLcall("glBufferSubData");
        }
        else
        {
            for (i = 0; i < MAX_VERTEX_BLENDS; ++i)
            {
                if (prog->vs.modelview_matrix_location[i] == -1)
                    break;

                get_modelview_matrix(context, state, i, &mat);
                GL_EXTCALL(glUniformMatrix4fv(prog->vs.modelview_matrix_location[i], 1, FALSE, &mat._11));
                checkGLcall("glUniformMatrix4fv");
            }
        }
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_PROJ)
    {
        struct wined3d_matrix projection;

        get_projection_matrix(context, state, &projection);
        GL_EXTCALL(glUniformMatrix4fv(prog->vs.projection_matrix_location, 1, FALSE, &projection._11));
        checkGLcall("glUniformMatrix4fv");
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_TEXMATRIX)
    {
        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
            shader_glsl_ffp_vertex_texmatrix_uniform(context_gl, state, i, prog);
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_MATERIAL)
        shader_glsl_ffp_vertex_material_uniform(context_gl, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_FFP_LIGHTS)
    {
        unsigned int point_idx, spot_idx, directional_idx, parallel_point_idx;
        DWORD point_count = 0;
        DWORD spot_count = 0;
        DWORD directional_count = 0;
        DWORD parallel_point_count = 0;

        for (i = 0; i < WINED3D_MAX_ACTIVE_LIGHTS; ++i)
        {
            if (!state->light_state.lights[i])
                continue;

            switch (state->light_state.lights[i]->OriginalParms.type)
            {
                case WINED3D_LIGHT_POINT:
                    ++point_count;
                    break;
                case WINED3D_LIGHT_SPOT:
                    ++spot_count;
                    break;
                case WINED3D_LIGHT_DIRECTIONAL:
                    ++directional_count;
                    break;
                case WINED3D_LIGHT_PARALLELPOINT:
                    ++parallel_point_count;
                    break;
                default:
                    FIXME("Unhandled light type %#x.\n", state->light_state.lights[i]->OriginalParms.type);
                    break;
            }
        }
        point_idx = 0;
        spot_idx = point_idx + point_count;
        directional_idx = spot_idx + spot_count;
        parallel_point_idx = directional_idx + directional_count;

        shader_glsl_ffp_vertex_lightambient_uniform(context_gl, state, prog);
        for (i = 0; i < WINED3D_MAX_ACTIVE_LIGHTS; ++i)
        {
            const struct wined3d_light_info *light_info = state->light_state.lights[i];
            unsigned int idx;

            if (!light_info)
                continue;

            switch (light_info->OriginalParms.type)
            {
                case WINED3D_LIGHT_POINT:
                    idx = point_idx++;
                    break;
                case WINED3D_LIGHT_SPOT:
                    idx = spot_idx++;
                    break;
                case WINED3D_LIGHT_DIRECTIONAL:
                    idx = directional_idx++;
                    break;
                case WINED3D_LIGHT_PARALLELPOINT:
                    idx = parallel_point_idx++;
                    break;
                default:
                    FIXME("Unhandled light type %#x.\n", light_info->OriginalParms.type);
                    continue;
            }
            shader_glsl_ffp_vertex_light_uniform(context_gl, state, idx, light_info, prog);
        }
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_F)
        shader_glsl_load_constants_f(pshader, gl_info, state->ps_consts_f,
                prog->ps.uniform_f_locations, &priv->pconst_heap, priv->stack, constant_version,
                priv, FALSE);

    if (update_mask & WINED3D_SHADER_CONST_PS_I)
        shader_glsl_load_constants_i(pshader, gl_info, state->ps_consts_i,
                prog->ps.uniform_i_locations, pshader->reg_maps.integer_constants);

    if (update_mask & WINED3D_SHADER_CONST_PS_B)
        shader_glsl_load_constantsB(pshader, gl_info, prog->ps.uniform_b_locations, state->ps_consts_b,
                pshader->reg_maps.boolean_constants);

    if (update_mask & WINED3D_SHADER_CONST_PS_BUMP_ENV)
    {
        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (prog->ps.bumpenv_mat_location[i] == -1)
                continue;

            GL_EXTCALL(glUniformMatrix2fv(prog->ps.bumpenv_mat_location[i], 1, 0,
                    (const GLfloat *)&state->texture_states[i][WINED3D_TSS_BUMPENV_MAT00]));

            if (prog->ps.bumpenv_lum_scale_location[i] != -1)
            {
                GL_EXTCALL(glUniform1fv(prog->ps.bumpenv_lum_scale_location[i], 1,
                        (const GLfloat *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LSCALE]));
                GL_EXTCALL(glUniform1fv(prog->ps.bumpenv_lum_offset_location[i], 1,
                        (const GLfloat *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LOFFSET]));
            }
        }

        checkGLcall("bump env uniforms");
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_Y_CORR)
    {
        const struct wined3d_vec4 correction_params =
        {
            /* Position is relative to the framebuffer, not the viewport. */
            context->render_offscreen ? 0.0f : (float)state->fb->render_targets[0]->height,
            context->render_offscreen ? 1.0f : -1.0f,
            0.0f,
            0.0f,
        };

        GL_EXTCALL(glUniform4fv(prog->ps.ycorrection_location, 1, &correction_params.x));
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_NP2_FIXUP)
        shader_glsl_load_np2fixup_constants(&prog->ps, gl_info, state);
    if (update_mask & WINED3D_SHADER_CONST_FFP_COLOR_KEY)
        shader_glsl_load_color_key_constant(&prog->ps, gl_info, state);

    if (update_mask & WINED3D_SHADER_CONST_FFP_PS)
    {
        struct wined3d_color color;

        if (prog->ps.tex_factor_location != -1)
        {
            wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_TEXTUREFACTOR]);
            GL_EXTCALL(glUniform4fv(prog->ps.tex_factor_location, 1, &color.r));
        }

        if (state->render_states[WINED3D_RS_SPECULARENABLE])
            GL_EXTCALL(glUniform4f(prog->ps.specular_enable_location, 1.0f, 1.0f, 1.0f, 0.0f));
        else
            GL_EXTCALL(glUniform4f(prog->ps.specular_enable_location, 0.0f, 0.0f, 0.0f, 0.0f));

        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (prog->ps.tss_constant_location[i] == -1)
                continue;

            wined3d_color_from_d3dcolor(&color, state->texture_states[i][WINED3D_TSS_CONSTANT]);
            GL_EXTCALL(glUniform4fv(prog->ps.tss_constant_location[i], 1, &color.r));
        }

        checkGLcall("fixed function uniforms");
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_FOG)
        shader_glsl_load_fog_uniform(context_gl, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_PS_ALPHA_TEST)
    {
        float ref = wined3d_alpha_ref(state);

        GL_EXTCALL(glUniform1f(prog->ps.alpha_test_ref_location, ref));
        checkGLcall("alpha test emulation uniform");
    }

    if (priv->next_constant_version == UINT_MAX)
    {
        TRACE("Max constant version reached, resetting to 0.\n");
        wine_rb_for_each_entry(&priv->program_lookup, reset_program_constant_version, NULL);
        priv->next_constant_version = 1;
    }
    else
    {
        prog->constant_version = priv->next_constant_version++;
    }
}

static void update_heap_entry(struct constant_heap *heap, unsigned int idx, DWORD new_version)
{
    struct constant_entry *entries = heap->entries;
    unsigned int *positions = heap->positions;
    unsigned int heap_idx, parent_idx;

    if (!heap->contained[idx])
    {
        heap_idx = heap->size++;
        heap->contained[idx] = TRUE;
    }
    else
    {
        heap_idx = positions[idx];
    }

    while (heap_idx > 1)
    {
        parent_idx = heap_idx >> 1;

        if (new_version <= entries[parent_idx].version) break;

        entries[heap_idx] = entries[parent_idx];
        positions[entries[parent_idx].idx] = heap_idx;
        heap_idx = parent_idx;
    }

    entries[heap_idx].version = new_version;
    entries[heap_idx].idx = idx;
    positions[idx] = heap_idx;
}

static void shader_glsl_update_float_vertex_constants(struct wined3d_device *device, UINT start, UINT count)
{
    struct shader_glsl_priv *priv = device->shader_priv;
    struct constant_heap *heap = &priv->vconst_heap;
    UINT i;

    if (priv->consts_ubo)
        return;

    for (i = start; i < count + start; ++i)
    {
        update_heap_entry(heap, i, priv->next_constant_version);
    }
}

static void shader_glsl_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count)
{
    struct shader_glsl_priv *priv = device->shader_priv;
    struct constant_heap *heap = &priv->pconst_heap;
    UINT i;

    for (i = start; i < count + start; ++i)
    {
        update_heap_entry(heap, i, priv->next_constant_version);
    }
}

static unsigned int vec4_varyings(DWORD shader_major, const struct wined3d_gl_info *gl_info)
{
    unsigned int ret = gl_info->limits.glsl_varyings / 4;
    /* 4.0 shaders do not write clip coords because d3d10 does not support user clipplanes */
    if(shader_major > 3) return ret;

    /* 3.0 shaders may need an extra varying for the clip coord on some cards(mostly dx10 ones) */
    if (gl_info->quirks & WINED3D_QUIRK_GLSL_CLIP_VARYING) ret -= 1;
    return ret;
}

static BOOL needs_legacy_glsl_syntax(const struct wined3d_gl_info *gl_info)
{
    return gl_info->glsl_version < MAKEDWORD_VERSION(1, 30);
}

static BOOL shader_glsl_use_explicit_attrib_location(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_EXPLICIT_ATTRIB_LOCATION]
            && shader_glsl_use_layout_qualifier(gl_info) && !needs_legacy_glsl_syntax(gl_info);
}

static BOOL shader_glsl_use_interface_blocks(const struct wined3d_gl_info *gl_info)
{
    return shader_glsl_get_version(gl_info) >= 150;
}

static const char *get_attribute_keyword(const struct wined3d_gl_info *gl_info)
{
    return needs_legacy_glsl_syntax(gl_info) ? "attribute" : "in";
}

static void PRINTF_ATTR(4, 5) declare_in_varying(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, BOOL flat, const char *format, ...)
{
    va_list args;
    int ret;

    shader_addline(buffer, "%s%s ", flat ? "flat " : "",
            needs_legacy_glsl_syntax(gl_info) ? "varying" : "in");
    for (;;)
    {
        va_start(args, format);
        ret = shader_vaddline(buffer, format, args);
        va_end(args);
        if (!ret)
            return;
        if (!string_buffer_resize(buffer, ret))
            return;
    }
}

static void PRINTF_ATTR(4, 5) declare_out_varying(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, BOOL flat, const char *format, ...)
{
    va_list args;
    int ret;

    shader_addline(buffer, "%s%s ", flat ? "flat " : "",
            needs_legacy_glsl_syntax(gl_info) ? "varying" : "out");
    for (;;)
    {
        va_start(args, format);
        ret = shader_vaddline(buffer, format, args);
        va_end(args);
        if (!ret)
            return;
        if (!string_buffer_resize(buffer, ret))
            return;
    }
}

static const char *shader_glsl_shader_input_name(const struct wined3d_gl_info *gl_info)
{
    return shader_glsl_use_interface_blocks(gl_info) ? "shader_in.reg" : "ps_link";
}

static const char *shader_glsl_shader_output_name(const struct wined3d_gl_info *gl_info)
{
    return shader_glsl_use_interface_blocks(gl_info) ? "shader_out.reg" : "ps_link";
}

static const char *shader_glsl_interpolation_qualifiers(enum wined3d_shader_interpolation_mode mode)
{
    switch (mode)
    {
        case WINED3DSIM_CONSTANT:
            return "flat ";
        case WINED3DSIM_LINEAR_NOPERSPECTIVE:
            return "noperspective ";
        default:
            FIXME("Unhandled interpolation mode %#x.\n", mode);
        case WINED3DSIM_NONE:
        case WINED3DSIM_LINEAR:
            return "";
    }
}

static enum wined3d_shader_interpolation_mode wined3d_extract_interpolation_mode(
        const DWORD *packed_interpolation_mode, unsigned int register_idx)
{
    return wined3d_extract_bits(packed_interpolation_mode,
            register_idx * WINED3D_PACKED_INTERPOLATION_BIT_COUNT, WINED3D_PACKED_INTERPOLATION_BIT_COUNT);
}

static void shader_glsl_declare_shader_inputs(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, unsigned int element_count,
        const DWORD *interpolation_mode, BOOL unroll)
{
    enum wined3d_shader_interpolation_mode mode;
    unsigned int i;

    if (shader_glsl_use_interface_blocks(gl_info))
    {
        if (unroll)
        {
            shader_addline(buffer, "in shader_in_out {\n");
            for (i = 0; i < element_count; ++i)
            {
                mode = wined3d_extract_interpolation_mode(interpolation_mode, i);
                shader_addline(buffer, "    %svec4 reg%u;\n", shader_glsl_interpolation_qualifiers(mode), i);
            }
            shader_addline(buffer, "} shader_in;\n");
        }
        else
        {
            shader_addline(buffer, "in shader_in_out { vec4 reg[%u]; } shader_in;\n", element_count);
        }
    }
    else
    {
        declare_in_varying(gl_info, buffer, FALSE, "vec4 ps_link[%u];\n", element_count);
    }
}

static BOOL needs_interpolation_qualifiers_for_shader_outputs(const struct wined3d_gl_info *gl_info)
{
    /* In GLSL 4.40+ it is fine to specify interpolation qualifiers only in
     * fragment shaders. In older GLSL versions interpolation qualifiers must
     * match between shader stages. */
    return gl_info->glsl_version < MAKEDWORD_VERSION(4, 40);
}

static void shader_glsl_declare_shader_outputs(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, unsigned int element_count, BOOL rasterizer_setup,
        const DWORD *interpolation_mode)
{
    enum wined3d_shader_interpolation_mode mode;
    unsigned int i;

    if (shader_glsl_use_interface_blocks(gl_info))
    {
        if (rasterizer_setup)
        {
            shader_addline(buffer, "out shader_in_out {\n");
            for (i = 0; i < element_count; ++i)
            {
                const char *interpolation_qualifiers = "";
                if (needs_interpolation_qualifiers_for_shader_outputs(gl_info))
                {
                    mode = wined3d_extract_interpolation_mode(interpolation_mode, i);
                    interpolation_qualifiers = shader_glsl_interpolation_qualifiers(mode);
                }
                shader_addline(buffer, "    %svec4 reg%u;\n", interpolation_qualifiers, i);
            }
            shader_addline(buffer, "} shader_out;\n");
        }
        else
        {
            shader_addline(buffer, "out shader_in_out { vec4 reg[%u]; } shader_out;\n", element_count);
        }
    }
    else
    {
        declare_out_varying(gl_info, buffer, FALSE, "vec4 ps_link[%u];\n", element_count);
    }
}

static const char *get_fragment_output(const struct wined3d_gl_info *gl_info)
{
    return needs_legacy_glsl_syntax(gl_info) ? "gl_FragData" : "ps_out";
}

static const char *glsl_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    switch (primitive_type)
    {
        case WINED3D_PT_POINTLIST:
            return "points";

        case WINED3D_PT_LINELIST:
            return "lines";

        case WINED3D_PT_LINESTRIP:
            return "line_strip";

        case WINED3D_PT_TRIANGLELIST:
            return "triangles";

        case WINED3D_PT_TRIANGLESTRIP:
            return "triangle_strip";

        case WINED3D_PT_LINELIST_ADJ:
            return "lines_adjacency";

        case WINED3D_PT_TRIANGLELIST_ADJ:
            return "triangles_adjacency";

        default:
            FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(primitive_type));
            return "";
    }
}

static BOOL glsl_is_color_reg_read(const struct wined3d_shader *shader, unsigned int idx)
{
    const struct wined3d_shader_signature *input_signature = &shader->input_signature;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    DWORD input_reg_used = shader->u.ps.input_reg_used;
    unsigned int i;

    if (reg_maps->shader_version.major < 3)
        return input_reg_used & (1u << idx);

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &input_signature->elements[i];

        if (!(reg_maps->input_registers & (1u << input->register_idx)))
            continue;

        if (shader_match_semantic(input->semantic_name, WINED3D_DECL_USAGE_COLOR)
                && input->semantic_idx == idx)
            return input_reg_used & (1u << input->register_idx);
    }
    return FALSE;
}

static BOOL glsl_is_shadow_sampler(const struct wined3d_shader *shader,
        const struct ps_compile_args *ps_args, unsigned int resource_idx, unsigned int sampler_idx)
{
    const struct wined3d_shader_version *version = &shader->reg_maps.shader_version;

    if (version->major >= 4)
        return shader->reg_maps.sampler_comparison_mode & (1u << sampler_idx);
    else
        return version->type == WINED3D_SHADER_TYPE_PIXEL && (ps_args->shadow & (1u << resource_idx));
}

static void shader_glsl_declare_typed_vertex_attribute(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const char *vector_type, const char *scalar_type,
        unsigned int index)
{
    shader_addline(buffer, "%s %s4 vs_in_%s%u;\n",
            get_attribute_keyword(gl_info), vector_type, scalar_type, index);
    shader_addline(buffer, "vec4 vs_in%u = %sBitsToFloat(vs_in_%s%u);\n",
            index, scalar_type, scalar_type, index);
}

static void shader_glsl_declare_generic_vertex_attribute(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct wined3d_shader_signature_element *e)
{
    unsigned int index = e->register_idx;
    enum wined3d_component_type type;

    if (e->sysval_semantic == WINED3D_SV_VERTEX_ID)
    {
        shader_addline(buffer, "uniform int base_vertex_id;\n");
        shader_addline(buffer, "vec4 vs_in%u = vec4(intBitsToFloat(gl_VertexID - base_vertex_id), 0.0, 0.0, 0.0);\n", index);
        return;
    }
    if (e->sysval_semantic == WINED3D_SV_INSTANCE_ID)
    {
        shader_addline(buffer, "vec4 vs_in%u = vec4(intBitsToFloat(gl_InstanceID), 0.0, 0.0, 0.0);\n",
                index);
        return;
    }
    if (e->sysval_semantic && e->sysval_semantic != WINED3D_SV_POSITION)
        FIXME("Unhandled sysval semantic %#x.\n", e->sysval_semantic);

    if (shader_glsl_use_explicit_attrib_location(gl_info))
        shader_addline(buffer, "layout(location = %u) ", index);

    type = e->component_type;
    if ((unsigned int)type >= ARRAY_SIZE(component_type_info))
    {
        FIXME("Unhandled type %#x.\n", type);
        type = WINED3D_TYPE_FLOAT;
    }
    if (type == WINED3D_TYPE_FLOAT || type == WINED3D_TYPE_UNKNOWN)
        shader_addline(buffer, "%s vec4 vs_in%u;\n", get_attribute_keyword(gl_info), index);
    else
        shader_glsl_declare_typed_vertex_attribute(buffer, gl_info,
                component_type_info[type].glsl_vector_type,
                component_type_info[type].glsl_scalar_type, index);
}

/** Generate the variable & register declarations for the GLSL output target */
static void shader_generate_glsl_declarations(const struct wined3d_context_gl *context_gl,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader *shader,
        const struct wined3d_shader_reg_maps *reg_maps, const struct shader_glsl_ctx_priv *ctx_priv)
{
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    struct shader_glsl_priv *priv = context_gl->c.device->shader_priv;
    const struct vs_compile_args *vs_args = ctx_priv->cur_vs_args;
    const struct ps_compile_args *ps_args = ctx_priv->cur_ps_args;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_shader_indexable_temp *idx_temp_reg;
    unsigned int uniform_block_base, uniform_block_count;
    enum wined3d_shader_resource_type resource_type;
    const struct wined3d_shader_lconst *lconst;
    const char *prefix;
    unsigned int i;
    DWORD map;

    if (wined3d_settings.strict_shader_math)
        shader_addline(buffer, "#pragma optionNV(fastmath off)\n");

    if (wined3d_settings.multiply_special == 2 && version->major < 4)
    {
        shader_addline(buffer, "float dot1(float v1, float v2) {return abs(v1) == 0.0 || abs(v2) == 0.0 ? 0.0 : v1 * v2;}\n");
        shader_addline(buffer, "float dot2(vec2 v1, vec2 v2) {return dot1(v1.x, v2.x) + dot1(v1.y, v2.y);}\n");
        shader_addline(buffer, "float dot3(vec3 v1, vec3 v2) {return dot2(v1.xy, v2.xy) + dot1(v1.z, v2.z);}\n");
        shader_addline(buffer, "float dot4(vec4 v1, vec4 v2) {return dot2(v1.xy, v2.xy) + dot2(v1.zw, v2.zw);}\n");

        shader_addline(buffer, "float mul1(float v1, float v2) {return abs(v1) == 0.0 || abs(v2) == 0.0 ? 0.0 : v1 * v2;}\n");
        shader_addline(buffer, "vec2 mul2(vec2 v1, vec2 v2) {return vec2(mul1(v1.x, v2.x), mul1(v1.y, v2.y));}\n");
        shader_addline(buffer, "vec3 mul3(vec3 v1, vec3 v2) {return vec3(mul2(v1.xy, v2.xy), mul1(v1.z, v2.z));}\n");
        shader_addline(buffer, "vec4 mul4(vec4 v1, vec4 v2) {return vec4(mul2(v1.xy, v2.xy), mul2(v1.zw, v2.zw));}\n");
    }

    prefix = shader_glsl_get_prefix(version->type);

    /* Prototype the subroutines */
    for (i = 0, map = reg_maps->labels; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "void subroutine%u();\n", i);
    }

    /* Declare the constants (aka uniforms) */
    if (shader->limits->constant_float > 0 && priv->consts_ubo
            && version->type == WINED3D_SHADER_TYPE_VERTEX)
    {
        shader_addline(buffer,"layout(std140) uniform vs_c_ubo\n"
                "{ \n"
                "    vec4 %s_c[%u];\n"
                "};\n", prefix, min(shader->limits->constant_float, priv->max_vs_consts_f));
    }
    else if (shader->limits->constant_float > 0)
    {
        unsigned max_constantsF;

        /* Unless the shader uses indirect addressing, always declare the
         * maximum array size and ignore that we need some uniforms privately.
         * E.g. if GL supports 256 uniforms, and we need 2 for the pos fixup
         * and immediate values, still declare VC[256]. If the shader needs
         * more uniforms than we have it won't work in any case. If it uses
         * less, the compiler will figure out which uniforms are really used
         * and strip them out. This allows a shader to use c255 on a dx9 card,
         * as long as it doesn't also use all the other constants.
         *
         * If the shader uses indirect addressing the compiler must assume
         * that all declared uniforms are used. In this case, declare only the
         * amount that we're assured to have.
         *
         * Thus we run into problems in these two cases:
         * 1) The shader really uses more uniforms than supported.
         * 2) The shader uses indirect addressing, less constants than
         *    supported, but uses a constant index > #supported consts. */
        if (version->type == WINED3D_SHADER_TYPE_PIXEL)
        {
            /* No indirect addressing here. */
            max_constantsF = gl_info->limits.glsl_ps_float_constants;
        }
        else
        {
            if (reg_maps->usesrelconstF)
            {
                /* Subtract the other potential uniforms from the max
                 * available (bools, ints, and 1 row of projection matrix).
                 * Subtract another uniform for immediate values, which have
                 * to be loaded via uniform by the driver as well. The shader
                 * code only uses 0.5, 2.0, 1.0, 128 and -128 in vertex
                 * shader code, so one vec4 should be enough. (Unfortunately
                 * the Nvidia driver doesn't store 128 and -128 in one float).
                 *
                 * Writing gl_ClipVertex requires one uniform for each
                 * clipplane as well. */
                max_constantsF = gl_info->limits.glsl_vs_float_constants - 3;
                if (vs_args->clip_enabled)
                    max_constantsF -= gl_info->limits.user_clip_distances;
                max_constantsF -= wined3d_popcount(reg_maps->integer_constants);
                /* Strictly speaking a bool only uses one scalar, but the nvidia(Linux) compiler doesn't pack them properly,
                 * so each scalar requires a full vec4. We could work around this by packing the booleans ourselves, but
                 * for now take this into account when calculating the number of available constants
                 */
                max_constantsF -= wined3d_popcount(reg_maps->boolean_constants);
                /* Set by driver quirks in directx.c */
                max_constantsF -= gl_info->reserved_glsl_constants;

                if (max_constantsF < shader->limits->constant_float)
                {
                    static unsigned int once;

                    if (!once++)
                        ERR_(winediag)("The hardware does not support enough uniform components to run this shader,"
                                " it may not render correctly.\n");
                    else
                        WARN("The hardware does not support enough uniform components to run this shader.\n");
                }
            }
            else
            {
                max_constantsF = reg_maps->constant_float_count;
            }
        }
        max_constantsF = min(shader->limits->constant_float, max_constantsF);
        if (max_constantsF)
            shader_addline(buffer, "uniform vec4 %s_c[%u];\n", prefix, max_constantsF);
    }

    /* Always declare the full set of constants, the compiler can remove the
     * unused ones because d3d doesn't (yet) support indirect int and bool
     * constant addressing. This avoids problems if the app uses e.g. i0 and i9. */
    if (shader->limits->constant_int > 0 && reg_maps->integer_constants)
        shader_addline(buffer, "uniform ivec4 %s_i[%u];\n", prefix, shader->limits->constant_int);

    if (shader->limits->constant_bool > 0 && reg_maps->boolean_constants)
        shader_addline(buffer, "uniform bool %s_b[%u];\n", prefix, shader->limits->constant_bool);

    /* Declare immediate constant buffer */
    if (reg_maps->icb)
        shader_addline(buffer, "uniform vec4 %s_icb[%u];\n", prefix, reg_maps->icb->vec4_count);

    /* Declare constant buffers */
    wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, version->type,
            &uniform_block_base, &uniform_block_count);
    for (i = 0; i < min(uniform_block_count, WINED3D_MAX_CBS); ++i)
    {
        if (reg_maps->cb_sizes[i])
        {
            shader_addline(buffer, "layout(std140");
            if (shader_glsl_use_layout_binding_qualifier(gl_info))
                shader_addline(buffer, ", binding = %u", uniform_block_base + i);
            shader_addline(buffer, ") uniform block_%s_cb%u { vec4 %s_cb%u[%u]; };\n",
                    prefix, i, prefix, i, reg_maps->cb_sizes[i]);
        }
    }

    /* Declare texture samplers */
    for (i = 0; i < reg_maps->sampler_map.count; ++i)
    {
        struct wined3d_shader_sampler_map_entry *entry;
        const char *sampler_type_prefix, *sampler_type;
        BOOL shadow_sampler, tex_rect;

        entry = &reg_maps->sampler_map.entries[i];

        if (entry->resource_idx >= ARRAY_SIZE(reg_maps->resource_info))
        {
            ERR("Invalid resource index %u.\n", entry->resource_idx);
            continue;
        }

        switch (reg_maps->resource_info[entry->resource_idx].data_type)
        {
            case WINED3D_DATA_FLOAT:
            case WINED3D_DATA_UNORM:
            case WINED3D_DATA_SNORM:
                sampler_type_prefix = "";
                break;

            case WINED3D_DATA_INT:
                sampler_type_prefix = "i";
                break;

            case WINED3D_DATA_UINT:
                sampler_type_prefix = "u";
                break;

            default:
                sampler_type_prefix = "";
                ERR("Unhandled resource data type %#x.\n", reg_maps->resource_info[i].data_type);
                break;
        }

        shadow_sampler = glsl_is_shadow_sampler(shader, ps_args, entry->resource_idx, entry->sampler_idx);
        resource_type = version->type == WINED3D_SHADER_TYPE_PIXEL
                ? pixelshader_get_resource_type(reg_maps, entry->resource_idx, ps_args->tex_types)
                : reg_maps->resource_info[entry->resource_idx].type;

        switch (resource_type)
        {
            case WINED3D_SHADER_RESOURCE_BUFFER:
                sampler_type = "samplerBuffer";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
                if (shadow_sampler)
                    sampler_type = "sampler1DShadow";
                else
                    sampler_type = "sampler1D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                tex_rect = version->type == WINED3D_SHADER_TYPE_PIXEL
                        && (ps_args->np2_fixup & (1u << entry->resource_idx))
                        && gl_info->supported[ARB_TEXTURE_RECTANGLE];
                if (shadow_sampler)
                {
                    if (tex_rect)
                        sampler_type = "sampler2DRectShadow";
                    else
                        sampler_type = "sampler2DShadow";
                }
                else
                {
                    if (tex_rect)
                        sampler_type = "sampler2DRect";
                    else
                        sampler_type = "sampler2D";
                }
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                if (shadow_sampler)
                    FIXME("Unsupported 3D shadow sampler.\n");
                sampler_type = "sampler3D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                if (shadow_sampler)
                    sampler_type = "samplerCubeShadow";
                else
                    sampler_type = "samplerCube";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY:
                if (shadow_sampler)
                    sampler_type = "sampler1DArrayShadow";
                else
                    sampler_type = "sampler1DArray";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY:
                if (shadow_sampler)
                    sampler_type = "sampler2DArrayShadow";
                else
                    sampler_type = "sampler2DArray";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY:
                if (shadow_sampler)
                    sampler_type = "samplerCubeArrayShadow";
                else
                    sampler_type = "samplerCubeArray";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMS:
                sampler_type = "sampler2DMS";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY:
                sampler_type = "sampler2DMSArray";
                break;

            default:
                sampler_type = "unsupported_sampler";
                FIXME("Unhandled resource type %#x.\n", resource_type);
                break;
        }

        if (shader_glsl_use_layout_binding_qualifier(gl_info))
            shader_glsl_append_sampler_binding_qualifier(buffer, &context_gl->c, version, entry->bind_idx);
        shader_addline(buffer, "uniform %s%s %s_sampler%u;\n",
                sampler_type_prefix, sampler_type, prefix, entry->bind_idx);
    }

    /* Declare images */
    for (i = 0; i < ARRAY_SIZE(reg_maps->uav_resource_info); ++i)
    {
        const char *image_type_prefix, *image_type, *read_format;

        if (!reg_maps->uav_resource_info[i].type)
            continue;

        switch (reg_maps->uav_resource_info[i].data_type)
        {
            case WINED3D_DATA_FLOAT:
            case WINED3D_DATA_UNORM:
            case WINED3D_DATA_SNORM:
                image_type_prefix = "";
                read_format = "r32f";
                break;

            case WINED3D_DATA_INT:
                image_type_prefix = "i";
                read_format = "r32i";
                break;

            case WINED3D_DATA_UINT:
                image_type_prefix = "u";
                read_format = "r32ui";
                break;

            default:
                image_type_prefix = "";
                read_format = "";
                ERR("Unhandled resource data type %#x.\n", reg_maps->uav_resource_info[i].data_type);
                break;
        }

        switch (reg_maps->uav_resource_info[i].type)
        {
            case WINED3D_SHADER_RESOURCE_BUFFER:
                image_type = "imageBuffer";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
                image_type = "image1D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                image_type = "image2D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                image_type = "image3D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY:
                image_type = "image1DArray";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY:
                image_type = "image2DArray";
                break;

            default:
                image_type = "unsupported_image";
                FIXME("Unhandled resource type %#x.\n", reg_maps->uav_resource_info[i].type);
                break;
        }

        if (shader_glsl_use_layout_binding_qualifier(gl_info))
            shader_addline(buffer, "layout(binding = %u)\n", i);
        if (reg_maps->uav_read_mask & (1u << i))
            shader_addline(buffer, "layout(%s) uniform %s%s %s_image%u;\n",
                    read_format, image_type_prefix, image_type, prefix, i);
        else
            shader_addline(buffer, "writeonly uniform %s%s %s_image%u;\n",
                    image_type_prefix, image_type, prefix, i);

        if (reg_maps->uav_counter_mask & (1u << i))
            shader_addline(buffer, "layout(binding = %u) uniform atomic_uint %s_counter%u;\n",
                    i, prefix, i);
    }

    /* Declare address variables */
    for (i = 0, map = reg_maps->address; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "ivec4 A%u;\n", i);
    }

    /* Declare output register temporaries */
    if (shader->limits->packed_output)
        shader_addline(buffer, "vec4 %s_out[%u];\n", prefix, shader->limits->packed_output);

    /* Declare temporary variables */
    if (reg_maps->temporary_count)
    {
        for (i = 0; i < reg_maps->temporary_count; ++i)
            shader_addline(buffer, "vec4 R%u;\n", i);
    }
    else if (version->major < 4)
    {
        for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
        {
            if (map & 1)
                shader_addline(buffer, "vec4 R%u;\n", i);
        }
    }

    /* Declare indexable temporary variables */
    LIST_FOR_EACH_ENTRY(idx_temp_reg, &reg_maps->indexable_temps, struct wined3d_shader_indexable_temp, entry)
    {
        if (idx_temp_reg->component_count != 4)
            FIXME("Ignoring component count %u.\n", idx_temp_reg->component_count);
        shader_addline(buffer, "vec4 X%u[%u];\n", idx_temp_reg->register_idx, idx_temp_reg->register_size);
    }

    /* Declare loop registers aLx */
    if (version->major < 4)
    {
        for (i = 0; i < reg_maps->loop_depth; ++i)
        {
            shader_addline(buffer, "int aL%u;\n", i);
            shader_addline(buffer, "int tmpInt%u;\n", i);
        }
    }

    /* Temporary variables for matrix operations */
    shader_addline(buffer, "vec4 tmp0;\n");
    shader_addline(buffer, "vec4 tmp1;\n");

    if (!shader->load_local_constsF)
    {
        LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
        {
            shader_addline(buffer, "const vec4 %s_lc%u = ", prefix, lconst->idx);
            shader_glsl_append_imm_vec(buffer, (const float *)lconst->value, ARRAY_SIZE(lconst->value), gl_info);
            shader_addline(buffer, ";\n");
        }
    }
}

/* Prototypes */
static void shader_glsl_add_src_param_ext(const struct wined3d_shader_context *ctx,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, struct glsl_src_param *glsl_src,
        enum wined3d_data_type data_type);

/** Used for opcode modifiers - They multiply the result by the specified amount */
static const char * const shift_glsl_tab[] = {
    "",           /*  0 (none) */
    "2.0 * ",     /*  1 (x2)   */
    "4.0 * ",     /*  2 (x4)   */
    "8.0 * ",     /*  3 (x8)   */
    "16.0 * ",    /*  4 (x16)  */
    "32.0 * ",    /*  5 (x32)  */
    "",           /*  6 (x64)  */
    "",           /*  7 (x128) */
    "",           /*  8 (d256) */
    "",           /*  9 (d128) */
    "",           /* 10 (d64)  */
    "",           /* 11 (d32)  */
    "0.0625 * ",  /* 12 (d16)  */
    "0.125 * ",   /* 13 (d8)   */
    "0.25 * ",    /* 14 (d4)   */
    "0.5 * "      /* 15 (d2)   */
};

/* Generate a GLSL parameter that does the input modifier computation and return the input register/mask to use */
static void shader_glsl_gen_modifier(enum wined3d_shader_src_modifier src_modifier,
        const char *in_reg, const char *in_regswizzle, char *out_str)
{
    switch (src_modifier)
    {
    case WINED3DSPSM_DZ: /* Need to handle this in the instructions itself (texld & texcrd). */
    case WINED3DSPSM_DW:
    case WINED3DSPSM_NONE:
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NEG:
        sprintf(out_str, "-%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NOT:
        sprintf(out_str, "!%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_BIAS:
        sprintf(out_str, "(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_BIASNEG:
        sprintf(out_str, "-(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_SIGN:
        sprintf(out_str, "(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_SIGNNEG:
        sprintf(out_str, "-(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_COMP:
        sprintf(out_str, "(1.0 - %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2:
        sprintf(out_str, "(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2NEG:
        sprintf(out_str, "-(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_ABS:
        sprintf(out_str, "abs(%s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_ABSNEG:
        sprintf(out_str, "-abs(%s%s)", in_reg, in_regswizzle);
        break;
    default:
        FIXME("Unhandled modifier %u\n", src_modifier);
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
    }
}

static void shader_glsl_fixup_scalar_register_variable(struct wined3d_string_buffer *register_name,
        const char *glsl_variable, const struct wined3d_gl_info *gl_info)
{
    /* The ARB_shading_language_420pack extension allows swizzle operations on
     * scalars. */
    if (gl_info->supported[ARB_SHADING_LANGUAGE_420PACK])
        string_buffer_sprintf(register_name, "%s", glsl_variable);
    else
        string_buffer_sprintf(register_name, "ivec2(%s, 0)", glsl_variable);
}

/** Writes the GLSL variable name that corresponds to the register that the
 * DX opcode parameter is trying to access */
static void shader_glsl_get_register_name(const struct wined3d_shader_register *reg,
        enum wined3d_data_type data_type, struct wined3d_string_buffer *register_name,
        BOOL *is_swizzled, const struct wined3d_shader_context *ctx)
{
    /* oPos, oFog and oPts in D3D */
    static const char * const hwrastout_reg_names[] = {"vs_out[10]", "vs_out[11].x", "vs_out[11].y"};

    const struct wined3d_shader *shader = ctx->shader;
    const struct wined3d_shader_reg_maps *reg_maps = ctx->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    const char *prefix = shader_glsl_get_prefix(version->type);
    struct glsl_src_param rel_param0, rel_param1;

    if (reg->idx[0].offset != ~0u && reg->idx[0].rel_addr)
        shader_glsl_add_src_param_ext(ctx, reg->idx[0].rel_addr, WINED3DSP_WRITEMASK_0,
                &rel_param0, reg->idx[0].rel_addr->reg.data_type);
    if (reg->idx[1].offset != ~0u && reg->idx[1].rel_addr)
        shader_glsl_add_src_param_ext(ctx, reg->idx[1].rel_addr, WINED3DSP_WRITEMASK_0,
                &rel_param1, reg->idx[1].rel_addr->reg.data_type);
    if (is_swizzled)
        *is_swizzled = FALSE;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            string_buffer_sprintf(register_name, "R%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_INPUT:
        case WINED3DSPR_INCONTROLPOINT:
            if (version->type == WINED3D_SHADER_TYPE_VERTEX)
            {
                struct shader_glsl_ctx_priv *priv = ctx->backend_data;

                if (reg->idx[0].rel_addr)
                    FIXME("VS3 input registers relative addressing.\n");
                if (is_swizzled && priv->cur_vs_args->swizzle_map & (1u << reg->idx[0].offset))
                    *is_swizzled = TRUE;
                if (reg->idx[0].rel_addr)
                {
                    string_buffer_sprintf(register_name, "%s_in[%s + %u]",
                            prefix, rel_param0.param_str, reg->idx[0].offset);
                }
                else
                {
                    string_buffer_sprintf(register_name, "%s_in%u", prefix, reg->idx[0].offset);
                }
                break;
            }

            if (version->type == WINED3D_SHADER_TYPE_HULL
                    || version->type == WINED3D_SHADER_TYPE_DOMAIN
                    || version->type == WINED3D_SHADER_TYPE_GEOMETRY)
            {
                if (reg->idx[0].rel_addr)
                {
                    if (reg->idx[1].rel_addr)
                        string_buffer_sprintf(register_name, "shader_in[%s + %u].reg[%s + %u]",
                                rel_param0.param_str, reg->idx[0].offset,
                                rel_param1.param_str, reg->idx[1].offset);
                    else
                        string_buffer_sprintf(register_name, "shader_in[%s + %u].reg[%u]",
                                rel_param0.param_str, reg->idx[0].offset,
                                reg->idx[1].offset);
                }
                else if (reg->idx[1].rel_addr)
                    string_buffer_sprintf(register_name, "shader_in[%u].reg[%s + %u]", reg->idx[0].offset,
                            rel_param1.param_str, reg->idx[1].offset);
                else
                    string_buffer_sprintf(register_name, "shader_in[%u].reg[%u]",
                            reg->idx[0].offset, reg->idx[1].offset);
                break;
            }

            /* pixel shaders >= 3.0 */
            if (version->major >= 3)
            {
                DWORD idx = shader->u.ps.input_reg_map[reg->idx[0].offset];
                unsigned int in_count = vec4_varyings(version->major, gl_info);

                if (reg->idx[0].rel_addr)
                {
                    /* Removing a + 0 would be an obvious optimization, but
                     * OS X doesn't see the NOP operation there. */
                    if (idx)
                    {
                        if (needs_legacy_glsl_syntax(gl_info)
                                && shader->u.ps.declared_in_count > in_count)
                        {
                            string_buffer_sprintf(register_name,
                                    "((%s + %u) > %u ? (%s + %u) > %u ? gl_SecondaryColor : gl_Color : %s_in[%s + %u])",
                                    rel_param0.param_str, idx, in_count - 1, rel_param0.param_str, idx, in_count,
                                    prefix, rel_param0.param_str, idx);
                        }
                        else
                        {
                            string_buffer_sprintf(register_name, "%s_in[%s + %u]", prefix, rel_param0.param_str, idx);
                        }
                    }
                    else
                    {
                        if (needs_legacy_glsl_syntax(gl_info)
                                && shader->u.ps.declared_in_count > in_count)
                        {
                            string_buffer_sprintf(register_name,
                                    "((%s) > %u ? (%s) > %u ? gl_SecondaryColor : gl_Color : %s_in[%s])",
                                    rel_param0.param_str, in_count - 1, rel_param0.param_str, in_count,
                                    prefix, rel_param0.param_str);
                        }
                        else
                        {
                            string_buffer_sprintf(register_name, "%s_in[%s]", prefix, rel_param0.param_str);
                        }
                    }
                }
                else
                {
                    if (idx == in_count)
                        string_buffer_sprintf(register_name, "gl_Color");
                    else if (idx == in_count + 1)
                        string_buffer_sprintf(register_name, "gl_SecondaryColor");
                    else
                        string_buffer_sprintf(register_name, "%s_in[%u]", prefix, idx);
                }
            }
            else
            {
                if (!reg->idx[0].offset)
                    string_buffer_sprintf(register_name, "ffp_varying_diffuse");
                else
                    string_buffer_sprintf(register_name, "ffp_varying_specular");
                break;
            }
            break;

        case WINED3DSPR_CONST:
            {
                /* Relative addressing */
                if (reg->idx[0].rel_addr)
                {
                    if (wined3d_settings.check_float_constants)
                        string_buffer_sprintf(register_name,
                                "(%s + %u >= 0 && %s + %u < %u ? %s_c[%s + %u] : vec4(0.0))",
                                rel_param0.param_str, reg->idx[0].offset,
                                rel_param0.param_str, reg->idx[0].offset, shader->limits->constant_float,
                                prefix, rel_param0.param_str, reg->idx[0].offset);
                    else if (reg->idx[0].offset)
                        string_buffer_sprintf(register_name, "%s_c[%s + %u]",
                                prefix, rel_param0.param_str, reg->idx[0].offset);
                    else
                        string_buffer_sprintf(register_name, "%s_c[%s]", prefix, rel_param0.param_str);
                }
                else
                {
                    if (shader_constant_is_local(shader, reg->idx[0].offset))
                        string_buffer_sprintf(register_name, "%s_lc%u", prefix, reg->idx[0].offset);
                    else
                        string_buffer_sprintf(register_name, "%s_c[%u]", prefix, reg->idx[0].offset);
                }
            }
            break;

        case WINED3DSPR_CONSTINT:
            string_buffer_sprintf(register_name, "%s_i[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_CONSTBOOL:
            string_buffer_sprintf(register_name, "%s_b[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
            if (version->type == WINED3D_SHADER_TYPE_PIXEL)
                string_buffer_sprintf(register_name, "T%u", reg->idx[0].offset);
            else
                string_buffer_sprintf(register_name, "A%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_LOOP:
            string_buffer_sprintf(register_name, "aL%u", ctx->state->current_loop_reg - 1);
            break;

        case WINED3DSPR_SAMPLER:
            string_buffer_sprintf(register_name, "%s_sampler%u", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_COLOROUT:
            /* FIXME: should check dual_buffers when dual blending is enabled */
            if (reg->idx[0].offset >= gl_info->limits.buffers)
                WARN("Write to render target %u, only %d supported.\n",
                        reg->idx[0].offset, gl_info->limits.buffers);

            string_buffer_sprintf(register_name, "%s[%u]", get_fragment_output(gl_info), reg->idx[0].offset);
            break;

        case WINED3DSPR_RASTOUT:
            string_buffer_sprintf(register_name, "%s", hwrastout_reg_names[reg->idx[0].offset]);
            break;

        case WINED3DSPR_DEPTHOUT:
        case WINED3DSPR_DEPTHOUTGE:
        case WINED3DSPR_DEPTHOUTLE:
            string_buffer_sprintf(register_name, "gl_FragDepth");
            break;

        case WINED3DSPR_ATTROUT:
            if (!reg->idx[0].offset)
                string_buffer_sprintf(register_name, "%s_out[8]", prefix);
            else
                string_buffer_sprintf(register_name, "%s_out[9]", prefix);
            break;

        case WINED3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0: WINED3DSPR_OUTPUT */
            if (reg->idx[0].rel_addr)
                string_buffer_sprintf(register_name, "%s_out[%s + %u]",
                        prefix, rel_param0.param_str, reg->idx[0].offset);
            else
                string_buffer_sprintf(register_name, "%s_out[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_MISCTYPE:
            if (!reg->idx[0].offset)
            {
                /* vPos */
                string_buffer_sprintf(register_name, "vpos");
            }
            else if (reg->idx[0].offset == 1)
            {
                /* Note that gl_FrontFacing is a bool, while vFace is
                 * a float for which the sign determines front/back */
                string_buffer_sprintf(register_name, "(gl_FrontFacing ? 1.0 : -1.0)");
            }
            else
            {
                FIXME("Unhandled misctype register %u.\n", reg->idx[0].offset);
                string_buffer_sprintf(register_name, "unrecognized_register");
            }
            break;

        case WINED3DSPR_IMMCONST:
            switch (reg->immconst_type)
            {
                case WINED3D_IMMCONST_SCALAR:
                    switch (data_type)
                    {
                        case WINED3D_DATA_UNORM:
                        case WINED3D_DATA_SNORM:
                        case WINED3D_DATA_FLOAT:
                            string_buffer_clear(register_name);
                            shader_glsl_append_imm_vec(register_name, (const float *)reg->u.immconst_data, 1, gl_info);
                            break;
                        case WINED3D_DATA_INT:
                            string_buffer_sprintf(register_name, "%#x", reg->u.immconst_data[0]);
                            break;
                        case WINED3D_DATA_RESOURCE:
                        case WINED3D_DATA_SAMPLER:
                        case WINED3D_DATA_UINT:
                            string_buffer_sprintf(register_name, "%#xu", reg->u.immconst_data[0]);
                            break;
                        default:
                            string_buffer_sprintf(register_name, "<unhandled data type %#x>", data_type);
                            break;
                    }
                    break;

                case WINED3D_IMMCONST_VEC4:
                    switch (data_type)
                    {
                        case WINED3D_DATA_UNORM:
                        case WINED3D_DATA_SNORM:
                        case WINED3D_DATA_FLOAT:
                            string_buffer_clear(register_name);
                            shader_glsl_append_imm_vec(register_name, (const float *)reg->u.immconst_data, 4, gl_info);
                            break;
                        case WINED3D_DATA_INT:
                            string_buffer_sprintf(register_name, "ivec4(%#x, %#x, %#x, %#x)",
                                    reg->u.immconst_data[0], reg->u.immconst_data[1],
                                    reg->u.immconst_data[2], reg->u.immconst_data[3]);
                            break;
                        case WINED3D_DATA_RESOURCE:
                        case WINED3D_DATA_SAMPLER:
                        case WINED3D_DATA_UINT:
                            string_buffer_sprintf(register_name, "uvec4(%#xu, %#xu, %#xu, %#xu)",
                                    reg->u.immconst_data[0], reg->u.immconst_data[1],
                                    reg->u.immconst_data[2], reg->u.immconst_data[3]);
                            break;
                        default:
                            string_buffer_sprintf(register_name, "<unhandled data type %#x>", data_type);
                            break;
                    }
                    break;

                default:
                    FIXME("Unhandled immconst type %#x\n", reg->immconst_type);
                    string_buffer_sprintf(register_name, "<unhandled_immconst_type %#x>", reg->immconst_type);
            }
            break;

        case WINED3DSPR_CONSTBUFFER:
            if (reg->idx[1].rel_addr)
                string_buffer_sprintf(register_name, "%s_cb%u[%s + %u]",
                        prefix, reg->idx[0].offset, rel_param1.param_str, reg->idx[1].offset);
            else
                string_buffer_sprintf(register_name, "%s_cb%u[%u]", prefix, reg->idx[0].offset, reg->idx[1].offset);
            break;

        case WINED3DSPR_IMMCONSTBUFFER:
            if (reg->idx[0].rel_addr)
                string_buffer_sprintf(register_name, "%s_icb[%s + %u]",
                        prefix, rel_param0.param_str, reg->idx[0].offset);
            else
                string_buffer_sprintf(register_name, "%s_icb[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_PRIMID:
            if (version->type == WINED3D_SHADER_TYPE_GEOMETRY)
                string_buffer_sprintf(register_name, "gl_PrimitiveIDIn");
            else
                string_buffer_sprintf(register_name, "gl_PrimitiveID");
            break;

        case WINED3DSPR_IDXTEMP:
            if (reg->idx[1].rel_addr)
                string_buffer_sprintf(register_name, "X%u[%s + %u]",
                        reg->idx[0].offset, rel_param1.param_str, reg->idx[1].offset);
            else
                string_buffer_sprintf(register_name, "X%u[%u]", reg->idx[0].offset, reg->idx[1].offset);
            break;

        case WINED3DSPR_LOCALTHREADINDEX:
            shader_glsl_fixup_scalar_register_variable(register_name,
                    "int(gl_LocalInvocationIndex)", gl_info);
            break;

        case WINED3DSPR_GSINSTID:
        case WINED3DSPR_OUTPOINTID:
            shader_glsl_fixup_scalar_register_variable(register_name,
                    "gl_InvocationID", gl_info);
            break;

        case WINED3DSPR_THREADID:
            string_buffer_sprintf(register_name, "ivec3(gl_GlobalInvocationID)");
            break;

        case WINED3DSPR_THREADGROUPID:
            string_buffer_sprintf(register_name, "ivec3(gl_WorkGroupID)");
            break;

        case WINED3DSPR_LOCALTHREADID:
            string_buffer_sprintf(register_name, "ivec3(gl_LocalInvocationID)");
            break;

        case WINED3DSPR_FORKINSTID:
        case WINED3DSPR_JOININSTID:
            shader_glsl_fixup_scalar_register_variable(register_name,
                    "phase_instance_id", gl_info);
            break;

        case WINED3DSPR_TESSCOORD:
            string_buffer_sprintf(register_name, "gl_TessCoord");
            break;

        case WINED3DSPR_OUTCONTROLPOINT:
            if (reg->idx[0].rel_addr)
            {
                if (reg->idx[1].rel_addr)
                    string_buffer_sprintf(register_name, "shader_out[%s + %u].reg[%s + %u]",
                            rel_param0.param_str, reg->idx[0].offset,
                            rel_param1.param_str, reg->idx[1].offset);
                else
                    string_buffer_sprintf(register_name, "shader_out[%s + %u].reg[%u]",
                            rel_param0.param_str, reg->idx[0].offset,
                            reg->idx[1].offset);
            }
            else if (reg->idx[1].rel_addr)
            {
                string_buffer_sprintf(register_name, "shader_out[%u].reg[%s + %u]",
                        reg->idx[0].offset, rel_param1.param_str,
                        reg->idx[1].offset);
            }
            else
            {
                string_buffer_sprintf(register_name, "shader_out[%u].reg[%u]",
                        reg->idx[0].offset, reg->idx[1].offset);
            }
            break;

        case WINED3DSPR_PATCHCONST:
            if (version->type == WINED3D_SHADER_TYPE_HULL)
                string_buffer_sprintf(register_name, "hs_out[%u]", reg->idx[0].offset);
            else
                string_buffer_sprintf(register_name, "vpc[%u]", reg->idx[0].offset);
            break;

        case WINED3DSPR_COVERAGE:
            string_buffer_sprintf(register_name, "gl_SampleMaskIn[0]");
            break;

        case WINED3DSPR_SAMPLEMASK:
            string_buffer_sprintf(register_name, "sample_mask");
            break;

        default:
            FIXME("Unhandled register type %#x.\n", reg->type);
            string_buffer_sprintf(register_name, "unrecognised_register");
            break;
    }
}

static void shader_glsl_write_mask_to_str(DWORD write_mask, char *str)
{
    *str++ = '.';
    if (write_mask & WINED3DSP_WRITEMASK_0) *str++ = 'x';
    if (write_mask & WINED3DSP_WRITEMASK_1) *str++ = 'y';
    if (write_mask & WINED3DSP_WRITEMASK_2) *str++ = 'z';
    if (write_mask & WINED3DSP_WRITEMASK_3) *str++ = 'w';
    *str = '\0';
}

/* Get the GLSL write mask for the destination register */
static DWORD shader_glsl_get_write_mask(const struct wined3d_shader_dst_param *param, char *write_mask)
{
    DWORD mask = param->write_mask;

    if (shader_is_scalar(&param->reg))
    {
        mask = WINED3DSP_WRITEMASK_0;
        *write_mask = '\0';
    }
    else
    {
        shader_glsl_write_mask_to_str(mask, write_mask);
    }

    return mask;
}

static unsigned int shader_glsl_get_write_mask_size(DWORD write_mask)
{
    unsigned int size = 0;

    if (write_mask & WINED3DSP_WRITEMASK_0) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_1) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_2) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_3) ++size;

    return size;
}

static unsigned int shader_glsl_swizzle_get_component(DWORD swizzle,
        unsigned int component_idx)
{
    /* swizzle bits fields: wwzzyyxx */
    return (swizzle >> (2 * component_idx)) & 0x3;
}

static void shader_glsl_swizzle_to_str(DWORD swizzle, BOOL fixup, DWORD mask, char *str)
{
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";
    unsigned int i;

    *str++ = '.';
    for (i = 0; i < 4; ++i)
    {
        if (mask & (WINED3DSP_WRITEMASK_0 << i))
            *str++ = swizzle_chars[shader_glsl_swizzle_get_component(swizzle, i)];
    }
    *str = '\0';
}

static void shader_glsl_get_swizzle(const struct wined3d_shader_src_param *param,
        BOOL fixup, DWORD mask, char *swizzle_str)
{
    if (shader_is_scalar(&param->reg))
        *swizzle_str = '\0';
    else
        shader_glsl_swizzle_to_str(param->swizzle, fixup, mask, swizzle_str);
}

static void shader_glsl_sprintf_cast(struct wined3d_string_buffer *dst_param, const char *src_param,
        enum wined3d_data_type dst_data_type, enum wined3d_data_type src_data_type)
{
    if (dst_data_type == src_data_type)
    {
        string_buffer_sprintf(dst_param, "%s", src_param);
        return;
    }

    if (src_data_type == WINED3D_DATA_FLOAT)
    {
        switch (dst_data_type)
        {
            case WINED3D_DATA_INT:
                string_buffer_sprintf(dst_param, "floatBitsToInt(%s)", src_param);
                return;
            case WINED3D_DATA_RESOURCE:
            case WINED3D_DATA_SAMPLER:
            case WINED3D_DATA_UINT:
                string_buffer_sprintf(dst_param, "floatBitsToUint(%s)", src_param);
                return;
            default:
                break;
        }
    }

    if (src_data_type == WINED3D_DATA_UINT && dst_data_type == WINED3D_DATA_FLOAT)
    {
        string_buffer_sprintf(dst_param, "uintBitsToFloat(%s)", src_param);
        return;
    }

    if (src_data_type == WINED3D_DATA_INT && dst_data_type == WINED3D_DATA_FLOAT)
    {
        string_buffer_sprintf(dst_param, "intBitsToFloat(%s)", src_param);
        return;
    }

    FIXME("Unhandled cast from %#x to %#x.\n", src_data_type, dst_data_type);
    string_buffer_sprintf(dst_param, "%s", src_param);
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static void shader_glsl_add_src_param_ext(const struct wined3d_shader_context *ctx,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, struct glsl_src_param *glsl_src,
        enum wined3d_data_type data_type)
{
    struct shader_glsl_ctx_priv *priv = ctx->backend_data;
    struct wined3d_string_buffer *param_str = string_buffer_get(priv->string_buffers);
    struct wined3d_string_buffer *reg_name = string_buffer_get(priv->string_buffers);
    enum wined3d_data_type param_data_type;
    BOOL is_color = FALSE;
    char swizzle_str[6];

    glsl_src->param_str[0] = '\0';
    swizzle_str[0] = '\0';

    shader_glsl_get_register_name(&wined3d_src->reg, data_type, reg_name, &is_color, ctx);
    shader_glsl_get_swizzle(wined3d_src, is_color, mask, swizzle_str);

    switch (wined3d_src->reg.type)
    {
        case WINED3DSPR_IMMCONST:
            param_data_type = data_type;
            break;
        case WINED3DSPR_FORKINSTID:
        case WINED3DSPR_GSINSTID:
        case WINED3DSPR_JOININSTID:
        case WINED3DSPR_LOCALTHREADID:
        case WINED3DSPR_LOCALTHREADINDEX:
        case WINED3DSPR_OUTPOINTID:
        case WINED3DSPR_PRIMID:
        case WINED3DSPR_THREADGROUPID:
        case WINED3DSPR_THREADID:
            param_data_type = WINED3D_DATA_INT;
            break;
        default:
            param_data_type = WINED3D_DATA_FLOAT;
            break;
    }

    shader_glsl_sprintf_cast(param_str, reg_name->buffer, data_type, param_data_type);
    shader_glsl_gen_modifier(wined3d_src->modifiers, param_str->buffer, swizzle_str, glsl_src->param_str);

    string_buffer_release(priv->string_buffers, reg_name);
    string_buffer_release(priv->string_buffers, param_str);
}

static void shader_glsl_add_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, struct glsl_src_param *glsl_src)
{
    shader_glsl_add_src_param_ext(ins->ctx, wined3d_src, mask, glsl_src, wined3d_src->reg.data_type);
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static DWORD shader_glsl_add_dst_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_dst_param *wined3d_dst, struct glsl_dst_param *glsl_dst)
{
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *reg_name;
    size_t len;

    glsl_dst->mask_str[0] = '\0';

    reg_name = string_buffer_get(priv->string_buffers);
    shader_glsl_get_register_name(&wined3d_dst->reg, wined3d_dst->reg.data_type, reg_name, NULL, ins->ctx);
    len = min(reg_name->content_size, ARRAY_SIZE(glsl_dst->reg_name) - 1);
    memcpy(glsl_dst->reg_name, reg_name->buffer, len);
    glsl_dst->reg_name[len] = '\0';
    string_buffer_release(priv->string_buffers, reg_name);

    return shader_glsl_get_write_mask(wined3d_dst, glsl_dst->mask_str);
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst_ext(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_instruction *ins, const struct wined3d_shader_dst_param *dst,
        enum wined3d_data_type data_type)
{
    struct glsl_dst_param glsl_dst;
    DWORD mask;

    if ((mask = shader_glsl_add_dst_param(ins, dst, &glsl_dst)))
    {
        switch (data_type)
        {
            case WINED3D_DATA_FLOAT:
                shader_addline(buffer, "%s%s = %s(",
                        glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);
                break;
            case WINED3D_DATA_INT:
                shader_addline(buffer, "%s%s = %sintBitsToFloat(",
                        glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);
                break;
            case WINED3D_DATA_RESOURCE:
            case WINED3D_DATA_SAMPLER:
            case WINED3D_DATA_UINT:
                shader_addline(buffer, "%s%s = %suintBitsToFloat(",
                        glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);
                break;
            default:
                FIXME("Unhandled data type %#x.\n", data_type);
                shader_addline(buffer, "%s%s = %s(",
                        glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);
                break;
        }
    }

    return mask;
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst(struct wined3d_string_buffer *buffer, const struct wined3d_shader_instruction *ins)
{
    return shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], ins->dst[0].reg.data_type);
}

/** Process GLSL instruction modifiers */
static void shader_glsl_add_instruction_modifiers(const struct wined3d_shader_instruction *ins)
{
    struct glsl_dst_param dst_param;
    DWORD modifiers;

    if (!ins->dst_count) return;

    modifiers = ins->dst[0].modifiers;
    if (!modifiers) return;

    shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);

    if (modifiers & WINED3DSPDM_SATURATE)
    {
        /* _SAT means to clamp the value of the register to between 0 and 1 */
        shader_addline(ins->ctx->buffer, "%s%s = clamp(%s%s, 0.0, 1.0);\n", dst_param.reg_name,
                dst_param.mask_str, dst_param.reg_name, dst_param.mask_str);
    }

    if (modifiers & WINED3DSPDM_MSAMPCENTROID)
    {
        FIXME("_centroid modifier not handled\n");
    }

    if (modifiers & WINED3DSPDM_PARTIALPRECISION)
    {
        /* MSDN says this modifier can be safely ignored, so that's what we'll do. */
    }
}

static const char *shader_glsl_get_rel_op(enum wined3d_shader_rel_op op)
{
    switch (op)
    {
        case WINED3D_SHADER_REL_OP_GT: return ">";
        case WINED3D_SHADER_REL_OP_EQ: return "==";
        case WINED3D_SHADER_REL_OP_GE: return ">=";
        case WINED3D_SHADER_REL_OP_LT: return "<";
        case WINED3D_SHADER_REL_OP_NE: return "!=";
        case WINED3D_SHADER_REL_OP_LE: return "<=";
        default:
            FIXME("Unrecognized operator %#x.\n", op);
            return "(\?\?)";
    }
}

static BOOL shader_glsl_has_core_grad(const struct wined3d_gl_info *gl_info)
{
    return shader_glsl_get_version(gl_info) >= 130 || gl_info->supported[EXT_GPU_SHADER4];
}

static void shader_glsl_get_coord_size(enum wined3d_shader_resource_type resource_type,
        unsigned int *coord_size, unsigned int *deriv_size)
{
    const BOOL is_array = resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY
            || resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY;

    *coord_size = resource_type_info[resource_type].coord_size;
    *deriv_size = *coord_size;
    if (is_array)
        --(*deriv_size);
}

static void shader_glsl_get_sample_function(const struct wined3d_shader_context *ctx,
        DWORD resource_idx, DWORD sampler_idx, DWORD flags, struct glsl_sample_function *sample_function)
{
    enum wined3d_shader_resource_type resource_type;
    struct shader_glsl_ctx_priv *priv = ctx->backend_data;
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    BOOL shadow = glsl_is_shadow_sampler(ctx->shader, priv->cur_ps_args, resource_idx, sampler_idx);
    BOOL projected = flags & WINED3D_GLSL_SAMPLE_PROJECTED;
    BOOL texrect = ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            && priv->cur_ps_args->np2_fixup & (1u << resource_idx)
            && gl_info->supported[ARB_TEXTURE_RECTANGLE];
    BOOL lod = flags & WINED3D_GLSL_SAMPLE_LOD;
    BOOL grad = flags & WINED3D_GLSL_SAMPLE_GRAD;
    BOOL offset = flags & WINED3D_GLSL_SAMPLE_OFFSET;
    const char *base = "texture", *type_part = "", *suffix = "";
    unsigned int coord_size, deriv_size;

    resource_type = ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            ? pixelshader_get_resource_type(ctx->reg_maps, resource_idx, priv->cur_ps_args->tex_types)
            : ctx->reg_maps->resource_info[resource_idx].type;

    sample_function->data_type = ctx->reg_maps->resource_info[resource_idx].data_type;

    if (resource_type >= ARRAY_SIZE(resource_type_info))
    {
        ERR("Unexpected resource type %#x.\n", resource_type);
        resource_type = WINED3D_SHADER_RESOURCE_TEXTURE_2D;
    }

    /* Note that there's no such thing as a projected cube texture. */
    if (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_CUBE)
        projected = FALSE;

    if (needs_legacy_glsl_syntax(gl_info))
    {
        if (shadow)
            base = "shadow";

        type_part = resource_type_info[resource_type].type_part;
        if (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2D && texrect)
            type_part = "2DRect";
        if (!type_part[0] && resource_type != WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY)
            FIXME("Unhandled resource type %#x.\n", resource_type);

        if (!lod && grad && !shader_glsl_has_core_grad(gl_info))
        {
            if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                suffix = "ARB";
            else
                FIXME("Unsupported grad function.\n");
        }
    }

    if (flags & WINED3D_GLSL_SAMPLE_LOAD)
    {
        static const DWORD texel_fetch_flags = WINED3D_GLSL_SAMPLE_LOAD | WINED3D_GLSL_SAMPLE_OFFSET;
        if (flags & ~texel_fetch_flags)
            ERR("Unexpected flags %#x for texelFetch.\n", flags & ~texel_fetch_flags);

        base = "texelFetch";
        type_part = "";
    }

    sample_function->name = string_buffer_get(priv->string_buffers);
    string_buffer_sprintf(sample_function->name, "%s%s%s%s%s%s", base, type_part, projected ? "Proj" : "",
            lod ? "Lod" : grad ? "Grad" : "", offset ? "Offset" : "", suffix);

    shader_glsl_get_coord_size(resource_type, &coord_size, &deriv_size);
    if (shadow)
        ++coord_size;
    sample_function->offset_size = offset ? deriv_size : 0;
    sample_function->coord_mask = (1u << coord_size) - 1;
    sample_function->deriv_mask = (1u << deriv_size) - 1;
    sample_function->output_single_component = shadow && !needs_legacy_glsl_syntax(gl_info);
}

static void shader_glsl_release_sample_function(const struct wined3d_shader_context *ctx,
        struct glsl_sample_function *sample_function)
{
    const struct shader_glsl_ctx_priv *priv = ctx->backend_data;

    string_buffer_release(priv->string_buffers, sample_function->name);
}

static void shader_glsl_append_fixup_arg(char *arguments, const char *reg_name,
        BOOL sign_fixup, enum fixup_channel_source channel_source)
{
    switch(channel_source)
    {
        case CHANNEL_SOURCE_ZERO:
            strcat(arguments, "0.0");
            break;

        case CHANNEL_SOURCE_ONE:
            strcat(arguments, "1.0");
            break;

        case CHANNEL_SOURCE_X:
            strcat(arguments, reg_name);
            strcat(arguments, ".x");
            break;

        case CHANNEL_SOURCE_Y:
            strcat(arguments, reg_name);
            strcat(arguments, ".y");
            break;

        case CHANNEL_SOURCE_Z:
            strcat(arguments, reg_name);
            strcat(arguments, ".z");
            break;

        case CHANNEL_SOURCE_W:
            strcat(arguments, reg_name);
            strcat(arguments, ".w");
            break;

        default:
            FIXME("Unhandled channel source %#x\n", channel_source);
            strcat(arguments, "undefined");
            break;
    }

    if (sign_fixup) strcat(arguments, " * 2.0 - 1.0");
}

static void shader_glsl_color_correction_ext(struct wined3d_string_buffer *buffer,
        const char *reg_name, DWORD mask, struct color_fixup_desc fixup)
{
    unsigned int mask_size, remaining;
    DWORD fixup_mask = 0;
    char arguments[256];
    char mask_str[6];

    if (fixup.x_sign_fixup || fixup.x_source != CHANNEL_SOURCE_X) fixup_mask |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_sign_fixup || fixup.y_source != CHANNEL_SOURCE_Y) fixup_mask |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_sign_fixup || fixup.z_source != CHANNEL_SOURCE_Z) fixup_mask |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_sign_fixup || fixup.w_source != CHANNEL_SOURCE_W) fixup_mask |= WINED3DSP_WRITEMASK_3;
    if (!(mask &= fixup_mask))
        return;

    if (is_complex_fixup(fixup))
    {
        enum complex_fixup complex_fixup = get_complex_fixup(fixup);
        FIXME("Complex fixup (%#x) not supported\n",complex_fixup);
        return;
    }

    shader_glsl_write_mask_to_str(mask, mask_str);
    mask_size = shader_glsl_get_write_mask_size(mask);

    arguments[0] = '\0';
    remaining = mask_size;
    if (mask & WINED3DSP_WRITEMASK_0)
    {
        shader_glsl_append_fixup_arg(arguments, reg_name, fixup.x_sign_fixup, fixup.x_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_1)
    {
        shader_glsl_append_fixup_arg(arguments, reg_name, fixup.y_sign_fixup, fixup.y_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_2)
    {
        shader_glsl_append_fixup_arg(arguments, reg_name, fixup.z_sign_fixup, fixup.z_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_3)
    {
        shader_glsl_append_fixup_arg(arguments, reg_name, fixup.w_sign_fixup, fixup.w_source);
        if (--remaining) strcat(arguments, ", ");
    }

    if (mask_size > 1)
        shader_addline(buffer, "%s%s = vec%u(%s);\n", reg_name, mask_str, mask_size, arguments);
    else
        shader_addline(buffer, "%s%s = %s;\n", reg_name, mask_str, arguments);
}

static void shader_glsl_color_correction(const struct wined3d_shader_instruction *ins, struct color_fixup_desc fixup)
{
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *reg_name;

    reg_name = string_buffer_get(priv->string_buffers);
    shader_glsl_get_register_name(&ins->dst[0].reg, ins->dst[0].reg.data_type, reg_name, NULL, ins->ctx);
    shader_glsl_color_correction_ext(ins->ctx->buffer, reg_name->buffer, ins->dst[0].write_mask, fixup);
    string_buffer_release(priv->string_buffers, reg_name);
}

static void PRINTF_ATTR(9, 10) shader_glsl_gen_sample_code(const struct wined3d_shader_instruction *ins,
        unsigned int sampler_bind_idx, const struct glsl_sample_function *sample_function, DWORD swizzle,
        const char *dx, const char *dy, const char *bias, const struct wined3d_shader_texel_offset *offset,
        const char *coord_reg_fmt, ...)
{
    const struct wined3d_shader_version *version = &ins->ctx->reg_maps->shader_version;
    char dst_swizzle[6];
    struct color_fixup_desc fixup;
    BOOL np2_fixup = FALSE;
    va_list args;
    int ret;

    shader_glsl_swizzle_to_str(swizzle, FALSE, ins->dst[0].write_mask, dst_swizzle);

    /* If ARB_texture_swizzle is supported we don't need to do anything here.
     * We actually rely on it for vertex shaders and SM4+. */
    if (version->type == WINED3D_SHADER_TYPE_PIXEL && version->major < 4)
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        fixup = priv->cur_ps_args->color_fixup[sampler_bind_idx];

        if (priv->cur_ps_args->np2_fixup & (1u << sampler_bind_idx))
            np2_fixup = TRUE;
    }
    else
    {
        fixup = COLOR_FIXUP_IDENTITY;
    }

    shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &ins->dst[0], sample_function->data_type);

    if (sample_function->output_single_component)
        shader_addline(ins->ctx->buffer, "vec4(");

    shader_addline(ins->ctx->buffer, "%s(%s_sampler%u, ",
            sample_function->name->buffer, shader_glsl_get_prefix(version->type), sampler_bind_idx);

    for (;;)
    {
        va_start(args, coord_reg_fmt);
        ret = shader_vaddline(ins->ctx->buffer, coord_reg_fmt, args);
        va_end(args);
        if (!ret)
            break;
        if (!string_buffer_resize(ins->ctx->buffer, ret))
            break;
    }

    if (np2_fixup)
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        const unsigned char idx = priv->cur_np2fixup_info->idx[sampler_bind_idx];

        switch (shader_glsl_get_write_mask_size(sample_function->coord_mask))
        {
            case 1:
                shader_addline(ins->ctx->buffer, " * ps_samplerNP2Fixup[%u].%s",
                        idx >> 1, (idx % 2) ? "z" : "x");
                break;
            case 2:
                shader_addline(ins->ctx->buffer, " * ps_samplerNP2Fixup[%u].%s",
                        idx >> 1, (idx % 2) ? "zw" : "xy");
                break;
            case 3:
                shader_addline(ins->ctx->buffer, " * vec3(ps_samplerNP2Fixup[%u].%s, 1.0)",
                        idx >> 1, (idx % 2) ? "zw" : "xy");
                break;
            case 4:
                shader_addline(ins->ctx->buffer, " * vec4(ps_samplerNP2Fixup[%u].%s, 1.0, 1.0)",
                        idx >> 1, (idx % 2) ? "zw" : "xy");
                break;
        }
    }
    if (dx && dy)
        shader_addline(ins->ctx->buffer, ", %s, %s", dx, dy);
    else if (bias)
        shader_addline(ins->ctx->buffer, ", %s", bias);
    if (sample_function->offset_size)
    {
        int offset_immdata[4] = {offset->u, offset->v, offset->w};
        shader_addline(ins->ctx->buffer, ", ");
        shader_glsl_append_imm_ivec(ins->ctx->buffer, offset_immdata, sample_function->offset_size);
    }
    shader_addline(ins->ctx->buffer, ")");

    if (sample_function->output_single_component)
        shader_addline(ins->ctx->buffer, ")");

    shader_addline(ins->ctx->buffer, "%s);\n", dst_swizzle);

    if (!is_identity_fixup(fixup))
        shader_glsl_color_correction(ins, fixup);
}

static void shader_glsl_fixup_position(struct wined3d_string_buffer *buffer, BOOL use_viewport_index)
{
    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while D3D coords
     * specify the corner. The offsets are stored in z and w in
     * pos_fixup. pos_fixup.y contains 1.0 or -1.0 to turn the rendering
     * upside down for offscreen rendering. pos_fixup.x contains 1.0 to allow
     * a MAD. */
    if (use_viewport_index)
    {
        shader_addline(buffer, "gl_Position.y = gl_Position.y * pos_fixup[gl_ViewportIndex].y;\n");
        shader_addline(buffer, "gl_Position.xy += pos_fixup[gl_ViewportIndex].zw * gl_Position.ww;\n");
    }
    else
    {
        shader_addline(buffer, "gl_Position.y = gl_Position.y * pos_fixup.y;\n");
        shader_addline(buffer, "gl_Position.xy += pos_fixup.zw * gl_Position.ww;\n");
    }

    /* Z coord [0;1]->[-1;1] mapping, see comment in get_projection_matrix()
     * in utils.c
     *
     * Basically we want (in homogeneous coordinates) z = z * 2 - 1. However,
     * shaders are run before the homogeneous divide, so we have to take the w
     * into account: z = ((z / w) * 2 - 1) * w, which is the same as
     * z = z * 2 - w. */
    shader_addline(buffer, "gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");
}

/*****************************************************************************
 * Begin processing individual instruction opcodes
 ****************************************************************************/

static void shader_glsl_binop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD write_mask;
    const char *op;

    /* Determine the GLSL operator to use based on the opcode */
    switch (ins->handler_idx)
    {
        case WINED3DSIH_ADD:  op = "+";  break;
        case WINED3DSIH_AND:  op = "&";  break;
        case WINED3DSIH_DIV:  op = "/";  break;
        case WINED3DSIH_IADD: op = "+";  break;
        case WINED3DSIH_ISHL: op = "<<"; break;
        case WINED3DSIH_ISHR: op = ">>"; break;
        case WINED3DSIH_MUL:  op = "*";  break;
        case WINED3DSIH_OR:   op = "|";  break;
        case WINED3DSIH_SUB:  op = "-";  break;
        case WINED3DSIH_USHR: op = ">>"; break;
        case WINED3DSIH_XOR:  op = "^";  break;
        default:
            op = "<unhandled operator>";
            FIXME("Opcode %s not yet handled in GLSL.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
    if (wined3d_settings.multiply_special == 2 && ins->ctx->reg_maps->shader_version.major < 4
            && ins->handler_idx == WINED3DSIH_MUL)
        shader_addline(buffer, "mul%d(%s, %s));\n", shader_glsl_get_write_mask_size(write_mask),
                src0_param.param_str, src1_param.param_str);
    else
        shader_addline(buffer, "%s %s %s);\n", src0_param.param_str, op, src1_param.param_str);
}

static void shader_glsl_relop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    unsigned int mask_size;
    DWORD write_mask;
    const char *op;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);

    if (mask_size > 1)
    {
        switch (ins->handler_idx)
        {
            case WINED3DSIH_EQ:  op = "equal"; break;
            case WINED3DSIH_IEQ: op = "equal"; break;
            case WINED3DSIH_GE:  op = "greaterThanEqual"; break;
            case WINED3DSIH_IGE: op = "greaterThanEqual"; break;
            case WINED3DSIH_UGE: op = "greaterThanEqual"; break;
            case WINED3DSIH_LT:  op = "lessThan"; break;
            case WINED3DSIH_ILT: op = "lessThan"; break;
            case WINED3DSIH_ULT: op = "lessThan"; break;
            case WINED3DSIH_NE:  op = "notEqual"; break;
            case WINED3DSIH_INE: op = "notEqual"; break;
            default:
                op = "<unhandled operator>";
                ERR("Unhandled opcode %#x.\n", ins->handler_idx);
                break;
        }

        shader_addline(buffer, "uvec%u(%s(%s, %s)) * 0xffffffffu);\n",
                mask_size, op, src0_param.param_str, src1_param.param_str);
    }
    else
    {
        switch (ins->handler_idx)
        {
            case WINED3DSIH_EQ:  op = "=="; break;
            case WINED3DSIH_IEQ: op = "=="; break;
            case WINED3DSIH_GE:  op = ">="; break;
            case WINED3DSIH_IGE: op = ">="; break;
            case WINED3DSIH_UGE: op = ">="; break;
            case WINED3DSIH_LT:  op = "<"; break;
            case WINED3DSIH_ILT: op = "<"; break;
            case WINED3DSIH_ULT: op = "<"; break;
            case WINED3DSIH_NE:  op = "!="; break;
            case WINED3DSIH_INE: op = "!="; break;
            default:
                op = "<unhandled operator>";
                ERR("Unhandled opcode %#x.\n", ins->handler_idx);
                break;
        }

        shader_addline(buffer, "%s %s %s ? 0xffffffffu : 0u);\n",
                src0_param.param_str, op, src1_param.param_str);
    }
}

static void shader_glsl_unary_op(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src_param;
    DWORD write_mask;
    const char *op;

    switch (ins->handler_idx)
    {
        case WINED3DSIH_INEG: op = "-"; break;
        case WINED3DSIH_NOT:  op = "~"; break;
        default:
            op = "<unhandled operator>";
            ERR("Unhandled opcode %s.\n",
                    debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);
    shader_addline(ins->ctx->buffer, "%s%s);\n", op, src_param.param_str);
}

static void shader_glsl_mul_extended(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD write_mask;

    /* If we have ARB_gpu_shader5, we can use imulExtended() / umulExtended().
     * If not, we can emulate it. */
    if (ins->dst[0].reg.type != WINED3DSPR_NULL)
        FIXME("64-bit integer multiplies not implemented.\n");

    if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);

        shader_addline(ins->ctx->buffer, "%s * %s);\n",
                src0_param.param_str, src1_param.param_str);
    }
}

static void shader_glsl_udiv(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param, src1_param;
    DWORD write_mask;

    if (ins->dst[0].reg.type != WINED3DSPR_NULL)
    {
        if (ins->dst[1].reg.type != WINED3DSPR_NULL)
        {
            char dst_mask[6];

            write_mask = shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "tmp0%s = uintBitsToFloat(%s / %s);\n",
                    dst_mask, src0_param.param_str, src1_param.param_str);

            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "%s %% %s);\n", src0_param.param_str, src1_param.param_str);

            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], WINED3D_DATA_FLOAT);
            shader_addline(buffer, "tmp0%s);\n", dst_mask);
        }
        else
        {
            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], ins->dst[0].reg.data_type);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "%s / %s);\n", src0_param.param_str, src1_param.param_str);
        }
    }
    else if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_addline(buffer, "%s %% %s);\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Process the WINED3DSIO_MOV opcode using GLSL (dst = src) */
static void shader_glsl_mov(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);

    /* In vs_1_1 WINED3DSIO_MOV can write to the address register. In later
     * shader versions WINED3DSIO_MOVA is used for this. */
    if (ins->ctx->reg_maps->shader_version.major == 1
            && ins->ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_VERTEX
            && ins->dst[0].reg.type == WINED3DSPR_ADDR)
    {
        /* This is a simple floor() */
        unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
        if (mask_size > 1) {
            shader_addline(buffer, "ivec%d(floor(%s)));\n", mask_size, src0_param.param_str);
        } else {
            shader_addline(buffer, "int(floor(%s)));\n", src0_param.param_str);
        }
    }
    else if (ins->handler_idx == WINED3DSIH_MOVA)
    {
        unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);

        if (shader_glsl_get_version(gl_info) >= 130 || gl_info->supported[EXT_GPU_SHADER4])
        {
            if (mask_size > 1)
                shader_addline(buffer, "ivec%d(round(%s)));\n", mask_size, src0_param.param_str);
            else
                shader_addline(buffer, "int(round(%s)));\n", src0_param.param_str);
        }
        else
        {
            if (mask_size > 1)
                shader_addline(buffer, "ivec%d(floor(abs(%s) + vec%d(0.5)) * sign(%s)));\n",
                        mask_size, src0_param.param_str, mask_size, src0_param.param_str);
            else
                shader_addline(buffer, "int(floor(abs(%s) + 0.5) * sign(%s)));\n",
                        src0_param.param_str, src0_param.param_str);
        }
    }
    else
    {
        shader_addline(buffer, "%s);\n", src0_param.param_str);
    }
}

/* Process the dot product operators DP3 and DP4 in GLSL (dst = dot(src0, src1)) */
static void shader_glsl_dot(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD dst_write_mask, src_write_mask;
    unsigned int dst_size, src_size;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    /* dp4 works on vec4, dp3 on vec3, etc. */
    if (ins->handler_idx == WINED3DSIH_DP4)
    {
        src_write_mask = WINED3DSP_WRITEMASK_ALL;
        src_size = 4;
    }
    else if (ins->handler_idx == WINED3DSIH_DP3)
    {
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
        src_size = 3;
    }
    else
    {
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;
        src_size = 2;
    }
    shader_glsl_add_src_param(ins, &ins->src[0], src_write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_write_mask, &src1_param);

    if (dst_size > 1)
    {
        if (wined3d_settings.multiply_special == 2 && ins->ctx->reg_maps->shader_version.major < 4)
            shader_addline(buffer, "vec%d(dot%d(%s, %s)));\n", dst_size, src_size,
                    src0_param.param_str, src1_param.param_str);
        else
            shader_addline(buffer, "vec%d(dot(%s, %s)));\n", dst_size,
                    src0_param.param_str, src1_param.param_str);
    }
    else
    {
        if (wined3d_settings.multiply_special == 2 && ins->ctx->reg_maps->shader_version.major < 4)
            shader_addline(buffer, "dot%d(%s, %s));\n", src_size, src0_param.param_str, src1_param.param_str);
        else
            shader_addline(buffer, "dot(%s, %s));\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Note that this instruction has some restrictions. The destination write mask
 * can't contain the w component, and the source swizzles have to be .xyzw */
static void shader_glsl_cross(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    char dst_mask[6];

    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_mask, &src1_param);
    shader_addline(ins->ctx->buffer, "cross(%s, %s)%s);\n", src0_param.param_str, src1_param.param_str, dst_mask);
}

static void shader_glsl_cut(const struct wined3d_shader_instruction *ins)
{
    unsigned int stream = ins->handler_idx == WINED3DSIH_CUT ? 0 : ins->src[0].reg.idx[0].offset;

    if (!stream)
        shader_addline(ins->ctx->buffer, "EndPrimitive();\n");
    else
        FIXME("Unhandled primitive stream %u.\n", stream);
}

/* Process the WINED3DSIO_POW instruction in GLSL (dst = |src0|^src1)
 * Src0 and src1 are scalars. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_pow(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    static const float max_float = FLT_MAX;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD dst_write_mask;
    unsigned int dst_size;
    BOOL guard_inf;

    guard_inf = wined3d_settings.multiply_special == 1 && ins->ctx->reg_maps->shader_version.major < 4;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    if (dst_size > 1)
    {
        if (guard_inf)
        {
            shader_addline(buffer, "vec%u(%s == 0.0 ? 1.0 : min(pow(abs(%s), %s), ",
                    dst_size, src1_param.param_str, src0_param.param_str, src1_param.param_str);
            shader_glsl_append_imm_vec(buffer, &max_float, 1, ins->ctx->gl_info);
            shader_addline(buffer, "));\n");
        }
        else
        {
            shader_addline(buffer, "vec%u(%s == 0.0 ? 1.0 : pow(abs(%s), %s)));\n",
                    dst_size, src1_param.param_str, src0_param.param_str, src1_param.param_str);
        }
    }
    else
    {
        if (guard_inf)
        {
            shader_addline(buffer, "%s == 0.0 ? 1.0 : min(pow(abs(%s), %s), ",
                    src1_param.param_str, src0_param.param_str, src1_param.param_str);
            shader_glsl_append_imm_vec(buffer, &max_float, 1, ins->ctx->gl_info);
            shader_addline(buffer, "));\n");
        }
        else
        {
            shader_addline(buffer, "%s == 0.0 ? 1.0 : pow(abs(%s), %s));\n",
                    src1_param.param_str, src0_param.param_str, src1_param.param_str);
        }
    }
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
static void shader_glsl_map2gl(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    const char *instruction;
    DWORD write_mask;
    unsigned i;

    /* Determine the GLSL function to use based on the opcode */
    /* TODO: Possibly make this a table for faster lookups */
    switch (ins->handler_idx)
    {
        case WINED3DSIH_ABS: instruction = "abs"; break;
        case WINED3DSIH_BFREV: instruction = "bitfieldReverse"; break;
        case WINED3DSIH_COUNTBITS: instruction = "bitCount"; break;
        case WINED3DSIH_DSX: instruction = "dFdx"; break;
        case WINED3DSIH_DSX_COARSE: instruction = "dFdxCoarse"; break;
        case WINED3DSIH_DSX_FINE: instruction = "dFdxFine"; break;
        case WINED3DSIH_DSY: instruction = "ycorrection.y * dFdy"; break;
        case WINED3DSIH_DSY_COARSE: instruction = "ycorrection.y * dFdyCoarse"; break;
        case WINED3DSIH_DSY_FINE: instruction = "ycorrection.y * dFdyFine"; break;
        case WINED3DSIH_FIRSTBIT_HI: instruction = "findMSB"; break;
        case WINED3DSIH_FIRSTBIT_LO: instruction = "findLSB"; break;
        case WINED3DSIH_FIRSTBIT_SHI: instruction = "findMSB"; break;
        case WINED3DSIH_FRC: instruction = "fract"; break;
        case WINED3DSIH_IMAX: instruction = "max"; break;
        case WINED3DSIH_IMIN: instruction = "min"; break;
        case WINED3DSIH_MAX: instruction = "max"; break;
        case WINED3DSIH_MIN: instruction = "min"; break;
        case WINED3DSIH_ROUND_NE: instruction = "roundEven"; break;
        case WINED3DSIH_ROUND_NI: instruction = "floor"; break;
        case WINED3DSIH_ROUND_PI: instruction = "ceil"; break;
        case WINED3DSIH_ROUND_Z: instruction = "trunc"; break;
        case WINED3DSIH_SQRT: instruction = "sqrt"; break;
        case WINED3DSIH_UMAX: instruction = "max"; break;
        case WINED3DSIH_UMIN: instruction = "min"; break;
        default: instruction = "";
            ERR("Opcode %s not yet handled in GLSL.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, ins);

    /* In D3D bits are numbered from the most significant bit. */
    if (ins->handler_idx == WINED3DSIH_FIRSTBIT_HI || ins->handler_idx == WINED3DSIH_FIRSTBIT_SHI)
        shader_addline(buffer, "31 - ");
    shader_addline(buffer, "%s(", instruction);

    if (ins->src_count)
    {
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);
        shader_addline(buffer, "%s", src_param.param_str);
        for (i = 1; i < ins->src_count; ++i)
        {
            shader_glsl_add_src_param(ins, &ins->src[i], write_mask, &src_param);
            shader_addline(buffer, ", %s", src_param.param_str);
        }
    }

    shader_addline(buffer, "));\n");
}

static void shader_glsl_float16(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_dst_param dst;
    struct glsl_src_param src;
    DWORD write_mask;
    const char *fmt;
    unsigned int i;

    fmt = ins->handler_idx == WINED3DSIH_F16TOF32
            ? "unpackHalf2x16(%s).x);\n" : "packHalf2x16(vec2(%s, 0.0)));\n";

    dst = ins->dst[0];
    for (i = 0; i < 4; ++i)
    {
        dst.write_mask = ins->dst[0].write_mask & (WINED3DSP_WRITEMASK_0 << i);
        if (!(write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins,
                &dst, dst.reg.data_type)))
            continue;

        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src);
        shader_addline(ins->ctx->buffer, fmt, src.param_str);
    }
}

static void shader_glsl_bitwise_op(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_shader_dst_param dst;
    struct glsl_src_param src[4];
    const char *instruction;
    BOOL tmp_dst = FALSE;
    char mask_char[6];
    unsigned int i, j;
    DWORD write_mask;

    switch (ins->handler_idx)
    {
        case WINED3DSIH_BFI:  instruction = "bitfieldInsert";  break;
        case WINED3DSIH_IBFE: instruction = "bitfieldExtract"; break;
        case WINED3DSIH_UBFE: instruction = "bitfieldExtract"; break;
        default:
            ERR("Unhandled opcode %#x.\n", ins->handler_idx);
            return;
    }

    for (i = 0; i < ins->src_count; ++i)
    {
        if (ins->dst[0].reg.idx[0].offset == ins->src[i].reg.idx[0].offset
                && ins->dst[0].reg.type == ins->src[i].reg.type)
            tmp_dst = TRUE;
    }

    dst = ins->dst[0];
    for (i = 0; i < 4; ++i)
    {
        dst.write_mask = ins->dst[0].write_mask & (WINED3DSP_WRITEMASK_0 << i);
        if (tmp_dst && (write_mask = shader_glsl_get_write_mask(&dst, mask_char)))
            shader_addline(buffer, "tmp0%s = %sBitsToFloat(", mask_char,
                    dst.reg.data_type == WINED3D_DATA_INT ? "int" : "uint");
        else if (!(write_mask = shader_glsl_append_dst_ext(buffer, ins, &dst, dst.reg.data_type)))
            continue;

        for (j = 0; j < ins->src_count; ++j)
            shader_glsl_add_src_param(ins, &ins->src[j], write_mask, &src[j]);
        shader_addline(buffer, "%s(", instruction);
        for (j = 0; j < ins->src_count - 2; ++j)
            shader_addline(buffer, "%s, ", src[ins->src_count - j - 1].param_str);
        shader_addline(buffer, "%s & 0x1f, %s & 0x1f));\n", src[1].param_str, src[0].param_str);
    }

    if (tmp_dst)
    {
        shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], WINED3D_DATA_FLOAT);
        shader_glsl_get_write_mask(&ins->dst[0], mask_char);
        shader_addline(buffer, "tmp0%s);\n", mask_char);
    }
}

static void shader_glsl_nop(const struct wined3d_shader_instruction *ins) {}

static void shader_glsl_nrm(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    unsigned int mask_size;
    DWORD write_mask;
    char dst_mask[6];

    write_mask = shader_glsl_get_write_mask(ins->dst, dst_mask);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);

    if (mask_size > 3)
        shader_addline(buffer, "tmp0.x = dot(vec3(%s), vec3(%s));\n",
                src_param.param_str, src_param.param_str);
    else
        shader_addline(buffer, "tmp0.x = dot(%s, %s);\n",
                src_param.param_str, src_param.param_str);
    shader_glsl_append_dst(buffer, ins);

    shader_addline(buffer, "tmp0.x == 0.0 ? %s : (%s * inversesqrt(tmp0.x)));\n",
            src_param.param_str, src_param.param_str);
}

static void shader_glsl_scalar_op(const struct wined3d_shader_instruction *ins)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    static const float max_float = FLT_MAX, min_float = -FLT_MAX;
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_string_buffer *suffix;
    struct glsl_src_param src0_param;
    unsigned int dst_size;
    DWORD dst_write_mask;
    const char *prefix;
    BOOL guard_inf;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    if (shader_version < WINED3D_SHADER_VERSION(4, 0))
        dst_write_mask = WINED3DSP_WRITEMASK_3;

    shader_glsl_add_src_param(ins, &ins->src[0], dst_write_mask, &src0_param);

    guard_inf = wined3d_settings.multiply_special == 1 && shader_version < WINED3D_SHADER_VERSION(4, 0);
    suffix = string_buffer_get(priv->string_buffers);

    switch (ins->handler_idx)
    {
        case WINED3DSIH_EXP:
        case WINED3DSIH_EXPP:
            prefix = "exp2(";
            string_buffer_sprintf(suffix, ")");
            break;

        case WINED3DSIH_LOG:
        case WINED3DSIH_LOGP:
            if (guard_inf)
            {
                prefix = "max(log2(abs(";
                string_buffer_sprintf(suffix, ")), ");
                shader_glsl_append_imm_vec(suffix, &min_float, 1, ins->ctx->gl_info);
                shader_addline(suffix, ")");
            }
            else
            {
                prefix = "log2(abs(";
                string_buffer_sprintf(suffix, "))");
            }
            break;

        case WINED3DSIH_RCP:
            if (guard_inf)
            {
                prefix = "clamp(1.0 / ";
                string_buffer_sprintf(suffix, ", ");
                shader_glsl_append_imm_vec(suffix, &min_float, 1, ins->ctx->gl_info);
                shader_addline(suffix, ", ");
                shader_glsl_append_imm_vec(suffix, &max_float, 1, ins->ctx->gl_info);
                shader_addline(suffix, ")");
            }
            else
            {
                prefix = "1.0 / ";
                string_buffer_clear(suffix);
            }
            break;

        case WINED3DSIH_RSQ:
            if (guard_inf)
            {
                prefix = "min(inversesqrt(abs(";
                string_buffer_sprintf(suffix, ")), ");
                shader_glsl_append_imm_vec(suffix, &max_float, 1, ins->ctx->gl_info);
                shader_addline(suffix, ")");
            }
            else
            {
                prefix = "inversesqrt(abs(";
                string_buffer_sprintf(suffix, "))");
            }
            break;

        default:
            prefix = "";
            string_buffer_clear(suffix);
            FIXME("Unhandled instruction %#x.\n", ins->handler_idx);
            break;
    }

    if (dst_size > 1 && shader_version < WINED3D_SHADER_VERSION(4, 0))
        shader_addline(buffer, "vec%u(%s%s%s));\n", dst_size, prefix, src0_param.param_str, suffix->buffer);
    else
        shader_addline(buffer, "%s%s%s);\n", prefix, src0_param.param_str, suffix->buffer);

    string_buffer_release(priv->string_buffers, suffix);
}

/** Process the WINED3DSIO_EXPP instruction in GLSL:
 * For shader model 1.x, do the following (and honor the writemask, so use a temporary variable):
 *   dst.x = 2^(floor(src))
 *   dst.y = src - floor(src)
 *   dst.z = 2^src   (partial precision is allowed, but optional)
 *   dst.w = 1.0;
 * For 2.0 shaders, just do this (honoring writemask and swizzle):
 *   dst = 2^src;    (partial precision is allowed, but optional)
 */
static void shader_glsl_expp(const struct wined3d_shader_instruction *ins)
{
    if (ins->ctx->reg_maps->shader_version.major < 2)
    {
        struct glsl_src_param src_param;
        char dst_mask[6];

        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src_param);

        shader_addline(ins->ctx->buffer, "tmp0.x = exp2(floor(%s));\n", src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.y = %s - floor(%s);\n", src_param.param_str, src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.z = exp2(%s);\n", src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.w = 1.0;\n");

        shader_glsl_append_dst(ins->ctx->buffer, ins);
        shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
        shader_addline(ins->ctx->buffer, "tmp0%s);\n", dst_mask);
        return;
    }

    shader_glsl_scalar_op(ins);
}

static void shader_glsl_cast(const struct wined3d_shader_instruction *ins,
        const char *vector_constructor, const char *scalar_constructor)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    unsigned int mask_size;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);

    if (mask_size > 1)
        shader_addline(buffer, "%s%u(%s));\n", vector_constructor, mask_size, src_param.param_str);
    else
        shader_addline(buffer, "%s(%s));\n", scalar_constructor, src_param.param_str);
}

static void shader_glsl_to_int(const struct wined3d_shader_instruction *ins)
{
    shader_glsl_cast(ins, "ivec", "int");
}

static void shader_glsl_to_uint(const struct wined3d_shader_instruction *ins)
{
    shader_glsl_cast(ins, "uvec", "uint");
}

static void shader_glsl_to_float(const struct wined3d_shader_instruction *ins)
{
    shader_glsl_cast(ins, "vec", "float");
}

/** Process signed comparison opcodes in GLSL. */
static void shader_glsl_compare(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);

    if (mask_size > 1) {
        const char *compare;

        switch(ins->handler_idx)
        {
            case WINED3DSIH_SLT: compare = "lessThan"; break;
            case WINED3DSIH_SGE: compare = "greaterThanEqual"; break;
            default: compare = "";
                FIXME("Can't handle opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
        }

        shader_addline(ins->ctx->buffer, "vec%d(%s(%s, %s)));\n", mask_size, compare,
                src0_param.param_str, src1_param.param_str);
    } else {
        switch(ins->handler_idx)
        {
            case WINED3DSIH_SLT:
                /* Step(src0, src1) is not suitable here because if src0 == src1 SLT is supposed,
                 * to return 0.0 but step returns 1.0 because step is not < x
                 * An alternative is a bvec compare padded with an unused second component.
                 * step(src1 * -1.0, src0 * -1.0) is not an option because it suffers from the same
                 * issue. Playing with not() is not possible either because not() does not accept
                 * a scalar.
                 */
                shader_addline(ins->ctx->buffer, "(%s < %s) ? 1.0 : 0.0);\n",
                        src0_param.param_str, src1_param.param_str);
                break;
            case WINED3DSIH_SGE:
                /* Here we can use the step() function and safe a conditional */
                shader_addline(ins->ctx->buffer, "step(%s, %s));\n", src1_param.param_str, src0_param.param_str);
                break;
            default:
                FIXME("Can't handle opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
        }

    }
}

static void shader_glsl_swapc(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_shader_dst_param dst[2];
    struct glsl_src_param src[3];
    unsigned int i, j, k;
    char mask_char[6];
    DWORD write_mask;
    BOOL tmp_dst[2];

    for (i = 0; i < ins->dst_count; ++i)
    {
        tmp_dst[i] = FALSE;
        for (j = 0; j < ins->src_count; ++j)
        {
            if (ins->dst[i].reg.idx[0].offset == ins->src[j].reg.idx[0].offset
                    && ins->dst[i].reg.type == ins->src[j].reg.type)
                tmp_dst[i] = TRUE;
        }
    }

    dst[0] = ins->dst[0];
    dst[1] = ins->dst[1];
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < ARRAY_SIZE(dst); ++j)
        {
            dst[j].write_mask = ins->dst[j].write_mask & (WINED3DSP_WRITEMASK_0 << i);
            if (tmp_dst[j] && (write_mask = shader_glsl_get_write_mask(&dst[j], mask_char)))
                shader_addline(buffer, "tmp%u%s = (", j, mask_char);
            else if (!(write_mask = shader_glsl_append_dst_ext(buffer, ins, &dst[j], dst[j].reg.data_type)))
                continue;

            for (k = 0; k < ARRAY_SIZE(src); ++k)
                shader_glsl_add_src_param(ins, &ins->src[k], write_mask, &src[k]);

            shader_addline(buffer, "%sbool(%s) ? %s : %s);\n", !j ? "!" : "",
                    src[0].param_str, src[1].param_str, src[2].param_str);
        }
    }

    for (i = 0; i < ARRAY_SIZE(tmp_dst); ++i)
    {
        if (tmp_dst[i])
        {
            shader_glsl_get_write_mask(&ins->dst[i], mask_char);
            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[i], ins->dst[i].reg.data_type);
            shader_addline(buffer, "tmp%u%s);\n", i, mask_char);
        }
    }
}

static void shader_glsl_conditional_move(const struct wined3d_shader_instruction *ins)
{
    const char *condition_prefix, *condition_suffix;
    struct wined3d_shader_dst_param dst;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src2_param;
    BOOL temp_destination = FALSE;
    DWORD cmp_channel = 0;
    unsigned int i, j;
    char mask_char[6];
    DWORD write_mask;

    switch (ins->handler_idx)
    {
        case WINED3DSIH_CMP:
            condition_prefix = "";
            condition_suffix = " >= 0.0";
            break;

        case WINED3DSIH_CND:
            condition_prefix = "";
            condition_suffix = " > 0.5";
            break;

        case WINED3DSIH_MOVC:
            condition_prefix = "bool(";
            condition_suffix = ")";
            break;

        default:
            FIXME("Unhandled instruction %#x.\n", ins->handler_idx);
            condition_prefix = "<unhandled prefix>";
            condition_suffix = "<unhandled suffix>";
            break;
    }

    if (shader_is_scalar(&ins->dst[0].reg) || shader_is_scalar(&ins->src[0].reg))
    {
        write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

        shader_addline(ins->ctx->buffer, "%s%s%s ? %s : %s);\n",
                condition_prefix, src0_param.param_str, condition_suffix,
                src1_param.param_str, src2_param.param_str);
        return;
    }

    dst = ins->dst[0];

    /* Splitting the instruction up in multiple lines imposes a problem:
     * The first lines may overwrite source parameters of the following lines.
     * Deal with that by using a temporary destination register if needed. */
    if ((ins->src[0].reg.idx[0].offset == dst.reg.idx[0].offset
                && ins->src[0].reg.type == dst.reg.type)
            || (ins->src[1].reg.idx[0].offset == dst.reg.idx[0].offset
                && ins->src[1].reg.type == dst.reg.type)
            || (ins->src[2].reg.idx[0].offset == dst.reg.idx[0].offset
                && ins->src[2].reg.type == dst.reg.type))
        temp_destination = TRUE;

    /* Cycle through all source0 channels. */
    for (i = 0; i < 4; ++i)
    {
        write_mask = 0;
        /* Find the destination channels which use the current source0 channel. */
        for (j = 0; j < 4; ++j)
        {
            if (shader_glsl_swizzle_get_component(ins->src[0].swizzle, j) == i)
            {
                write_mask |= WINED3DSP_WRITEMASK_0 << j;
                cmp_channel = WINED3DSP_WRITEMASK_0 << j;
            }
        }
        dst.write_mask = ins->dst[0].write_mask & write_mask;

        if (temp_destination)
        {
            if (!(write_mask = shader_glsl_get_write_mask(&dst, mask_char)))
                continue;
            shader_addline(ins->ctx->buffer, "tmp0%s = (", mask_char);
        }
        else if (!(write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &dst, dst.reg.data_type)))
            continue;

        shader_glsl_add_src_param(ins, &ins->src[0], cmp_channel, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

        shader_addline(ins->ctx->buffer, "%s%s%s ? %s : %s);\n",
                condition_prefix, src0_param.param_str, condition_suffix,
                src1_param.param_str, src2_param.param_str);
    }

    if (temp_destination)
    {
        shader_glsl_get_write_mask(&ins->dst[0], mask_char);
        shader_glsl_append_dst(ins->ctx->buffer, ins);
        shader_addline(ins->ctx->buffer, "tmp0%s);\n", mask_char);
    }
}

/** Process the CND opcode in GLSL (dst = (src0 > 0.5) ? src1 : src2) */
/* For ps 1.1-1.3, only a single component of src0 is used. For ps 1.4
 * the compare is done per component of src0. */
static void shader_glsl_cnd(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src2_param;
    DWORD write_mask;
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);

    if (shader_version < WINED3D_SHADER_VERSION(1, 4))
    {
        write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

        if (ins->coissue && ins->dst->write_mask != WINED3DSP_WRITEMASK_3)
            shader_addline(ins->ctx->buffer, "%s /* COISSUE! */);\n", src1_param.param_str);
        else
            shader_addline(ins->ctx->buffer, "%s > 0.5 ? %s : %s);\n",
                    src0_param.param_str, src1_param.param_str, src2_param.param_str);
        return;
    }

    shader_glsl_conditional_move(ins);
}

/** GLSL code generation for WINED3DSIO_MAD: Multiply the first 2 opcodes, then add the last */
static void shader_glsl_mad(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src2_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
    shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);
    if (wined3d_settings.multiply_special == 2 && ins->ctx->reg_maps->shader_version.major < 4)
        shader_addline(ins->ctx->buffer, "mul%d(%s, %s) + %s);\n",
                shader_glsl_get_write_mask_size(write_mask), src0_param.param_str,
                src1_param.param_str, src2_param.param_str);
    else
        shader_addline(ins->ctx->buffer, "(%s * %s) + %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
}

/* Handles transforming all WINED3DSIO_M?x? opcodes for
   Vertex shaders to GLSL codes */
static void shader_glsl_mnxn(const struct wined3d_shader_instruction *ins)
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
            break;
    }

    tmp_dst = ins->dst[0];
    tmp_src[0] = ins->src[0];
    tmp_src[1] = ins->src[1];
    for (i = 0; i < nComponents; ++i)
    {
        tmp_dst.write_mask = WINED3DSP_WRITEMASK_0 << i;
        shader_glsl_dot(&tmp_ins);
        ++tmp_src[1].reg.idx[0].offset;
    }
}

/**
    The LRP instruction performs a component-wise linear interpolation
    between the second and third operands using the first operand as the
    blend factor.  Equation:  (dst = src2 + src0 * (src1 - src2))
    This is equivalent to mix(src2, src1, src0);
*/
static void shader_glsl_lrp(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src2_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);

    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
    shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

    shader_addline(ins->ctx->buffer, "mix(%s, %s, %s));\n",
            src2_param.param_str, src1_param.param_str, src0_param.param_str);
}

/** Process the WINED3DSIO_LIT instruction in GLSL:
 * dst.x = dst.w = 1.0
 * dst.y = (src0.x > 0) ? src0.x
 * dst.z = (src0.x > 0) ? ((src0.y > 0) ? pow(src0.y, src.w) : 0) : 0
 *                                        where src.w is clamped at +- 128
 */
static void shader_glsl_lit(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src3_param;
    char dst_mask[6];

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src3_param);

    /* The sdk specifies the instruction like this
     * dst.x = 1.0;
     * if(src.x > 0.0) dst.y = src.x
     * else dst.y = 0.0.
     * if(src.x > 0.0 && src.y > 0.0) dst.z = pow(src.y, power);
     * else dst.z = 0.0;
     * dst.w = 1.0;
     * (where power = src.w clamped between -128 and 128)
     *
     * Obviously that has quite a few conditionals in it which we don't like. So the first step is this:
     * dst.x = 1.0                                  ... No further explanation needed
     * dst.y = max(src.y, 0.0);                     ... If x < 0.0, use 0.0, otherwise x. Same as the conditional
     * dst.z = x > 0.0 ? pow(max(y, 0.0), p) : 0;   ... 0 ^ power is 0, and otherwise we use y anyway
     * dst.w = 1.0.                                 ... Nothing fancy.
     *
     * So we still have one conditional in there. So do this:
     * dst.z = pow(max(0.0, src.y) * step(0.0, src.x), power);
     *
     * step(0.0, x) will return 1 if src.x > 0.0, and 0 otherwise. So if y is 0 we get pow(0.0 * 1.0, power),
     * which sets dst.z to 0. If y > 0, but x = 0.0, we get pow(y * 0.0, power), which results in 0 too.
     * if both x and y are > 0, we get pow(y * 1.0, power), as it is supposed to.
     *
     * Unfortunately pow(0.0 ^ 0.0) returns NaN on most GPUs, but lit with src.y = 0 and src.w = 0 returns
     * a non-NaN value in dst.z. What we return doesn't matter, as long as it is not NaN. Return 0, which is
     * what all Windows HW drivers and GL_ARB_vertex_program's LIT do.
     */
    shader_addline(ins->ctx->buffer,
            "vec4(1.0, max(%s, 0.0), %s == 0.0 ? 0.0 : "
            "pow(max(0.0, %s) * step(0.0, %s), clamp(%s, -128.0, 128.0)), 1.0)%s);\n",
            src0_param.param_str, src3_param.param_str, src1_param.param_str,
            src0_param.param_str, src3_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
static void shader_glsl_dst(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0y_param;
    struct glsl_src_param src0z_param;
    struct glsl_src_param src1y_param;
    struct glsl_src_param src1w_param;
    char dst_mask[6];

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_1, &src0y_param);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_2, &src0z_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_1, &src1y_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_3, &src1w_param);

    shader_addline(ins->ctx->buffer, "vec4(1.0, %s * %s, %s, %s))%s;\n",
            src0y_param.param_str, src1y_param.param_str, src0z_param.param_str, src1w_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_SINCOS instruction in GLSL:
 * VS 2.0 requires that specific cosine and sine constants be passed to this instruction so the hardware
 * can handle it.  But, these functions are built-in for GLSL, so we can just ignore the last 2 params.
 *
 * dst.x = cos(src0.?)
 * dst.y = sin(src0.?)
 * dst.z = dst.z
 * dst.w = dst.w
 */
static void shader_glsl_sincos(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    DWORD write_mask;

    if (ins->ctx->reg_maps->shader_version.major < 4)
    {
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);

        write_mask = shader_glsl_append_dst(buffer, ins);
        switch (write_mask)
        {
            case WINED3DSP_WRITEMASK_0:
                shader_addline(buffer, "cos(%s));\n", src0_param.param_str);
                break;

            case WINED3DSP_WRITEMASK_1:
                shader_addline(buffer, "sin(%s));\n", src0_param.param_str);
                break;

            case (WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1):
                shader_addline(buffer, "vec2(cos(%s), sin(%s)));\n",
                        src0_param.param_str, src0_param.param_str);
                break;

            default:
                ERR("Write mask should be .x, .y or .xy\n");
                break;
        }

        return;
    }

    if (ins->dst[0].reg.type != WINED3DSPR_NULL)
    {

        if (ins->dst[1].reg.type != WINED3DSPR_NULL)
        {
            char dst_mask[6];

            write_mask = shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_addline(buffer, "tmp0%s = sin(%s);\n", dst_mask, src0_param.param_str);

            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_addline(buffer, "cos(%s));\n", src0_param.param_str);

            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], ins->dst[0].reg.data_type);
            shader_addline(buffer, "tmp0%s);\n", dst_mask);
        }
        else
        {
            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], ins->dst[0].reg.data_type);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_addline(buffer, "sin(%s));\n", src0_param.param_str);
        }
    }
    else if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_addline(buffer, "cos(%s));\n", src0_param.param_str);
    }
}

/* sgn in vs_2_0 has 2 extra parameters(registers for temporary storage) which we don't use
 * here. But those extra parameters require a dedicated function for sgn, since map2gl would
 * generate invalid code
 */
static void shader_glsl_sgn(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);

    shader_addline(ins->ctx->buffer, "sign(%s));\n", src0_param.param_str);
}

/** Process the WINED3DSIO_LOOP instruction in GLSL:
 * Start a for() loop where src1.y is the initial value of aL,
 *  increment aL by src1.z for a total of src1.x iterations.
 *  Need to use a temporary variable for this operation.
 */
/* FIXME: I don't think nested loops will work correctly this way. */
static void shader_glsl_loop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_parser_state *state = ins->ctx->state;
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_lconst *constant;
    struct wined3d_string_buffer *reg_name;
    const DWORD *control_values = NULL;

    if (ins->ctx->reg_maps->shader_version.major < 4)
    {
        /* Try to hardcode the loop control parameters if possible. Direct3D 9
         * class hardware doesn't support real varying indexing, but Microsoft
         * designed this feature for Shader model 2.x+. If the loop control is
         * known at compile time, the GLSL compiler can unroll the loop, and
         * replace indirect addressing with direct addressing. */
        if (ins->src[1].reg.type == WINED3DSPR_CONSTINT)
        {
            LIST_FOR_EACH_ENTRY(constant, &shader->constantsI, struct wined3d_shader_lconst, entry)
            {
                if (constant->idx == ins->src[1].reg.idx[0].offset)
                {
                    control_values = constant->value;
                    break;
                }
            }
        }

        if (control_values)
        {
            struct wined3d_shader_loop_control loop_control;
            loop_control.count = control_values[0];
            loop_control.start = control_values[1];
            loop_control.step = (int)control_values[2];

            if (loop_control.step > 0)
            {
                shader_addline(buffer, "for (aL%u = %u; aL%u < (%u * %d + %u); aL%u += %d)\n{\n",
                        state->current_loop_depth, loop_control.start,
                        state->current_loop_depth, loop_control.count, loop_control.step, loop_control.start,
                        state->current_loop_depth, loop_control.step);
            }
            else if (loop_control.step < 0)
            {
                shader_addline(buffer, "for (aL%u = %u; aL%u > (%u * %d + %u); aL%u += %d)\n{\n",
                        state->current_loop_depth, loop_control.start,
                        state->current_loop_depth, loop_control.count, loop_control.step, loop_control.start,
                        state->current_loop_depth, loop_control.step);
            }
            else
            {
                shader_addline(buffer, "for (aL%u = %u, tmpInt%u = 0; tmpInt%u < %u; tmpInt%u++)\n{\n",
                        state->current_loop_depth, loop_control.start, state->current_loop_depth,
                        state->current_loop_depth, loop_control.count,
                        state->current_loop_depth);
            }
        }
        else
        {
            reg_name = string_buffer_get(priv->string_buffers);
            shader_glsl_get_register_name(&ins->src[1].reg, ins->src[1].reg.data_type, reg_name, NULL, ins->ctx);

            shader_addline(buffer, "for (tmpInt%u = 0, aL%u = %s.y; tmpInt%u < %s.x; tmpInt%u++, aL%u += %s.z)\n{\n",
                    state->current_loop_depth, state->current_loop_reg, reg_name->buffer,
                    state->current_loop_depth, reg_name->buffer,
                    state->current_loop_depth, state->current_loop_reg, reg_name->buffer);

            string_buffer_release(priv->string_buffers, reg_name);

        }

        ++state->current_loop_reg;
    }
    else
    {
        shader_addline(buffer, "for (;;)\n{\n");
    }

    ++state->current_loop_depth;
}

static void shader_glsl_end(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_parser_state *state = ins->ctx->state;

    shader_addline(ins->ctx->buffer, "}\n");

    if (ins->handler_idx == WINED3DSIH_ENDLOOP)
    {
        --state->current_loop_depth;
        --state->current_loop_reg;
    }

    if (ins->handler_idx == WINED3DSIH_ENDREP)
    {
        --state->current_loop_depth;
    }
}

static void shader_glsl_rep(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_parser_state *state = ins->ctx->state;
    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_lconst *constant;
    struct glsl_src_param src0_param;
    const DWORD *control_values = NULL;

    /* Try to hardcode local values to help the GLSL compiler to unroll and optimize the loop */
    if (ins->src[0].reg.type == WINED3DSPR_CONSTINT)
    {
        LIST_FOR_EACH_ENTRY(constant, &shader->constantsI, struct wined3d_shader_lconst, entry)
        {
            if (constant->idx == ins->src[0].reg.idx[0].offset)
            {
                control_values = constant->value;
                break;
            }
        }
    }

    if (control_values)
    {
        shader_addline(ins->ctx->buffer, "for (tmpInt%d = 0; tmpInt%d < %d; tmpInt%d++) {\n",
                state->current_loop_depth, state->current_loop_depth,
                control_values[0], state->current_loop_depth);
    }
    else
    {
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
        shader_addline(ins->ctx->buffer, "for (tmpInt%d = 0; tmpInt%d < %s; tmpInt%d++) {\n",
                state->current_loop_depth, state->current_loop_depth,
                src0_param.param_str, state->current_loop_depth);
    }

    ++state->current_loop_depth;
}

static void shader_glsl_switch(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(ins->ctx->buffer, "switch (%s)\n{\n", src0_param.param_str);
}

static void shader_glsl_case(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(ins->ctx->buffer, "case %s:\n", src0_param.param_str);
}

static void shader_glsl_default(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "default:\n");
}

static void shader_glsl_generate_condition(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src_param;
    const char *condition;

    condition = ins->flags == WINED3D_SHADER_CONDITIONAL_OP_NZ ? "bool" : "!bool";
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src_param);
    shader_addline(ins->ctx->buffer, "if (%s(%s))\n", condition, src_param.param_str);
}

static void shader_glsl_if(const struct wined3d_shader_instruction *ins)
{
    shader_glsl_generate_condition(ins);
    shader_addline(ins->ctx->buffer, "{\n");
}

static void shader_glsl_ifc(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(ins->ctx->buffer, "if (%s %s %s) {\n",
            src0_param.param_str, shader_glsl_get_rel_op(ins->flags), src1_param.param_str);
}

static void shader_glsl_else(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "} else {\n");
}

static void shader_glsl_emit(const struct wined3d_shader_instruction *ins)
{
    unsigned int stream = ins->handler_idx == WINED3DSIH_EMIT ? 0 : ins->src[0].reg.idx[0].offset;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;

    shader_addline(ins->ctx->buffer, "setup_gs_output(gs_out);\n");
    if (!ins->ctx->gl_info->supported[ARB_CLIP_CONTROL])
        shader_glsl_fixup_position(ins->ctx->buffer, reg_maps->viewport_array);

    if (!stream)
        shader_addline(ins->ctx->buffer, "EmitVertex();\n");
    else
        FIXME("Unhandled primitive stream %u.\n", stream);
}

static void shader_glsl_break(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "break;\n");
}

/* FIXME: According to MSDN the compare is done per component. */
static void shader_glsl_breakc(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(ins->ctx->buffer, "if (%s %s %s) break;\n",
            src0_param.param_str, shader_glsl_get_rel_op(ins->flags), src1_param.param_str);
}

static void shader_glsl_conditional_op(const struct wined3d_shader_instruction *ins)
{
    const char *op;

    switch (ins->handler_idx)
    {
        case WINED3DSIH_BREAKP:
            op = "break;";
            break;
        case WINED3DSIH_CONTINUEP:
            op = "continue;";
            break;
        case WINED3DSIH_RETP:
            op = "return;";
            break;
        default:
            ERR("Unhandled opcode %#x.\n", ins->handler_idx);
            return;
    }

    shader_glsl_generate_condition(ins);
    if (ins->handler_idx == WINED3DSIH_RETP)
    {
        shader_addline(ins->ctx->buffer, "{\n");
        shader_glsl_generate_shader_epilogue(ins->ctx);
    }
    shader_addline(ins->ctx->buffer, "    %s\n", op);
    if (ins->handler_idx == WINED3DSIH_RETP)
        shader_addline(ins->ctx->buffer, "}\n");
}

static void shader_glsl_continue(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "continue;\n");
}

static void shader_glsl_label(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "}\n");
    shader_addline(ins->ctx->buffer, "void subroutine%u()\n{\n", ins->src[0].reg.idx[0].offset);

    /* Subroutines appear at the end of the shader. */
    ins->ctx->state->in_subroutine = TRUE;
}

static void shader_glsl_call(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "subroutine%u();\n", ins->src[0].reg.idx[0].offset);
}

static void shader_glsl_callnz(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src1_param;

    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);
    shader_addline(ins->ctx->buffer, "if (%s) subroutine%u();\n",
            src1_param.param_str, ins->src[0].reg.idx[0].offset);
}

static void shader_glsl_ret(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_version *version = &ins->ctx->shader->reg_maps.shader_version;

    if (version->major >= 4 && !ins->ctx->state->in_subroutine)
    {
        shader_glsl_generate_shader_epilogue(ins->ctx);
        shader_addline(ins->ctx->buffer, "return;\n");
    }
}

static void shader_glsl_tex(const struct wined3d_shader_instruction *ins)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    struct glsl_sample_function sample_function;
    DWORD sample_flags = 0;
    DWORD resource_idx;
    DWORD mask = 0, swizzle;
    const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    enum wined3d_shader_resource_type resource_type;

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0))
        resource_idx = ins->dst[0].reg.idx[0].offset;
    else
        resource_idx = ins->src[1].reg.idx[0].offset;

    resource_type = ins->ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            ? pixelshader_get_resource_type(ins->ctx->reg_maps, resource_idx, priv->cur_ps_args->tex_types)
            : ins->ctx->reg_maps->resource_info[resource_idx].type;

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD flags = (priv->cur_ps_args->tex_transform >> resource_idx * WINED3D_PSARGS_TEXTRANSFORM_SHIFT)
                & WINED3D_PSARGS_TEXTRANSFORM_MASK;

        /* Projected cube textures don't make a lot of sense, the resulting coordinates stay the same. */
        if (flags & WINED3D_PSARGS_PROJECTED && resource_type != WINED3D_SHADER_RESOURCE_TEXTURE_CUBE)
        {
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            switch (flags & ~WINED3D_PSARGS_PROJECTED)
            {
                case WINED3D_TTFF_COUNT1:
                    FIXME("WINED3D_TTFF_PROJECTED with WINED3D_TTFF_COUNT1?\n");
                    break;
                case WINED3D_TTFF_COUNT2:
                    mask = WINED3DSP_WRITEMASK_1;
                    break;
                case WINED3D_TTFF_COUNT3:
                    mask = WINED3DSP_WRITEMASK_2;
                    break;
                case WINED3D_TTFF_COUNT4:
                case WINED3D_TTFF_DISABLE:
                    mask = WINED3DSP_WRITEMASK_3;
                    break;
            }
        }
    }
    else if (shader_version < WINED3D_SHADER_VERSION(2,0))
    {
        enum wined3d_shader_src_modifier src_mod = ins->src[0].modifiers;

        if (src_mod == WINED3DSPSM_DZ) {
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            mask = WINED3DSP_WRITEMASK_2;
        } else if (src_mod == WINED3DSPSM_DW) {
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            mask = WINED3DSP_WRITEMASK_3;
        }
    }
    else
    {
        if ((ins->flags & WINED3DSI_TEXLD_PROJECT)
                && resource_type != WINED3D_SHADER_RESOURCE_TEXTURE_CUBE)
        {
            /* ps 2.0 texldp instruction always divides by the fourth component. */
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            mask = WINED3DSP_WRITEMASK_3;
        }
    }

    shader_glsl_get_sample_function(ins->ctx, resource_idx, resource_idx, sample_flags, &sample_function);
    mask |= sample_function.coord_mask;
    sample_function.coord_mask = mask;

    if (shader_version < WINED3D_SHADER_VERSION(2,0)) swizzle = WINED3DSP_NOSWIZZLE;
    else swizzle = ins->src[1].swizzle;

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        char coord_mask[6];
        shader_glsl_write_mask_to_str(mask, coord_mask);
        shader_glsl_gen_sample_code(ins, resource_idx, &sample_function, swizzle, NULL, NULL, NULL, NULL,
                "T%u%s", resource_idx, coord_mask);
    }
    else
    {
        struct glsl_src_param coord_param;
        shader_glsl_add_src_param(ins, &ins->src[0], mask, &coord_param);
        if (ins->flags & WINED3DSI_TEXLD_BIAS)
        {
            struct glsl_src_param bias;
            shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &bias);
            shader_glsl_gen_sample_code(ins, resource_idx, &sample_function, swizzle, NULL, NULL, bias.param_str,
                    NULL, "%s", coord_param.param_str);
        } else {
            shader_glsl_gen_sample_code(ins, resource_idx, &sample_function, swizzle, NULL, NULL, NULL, NULL,
                    "%s", coord_param.param_str);
        }
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_texldd(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, dx_param, dy_param;
    struct glsl_sample_function sample_function;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;

    if (!shader_glsl_has_core_grad(gl_info) && !gl_info->supported[ARB_SHADER_TEXTURE_LOD])
    {
        FIXME("texldd used, but not supported by hardware. Falling back to regular tex.\n");
        shader_glsl_tex(ins);
        return;
    }

    sampler_idx = ins->src[1].reg.idx[0].offset;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, WINED3D_GLSL_SAMPLE_GRAD, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[2], sample_function.deriv_mask, &dx_param);
    shader_glsl_add_src_param(ins, &ins->src[3], sample_function.deriv_mask, &dy_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, dx_param.param_str, dy_param.param_str,
            NULL, NULL, "%s", coord_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_texldl(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_version *shader_version = &ins->ctx->reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, lod_param;
    struct glsl_sample_function sample_function;
    DWORD swizzle = ins->src[1].swizzle;
    DWORD sampler_idx;

    sampler_idx = ins->src[1].reg.idx[0].offset;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, WINED3D_GLSL_SAMPLE_LOD, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &lod_param);

    if (shader_version->type == WINED3D_SHADER_TYPE_PIXEL && !shader_glsl_has_core_grad(gl_info)
            && !gl_info->supported[ARB_SHADER_TEXTURE_LOD])
    {
        /* Plain GLSL only supports Lod sampling functions in vertex shaders.
         * However, the NVIDIA drivers allow them in fragment shaders as well,
         * even without the appropriate extension. */
        WARN("Using %s in fragment shader.\n", sample_function.name->buffer);
    }
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, lod_param.param_str, NULL,
            "%s", coord_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static unsigned int shader_glsl_find_sampler(const struct wined3d_shader_sampler_map *sampler_map,
        unsigned int resource_idx, unsigned int sampler_idx)
{
    struct wined3d_shader_sampler_map_entry *entries = sampler_map->entries;
    unsigned int i;

    for (i = 0; i < sampler_map->count; ++i)
    {
        if (entries[i].resource_idx == resource_idx && entries[i].sampler_idx == sampler_idx)
            return entries[i].bind_idx;
    }

    ERR("No GLSL sampler found for resource %u / sampler %u.\n", resource_idx, sampler_idx);

    return ~0u;
}

static void shader_glsl_atomic(const struct wined3d_shader_instruction *ins)
{
    const BOOL is_imm_instruction = WINED3DSIH_IMM_ATOMIC_AND <= ins->handler_idx
            && ins->handler_idx <= WINED3DSIH_IMM_ATOMIC_XOR;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct glsl_src_param structure_idx, offset, data, data2;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    enum wined3d_shader_resource_type resource_type;
    struct wined3d_string_buffer *address;
    enum wined3d_data_type data_type;
    unsigned int resource_idx, stride;
    const char *op, *resource;
    DWORD coord_mask;
    BOOL is_tgsm;

    resource_idx = ins->dst[is_imm_instruction].reg.idx[0].offset;
    is_tgsm = ins->dst[is_imm_instruction].reg.type == WINED3DSPR_GROUPSHAREDMEM;
    if (is_tgsm)
    {
        if (resource_idx >= reg_maps->tgsm_count)
        {
            ERR("Invalid TGSM index %u.\n", resource_idx);
            return;
        }
        resource = "g";
        data_type = WINED3D_DATA_UINT;
        coord_mask = 1;
        stride = reg_maps->tgsm[resource_idx].stride;
    }
    else
    {
        if (resource_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
        {
            ERR("Invalid UAV index %u.\n", resource_idx);
            return;
        }
        resource_type = reg_maps->uav_resource_info[resource_idx].type;
        if (resource_type >= ARRAY_SIZE(resource_type_info))
        {
            ERR("Unexpected resource type %#x.\n", resource_type);
            return;
        }
        resource = "image";
        data_type = reg_maps->uav_resource_info[resource_idx].data_type;
        coord_mask = (1u << resource_type_info[resource_type].coord_size) - 1;
        stride = reg_maps->uav_resource_info[resource_idx].stride;
    }

    switch (ins->handler_idx)
    {
        case WINED3DSIH_ATOMIC_AND:
        case WINED3DSIH_IMM_ATOMIC_AND:
            if (is_tgsm)
                op = "atomicAnd";
            else
                op = "imageAtomicAnd";
            break;
        case WINED3DSIH_ATOMIC_CMP_STORE:
        case WINED3DSIH_IMM_ATOMIC_CMP_EXCH:
            if (is_tgsm)
                op = "atomicCompSwap";
            else
                op = "imageAtomicCompSwap";
            break;
        case WINED3DSIH_ATOMIC_IADD:
        case WINED3DSIH_IMM_ATOMIC_IADD:
            if (is_tgsm)
                op = "atomicAdd";
            else
                op = "imageAtomicAdd";
            break;
        case WINED3DSIH_ATOMIC_IMAX:
        case WINED3DSIH_IMM_ATOMIC_IMAX:
            if (is_tgsm)
                op = "atomicMax";
            else
                op = "imageAtomicMax";
            if (data_type != WINED3D_DATA_INT)
            {
                FIXME("Unhandled opcode %#x for unsigned integers.\n", ins->handler_idx);
                return;
            }
            break;
        case WINED3DSIH_ATOMIC_IMIN:
        case WINED3DSIH_IMM_ATOMIC_IMIN:
            if (is_tgsm)
                op = "atomicMin";
            else
                op = "imageAtomicMin";
            if (data_type != WINED3D_DATA_INT)
            {
                FIXME("Unhandled opcode %#x for unsigned integers.\n", ins->handler_idx);
                return;
            }
            break;
        case WINED3DSIH_ATOMIC_OR:
        case WINED3DSIH_IMM_ATOMIC_OR:
            if (is_tgsm)
                op = "atomicOr";
            else
                op = "imageAtomicOr";
            break;
        case WINED3DSIH_ATOMIC_UMAX:
        case WINED3DSIH_IMM_ATOMIC_UMAX:
            if (is_tgsm)
                op = "atomicMax";
            else
                op = "imageAtomicMax";
            if (data_type != WINED3D_DATA_UINT)
            {
                FIXME("Unhandled opcode %#x for signed integers.\n", ins->handler_idx);
                return;
            }
            break;
        case WINED3DSIH_ATOMIC_UMIN:
        case WINED3DSIH_IMM_ATOMIC_UMIN:
            if (is_tgsm)
                op = "atomicMin";
            else
                op = "imageAtomicMin";
            if (data_type != WINED3D_DATA_UINT)
            {
                FIXME("Unhandled opcode %#x for signed integers.\n", ins->handler_idx);
                return;
            }
            break;
        case WINED3DSIH_ATOMIC_XOR:
        case WINED3DSIH_IMM_ATOMIC_XOR:
            if (is_tgsm)
                op = "atomicXor";
            else
                op = "imageAtomicXor";
            break;
        case WINED3DSIH_IMM_ATOMIC_EXCH:
            if (is_tgsm)
                op = "atomicExchange";
            else
                op = "imageAtomicExchange";
            break;
        default:
            ERR("Unhandled opcode %#x.\n", ins->handler_idx);
            return;
    }

    address = string_buffer_get(priv->string_buffers);
    if (stride)
    {
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &structure_idx);
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_1, &offset);
        string_buffer_sprintf(address, "%s * %u + %s / 4", structure_idx.param_str, stride, offset.param_str);
    }
    else
    {
        shader_glsl_add_src_param(ins, &ins->src[0], coord_mask, &offset);
        string_buffer_sprintf(address, "%s", offset.param_str);
        if (is_tgsm || (reg_maps->uav_resource_info[resource_idx].flags & WINED3D_VIEW_BUFFER_RAW))
            shader_addline(address, "/ 4");
    }

    if (is_imm_instruction)
        shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &ins->dst[0], data_type);

    if (is_tgsm)
        shader_addline(buffer, "%s(%s_%s%u[%s], ",
                op, shader_glsl_get_prefix(version->type), resource, resource_idx, address->buffer);
    else
        shader_addline(buffer, "%s(%s_%s%u, %s, ",
                op, shader_glsl_get_prefix(version->type), resource, resource_idx, address->buffer);

    shader_glsl_add_src_param_ext(ins->ctx, &ins->src[1], WINED3DSP_WRITEMASK_0, &data, data_type);
    shader_addline(buffer, "%s", data.param_str);
    if (ins->src_count >= 3)
    {
        shader_glsl_add_src_param_ext(ins->ctx, &ins->src[2], WINED3DSP_WRITEMASK_0, &data2, data_type);
        shader_addline(buffer, ", %s", data2.param_str);
    }

    if (is_imm_instruction)
        shader_addline(buffer, ")");
    shader_addline(buffer, ");\n");

    string_buffer_release(priv->string_buffers, address);
}

static void shader_glsl_uav_counter(const struct wined3d_shader_instruction *ins)
{
    const char *prefix = shader_glsl_get_prefix(ins->ctx->reg_maps->shader_version.type);
    const char *op;

    if (ins->handler_idx == WINED3DSIH_IMM_ATOMIC_ALLOC)
        op = "atomicCounterIncrement";
    else
        op = "atomicCounterDecrement";

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_addline(ins->ctx->buffer, "%s(%s_counter%u));\n", op, prefix, ins->src[0].reg.idx[0].offset);
}

static void shader_glsl_ld_uav(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    enum wined3d_shader_resource_type resource_type;
    struct glsl_src_param image_coord_param;
    enum wined3d_data_type data_type;
    DWORD coord_mask, write_mask;
    unsigned int uav_idx;
    char dst_swizzle[6];

    uav_idx = ins->src[1].reg.idx[0].offset;
    if (uav_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
    {
        ERR("Invalid UAV index %u.\n", uav_idx);
        return;
    }
    resource_type = reg_maps->uav_resource_info[uav_idx].type;
    if (resource_type >= ARRAY_SIZE(resource_type_info))
    {
        ERR("Unexpected resource type %#x.\n", resource_type);
        resource_type = WINED3D_SHADER_RESOURCE_TEXTURE_2D;
    }
    data_type = reg_maps->uav_resource_info[uav_idx].data_type;
    coord_mask = (1u << resource_type_info[resource_type].coord_size) - 1;

    write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &ins->dst[0], data_type);
    shader_glsl_get_swizzle(&ins->src[1], FALSE, write_mask, dst_swizzle);

    shader_glsl_add_src_param(ins, &ins->src[0], coord_mask, &image_coord_param);
    shader_addline(ins->ctx->buffer, "imageLoad(%s_image%u, %s)%s);\n",
            shader_glsl_get_prefix(version->type), uav_idx, image_coord_param.param_str, dst_swizzle);
}

static void shader_glsl_ld_raw_structured(const struct wined3d_shader_instruction *ins)
{
    const char *prefix = shader_glsl_get_prefix(ins->ctx->reg_maps->shader_version.type);
    const struct wined3d_shader_src_param *src = &ins->src[ins->src_count - 1];
    unsigned int i, swizzle, resource_idx, bind_idx, stride, src_idx = 0;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param structure_idx, offset;
    struct wined3d_string_buffer *address;
    struct wined3d_shader_dst_param dst;
    const char *function, *resource;

    resource_idx = src->reg.idx[0].offset;
    if (src->reg.type == WINED3DSPR_RESOURCE)
    {
        if (resource_idx >= ARRAY_SIZE(reg_maps->resource_info))
        {
            ERR("Invalid resource index %u.\n", resource_idx);
            return;
        }
        stride = reg_maps->resource_info[resource_idx].stride;
        bind_idx = shader_glsl_find_sampler(&reg_maps->sampler_map, resource_idx, WINED3D_SAMPLER_DEFAULT);
        function = "texelFetch";
        resource = "sampler";
    }
    else if (src->reg.type == WINED3DSPR_UAV)
    {
        if (resource_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
        {
            ERR("Invalid UAV index %u.\n", resource_idx);
            return;
        }
        stride = reg_maps->uav_resource_info[resource_idx].stride;
        bind_idx = resource_idx;
        function = "imageLoad";
        resource = "image";
    }
    else
    {
        if (resource_idx >= reg_maps->tgsm_count)
        {
            ERR("Invalid TGSM index %u.\n", resource_idx);
            return;
        }
        stride = reg_maps->tgsm[resource_idx].stride;
        bind_idx = resource_idx;
        function = NULL;
        resource = "g";
    }

    address = string_buffer_get(priv->string_buffers);
    if (ins->handler_idx == WINED3DSIH_LD_STRUCTURED)
    {
        shader_glsl_add_src_param(ins, &ins->src[src_idx++], WINED3DSP_WRITEMASK_0, &structure_idx);
        shader_addline(address, "%s * %u + ", structure_idx.param_str, stride);
    }
    shader_glsl_add_src_param(ins, &ins->src[src_idx++], WINED3DSP_WRITEMASK_0, &offset);
    shader_addline(address, "%s / 4", offset.param_str);

    dst = ins->dst[0];
    if (shader_glsl_get_write_mask_size(dst.write_mask) > 1)
    {
        /* The instruction is split into multiple lines. The first lines may
         * overwrite source parameters of the following lines. */
        shader_addline(buffer, "tmp0.x = intBitsToFloat(%s);\n", address->buffer);
        string_buffer_sprintf(address, "floatBitsToInt(tmp0.x)");
    }

    for (i = 0; i < 4; ++i)
    {
        dst.write_mask = ins->dst[0].write_mask & (WINED3DSP_WRITEMASK_0 << i);
        if (!shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &dst, dst.reg.data_type))
            continue;

        swizzle = shader_glsl_swizzle_get_component(src->swizzle, i);
        if (function)
            shader_addline(buffer, "%s(%s_%s%u, %s + %u).x);\n",
                    function, prefix, resource, bind_idx, address->buffer, swizzle);
        else
            shader_addline(buffer, "%s_%s%u[%s + %u]);\n",
                    prefix, resource, bind_idx, address->buffer, swizzle);
    }

    string_buffer_release(priv->string_buffers, address);
}

static void shader_glsl_store_uav(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    struct glsl_src_param image_coord_param, image_data_param;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_data_type data_type;
    unsigned int uav_idx;
    DWORD coord_mask;

    uav_idx = ins->dst[0].reg.idx[0].offset;
    if (uav_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
    {
        ERR("Invalid UAV index %u.\n", uav_idx);
        return;
    }
    resource_type = reg_maps->uav_resource_info[uav_idx].type;
    if (resource_type >= ARRAY_SIZE(resource_type_info))
    {
        ERR("Unexpected resource type %#x.\n", resource_type);
        return;
    }
    data_type = reg_maps->uav_resource_info[uav_idx].data_type;
    coord_mask = (1u << resource_type_info[resource_type].coord_size) - 1;

    shader_glsl_add_src_param(ins, &ins->src[0], coord_mask, &image_coord_param);
    shader_glsl_add_src_param_ext(ins->ctx, &ins->src[1], WINED3DSP_WRITEMASK_ALL, &image_data_param, data_type);
    shader_addline(ins->ctx->buffer, "imageStore(%s_image%u, %s, %s);\n",
            shader_glsl_get_prefix(version->type), uav_idx,
            image_coord_param.param_str, image_data_param.param_str);
}

static void shader_glsl_store_raw_structured(const struct wined3d_shader_instruction *ins)
{
    const char *prefix = shader_glsl_get_prefix(ins->ctx->reg_maps->shader_version.type);
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param structure_idx, offset, data;
    unsigned int i, resource_idx, stride, src_idx = 0;
    struct wined3d_string_buffer *address;
    DWORD write_mask;
    BOOL is_tgsm;

    resource_idx = ins->dst[0].reg.idx[0].offset;
    is_tgsm = ins->dst[0].reg.type == WINED3DSPR_GROUPSHAREDMEM;
    if (is_tgsm)
    {
        if (resource_idx >= reg_maps->tgsm_count)
        {
            ERR("Invalid TGSM index %u.\n", resource_idx);
            return;
        }
        stride = reg_maps->tgsm[resource_idx].stride;
    }
    else
    {
        if (resource_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
        {
            ERR("Invalid UAV index %u.\n", resource_idx);
            return;
        }
        stride = reg_maps->uav_resource_info[resource_idx].stride;
    }

    address = string_buffer_get(priv->string_buffers);
    if (ins->handler_idx == WINED3DSIH_STORE_STRUCTURED)
    {
        shader_glsl_add_src_param(ins, &ins->src[src_idx++], WINED3DSP_WRITEMASK_0, &structure_idx);
        shader_addline(address, "%s * %u + ", structure_idx.param_str, stride);
    }
    shader_glsl_add_src_param(ins, &ins->src[src_idx++], WINED3DSP_WRITEMASK_0, &offset);
    shader_addline(address, "%s / 4", offset.param_str);

    for (i = 0; i < 4; ++i)
    {
        if (!(write_mask = ins->dst[0].write_mask & (WINED3DSP_WRITEMASK_0 << i)))
            continue;

        shader_glsl_add_src_param(ins, &ins->src[src_idx], write_mask, &data);

        if (is_tgsm)
            shader_addline(buffer, "%s_g%u[%s + %u] = %s;\n",
                    prefix, resource_idx, address->buffer, i, data.param_str);
        else
            shader_addline(buffer, "imageStore(%s_image%u, %s + %u, uvec4(%s, 0, 0, 0));\n",
                    prefix, resource_idx, address->buffer, i, data.param_str);
    }

    string_buffer_release(priv->string_buffers, address);
}

static void shader_glsl_sync(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    unsigned int sync_flags = ins->flags;

    if (sync_flags & WINED3DSSF_THREAD_GROUP)
    {
        shader_addline(buffer, "barrier();\n");
        sync_flags &= ~(WINED3DSSF_THREAD_GROUP | WINED3DSSF_GROUP_SHARED_MEMORY);
    }

    if (sync_flags & WINED3DSSF_GROUP_SHARED_MEMORY)
    {
        shader_addline(buffer, "memoryBarrierShared();\n");
        sync_flags &= ~WINED3DSSF_GROUP_SHARED_MEMORY;
    }

    if (sync_flags)
        FIXME("Unhandled sync flags %#x.\n", sync_flags);
}

static const struct wined3d_shader_resource_info *shader_glsl_get_resource_info(
        const struct wined3d_shader_instruction *ins, const struct wined3d_shader_register *reg)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    unsigned int idx = reg->idx[0].offset;

    if (reg->type == WINED3DSPR_RESOURCE)
    {
        if (idx >= ARRAY_SIZE(reg_maps->resource_info))
        {
            ERR("Invalid resource index %u.\n", idx);
            return NULL;
        }
        return &reg_maps->resource_info[idx];
    }

    if (reg->type == WINED3DSPR_UAV)
    {
        if (idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
        {
            ERR("Invalid UAV index %u.\n", idx);
            return NULL;
        }
        return &reg_maps->uav_resource_info[idx];
    }

    FIXME("Unhandled register type %#x.\n", reg->type);
    return NULL;
}

static void shader_glsl_bufinfo(const struct wined3d_shader_instruction *ins)
{
    const char *prefix = shader_glsl_get_prefix(ins->ctx->reg_maps->shader_version.type);
    const struct wined3d_shader_resource_info *resource_info;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    unsigned int resource_idx;
    char dst_swizzle[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_get_swizzle(&ins->src[0], FALSE, write_mask, dst_swizzle);

    if (!(resource_info = shader_glsl_get_resource_info(ins, &ins->src[0].reg)))
        return;
    resource_idx = ins->src[0].reg.idx[0].offset;

    shader_addline(buffer, "ivec2(");
    if (ins->src[0].reg.type == WINED3DSPR_RESOURCE)
    {
        unsigned int bind_idx = shader_glsl_find_sampler(&ins->ctx->reg_maps->sampler_map,
                resource_idx, WINED3D_SAMPLER_DEFAULT);
        shader_addline(buffer, "textureSize(%s_sampler%u)", prefix, bind_idx);
    }
    else
    {
        shader_addline(buffer, "imageSize(%s_image%u)", prefix, resource_idx);
    }
    if (resource_info->stride)
        shader_addline(buffer, " / %u", resource_info->stride);
    else if (resource_info->flags & WINED3D_VIEW_BUFFER_RAW)
        shader_addline(buffer, " * 4");
    shader_addline(buffer, ", %u)%s);\n", resource_info->stride, dst_swizzle);
}

static BOOL is_multisampled(enum wined3d_shader_resource_type resource_type)
{
    return resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2DMS
            || resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY;
}

static BOOL is_mipmapped(enum wined3d_shader_resource_type resource_type)
{
    return resource_type != WINED3D_SHADER_RESOURCE_BUFFER && !is_multisampled(resource_type);
}

static void shader_glsl_resinfo(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_version *version = &ins->ctx->reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_shader_register_type reg_type;
    unsigned int resource_idx, bind_idx, i;
    enum wined3d_data_type dst_data_type;
    struct glsl_src_param lod_param;
    BOOL supports_mipmaps;
    char dst_swizzle[6];
    DWORD write_mask;

    dst_data_type = ins->dst[0].reg.data_type;
    if (ins->flags == WINED3DSI_RESINFO_UINT)
        dst_data_type = WINED3D_DATA_UINT;
    else if (ins->flags)
        FIXME("Unhandled flags %#x.\n", ins->flags);

    reg_type = ins->src[1].reg.type;
    resource_idx = ins->src[1].reg.idx[0].offset;
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &lod_param);
    if (reg_type == WINED3DSPR_RESOURCE)
    {
        resource_type = ins->ctx->reg_maps->resource_info[resource_idx].type;
        bind_idx = shader_glsl_find_sampler(&ins->ctx->reg_maps->sampler_map,
                resource_idx, WINED3D_SAMPLER_DEFAULT);
    }
    else
    {
        resource_type = ins->ctx->reg_maps->uav_resource_info[resource_idx].type;
        bind_idx = resource_idx;
    }

    if (resource_type >= ARRAY_SIZE(resource_type_info))
    {
        ERR("Unexpected resource type %#x.\n", resource_type);
        return;
    }

    write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], dst_data_type);
    shader_glsl_get_swizzle(&ins->src[1], FALSE, write_mask, dst_swizzle);

    if (dst_data_type == WINED3D_DATA_UINT)
        shader_addline(buffer, "uvec4(");
    else
        shader_addline(buffer, "vec4(");

    if (reg_type == WINED3DSPR_RESOURCE)
    {
        shader_addline(buffer, "textureSize(%s_sampler%u",
                shader_glsl_get_prefix(version->type), bind_idx);
    }
    else
    {
        shader_addline(buffer, "imageSize(%s_image%u",
                shader_glsl_get_prefix(version->type), bind_idx);
    }

    supports_mipmaps = is_mipmapped(resource_type) && reg_type != WINED3DSPR_UAV;
    if (supports_mipmaps)
        shader_addline(buffer, ", %s", lod_param.param_str);
    shader_addline(buffer, "), ");

    for (i = 0; i < 3 - resource_type_info[resource_type].resinfo_size; ++i)
        shader_addline(buffer, "0, ");

    if (supports_mipmaps)
    {
        if (gl_info->supported[ARB_TEXTURE_QUERY_LEVELS])
        {
            shader_addline(buffer, "textureQueryLevels(%s_sampler%u)",
                    shader_glsl_get_prefix(version->type), bind_idx);
        }
        else
        {
            FIXME("textureQueryLevels is not supported, returning 1 level.\n");
            shader_addline(buffer, "1");
        }
    }
    else
    {
        shader_addline(buffer, "1");
    }

    shader_addline(buffer, ")%s);\n", dst_swizzle);
}

static void shader_glsl_sample_info(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const struct wined3d_shader_dst_param *dst = ins->dst;
    const struct wined3d_shader_src_param *src = ins->src;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_data_type dst_data_type;
    unsigned int resource_idx, bind_idx;
    char dst_swizzle[6];
    DWORD write_mask;

    dst_data_type = dst->reg.data_type;
    if (ins->flags == WINED3DSI_SAMPLE_INFO_UINT)
        dst_data_type = WINED3D_DATA_UINT;
    else if (ins->flags)
        FIXME("Unhandled flags %#x.\n", ins->flags);

    write_mask = shader_glsl_append_dst_ext(buffer, ins, dst, dst_data_type);
    shader_glsl_get_swizzle(src, FALSE, write_mask, dst_swizzle);

    if (dst_data_type == WINED3D_DATA_UINT)
        shader_addline(buffer, "uvec4(");
    else
        shader_addline(buffer, "vec4(");

    if (src->reg.type == WINED3DSPR_RASTERIZER)
    {
        if (gl_info->supported[ARB_SAMPLE_SHADING])
        {
            shader_addline(buffer, "gl_NumSamples");
        }
        else
        {
            FIXME("OpenGL implementation does not support ARB_sample_shading.\n");
            shader_addline(buffer, "1");
        }
    }
    else
    {
        resource_idx = src->reg.idx[0].offset;
        resource_type = reg_maps->resource_info[resource_idx].type;
        if (resource_type >= ARRAY_SIZE(resource_type_info))
        {
            ERR("Unexpected resource type %#x.\n", resource_type);
            return;
        }
        bind_idx = shader_glsl_find_sampler(&reg_maps->sampler_map, resource_idx, WINED3D_SAMPLER_DEFAULT);

        if (gl_info->supported[ARB_SHADER_TEXTURE_IMAGE_SAMPLES])
        {
            shader_addline(buffer, "textureSamples(%s_sampler%u)",
                    shader_glsl_get_prefix(reg_maps->shader_version.type), bind_idx);
        }
        else
        {
            FIXME("textureSamples() is not supported.\n");
            shader_addline(buffer, "1");
        }
    }

    shader_addline(buffer, ", 0, 0, 0)%s);\n", dst_swizzle);
}

static void shader_glsl_interpolate(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_src_param *input = &ins->src[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param sample_param;
    char dst_swizzle[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_get_swizzle(input, FALSE, write_mask, dst_swizzle);

    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &sample_param);
    shader_addline(buffer, "interpolateAtSample(shader_in.reg%u, %s)%s);\n",
            input->reg.idx[0].offset, sample_param.param_str, dst_swizzle);
}

static void shader_glsl_ld(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    struct glsl_src_param coord_param, lod_param, sample_param;
    unsigned int resource_idx, sampler_idx, sampler_bind_idx;
    struct glsl_sample_function sample_function;
    DWORD flags = WINED3D_GLSL_SAMPLE_LOAD;
    BOOL has_lod_param;

    if (wined3d_shader_instruction_has_texel_offset(ins))
        flags |= WINED3D_GLSL_SAMPLE_OFFSET;

    resource_idx = ins->src[1].reg.idx[0].offset;
    sampler_idx = WINED3D_SAMPLER_DEFAULT;

    if (resource_idx >= ARRAY_SIZE(reg_maps->resource_info))
    {
        ERR("Invalid resource index %u.\n", resource_idx);
        return;
    }
    has_lod_param = is_mipmapped(reg_maps->resource_info[resource_idx].type);

    shader_glsl_get_sample_function(ins->ctx, resource_idx, sampler_idx, flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &lod_param);
    sampler_bind_idx = shader_glsl_find_sampler(&reg_maps->sampler_map, resource_idx, sampler_idx);
    if (is_multisampled(reg_maps->resource_info[resource_idx].type))
    {
        shader_glsl_add_src_param(ins, &ins->src[2], WINED3DSP_WRITEMASK_0, &sample_param);
        shader_glsl_gen_sample_code(ins, sampler_bind_idx, &sample_function, ins->src[1].swizzle,
                NULL, NULL, NULL, &ins->texel_offset, "%s, %s", coord_param.param_str, sample_param.param_str);
    }
    else
    {
        shader_glsl_gen_sample_code(ins, sampler_bind_idx, &sample_function, ins->src[1].swizzle,
                NULL, NULL, has_lod_param ? lod_param.param_str : NULL, &ins->texel_offset,
                "%s", coord_param.param_str);
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_sample(const struct wined3d_shader_instruction *ins)
{
    const char *lod_param_str = NULL, *dx_param_str = NULL, *dy_param_str = NULL;
    struct glsl_src_param coord_param, lod_param, dx_param, dy_param;
    unsigned int resource_idx, sampler_idx, sampler_bind_idx;
    struct glsl_sample_function sample_function;
    DWORD flags = 0;

    if (ins->handler_idx == WINED3DSIH_SAMPLE_GRAD)
        flags |= WINED3D_GLSL_SAMPLE_GRAD;
    if (ins->handler_idx == WINED3DSIH_SAMPLE_LOD)
        flags |= WINED3D_GLSL_SAMPLE_LOD;
    if (wined3d_shader_instruction_has_texel_offset(ins))
        flags |= WINED3D_GLSL_SAMPLE_OFFSET;

    resource_idx = ins->src[1].reg.idx[0].offset;
    sampler_idx = ins->src[2].reg.idx[0].offset;

    shader_glsl_get_sample_function(ins->ctx, resource_idx, sampler_idx, flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);

    switch (ins->handler_idx)
    {
        case WINED3DSIH_SAMPLE:
            break;
        case WINED3DSIH_SAMPLE_B:
            shader_glsl_add_src_param(ins, &ins->src[3], WINED3DSP_WRITEMASK_0, &lod_param);
            lod_param_str = lod_param.param_str;
            break;
        case WINED3DSIH_SAMPLE_GRAD:
            shader_glsl_add_src_param(ins, &ins->src[3], sample_function.deriv_mask, &dx_param);
            shader_glsl_add_src_param(ins, &ins->src[4], sample_function.deriv_mask, &dy_param);
            dx_param_str = dx_param.param_str;
            dy_param_str = dy_param.param_str;
            break;
        case WINED3DSIH_SAMPLE_LOD:
            shader_glsl_add_src_param(ins, &ins->src[3], WINED3DSP_WRITEMASK_0, &lod_param);
            lod_param_str = lod_param.param_str;
            break;
        default:
            ERR("Unhandled opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    sampler_bind_idx = shader_glsl_find_sampler(&ins->ctx->reg_maps->sampler_map, resource_idx, sampler_idx);
    shader_glsl_gen_sample_code(ins, sampler_bind_idx, &sample_function, ins->src[1].swizzle,
            dx_param_str, dy_param_str, lod_param_str, &ins->texel_offset, "%s", coord_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/* GLSL doesn't provide a function to sample from level zero with depth
 * comparison for array textures and cube textures. We use textureGrad*()
 * to implement sample_c_lz.
 */
static void shader_glsl_gen_sample_c_lz_emulation(const struct wined3d_shader_instruction *ins,
        unsigned int sampler_bind_idx, const struct glsl_sample_function *sample_function,
        unsigned int coord_size, const char *coord_param, const char *ref_param)
{
    const struct wined3d_shader_version *version = &ins->ctx->reg_maps->shader_version;
    unsigned int deriv_size = wined3d_popcount(sample_function->deriv_mask);
    const struct wined3d_shader_texel_offset *offset = &ins->texel_offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_swizzle[6];

    WARN("Emitting textureGrad() for sample_c_lz.\n");

    shader_glsl_swizzle_to_str(WINED3DSP_NOSWIZZLE, FALSE, ins->dst[0].write_mask, dst_swizzle);
    shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], sample_function->data_type);
    shader_addline(buffer, "vec4(textureGrad%s(%s_sampler%u, vec%u(%s, %s), vec%u(0.0), vec%u(0.0)",
            sample_function->offset_size ? "Offset" : "",
            shader_glsl_get_prefix(version->type), sampler_bind_idx,
            coord_size, coord_param, ref_param, deriv_size, deriv_size);
    if (sample_function->offset_size)
    {
        int offset_immdata[4] = {offset->u, offset->v, offset->w};
        shader_addline(buffer, ", ");
        shader_glsl_append_imm_ivec(buffer, offset_immdata, sample_function->offset_size);
    }
    shader_addline(buffer, "))%s);\n", dst_swizzle);
}

static void shader_glsl_sample_c(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    unsigned int resource_idx, sampler_idx, sampler_bind_idx;
    const struct wined3d_shader_resource_info *resource_info;
    struct glsl_src_param coord_param, compare_param;
    struct glsl_sample_function sample_function;
    const char *lod_param = NULL;
    unsigned int coord_size;
    DWORD flags = 0;

    if (ins->handler_idx == WINED3DSIH_SAMPLE_C_LZ)
    {
        lod_param = "0";
        flags |= WINED3D_GLSL_SAMPLE_LOD;
    }

    if (wined3d_shader_instruction_has_texel_offset(ins))
        flags |= WINED3D_GLSL_SAMPLE_OFFSET;

    if (!(resource_info = shader_glsl_get_resource_info(ins, &ins->src[1].reg)))
        return;
    resource_idx = ins->src[1].reg.idx[0].offset;
    sampler_idx = ins->src[2].reg.idx[0].offset;

    shader_glsl_get_sample_function(ins->ctx, resource_idx, sampler_idx, flags, &sample_function);
    coord_size = shader_glsl_get_write_mask_size(sample_function.coord_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask >> 1, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[3], WINED3DSP_WRITEMASK_0, &compare_param);
    sampler_bind_idx = shader_glsl_find_sampler(&ins->ctx->reg_maps->sampler_map, resource_idx, sampler_idx);
    if (ins->handler_idx == WINED3DSIH_SAMPLE_C_LZ
            && !gl_info->supported[EXT_TEXTURE_SHADOW_LOD]
            && (resource_info->type == WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY
            || resource_info->type == WINED3D_SHADER_RESOURCE_TEXTURE_CUBE))
    {
        shader_glsl_gen_sample_c_lz_emulation(ins, sampler_bind_idx, &sample_function,
                coord_size, coord_param.param_str, compare_param.param_str);
    }
    else
    {
        shader_glsl_gen_sample_code(ins, sampler_bind_idx, &sample_function, WINED3DSP_NOSWIZZLE,
                NULL, NULL, lod_param, &ins->texel_offset, "vec%u(%s, %s)",
                coord_size, coord_param.param_str, compare_param.param_str);
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_gather4(const struct wined3d_shader_instruction *ins)
{
    unsigned int resource_param_idx, resource_idx, sampler_idx, sampler_bind_idx, component_idx;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const char *prefix = shader_glsl_get_prefix(reg_maps->shader_version.type);
    struct glsl_src_param coord_param, compare_param, offset_param;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    const struct wined3d_shader_resource_info *resource_info;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    unsigned int coord_size, offset_size;
    char dst_swizzle[6];
    BOOL has_offset;

    if (!gl_info->supported[ARB_TEXTURE_GATHER])
    {
        FIXME("OpenGL implementation does not support textureGather.\n");
        return;
    }

    has_offset = ins->handler_idx == WINED3DSIH_GATHER4_PO
            || ins->handler_idx == WINED3DSIH_GATHER4_PO_C
            || wined3d_shader_instruction_has_texel_offset(ins);

    resource_param_idx =
            (ins->handler_idx == WINED3DSIH_GATHER4_PO || ins->handler_idx == WINED3DSIH_GATHER4_PO_C) ? 2 : 1;
    resource_idx = ins->src[resource_param_idx].reg.idx[0].offset;
    sampler_idx = ins->src[resource_param_idx + 1].reg.idx[0].offset;
    component_idx = shader_glsl_swizzle_get_component(ins->src[resource_param_idx + 1].swizzle, 0);
    sampler_bind_idx = shader_glsl_find_sampler(&reg_maps->sampler_map, resource_idx, sampler_idx);

    if (!(resource_info = shader_glsl_get_resource_info(ins, &ins->src[resource_param_idx].reg)))
        return;

    if (resource_info->type >= ARRAY_SIZE(resource_type_info))
    {
        ERR("Unexpected resource type %#x.\n", resource_info->type);
        return;
    }
    shader_glsl_get_coord_size(resource_info->type, &coord_size, &offset_size);

    shader_glsl_swizzle_to_str(ins->src[resource_param_idx].swizzle, FALSE, ins->dst[0].write_mask, dst_swizzle);
    shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], resource_info->data_type);

    shader_glsl_add_src_param(ins, &ins->src[0], (1u << coord_size) - 1, &coord_param);

    shader_addline(buffer, "textureGather%s(%s_sampler%u, %s",
            has_offset ? "Offset" : "", prefix, sampler_bind_idx, coord_param.param_str);
    if (ins->handler_idx == WINED3DSIH_GATHER4_C || ins->handler_idx == WINED3DSIH_GATHER4_PO_C)
    {
        shader_glsl_add_src_param(ins, &ins->src[resource_param_idx + 2], WINED3DSP_WRITEMASK_0, &compare_param);
        shader_addline(buffer, ", %s", compare_param.param_str);
    }
    if (ins->handler_idx == WINED3DSIH_GATHER4_PO || ins->handler_idx == WINED3DSIH_GATHER4_PO_C)
    {
        shader_glsl_add_src_param(ins, &ins->src[1], (1u << offset_size) - 1, &offset_param);
        shader_addline(buffer, ", %s", offset_param.param_str);
    }
    else if (has_offset)
    {
        int offset_immdata[4] = {ins->texel_offset.u, ins->texel_offset.v, ins->texel_offset.w};
        shader_addline(buffer, ", ");
        shader_glsl_append_imm_ivec(buffer, offset_immdata, offset_size);
    }
    if (component_idx)
        shader_addline(buffer, ", %u", component_idx);

    shader_addline(buffer, ")%s);\n", dst_swizzle);
}

static void shader_glsl_texcoord(const struct wined3d_shader_instruction *ins)
{
    /* FIXME: Make this work for more than just 2D textures */
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    DWORD write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);

    if (!(ins->ctx->reg_maps->shader_version.major == 1 && ins->ctx->reg_maps->shader_version.minor == 4))
    {
        char dst_mask[6];

        shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
        shader_addline(buffer, "clamp(ffp_texcoord[%u], 0.0, 1.0)%s);\n",
                ins->dst[0].reg.idx[0].offset, dst_mask);
    }
    else
    {
        enum wined3d_shader_src_modifier src_mod = ins->src[0].modifiers;
        DWORD reg = ins->src[0].reg.idx[0].offset;
        char dst_swizzle[6];

        shader_glsl_get_swizzle(&ins->src[0], FALSE, write_mask, dst_swizzle);

        if (src_mod == WINED3DSPSM_DZ || src_mod == WINED3DSPSM_DW)
        {
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
            struct glsl_src_param div_param;
            DWORD src_writemask = src_mod == WINED3DSPSM_DZ ? WINED3DSP_WRITEMASK_2 : WINED3DSP_WRITEMASK_3;

            shader_glsl_add_src_param(ins, &ins->src[0], src_writemask, &div_param);

            if (mask_size > 1)
                shader_addline(buffer, "ffp_texcoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            else
                shader_addline(buffer, "ffp_texcoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
        }
        else
        {
            shader_addline(buffer, "ffp_texcoord[%u]%s);\n", reg, dst_swizzle);
        }
    }
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
static void shader_glsl_texdp3tex(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct glsl_src_param src0_param;
    UINT mask_size;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    /* Do I have to take care about the projected bit? I don't think so, since the dp3 returns only one
     * scalar, and projected sampling would require 4.
     *
     * It is a dependent read - not valid with conditional NP2 textures
     */
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, 0, &sample_function);
    mask_size = shader_glsl_get_write_mask_size(sample_function.coord_mask);

    switch(mask_size)
    {
        case 1:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    NULL, "dot(ffp_texcoord[%u].xyz, %s)", sampler_idx, src0_param.param_str);
            break;

        case 2:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    NULL, "vec2(dot(ffp_texcoord[%u].xyz, %s), 0.0)", sampler_idx, src0_param.param_str);
            break;

        case 3:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    NULL, "vec3(dot(ffp_texcoord[%u].xyz, %s), 0.0, 0.0)", sampler_idx, src0_param.param_str);
            break;

        default:
            FIXME("Unexpected mask size %u\n", mask_size);
            break;
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXDP3 instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
static void shader_glsl_texdp3(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dstreg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;
    DWORD dst_mask;
    unsigned int mask_size;

    dst_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(dst_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    if (mask_size > 1) {
        shader_addline(ins->ctx->buffer, "vec%d(dot(T%u.xyz, %s)));\n", mask_size, dstreg, src0_param.param_str);
    } else {
        shader_addline(ins->ctx->buffer, "dot(T%u.xyz, %s));\n", dstreg, src0_param.param_str);
    }
}

/** Process the WINED3DSIO_TEXDEPTH instruction in GLSL:
 * Calculate the depth as dst.x / dst.y   */
static void shader_glsl_texdepth(const struct wined3d_shader_instruction *ins)
{
    struct glsl_dst_param dst_param;

    shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);

    /* Tests show that texdepth never returns anything below 0.0, and that r5.y is clamped to 1.0.
     * Negative input is accepted, -0.25 / -0.5 returns 0.5. GL should clamp gl_FragDepth to [0;1], but
     * this doesn't always work, so clamp the results manually. Whether or not the x value is clamped at 1
     * too is irrelevant, since if x = 0, any y value < 1.0 (and > 1.0 is not allowed) results in a result
     * >= 1.0 or < 0.0
     */
    shader_addline(ins->ctx->buffer, "gl_FragDepth = clamp((%s.x / min(%s.y, 1.0)), 0.0, 1.0);\n",
            dst_param.reg_name, dst_param.reg_name);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in GLSL:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
static void shader_glsl_texm3x2depth(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dstreg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    shader_addline(ins->ctx->buffer, "tmp0.y = dot(T%u.xyz, %s);\n", dstreg, src0_param.param_str);
    shader_addline(ins->ctx->buffer, "gl_FragDepth = (tmp0.y == 0.0) ? 1.0 : clamp(tmp0.x / tmp0.y, 0.0, 1.0);\n");
}

/** Process the WINED3DSIO_TEXM3X2PAD instruction in GLSL
 * Calculate the 1st of a 2-row matrix multiplication. */
static void shader_glsl_texm3x2pad(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
static void shader_glsl_texm3x3pad(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.%c = dot(T%u.xyz, %s);\n", 'x' + tex_mx->current_row, reg, src0_param.param_str);
    tex_mx->texcoord_w[tex_mx->current_row++] = reg;
}

static void shader_glsl_texm3x2tex(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_sample_function sample_function;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.y = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    shader_glsl_get_sample_function(ins->ctx, reg, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL, "tmp0.xy");
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXM3X3TEX instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply, then sample the texture using the calculated coordinates */
static void shader_glsl_texm3x3tex(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    struct glsl_sample_function sample_function;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(ins->ctx->buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL, "tmp0.xyz");
    shader_glsl_release_sample_function(ins->ctx, &sample_function);

    tex_mx->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3 instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply */
static void shader_glsl_texm3x3(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;
    char dst_mask[6];

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
    shader_addline(ins->ctx->buffer, "vec4(tmp0.xy, dot(T%u.xyz, %s), 1.0)%s);\n", reg, src0_param.param_str, dst_mask);

    tex_mx->current_row = 0;
}

/* Process the WINED3DSIO_TEXM3X3SPEC instruction in GLSL
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void shader_glsl_texm3x3spec(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct glsl_sample_function sample_function;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    char coord_mask[6];

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_mask, &src1_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
    /* Reflection calculation */
    shader_addline(buffer, "tmp0.xyz = -reflect((%s), normalize(tmp0.xyz));\n", src1_param.param_str);

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, reg, 0, &sample_function);
    shader_glsl_write_mask_to_str(sample_function.coord_mask, coord_mask);

    /* Sample the texture */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE,
            NULL, NULL, NULL, NULL, "tmp0%s", coord_mask);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);

    tex_mx->current_row = 0;
}

/* Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void shader_glsl_texm3x3vspec(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct glsl_sample_function sample_function;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;
    char coord_mask[6];

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_param.param_str);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "tmp1.xyz = normalize(vec3(ffp_texcoord[%u].w, ffp_texcoord[%u].w, ffp_texcoord[%u].w));\n",
            tex_mx->texcoord_w[0], tex_mx->texcoord_w[1], reg);
    shader_addline(buffer, "tmp0.xyz = -reflect(tmp1.xyz, normalize(tmp0.xyz));\n");

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, reg, 0, &sample_function);
    shader_glsl_write_mask_to_str(sample_function.coord_mask, coord_mask);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE,
            NULL, NULL, NULL, NULL, "tmp0%s", coord_mask);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);

    tex_mx->current_row = 0;
}

/** Process the WINED3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * texbem is pshader <= 1.3 only, this saves a few version checks
 */
static void shader_glsl_texbem(const struct wined3d_shader_instruction *ins)
{
    const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    struct glsl_sample_function sample_function;
    struct glsl_src_param coord_param;
    DWORD sampler_idx;
    DWORD mask;
    DWORD flags;
    char coord_mask[6];

    sampler_idx = ins->dst[0].reg.idx[0].offset;
    flags = (priv->cur_ps_args->tex_transform >> sampler_idx * WINED3D_PSARGS_TEXTRANSFORM_SHIFT)
            & WINED3D_PSARGS_TEXTRANSFORM_MASK;

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, 0, &sample_function);
    mask = sample_function.coord_mask;

    shader_glsl_write_mask_to_str(mask, coord_mask);

    /* With projected textures, texbem only divides the static texture coord,
     * not the displacement, so we can't let GL handle this. */
    if (flags & WINED3D_PSARGS_PROJECTED)
    {
        DWORD div_mask=0;
        char coord_div_mask[3];
        switch (flags & ~WINED3D_PSARGS_PROJECTED)
        {
            case WINED3D_TTFF_COUNT1:
                FIXME("WINED3D_TTFF_PROJECTED with WINED3D_TTFF_COUNT1?\n");
                break;
            case WINED3D_TTFF_COUNT2:
                div_mask = WINED3DSP_WRITEMASK_1;
                break;
            case WINED3D_TTFF_COUNT3:
                div_mask = WINED3DSP_WRITEMASK_2;
                break;
            case WINED3D_TTFF_COUNT4:
            case WINED3D_TTFF_DISABLE:
                div_mask = WINED3DSP_WRITEMASK_3;
                break;
        }
        shader_glsl_write_mask_to_str(div_mask, coord_div_mask);
        shader_addline(ins->ctx->buffer, "T%u%s /= T%u%s;\n", sampler_idx, coord_mask, sampler_idx, coord_div_mask);
    }

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &coord_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL,
            "T%u%s + vec4(bumpenv_mat%u * %s, 0.0, 0.0)%s", sampler_idx, coord_mask, sampler_idx,
            coord_param.param_str, coord_mask);

    if (ins->handler_idx == WINED3DSIH_TEXBEML)
    {
        struct glsl_src_param luminance_param;
        struct glsl_dst_param dst_param;

        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_2, &luminance_param);
        shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);

        shader_addline(ins->ctx->buffer, "%s%s *= (%s * bumpenv_lum_scale%u + bumpenv_lum_offset%u);\n",
                dst_param.reg_name, dst_param.mask_str,
                luminance_param.param_str, sampler_idx, sampler_idx);
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_bem(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param, src1_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src1_param);

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_addline(ins->ctx->buffer, "%s + bumpenv_mat%u * %s);\n",
            src0_param.param_str, sampler_idx, src1_param.param_str);
}

/** Process the WINED3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
static void shader_glsl_texreg2ar(const struct wined3d_shader_instruction *ins)
{
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct wined3d_string_buffer *reg_name;

    reg_name = string_buffer_get(priv->string_buffers);
    shader_glsl_get_register_name(&ins->src[0].reg, ins->src[0].reg.data_type, reg_name, NULL, ins->ctx);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL,
            "%s.wx", reg_name->buffer);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);

    string_buffer_release(priv->string_buffers, reg_name);
}

/** Process the WINED3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
static void shader_glsl_texreg2gb(const struct wined3d_shader_instruction *ins)
{
    struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct wined3d_string_buffer *reg_name;

    reg_name = string_buffer_get(priv->string_buffers);
    shader_glsl_get_register_name(&ins->src[0].reg, ins->src[0].reg.data_type, reg_name, NULL, ins->ctx);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL,
            "%s.yz", reg_name->buffer);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);

    string_buffer_release(priv->string_buffers, reg_name);
}

/** Process the WINED3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
static void shader_glsl_texreg2rgb(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct glsl_src_param src0_param;

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sampler_idx, 0, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &src0_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, NULL,
            "%s", src0_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
static void shader_glsl_texkill(const struct wined3d_shader_instruction *ins)
{
    if (ins->ctx->reg_maps->shader_version.major >= 4)
    {
        shader_glsl_generate_condition(ins);
        shader_addline(ins->ctx->buffer, "    discard;\n");
    }
    else
    {
        struct glsl_dst_param dst_param;

        /* The argument is a destination parameter, and no writemasks are allowed */
        shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);

        /* 2.0 shaders compare all 4 components in texkill. */
        if (ins->ctx->reg_maps->shader_version.major >= 2)
            shader_addline(ins->ctx->buffer, "if (any(lessThan(%s.xyzw, vec4(0.0)))) discard;\n", dst_param.reg_name);
        /* 1.x shaders only compare the first 3 components, probably due to
         * the nature of the texkill instruction as a tex* instruction, and
         * phase, which kills all .w components. Even if all 4 components are
         * defined, only the first 3 are used. */
        else
            shader_addline(ins->ctx->buffer, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;\n", dst_param.reg_name);
    }
}

/** Process the WINED3DSIO_DP2ADD instruction in GLSL.
 * dst = dot2(src0, src1) + src2 */
static void shader_glsl_dp2add(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    struct glsl_src_param src2_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(ins, &ins->src[2], WINED3DSP_WRITEMASK_0, &src2_param);

    if (mask_size > 1) {
        shader_addline(ins->ctx->buffer, "vec%d(dot(%s, %s) + %s));\n",
                mask_size, src0_param.param_str, src1_param.param_str, src2_param.param_str);
    } else {
        shader_addline(ins->ctx->buffer, "dot(%s, %s) + %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
    }
}

static void shader_glsl_input_pack(const struct wined3d_shader *shader, struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_signature *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps,
        const struct ps_compile_args *args, const struct wined3d_gl_info *gl_info, BOOL unroll)
{
    unsigned int i;

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &input_signature->elements[i];
        const char *semantic_name;
        UINT semantic_idx;
        char reg_mask[6];

        /* Unused */
        if (!(reg_maps->input_registers & (1u << input->register_idx)))
            continue;

        semantic_name = input->semantic_name;
        semantic_idx = input->semantic_idx;
        shader_glsl_write_mask_to_str(input->mask, reg_mask);

        if (args->vp_mode == WINED3D_VP_MODE_SHADER)
        {
            if (input->sysval_semantic == WINED3D_SV_POSITION && !semantic_idx)
            {
                shader_addline(buffer, "ps_in[%u]%s = vpos%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
            }
            else if (args->pointsprite && shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
            {
                shader_addline(buffer, "ps_in[%u] = vec4(gl_PointCoord.xy, 0.0, 0.0);\n", input->register_idx);
            }
            else if (input->sysval_semantic == WINED3D_SV_IS_FRONT_FACE)
            {
                shader_addline(buffer, "ps_in[%u]%s = uintBitsToFloat(gl_FrontFacing ? 0xffffffffu : 0u);\n",
                        input->register_idx, reg_mask);
            }
            else if (input->sysval_semantic == WINED3D_SV_SAMPLE_INDEX)
            {
                if (gl_info->supported[ARB_SAMPLE_SHADING])
                    shader_addline(buffer, "ps_in[%u]%s = intBitsToFloat(gl_SampleID);\n",
                            input->register_idx, reg_mask);
                else
                    FIXME("ARB_sample_shading is not supported.\n");
            }
            else if (input->sysval_semantic == WINED3D_SV_RENDER_TARGET_ARRAY_INDEX && !semantic_idx)
            {
                if (gl_info->supported[ARB_FRAGMENT_LAYER_VIEWPORT])
                    shader_addline(buffer, "ps_in[%u]%s = intBitsToFloat(gl_Layer);\n",
                            input->register_idx, reg_mask);
                else
                    FIXME("ARB_fragment_layer_viewport is not supported.\n");
            }
            else if (input->sysval_semantic == WINED3D_SV_VIEWPORT_ARRAY_INDEX && !semantic_idx)
            {
                if (gl_info->supported[ARB_VIEWPORT_ARRAY])
                    shader_addline(buffer, "ps_in[%u]%s = intBitsToFloat(gl_ViewportIndex);\n",
                            input->register_idx, reg_mask);
                else
                    FIXME("ARB_viewport_array is not supported.\n");
            }
            else
            {
                if (input->sysval_semantic)
                    FIXME("Unhandled sysval semantic %#x.\n", input->sysval_semantic);
                shader_addline(buffer, unroll ? "ps_in[%u]%s = %s%u%s;\n" : "ps_in[%u]%s = %s[%u]%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask,
                        shader_glsl_shader_input_name(gl_info),
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask);
            }
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
        {
            if (args->pointsprite)
                shader_addline(buffer, "ps_in[%u] = vec4(gl_PointCoord.xy, 0.0, 0.0);\n",
                        shader->u.ps.input_reg_map[input->register_idx]);
            else if (args->vp_mode == WINED3D_VP_MODE_NONE && args->texcoords_initialized & (1u << semantic_idx))
                shader_addline(buffer, "ps_in[%u]%s = %s[%u]%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask,
                        needs_legacy_glsl_syntax(gl_info)
                        ? "gl_TexCoord" : "ffp_varying_texcoord", semantic_idx, reg_mask);
            else
                shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
        {
            if (!semantic_idx)
                shader_addline(buffer, "ps_in[%u]%s = vec4(ffp_varying_diffuse)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
            else if (semantic_idx == 1)
                shader_addline(buffer, "ps_in[%u]%s = vec4(ffp_varying_specular)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
            else
                shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
        }
        else
        {
            shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                    shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
        }
    }
}

static void add_glsl_program_entry(struct shader_glsl_priv *priv, struct glsl_shader_prog_link *entry)
{
    struct glsl_program_key key;

    key.vs_id = entry->vs.id;
    key.hs_id = entry->hs.id;
    key.ds_id = entry->ds.id;
    key.gs_id = entry->gs.id;
    key.ps_id = entry->ps.id;
    key.cs_id = entry->cs.id;

    if (wine_rb_put(&priv->program_lookup, &key, &entry->program_lookup_entry) == -1)
    {
        ERR("Failed to insert program entry.\n");
    }
}

static struct glsl_shader_prog_link *get_glsl_program_entry(const struct shader_glsl_priv *priv,
        const struct glsl_program_key *key)
{
    struct wine_rb_entry *entry;

    entry = wine_rb_get(&priv->program_lookup, key);
    return entry ? WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry) : NULL;
}

/* Context activation is done by the caller. */
static void delete_glsl_program_entry(struct shader_glsl_priv *priv, const struct wined3d_gl_info *gl_info,
        struct glsl_shader_prog_link *entry)
{
    wine_rb_remove(&priv->program_lookup, &entry->program_lookup_entry);

    GL_EXTCALL(glDeleteProgram(entry->id));
    if (entry->vs.id)
        list_remove(&entry->vs.shader_entry);
    if (entry->hs.id)
        list_remove(&entry->hs.shader_entry);
    if (entry->ds.id)
        list_remove(&entry->ds.shader_entry);
    if (entry->gs.id)
        list_remove(&entry->gs.shader_entry);
    if (entry->ps.id)
        list_remove(&entry->ps.shader_entry);
    if (entry->cs.id)
        list_remove(&entry->cs.shader_entry);
    heap_free(entry);
}

static void shader_glsl_setup_vs3_output(struct shader_glsl_priv *priv,
        const struct wined3d_gl_info *gl_info, const DWORD *map,
        const struct wined3d_shader_signature *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps_in,
        const struct wined3d_shader_signature *output_signature,
        const struct wined3d_shader_reg_maps *reg_maps_out)
{
    struct wined3d_string_buffer *destination = string_buffer_get(&priv->string_buffers);
    const char *out_array_name = shader_glsl_shader_output_name(gl_info);
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    unsigned int in_count = vec4_varyings(3, gl_info);
    unsigned int max_varyings = needs_legacy_glsl_syntax(gl_info) ? in_count + 2 : in_count;
    DWORD in_idx, *set = NULL;
    unsigned int i, j;
    char reg_mask[6];

    set = heap_calloc(max_varyings, sizeof(*set));

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &input_signature->elements[i];

        if (!(reg_maps_in->input_registers & (1u << input->register_idx)))
            continue;

        in_idx = map[input->register_idx];
        /* Declared, but not read register */
        if (in_idx == ~0u)
            continue;
        if (in_idx >= max_varyings)
        {
            FIXME("More input varyings declared than supported, expect issues.\n");
            continue;
        }

        if (in_idx == in_count)
            string_buffer_sprintf(destination, "gl_FrontColor");
        else if (in_idx == in_count + 1)
            string_buffer_sprintf(destination, "gl_FrontSecondaryColor");
        else
            string_buffer_sprintf(destination, "%s[%u]", out_array_name, in_idx);

        if (!set[in_idx])
            set[in_idx] = ~0u;

        for (j = 0; j < output_signature->element_count; ++j)
        {
            const struct wined3d_shader_signature_element *output = &output_signature->elements[j];
            DWORD mask;

            if (!(reg_maps_out->output_registers & (1u << output->register_idx))
                    || input->semantic_idx != output->semantic_idx
                    || strcmp(input->semantic_name, output->semantic_name)
                    || !(mask = input->mask & output->mask))
                continue;

            if (set[in_idx] == ~0u)
                set[in_idx] = 0;
            set[in_idx] |= mask & reg_maps_out->u.output_registers_mask[output->register_idx];
            shader_glsl_write_mask_to_str(mask, reg_mask);

            shader_addline(buffer, "%s%s = outputs[%u]%s;\n",
                    destination->buffer, reg_mask, output->register_idx, reg_mask);
        }
    }

    for (i = 0; i < max_varyings; ++i)
    {
        unsigned int size;

        if (!set[i] || set[i] == WINED3DSP_WRITEMASK_ALL)
            continue;

        if (set[i] == ~0u)
            set[i] = 0;

        size = 0;
        if (!(set[i] & WINED3DSP_WRITEMASK_0))
            reg_mask[size++] = 'x';
        if (!(set[i] & WINED3DSP_WRITEMASK_1))
            reg_mask[size++] = 'y';
        if (!(set[i] & WINED3DSP_WRITEMASK_2))
            reg_mask[size++] = 'z';
        if (!(set[i] & WINED3DSP_WRITEMASK_3))
            reg_mask[size++] = 'w';
        reg_mask[size] = '\0';

        if (i == in_count)
            string_buffer_sprintf(destination, "gl_FrontColor");
        else if (i == in_count + 1)
            string_buffer_sprintf(destination, "gl_FrontSecondaryColor");
        else
            string_buffer_sprintf(destination, "%s[%u]", out_array_name, i);

        if (size == 1)
            shader_addline(buffer, "%s.%s = 0.0;\n", destination->buffer, reg_mask);
        else
            shader_addline(buffer, "%s.%s = vec%u(0.0);\n", destination->buffer, reg_mask, size);
    }

    heap_free(set);
    string_buffer_release(&priv->string_buffers, destination);
}

static void shader_glsl_setup_sm4_shader_output(struct shader_glsl_priv *priv,
        unsigned int input_count, const struct wined3d_shader_signature *output_signature,
        const struct wined3d_shader_reg_maps *reg_maps_out, const char *output_variable_name,
        BOOL rasterizer_setup)
{
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    char reg_mask[6];
    unsigned int i;

    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *output = &output_signature->elements[i];

        if (!(reg_maps_out->output_registers & (1u << output->register_idx)))
            continue;

        if (output->stream_idx)
            continue;

        if (output->register_idx >= input_count)
            continue;

        shader_glsl_write_mask_to_str(output->mask, reg_mask);

        shader_addline(buffer,
                rasterizer_setup ? "%s.reg%u%s = outputs[%u]%s;\n" : "%s.reg[%u]%s = outputs[%u]%s;\n",
                output_variable_name, output->register_idx, reg_mask, output->register_idx, reg_mask);
    }
}

static void shader_glsl_generate_clip_or_cull_distances(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_signature_element *element, DWORD clip_or_cull_distance_mask)
{
    unsigned int i, clip_or_cull_index;
    const char *name;
    char reg_mask[6];

    name = element->sysval_semantic == WINED3D_SV_CLIP_DISTANCE ? "Clip" : "Cull";
    /* Assign consecutive indices starting from 0. */
    clip_or_cull_index = element->semantic_idx ? wined3d_popcount(clip_or_cull_distance_mask & 0xf) : 0;
    for (i = 0; i < 4; ++i)
    {
        if (!(element->mask & (WINED3DSP_WRITEMASK_0 << i)))
            continue;

        shader_glsl_write_mask_to_str(WINED3DSP_WRITEMASK_0 << i, reg_mask);
        shader_addline(buffer, "gl_%sDistance[%u] = outputs[%u]%s;\n",
                name, clip_or_cull_index, element->register_idx, reg_mask);
        ++clip_or_cull_index;
    }
}

static void shader_glsl_setup_sm3_rasterizer_input(struct shader_glsl_priv *priv,
        const struct wined3d_gl_info *gl_info, const DWORD *map,
        const struct wined3d_shader_signature *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps_in, unsigned int input_count,
        const struct wined3d_shader_signature *output_signature,
        const struct wined3d_shader_reg_maps *reg_maps_out, BOOL per_vertex_point_size)
{
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    const char *semantic_name;
    unsigned int semantic_idx;
    char reg_mask[6];
    unsigned int i;

    /* First, sort out position and point size system values. */
    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *output = &output_signature->elements[i];

        if (!(reg_maps_out->output_registers & (1u << output->register_idx)))
            continue;

        if (output->stream_idx)
            continue;

        semantic_name = output->semantic_name;
        semantic_idx = output->semantic_idx;
        shader_glsl_write_mask_to_str(output->mask, reg_mask);

        if (output->sysval_semantic == WINED3D_SV_POSITION && !semantic_idx)
        {
            shader_addline(buffer, "gl_Position%s = outputs[%u]%s;\n",
                    reg_mask, output->register_idx, reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE) && per_vertex_point_size)
        {
            shader_addline(buffer, "gl_PointSize = clamp(outputs[%u].%c, "
                    "ffp_point.size_min, ffp_point.size_max);\n", output->register_idx, reg_mask[1]);
        }
        else if (output->sysval_semantic == WINED3D_SV_RENDER_TARGET_ARRAY_INDEX && !semantic_idx)
        {
            shader_addline(buffer, "gl_Layer = floatBitsToInt(outputs[%u])%s;\n",
                    output->register_idx, reg_mask);
        }
        else if (output->sysval_semantic == WINED3D_SV_VIEWPORT_ARRAY_INDEX && !semantic_idx)
        {
            shader_addline(buffer, "gl_ViewportIndex = floatBitsToInt(outputs[%u])%s;\n",
                    output->register_idx, reg_mask);
        }
        else if (output->sysval_semantic == WINED3D_SV_CLIP_DISTANCE)
        {
            shader_glsl_generate_clip_or_cull_distances(buffer, output, reg_maps_out->clip_distance_mask);
        }
        else if (output->sysval_semantic == WINED3D_SV_CULL_DISTANCE)
        {
            shader_glsl_generate_clip_or_cull_distances(buffer, output, reg_maps_out->cull_distance_mask);
        }
        else if (output->sysval_semantic)
        {
            FIXME("Unhandled sysval semantic %#x.\n", output->sysval_semantic);
        }
    }

    /* Then, setup the pixel shader input. */
    if (reg_maps_out->shader_version.major < 4)
        shader_glsl_setup_vs3_output(priv, gl_info, map, input_signature, reg_maps_in,
                output_signature, reg_maps_out);
    else
        shader_glsl_setup_sm4_shader_output(priv, input_count, output_signature, reg_maps_out, "shader_out", TRUE);
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_vs3_rasterizer_input_setup(struct shader_glsl_priv *priv,
        const struct wined3d_shader *vs, const struct wined3d_shader *ps,
        BOOL per_vertex_point_size, BOOL flatshading, const struct wined3d_gl_info *gl_info)
{
    const BOOL legacy_syntax = needs_legacy_glsl_syntax(gl_info);
    DWORD ps_major = ps ? ps->reg_maps.shader_version.major : 0;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    const char *semantic_name;
    UINT semantic_idx;
    char reg_mask[6];
    unsigned int i;
    GLuint ret;

    string_buffer_clear(buffer);

    shader_glsl_add_version_declaration(buffer, gl_info);

    if (per_vertex_point_size)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    float size_min;\n");
        shader_addline(buffer, "    float size_max;\n");
        shader_addline(buffer, "} ffp_point;\n");
    }

    if (ps_major < 3)
    {
        DWORD colors_written_mask[2] = {0};
        DWORD texcoords_written_mask[WINED3D_MAX_TEXTURES] = {0};

        if (!legacy_syntax)
        {
            declare_out_varying(gl_info, buffer, flatshading, "vec4 ffp_varying_diffuse;\n");
            declare_out_varying(gl_info, buffer, flatshading, "vec4 ffp_varying_specular;\n");
            declare_out_varying(gl_info, buffer, FALSE, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
            declare_out_varying(gl_info, buffer, FALSE, "float ffp_varying_fogcoord;\n");
        }

        shader_addline(buffer, "void setup_vs_output(in vec4 outputs[%u])\n{\n", vs->limits->packed_output);

        for (i = 0; i < vs->output_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *output = &vs->output_signature.elements[i];
            DWORD write_mask;

            if (!(vs->reg_maps.output_registers & (1u << output->register_idx)))
                continue;

            semantic_name = output->semantic_name;
            semantic_idx = output->semantic_idx;
            write_mask = output->mask;
            shader_glsl_write_mask_to_str(write_mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR) && semantic_idx < 2)
            {
                if (legacy_syntax)
                    shader_addline(buffer, "gl_Front%sColor%s = outputs[%u]%s;\n",
                            semantic_idx ? "Secondary" : "", reg_mask, output->register_idx, reg_mask);
                else
                    shader_addline(buffer, "ffp_varying_%s%s = clamp(outputs[%u]%s, 0.0, 1.0);\n",
                            semantic_idx ? "specular" : "diffuse", reg_mask, output->register_idx, reg_mask);

                colors_written_mask[semantic_idx] = write_mask;
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_POSITION) && !semantic_idx)
            {
                shader_addline(buffer, "gl_Position%s = outputs[%u]%s;\n",
                        reg_mask, output->register_idx, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
            {
                if (semantic_idx < WINED3D_MAX_TEXTURES)
                {
                    shader_addline(buffer, "%s[%u]%s = outputs[%u]%s;\n",
                            legacy_syntax ? "gl_TexCoord" : "ffp_varying_texcoord",
                            semantic_idx, reg_mask, output->register_idx, reg_mask);
                    texcoords_written_mask[semantic_idx] = write_mask;
                }
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE) && per_vertex_point_size)
            {
                shader_addline(buffer, "gl_PointSize = clamp(outputs[%u].%c, "
                        "ffp_point.size_min, ffp_point.size_max);\n", output->register_idx, reg_mask[1]);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_FOG))
            {
                shader_addline(buffer, "%s = clamp(outputs[%u].%c, 0.0, 1.0);\n",
                        legacy_syntax ? "gl_FogFragCoord" : "ffp_varying_fogcoord",
                        output->register_idx, reg_mask[1]);
            }
        }

        for (i = 0; i < 2; ++i)
        {
            if (colors_written_mask[i] != WINED3DSP_WRITEMASK_ALL)
            {
                shader_glsl_write_mask_to_str(~colors_written_mask[i] & WINED3DSP_WRITEMASK_ALL, reg_mask);
                if (!i)
                    shader_addline(buffer, "%s%s = vec4(1.0)%s;\n",
                            legacy_syntax ? "gl_FrontColor" : "ffp_varying_diffuse",
                            reg_mask, reg_mask);
                else
                    shader_addline(buffer, "%s%s = vec4(0.0)%s;\n",
                            legacy_syntax ? "gl_FrontSecondaryColor" : "ffp_varying_specular",
                            reg_mask, reg_mask);
            }
        }
        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (ps && !(ps->reg_maps.texcoord & (1u << i)))
                continue;

            if (texcoords_written_mask[i] != WINED3DSP_WRITEMASK_ALL)
            {
                if (!shader_glsl_full_ffp_varyings(gl_info) && !texcoords_written_mask[i])
                    continue;

                shader_glsl_write_mask_to_str(~texcoords_written_mask[i] & WINED3DSP_WRITEMASK_ALL, reg_mask);
                shader_addline(buffer, "%s[%u]%s = vec4(0.0)%s;\n",
                        legacy_syntax ? "gl_TexCoord" : "ffp_varying_texcoord", i, reg_mask, reg_mask);
            }
        }
    }
    else
    {
        unsigned int in_count = min(vec4_varyings(ps_major, gl_info), ps->limits->packed_input);

        shader_glsl_declare_shader_outputs(gl_info, buffer, in_count, FALSE, NULL);
        shader_addline(buffer, "void setup_vs_output(in vec4 outputs[%u])\n{\n", vs->limits->packed_output);
        shader_glsl_setup_sm3_rasterizer_input(priv, gl_info, ps->u.ps.input_reg_map, &ps->input_signature,
                &ps->reg_maps, 0, &vs->output_signature, &vs->reg_maps, per_vertex_point_size);
    }

    shader_addline(buffer, "}\n");

    ret = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
    checkGLcall("glCreateShader(GL_VERTEX_SHADER)");
    shader_glsl_compile(gl_info, ret, buffer->buffer);

    return ret;
}

static void shader_glsl_generate_stream_output_setup(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader *shader)
{
    const struct wined3d_stream_output_desc *so_desc = &shader->u.gs.so_desc;
    unsigned int i, register_idx, component_idx;

    shader_addline(buffer, "out shader_in_out\n{\n");
    for (i = 0; i < so_desc->element_count; ++i)
    {
        const struct wined3d_stream_output_element *e = &so_desc->elements[i];

        if (e->stream_idx)
        {
            FIXME("Unhandled stream %u.\n", e->stream_idx);
            continue;
        }
        if (!e->semantic_name)
            continue;
        if (!shader_get_stream_output_register_info(shader, e, &register_idx, &component_idx))
            continue;

        if (component_idx || e->component_count != 4)
        {
            if (e->component_count == 1)
                shader_addline(buffer, "float");
            else
                shader_addline(buffer, "vec%u", e->component_count);

            shader_addline(buffer, " reg%u_%u_%u;\n",
                    register_idx, component_idx, component_idx + e->component_count - 1);
        }
        else
        {
            shader_addline(buffer, "vec4 reg%u;\n", register_idx);
        }
    }
    shader_addline(buffer, "} shader_out;\n");

    shader_addline(buffer, "void setup_gs_output(in vec4 outputs[%u])\n{\n",
            shader->limits->packed_output);
    for (i = 0; i < so_desc->element_count; ++i)
    {
        const struct wined3d_stream_output_element *e = &so_desc->elements[i];

        if (e->stream_idx)
        {
            FIXME("Unhandled stream %u.\n", e->stream_idx);
            continue;
        }
        if (!e->semantic_name)
            continue;
        if (!shader_get_stream_output_register_info(shader, e, &register_idx, &component_idx))
            continue;

        if (component_idx || e->component_count != 4)
        {
            DWORD write_mask;
            char str_mask[6];

            write_mask = ((1u << e->component_count) - 1) << component_idx;
            shader_glsl_write_mask_to_str(write_mask, str_mask);
            shader_addline(buffer, "shader_out.reg%u_%u_%u = outputs[%u]%s;\n",
                    register_idx, component_idx, component_idx + e->component_count - 1,
                    register_idx, str_mask);
        }
        else
        {
            shader_addline(buffer, "shader_out.reg%u = outputs[%u];\n", register_idx, register_idx);
        }
    }
    shader_addline(buffer, "}\n");
}

static void shader_glsl_generate_sm4_output_setup(struct shader_glsl_priv *priv,
        const struct wined3d_shader *shader, unsigned int input_count,
        const struct wined3d_gl_info *gl_info, BOOL rasterizer_setup, const DWORD *interpolation_mode)
{
    const char *prefix = shader_glsl_get_prefix(shader->reg_maps.shader_version.type);
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;

    if (rasterizer_setup)
        input_count = min(vec4_varyings(4, gl_info), input_count);

    if (input_count)
        shader_glsl_declare_shader_outputs(gl_info, buffer, input_count, rasterizer_setup, interpolation_mode);

    shader_addline(buffer, "void setup_%s_output(in vec4 outputs[%u])\n{\n",
            prefix, shader->limits->packed_output);

    if (rasterizer_setup)
        shader_glsl_setup_sm3_rasterizer_input(priv, gl_info, NULL, NULL,
                NULL, input_count, &shader->output_signature, &shader->reg_maps, FALSE);
    else
        shader_glsl_setup_sm4_shader_output(priv, input_count, &shader->output_signature,
                &shader->reg_maps, "shader_out", rasterizer_setup);

    shader_addline(buffer, "}\n");
}

static void shader_glsl_generate_patch_constant_name(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_signature_element *constant, unsigned int *user_constant_idx,
        const char *reg_mask)
{
    if (!constant->sysval_semantic)
    {
        shader_addline(buffer, "user_patch_constant[%u]%s", (*user_constant_idx)++, reg_mask);
        return;
    }

    switch (constant->sysval_semantic)
    {
        case WINED3D_SV_TESS_FACTOR_QUADEDGE:
        case WINED3D_SV_TESS_FACTOR_TRIEDGE:
        case WINED3D_SV_TESS_FACTOR_LINEDET:
        case WINED3D_SV_TESS_FACTOR_LINEDEN:
            shader_addline(buffer, "gl_TessLevelOuter[%u]", constant->semantic_idx);
            break;
        case WINED3D_SV_TESS_FACTOR_QUADINT:
        case WINED3D_SV_TESS_FACTOR_TRIINT:
            shader_addline(buffer, "gl_TessLevelInner[%u]", constant->semantic_idx);
            break;
        default:
            FIXME("Unhandled sysval semantic %#x.\n", constant->sysval_semantic);
            shader_addline(buffer, "vec4(0.0)%s", reg_mask);
    }
}

static void shader_glsl_generate_patch_constant_setup(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_signature *signature, BOOL input_setup)
{
    unsigned int i, register_count, user_constant_index, user_constant_count;

    register_count = user_constant_count = 0;
    for (i = 0; i < signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *constant = &signature->elements[i];
        register_count = max(constant->register_idx + 1, register_count);
        if (!constant->sysval_semantic)
            ++user_constant_count;
    }

    if (user_constant_count)
        shader_addline(buffer, "patch %s vec4 user_patch_constant[%u];\n",
                input_setup ? "in" : "out", user_constant_count);
    if (input_setup)
        shader_addline(buffer, "vec4 vpc[%u];\n", register_count);

    shader_addline(buffer, "void setup_patch_constant_%s()\n{\n", input_setup ? "input" : "output");
    for (i = 0, user_constant_index = 0; i < signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *constant = &signature->elements[i];
        char reg_mask[6];

        shader_glsl_write_mask_to_str(constant->mask, reg_mask);

        if (input_setup)
            shader_addline(buffer, "vpc[%u]%s", constant->register_idx, reg_mask);
        else
            shader_glsl_generate_patch_constant_name(buffer, constant, &user_constant_index, reg_mask);

        shader_addline(buffer, " = ");

        if (input_setup)
            shader_glsl_generate_patch_constant_name(buffer, constant, &user_constant_index, reg_mask);
        else
            shader_addline(buffer, "hs_out[%u]%s", constant->register_idx, reg_mask);

        shader_addline(buffer, ";\n");
    }
    shader_addline(buffer, "}\n");
}

static void shader_glsl_generate_srgb_write_correction(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info)
{
    const char *output = get_fragment_output(gl_info);

    shader_addline(buffer, "tmp0.xyz = pow(%s[0].xyz, vec3(srgb_const0.x));\n", output);
    shader_addline(buffer, "tmp0.xyz = tmp0.xyz * vec3(srgb_const0.y) - vec3(srgb_const0.z);\n");
    shader_addline(buffer, "tmp1.xyz = %s[0].xyz * vec3(srgb_const0.w);\n", output);
    shader_addline(buffer, "bvec3 srgb_compare = lessThan(%s[0].xyz, vec3(srgb_const1.x));\n", output);
    shader_addline(buffer, "%s[0].xyz = mix(tmp0.xyz, tmp1.xyz, vec3(srgb_compare));\n", output);
    shader_addline(buffer, "%s[0] = clamp(%s[0], 0.0, 1.0);\n", output, output);
}

static void shader_glsl_generate_fog_code(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, enum wined3d_ffp_ps_fog_mode mode)
{
    const char *output = get_fragment_output(gl_info);

    switch (mode)
    {
        case WINED3D_FFP_PS_FOG_OFF:
            return;

        case WINED3D_FFP_PS_FOG_LINEAR:
            shader_addline(buffer, "float fog = (ffp_fog.end - ffp_varying_fogcoord) * ffp_fog.scale;\n");
            break;

        case WINED3D_FFP_PS_FOG_EXP:
            shader_addline(buffer, "float fog = exp(-ffp_fog.density * ffp_varying_fogcoord);\n");
            break;

        case WINED3D_FFP_PS_FOG_EXP2:
            shader_addline(buffer, "float fog = exp(-ffp_fog.density * ffp_fog.density"
                    " * ffp_varying_fogcoord * ffp_varying_fogcoord);\n");
            break;

        default:
            ERR("Invalid fog mode %#x.\n", mode);
            return;
    }

    shader_addline(buffer, "%s[0].xyz = mix(ffp_fog.color.xyz, %s[0].xyz, clamp(fog, 0.0, 1.0));\n",
            output, output);
}

static void shader_glsl_generate_alpha_test(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, enum wined3d_cmp_func alpha_func)
{
    /* alpha_func is the PASS condition, not the DISCARD condition. Instead of
     * flipping all the operators here, just negate the comparison below. */
    static const char * const comparison_operator[] =
    {
        "",   /* WINED3D_CMP_NEVER */
        "<",  /* WINED3D_CMP_LESS */
        "==", /* WINED3D_CMP_EQUAL */
        "<=", /* WINED3D_CMP_LESSEQUAL */
        ">",  /* WINED3D_CMP_GREATER */
        "!=", /* WINED3D_CMP_NOTEQUAL */
        ">=", /* WINED3D_CMP_GREATEREQUAL */
        ""    /* WINED3D_CMP_ALWAYS */
    };

    if (alpha_func == WINED3D_CMP_ALWAYS)
        return;

    if (alpha_func != WINED3D_CMP_NEVER)
        shader_addline(buffer, "if (!(%s[0].a %s alpha_test_ref))\n",
                get_fragment_output(gl_info), comparison_operator[alpha_func - WINED3D_CMP_NEVER]);
    shader_addline(buffer, "    discard;\n");
}

static void shader_glsl_generate_colour_key_test(struct wined3d_string_buffer *buffer,
        const char *colour, const char *colour_key_low, const char *colour_key_high)
{
        shader_addline(buffer, "if (all(greaterThanEqual(%s, %s)) && all(lessThan(%s, %s)))\n",
                colour, colour_key_low, colour, colour_key_high);
        shader_addline(buffer, "    discard;\n");
}

static void shader_glsl_enable_extensions(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[ARB_CULL_DISTANCE])
        shader_addline(buffer, "#extension GL_ARB_cull_distance : enable\n");
    if (gl_info->supported[ARB_GPU_SHADER5])
        shader_addline(buffer, "#extension GL_ARB_gpu_shader5 : enable\n");
    if (gl_info->supported[ARB_SHADER_ATOMIC_COUNTERS])
        shader_addline(buffer, "#extension GL_ARB_shader_atomic_counters : enable\n");
    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[ARB_SHADER_IMAGE_LOAD_STORE])
        shader_addline(buffer, "#extension GL_ARB_shader_image_load_store : enable\n");
    if (gl_info->supported[ARB_SHADER_IMAGE_SIZE])
        shader_addline(buffer, "#extension GL_ARB_shader_image_size : enable\n");
    if (gl_info->supported[ARB_SHADER_STORAGE_BUFFER_OBJECT])
        shader_addline(buffer, "#extension GL_ARB_shader_storage_buffer_object : enable\n");
    if (gl_info->supported[ARB_SHADER_TEXTURE_IMAGE_SAMPLES])
        shader_addline(buffer, "#extension GL_ARB_shader_texture_image_samples : enable\n");
    if (gl_info->supported[ARB_SHADING_LANGUAGE_420PACK])
        shader_addline(buffer, "#extension GL_ARB_shading_language_420pack : enable\n");
    if (gl_info->supported[ARB_SHADING_LANGUAGE_PACKING])
        shader_addline(buffer, "#extension GL_ARB_shading_language_packing : enable\n");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
        shader_addline(buffer, "#extension GL_ARB_texture_cube_map_array : enable\n");
    if (gl_info->supported[ARB_TEXTURE_GATHER])
        shader_addline(buffer, "#extension GL_ARB_texture_gather : enable\n");
    if (gl_info->supported[ARB_TEXTURE_QUERY_LEVELS])
        shader_addline(buffer, "#extension GL_ARB_texture_query_levels : enable\n");
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        shader_addline(buffer, "#extension GL_ARB_uniform_buffer_object : enable\n");
    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
        shader_addline(buffer, "#extension GL_ARB_viewport_array : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");
    if (gl_info->supported[EXT_TEXTURE_ARRAY])
        shader_addline(buffer, "#extension GL_EXT_texture_array : enable\n");
    if (gl_info->supported[EXT_TEXTURE_SHADOW_LOD])
        shader_addline(buffer, "#extension GL_EXT_texture_shadow_lod : enable\n");
}

static void shader_glsl_generate_color_output(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct wined3d_shader *shader,
        const struct ps_compile_args *args, struct wined3d_string_buffer_list *string_buffers)
{
    const struct wined3d_shader_signature *output_signature = &shader->output_signature;
    struct wined3d_string_buffer *src, *assignment;
    enum wined3d_data_type dst_data_type;
    const char *swizzle;
    unsigned int i;

    if (output_signature->element_count)
    {
        src = string_buffer_get(string_buffers);
        assignment = string_buffer_get(string_buffers);
        for (i = 0; i < output_signature->element_count; ++i)
        {
            const struct wined3d_shader_signature_element *output = &output_signature->elements[i];

            /* register_idx is set to ~0u for non-color outputs. */
            if (output->register_idx == ~0u)
                continue;
            if ((unsigned int)output->component_type >= ARRAY_SIZE(component_type_info))
            {
                FIXME("Unhandled component type %#x.\n", output->component_type);
                continue;
            }
            dst_data_type = component_type_info[output->component_type].data_type;
            shader_addline(buffer, "color_out%u = ", output->semantic_idx);
            string_buffer_sprintf(src, "ps_out[%u]", output->semantic_idx);
            shader_glsl_sprintf_cast(assignment, src->buffer, dst_data_type, WINED3D_DATA_FLOAT);
            swizzle = args->rt_alpha_swizzle & (1u << output->semantic_idx) ? ".argb" : "";
            shader_addline(buffer, "%s%s;\n", assignment->buffer, swizzle);
        }
        string_buffer_release(string_buffers, src);
        string_buffer_release(string_buffers, assignment);
    }
    else
    {
        DWORD mask = shader->reg_maps.rt_mask;

        while (mask)
        {
            i = wined3d_bit_scan(&mask);
            swizzle = args->rt_alpha_swizzle & (1u << i) ? ".argb" : "";
            shader_addline(buffer, "color_out%u = ps_out[%u]%s;\n", i, i, swizzle);
        }
    }
}

static void shader_glsl_generate_ps_epilogue(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader *shader,
        const struct ps_compile_args *args, struct wined3d_string_buffer_list *string_buffers)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;

    /* Pixel shaders < 2.0 place the resulting color in R0 implicitly. */
    if (reg_maps->shader_version.major < 2)
        shader_addline(buffer, "%s[0] = R0;\n", get_fragment_output(gl_info));

    if (args->srgb_correction)
        shader_glsl_generate_srgb_write_correction(buffer, gl_info);

    /* SM < 3 does not replace the fog stage. */
    if (reg_maps->shader_version.major < 3)
        shader_glsl_generate_fog_code(buffer, gl_info, args->fog);

    shader_glsl_generate_alpha_test(buffer, gl_info, args->alpha_test_func + 1);

    if (reg_maps->sample_mask)
        shader_addline(buffer, "gl_SampleMask[0] = floatBitsToInt(sample_mask);\n");

    if (!needs_legacy_glsl_syntax(gl_info))
        shader_glsl_generate_color_output(buffer, gl_info, shader, args, string_buffers);
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_fragment_shader(const struct wined3d_context_gl *context_gl,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        const struct wined3d_shader *shader, const struct ps_compile_args *args,
        struct ps_np2fixup_info *np2fixup_info)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const char *prefix = shader_glsl_get_prefix(version->type);
    BOOL legacy_syntax = needs_legacy_glsl_syntax(gl_info);
    unsigned int i, extra_constants_needed = 0;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;
    DWORD map;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.cur_np2fixup_info = np2fixup_info;
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);
    if (gl_info->supported[ARB_CONSERVATIVE_DEPTH])
        shader_addline(buffer, "#extension GL_ARB_conservative_depth : enable\n");
    if (gl_info->supported[ARB_DERIVATIVE_CONTROL])
        shader_addline(buffer, "#extension GL_ARB_derivative_control : enable\n");
    if (shader_glsl_use_explicit_attrib_location(gl_info))
        shader_addline(buffer, "#extension GL_ARB_explicit_attrib_location : enable\n");
    if (gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS])
        shader_addline(buffer, "#extension GL_ARB_fragment_coord_conventions : enable\n");
    if (gl_info->supported[ARB_FRAGMENT_LAYER_VIEWPORT])
        shader_addline(buffer, "#extension GL_ARB_fragment_layer_viewport : enable\n");
    if (gl_info->supported[ARB_SAMPLE_SHADING])
        shader_addline(buffer, "#extension GL_ARB_sample_shading : enable\n");
    if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
        shader_addline(buffer, "#extension GL_ARB_shader_texture_lod : enable\n");
    /* The spec says that it doesn't have to be explicitly enabled, but the
     * nvidia drivers write a warning if we don't do so. */
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");

    /* Base Declarations */
    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    if (gl_info->supported[ARB_CONSERVATIVE_DEPTH])
    {
        if (shader->u.ps.depth_output == WINED3DSPR_DEPTHOUTGE)
            shader_addline(buffer, "layout (depth_greater) out float gl_FragDepth;\n");
        else if (shader->u.ps.depth_output == WINED3DSPR_DEPTHOUTLE)
            shader_addline(buffer, "layout (depth_less) out float gl_FragDepth;\n");
    }

    /* Declare uniforms for NP2 texcoord fixup:
     * This is NOT done inside the loop that declares the texture samplers
     * since the NP2 fixup code is currently only used for the GeforceFX
     * series and when forcing the ARB_npot extension off. Modern cards just
     * skip the code anyway, so put it inside a separate loop. */
    if (args->np2_fixup)
    {
        struct ps_np2fixup_info *fixup = priv_ctx.cur_np2fixup_info;
        unsigned int cur = 0;

        /* NP2/RECT textures in OpenGL use texcoords in the range [0,width]x[0,height]
         * while D3D has them in the (normalized) [0,1]x[0,1] range.
         * samplerNP2Fixup stores texture dimensions and is updated through
         * shader_glsl_load_np2fixup_constants when the sampler changes. */

        for (i = 0; i < shader->limits->sampler; ++i)
        {
            enum wined3d_shader_resource_type resource_type;

            resource_type = pixelshader_get_resource_type(reg_maps, i, args->tex_types);

            if (!resource_type || !(args->np2_fixup & (1u << i)))
                continue;

            if (resource_type != WINED3D_SHADER_RESOURCE_TEXTURE_2D)
            {
                FIXME("Non-2D texture is flagged for NP2 texcoord fixup.\n");
                continue;
            }

            fixup->idx[i] = cur++;
        }

        fixup->num_consts = (cur + 1) >> 1;
        fixup->active = args->np2_fixup;
        shader_addline(buffer, "uniform vec4 %s_samplerNP2Fixup[%u];\n", prefix, fixup->num_consts);
    }

    if (version->major < 3 || args->vp_mode != WINED3D_VP_MODE_SHADER)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    vec4 color;\n");
        shader_addline(buffer, "    float density;\n");
        shader_addline(buffer, "    float end;\n");
        shader_addline(buffer, "    float scale;\n");
        shader_addline(buffer, "} ffp_fog;\n");

        if (needs_legacy_glsl_syntax(gl_info))
        {
            if (glsl_is_color_reg_read(shader, 0))
                shader_addline(buffer, "vec4 ffp_varying_diffuse;\n");
            if (glsl_is_color_reg_read(shader, 1))
                shader_addline(buffer, "vec4 ffp_varying_specular;\n");
            shader_addline(buffer, "vec4 ffp_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
            shader_addline(buffer, "float ffp_varying_fogcoord;\n");
        }
        else
        {
            if (glsl_is_color_reg_read(shader, 0))
                declare_in_varying(gl_info, buffer, args->flatshading, "vec4 ffp_varying_diffuse;\n");
            if (glsl_is_color_reg_read(shader, 1))
                declare_in_varying(gl_info, buffer, args->flatshading, "vec4 ffp_varying_specular;\n");
            declare_in_varying(gl_info, buffer, FALSE, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
            shader_addline(buffer, "vec4 ffp_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
            declare_in_varying(gl_info, buffer, FALSE, "float ffp_varying_fogcoord;\n");
        }
    }

    if (version->major >= 3)
    {
        unsigned int in_count = min(vec4_varyings(version->major, gl_info), shader->limits->packed_input);

        if (args->vp_mode == WINED3D_VP_MODE_SHADER && reg_maps->input_registers)
            shader_glsl_declare_shader_inputs(gl_info, buffer, in_count,
                    shader->u.ps.interpolation_mode, version->major >= 4);
        shader_addline(buffer, "vec4 %s_in[%u];\n", prefix, in_count);
    }

    for (i = 0, map = reg_maps->bumpmat; map; map >>= 1, ++i)
    {
        if (!(map & 1))
            continue;

        shader_addline(buffer, "uniform mat2 bumpenv_mat%u;\n", i);

        if (reg_maps->luminanceparams & (1u << i))
        {
            shader_addline(buffer, "uniform float bumpenv_lum_scale%u;\n", i);
            shader_addline(buffer, "uniform float bumpenv_lum_offset%u;\n", i);
            extra_constants_needed++;
        }

        extra_constants_needed++;
    }

    if (args->srgb_correction)
    {
        shader_addline(buffer, "const vec4 srgb_const0 = ");
        shader_glsl_append_imm_vec(buffer, &wined3d_srgb_const[0].x, 4, gl_info);
        shader_addline(buffer, ";\n");
        shader_addline(buffer, "const vec4 srgb_const1 = ");
        shader_glsl_append_imm_vec(buffer, &wined3d_srgb_const[1].x, 4, gl_info);
        shader_addline(buffer, ";\n");
    }
    if (reg_maps->vpos || reg_maps->usesdsy)
    {
        if (reg_maps->usesdsy || !gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS])
        {
            ++extra_constants_needed;
            shader_addline(buffer, "uniform vec4 ycorrection;\n");
        }
        if (reg_maps->vpos)
        {
            if (gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS])
            {
                if (context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_PIXEL_CENTER_INTEGER)
                    shader_addline(buffer, "layout(%spixel_center_integer) in vec4 gl_FragCoord;\n",
                            args->render_offscreen ? "" : "origin_upper_left, ");
                else if (!args->render_offscreen)
                    shader_addline(buffer, "layout(origin_upper_left) in vec4 gl_FragCoord;\n");
            }
            shader_addline(buffer, "vec4 vpos;\n");
        }
    }

    if (args->alpha_test_func + 1 != WINED3D_CMP_ALWAYS)
        shader_addline(buffer, "uniform float alpha_test_ref;\n");

    if (!needs_legacy_glsl_syntax(gl_info))
    {
        const struct wined3d_shader_signature *output_signature = &shader->output_signature;

        if (args->dual_source_blend)
            shader_addline(buffer, "vec4 ps_out[%u];\n", gl_info->limits.dual_buffers * 2);
        else
            shader_addline(buffer, "vec4 ps_out[%u];\n", gl_info->limits.buffers);
        if (output_signature->element_count)
        {
            for (i = 0; i < output_signature->element_count; ++i)
            {
                const struct wined3d_shader_signature_element *output = &output_signature->elements[i];

                if (output->register_idx == ~0u)
                    continue;
                if ((unsigned int)output->component_type >= ARRAY_SIZE(component_type_info))
                {
                    FIXME("Unhandled component type %#x.\n", output->component_type);
                    continue;
                }
                if (shader_glsl_use_explicit_attrib_location(gl_info))
                {
                    if (args->dual_source_blend)
                        shader_addline(buffer, "layout(location = %u, index = %u) ", output->semantic_idx / 2, output->semantic_idx % 2);
                    else
                        shader_addline(buffer, "layout(location = %u) ", output->semantic_idx);
                }
                shader_addline(buffer, "out %s4 color_out%u;\n",
                        component_type_info[output->component_type].glsl_vector_type, output->semantic_idx);
            }
        }
        else
        {
            DWORD mask = reg_maps->rt_mask;

            while (mask)
            {
                i = wined3d_bit_scan(&mask);
                if (shader_glsl_use_explicit_attrib_location(gl_info))
                {
                    if (args->dual_source_blend)
                        shader_addline(buffer, "layout(location = %u, index = %u) ", i / 2, i % 2);
                    else
                        shader_addline(buffer, "layout(location = %u) ", i);
                }
                shader_addline(buffer, "out vec4 color_out%u;\n", i);
            }
        }
    }

    if (shader->limits->constant_float + extra_constants_needed >= gl_info->limits.glsl_ps_float_constants)
        FIXME("Insufficient uniforms to run this shader.\n");

    if (shader->u.ps.force_early_depth_stencil)
        shader_addline(buffer, "layout(early_fragment_tests) in;\n");

    shader_addline(buffer, "void main()\n{\n");

    if (reg_maps->sample_mask)
        shader_addline(buffer, "float sample_mask = uintBitsToFloat(0xffffffffu);\n");

    /* Direct3D applications expect integer vPos values, while OpenGL drivers
     * add approximately 0.5. This causes off-by-one problems as spotted by
     * the vPos d3d9 visual test. Unfortunately ATI cards do not add exactly
     * 0.5, but rather something like 0.49999999 or 0.50000001, which still
     * causes precision troubles when we just subtract 0.5.
     *
     * To deal with that, just floor() the position. This will eliminate the
     * fraction on all cards.
     *
     * TODO: Test how this behaves with multisampling.
     *
     * An advantage of floor is that it works even if the driver doesn't add
     * 0.5. It is somewhat questionable if 1.5, 2.5, ... are the proper values
     * to return in gl_FragCoord, even though coordinates specify the pixel
     * centers instead of the pixel corners. This code will behave correctly
     * on drivers that returns integer values. */
    if (reg_maps->vpos)
    {
        if (gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS])
            shader_addline(buffer, "vpos = gl_FragCoord;\n");
        else if (context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_PIXEL_CENTER_INTEGER)
            shader_addline(buffer,
                    "vpos = floor(vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1));\n");
        else
            shader_addline(buffer,
                    "vpos = vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1);\n");
    }

    if (reg_maps->shader_version.major < 3 || args->vp_mode != WINED3D_VP_MODE_SHADER)
    {
        unsigned int i;
        WORD map = reg_maps->texcoord;

        if (legacy_syntax)
        {
            if (glsl_is_color_reg_read(shader, 0))
                shader_addline(buffer, "ffp_varying_diffuse = gl_Color;\n");
            if (glsl_is_color_reg_read(shader, 1))
                shader_addline(buffer, "ffp_varying_specular = gl_SecondaryColor;\n");
        }

        for (i = 0; map; map >>= 1, ++i)
        {
            if (map & 1)
            {
                if (args->pointsprite)
                    shader_addline(buffer, "ffp_texcoord[%u] = vec4(gl_PointCoord.xy, 0.0, 0.0);\n", i);
                else if (args->texcoords_initialized & (1u << i))
                    shader_addline(buffer, "ffp_texcoord[%u] = %s[%u];\n", i,
                            legacy_syntax ? "gl_TexCoord" : "ffp_varying_texcoord", i);
                else
                    shader_addline(buffer, "ffp_texcoord[%u] = vec4(0.0);\n", i);
                shader_addline(buffer, "vec4 T%u = ffp_texcoord[%u];\n", i, i);
            }
        }

        if (legacy_syntax)
            shader_addline(buffer, "ffp_varying_fogcoord = gl_FogFragCoord;\n");
    }

    /* Pack 3.0 inputs */
    if (reg_maps->shader_version.major >= 3)
        shader_glsl_input_pack(shader, buffer, &shader->input_signature, reg_maps, args, gl_info,
                reg_maps->shader_version.major >= 4);

    /* Base Shader Body */
    if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
        return 0;

    /* In SM4+ the shader epilogue is generated by the "ret" instruction. */
    if (reg_maps->shader_version.major < 4)
        shader_glsl_generate_ps_epilogue(gl_info, buffer, shader, args, string_buffers);

    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static void shader_glsl_generate_vs_epilogue(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader *shader,
        const struct vs_compile_args *args)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const BOOL legacy_syntax = needs_legacy_glsl_syntax(gl_info);
    unsigned int i;

    /* Unpack outputs. */
    shader_addline(buffer, "setup_vs_output(vs_out);\n");

    /* The D3DRS_FOGTABLEMODE render state defines if the shader-generated fog coord is used
     * or if the fragment depth is used. If the fragment depth is used(FOGTABLEMODE != NONE),
     * the fog frag coord is thrown away. If the fog frag coord is used, but not written by
     * the shader, it is set to 0.0(fully fogged, since start = 1.0, end = 0.0).
     */
    if (reg_maps->shader_version.major < 3)
    {
        if (args->fog_src == VS_FOG_Z)
            shader_addline(buffer, "%s = gl_Position.z;\n",
                    legacy_syntax ? "gl_FogFragCoord" : "ffp_varying_fogcoord");
        else if (!reg_maps->fog)
            shader_addline(buffer, "%s = 0.0;\n",
                    legacy_syntax ? "gl_FogFragCoord" : "ffp_varying_fogcoord");
    }

    /* We always store the clipplanes without y inversion. */
    if (args->clip_enabled)
    {
        if (legacy_syntax)
            shader_addline(buffer, "gl_ClipVertex = gl_Position;\n");
        else
            for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
                shader_addline(buffer, "gl_ClipDistance[%u] = dot(gl_Position, clip_planes[%u]);\n", i, i);
    }

    if (args->point_size && !args->per_vertex_point_size)
        shader_addline(buffer, "gl_PointSize = clamp(ffp_point.size, ffp_point.size_min, ffp_point.size_max);\n");

    if (args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL && !gl_info->supported[ARB_CLIP_CONTROL])
        shader_glsl_fixup_position(buffer, FALSE);
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_vertex_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, const struct wined3d_shader *shader, const struct vs_compile_args *args)
{
    struct wined3d_string_buffer_list *string_buffers = &priv->string_buffers;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;
    unsigned int i;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);
    if (gl_info->supported[ARB_DRAW_INSTANCED])
        shader_addline(buffer, "#extension GL_ARB_draw_instanced : enable\n");
    if (shader_glsl_use_explicit_attrib_location(gl_info))
        shader_addline(buffer, "#extension GL_ARB_explicit_attrib_location : enable\n");
    if (gl_info->supported[ARB_SHADER_VIEWPORT_LAYER_ARRAY])
        shader_addline(buffer, "#extension GL_ARB_shader_viewport_layer_array : enable\n");

    /* Base Declarations */
    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    for (i = 0; i < shader->input_signature.element_count; ++i)
        shader_glsl_declare_generic_vertex_attribute(buffer, gl_info, &shader->input_signature.elements[i]);

    if (args->point_size && !args->per_vertex_point_size)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    float size;\n");
        shader_addline(buffer, "    float size_min;\n");
        shader_addline(buffer, "    float size_max;\n");
        shader_addline(buffer, "} ffp_point;\n");
    }

    if (!needs_legacy_glsl_syntax(gl_info))
    {
        if (args->clip_enabled)
            shader_addline(buffer, "uniform vec4 clip_planes[%u];\n", gl_info->limits.user_clip_distances);

        if (version->major < 3)
        {
            declare_out_varying(gl_info, buffer, args->flatshading, "vec4 ffp_varying_diffuse;\n");
            declare_out_varying(gl_info, buffer, args->flatshading, "vec4 ffp_varying_specular;\n");
            declare_out_varying(gl_info, buffer, FALSE, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
            declare_out_varying(gl_info, buffer, FALSE, "float ffp_varying_fogcoord;\n");
        }
    }

    if (version->major < 4)
        shader_addline(buffer, "void setup_vs_output(in vec4[%u]);\n", shader->limits->packed_output);

    if (args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL && !gl_info->supported[ARB_CLIP_CONTROL])
        shader_addline(buffer, "uniform vec4 pos_fixup;\n");

    if (reg_maps->shader_version.major >= 4)
        shader_glsl_generate_sm4_output_setup(priv, shader, args->next_shader_input_count,
                gl_info, args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL, args->interpolation_mode);

    shader_addline(buffer, "void main()\n{\n");

    if (reg_maps->input_rel_addressing)
    {
        unsigned int highest_input_register = wined3d_log2i(reg_maps->input_registers);
        shader_addline(buffer, "vec4 vs_in[%u];\n", highest_input_register + 1);
        for (i = 0; i < shader->input_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *e = &shader->input_signature.elements[i];
            shader_addline(buffer, "vs_in[%u] = vs_in%u;\n", e->register_idx, e->register_idx);
        }
    }

    if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
        return 0;

    /* In SM4+ the shader epilogue is generated by the "ret" instruction. */
    if (reg_maps->shader_version.major < 4)
        shader_glsl_generate_vs_epilogue(gl_info, buffer, shader, args);

    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static void shader_glsl_generate_default_control_point_phase(const struct wined3d_shader *shader,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader_reg_maps *reg_maps)
{
    const struct wined3d_shader_signature *output_signature = &shader->output_signature;
    char reg_mask[6];
    unsigned int i;

    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *output = &output_signature->elements[i];

        shader_glsl_write_mask_to_str(output->mask, reg_mask);
        shader_addline(buffer, "shader_out[gl_InvocationID].reg[%u]%s = shader_in[gl_InvocationID].reg[%u]%s;\n",
                output->register_idx, reg_mask, output->register_idx, reg_mask);
    }
}

static HRESULT shader_glsl_generate_shader_phase(const struct wined3d_shader *shader,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader_reg_maps *reg_maps,
        struct shader_glsl_ctx_priv *priv_ctx, const struct wined3d_shader_phase *phase,
        const char *phase_name, unsigned phase_idx)
{
    unsigned int i;
    HRESULT hr;

    shader_addline(buffer, "void hs_%s_phase%u(%s)\n{\n",
            phase_name, phase_idx, phase->instance_count ? "int phase_instance_id" : "");
    for (i = 0; i < phase->temporary_count; ++i)
        shader_addline(buffer, "vec4 R%u;\n", i);
    hr = shader_generate_code(shader, buffer, reg_maps, priv_ctx, phase->start, phase->end);
    shader_addline(buffer, "}\n");
    return hr;
}

static void shader_glsl_generate_shader_phase_invocation(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_phase *phase, const char *phase_name, unsigned int phase_idx)
{
    if (phase->instance_count)
    {
        shader_addline(buffer, "for (int i = 0; i < %u; ++i)\n{\n", phase->instance_count);
        shader_addline(buffer, "hs_%s_phase%u(i);\n", phase_name, phase_idx);
        shader_addline(buffer, "}\n");
    }
    else
    {
        shader_addline(buffer, "hs_%s_phase%u();\n", phase_name, phase_idx);
    }
}

static GLuint shader_glsl_generate_hull_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, const struct wined3d_shader *shader)
{
    struct wined3d_string_buffer_list *string_buffers = &priv->string_buffers;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    const struct wined3d_hull_shader *hs = &shader->u.hs;
    const struct wined3d_shader_phase *phase;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;
    unsigned int i;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);
    shader_addline(buffer, "#extension GL_ARB_tessellation_shader : enable\n");

    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    shader_addline(buffer, "layout(vertices = %u) out;\n", hs->output_vertex_count);

    shader_addline(buffer, "in shader_in_out { vec4 reg[%u]; } shader_in[];\n", shader->limits->packed_input);
    shader_addline(buffer, "out shader_in_out { vec4 reg[%u]; } shader_out[];\n", shader->limits->packed_output);

    shader_glsl_generate_patch_constant_setup(buffer, &shader->patch_constant_signature, FALSE);

    if (hs->phases.control_point)
    {
        shader_addline(buffer, "void setup_hs_output(in vec4 outputs[%u])\n{\n",
                shader->limits->packed_output);
        shader_glsl_setup_sm4_shader_output(priv, shader->limits->packed_output, &shader->output_signature,
                &shader->reg_maps, "shader_out[gl_InvocationID]", FALSE);
        shader_addline(buffer, "}\n");
    }

    shader_addline(buffer, "void hs_control_point_phase()\n{\n");
    if ((phase = hs->phases.control_point))
    {
        for (i = 0; i < phase->temporary_count; ++i)
            shader_addline(buffer, "vec4 R%u;\n", i);
        if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, phase->start, phase->end)))
            return 0;
        shader_addline(buffer, "setup_hs_output(hs_out);\n");
    }
    else
    {
        shader_glsl_generate_default_control_point_phase(shader, buffer, reg_maps);
    }
    shader_addline(buffer, "}\n");

    for (i = 0; i < hs->phases.fork_count; ++i)
    {
        if (FAILED(shader_glsl_generate_shader_phase(shader, buffer, reg_maps, &priv_ctx,
                &hs->phases.fork[i], "fork", i)))
            return 0;
    }

    for (i = 0; i < hs->phases.join_count; ++i)
    {
        if (FAILED(shader_glsl_generate_shader_phase(shader, buffer, reg_maps, &priv_ctx,
                &hs->phases.join[i], "join", i)))
            return 0;
    }

    shader_addline(buffer, "void main()\n{\n");
    shader_addline(buffer, "hs_control_point_phase();\n");
    if (reg_maps->vocp)
        shader_addline(buffer, "barrier();\n");
    for (i = 0; i < hs->phases.fork_count; ++i)
        shader_glsl_generate_shader_phase_invocation(buffer, &hs->phases.fork[i], "fork", i);
    for (i = 0; i < hs->phases.join_count; ++i)
        shader_glsl_generate_shader_phase_invocation(buffer, &hs->phases.join[i], "join", i);
    shader_addline(buffer, "setup_patch_constant_output();\n");
    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_TESS_CONTROL_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static void shader_glsl_generate_ds_epilogue(const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader *shader,
        const struct ds_compile_args *args)
{
    shader_addline(buffer, "setup_ds_output(ds_out);\n");

    if (args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL && !gl_info->supported[ARB_CLIP_CONTROL])
        shader_glsl_fixup_position(buffer, FALSE);
}

static GLuint shader_glsl_generate_domain_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, const struct wined3d_shader *shader, const struct ds_compile_args *args)
{
    struct wined3d_string_buffer_list *string_buffers = &priv->string_buffers;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ds_args = args;
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);
    shader_addline(buffer, "#extension GL_ARB_tessellation_shader : enable\n");

    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    shader_addline(buffer, "layout(");
    switch (shader->u.ds.tessellator_domain)
    {
        case WINED3D_TESSELLATOR_DOMAIN_LINE:
            shader_addline(buffer, "isolines");
            break;
        case WINED3D_TESSELLATOR_DOMAIN_QUAD:
            shader_addline(buffer, "quads");
            break;
        case WINED3D_TESSELLATOR_DOMAIN_TRIANGLE:
            shader_addline(buffer, "triangles");
            break;
    }
    switch (args->tessellator_output_primitive)
    {
        case WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CW:
            if (args->render_offscreen)
                shader_addline(buffer, ", ccw");
            else
                shader_addline(buffer, ", cw");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW:
            if (args->render_offscreen)
                shader_addline(buffer, ", cw");
            else
                shader_addline(buffer, ", ccw");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_POINT:
            shader_addline(buffer, ", point_mode");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_LINE:
            break;
    }
    switch (args->tessellator_partitioning)
    {
        case WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
            shader_addline(buffer, ", fractional_odd_spacing");
            break;
        case WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
            shader_addline(buffer, ", fractional_even_spacing");
            break;
        case WINED3D_TESSELLATOR_PARTITIONING_INTEGER:
        case WINED3D_TESSELLATOR_PARTITIONING_POW2:
            shader_addline(buffer, ", equal_spacing");
            break;
    }
    shader_addline(buffer, ") in;\n");

    shader_addline(buffer, "in shader_in_out { vec4 reg[%u]; } shader_in[];\n", shader->limits->packed_input);

    if (args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL && !gl_info->supported[ARB_CLIP_CONTROL])
        shader_addline(buffer, "uniform vec4 pos_fixup;\n");

    shader_glsl_generate_sm4_output_setup(priv, shader, args->output_count, gl_info,
            args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL, args->interpolation_mode);
    shader_glsl_generate_patch_constant_setup(buffer, &shader->patch_constant_signature, TRUE);

    shader_addline(buffer, "void main()\n{\n");
    shader_addline(buffer, "setup_patch_constant_input();\n");

    if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
        return 0;

    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_TESS_EVALUATION_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_geometry_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, const struct wined3d_shader *shader, const struct gs_compile_args *args)
{
    struct wined3d_string_buffer_list *string_buffers = &priv->string_buffers;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    const struct wined3d_shader_signature_element *output;
    enum wined3d_primitive_type primitive_type;
    struct shader_glsl_ctx_priv priv_ctx;
    unsigned int max_vertices;
    unsigned int i, j;
    GLuint shader_id;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);

    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    primitive_type = shader->u.gs.input_type ? shader->u.gs.input_type : args->primitive_type;
    shader_addline(buffer, "layout(%s", glsl_primitive_type_from_d3d(primitive_type));
    if (shader->u.gs.instance_count > 1)
        shader_addline(buffer, ", invocations = %u", shader->u.gs.instance_count);
    shader_addline(buffer, ") in;\n");

    primitive_type = shader->u.gs.output_type ? shader->u.gs.output_type : args->primitive_type;
    if (!(max_vertices = shader->u.gs.vertices_out))
    {
        switch (args->primitive_type)
        {
            case WINED3D_PT_POINTLIST:
                max_vertices = 1;
                break;
            case WINED3D_PT_LINELIST:
                max_vertices = 2;
                break;
            case WINED3D_PT_TRIANGLELIST:
                max_vertices = 3;
                break;
            default:
                FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(args->primitive_type));
                break;
        }
    }
    shader_addline(buffer, "layout(%s, max_vertices = %u) out;\n",
            glsl_primitive_type_from_d3d(primitive_type), max_vertices);
    shader_addline(buffer, "in shader_in_out { vec4 reg[%u]; } shader_in[];\n", shader->limits->packed_input);

    if (!gl_info->supported[ARB_CLIP_CONTROL])
    {
        shader_addline(buffer, "uniform vec4 pos_fixup");
        if (reg_maps->viewport_array)
            shader_addline(buffer, "[%u]", WINED3D_MAX_VIEWPORTS);
        shader_addline(buffer, ";\n");
    }

    if (is_rasterization_disabled(shader))
    {
        shader_glsl_generate_stream_output_setup(buffer, shader);
    }
    else
    {
        shader_glsl_generate_sm4_output_setup(priv, shader, args->output_count,
                gl_info, TRUE, args->interpolation_mode);
    }

    shader_addline(buffer, "void main()\n{\n");
    if (shader->function)
    {
        if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
            return 0;
    }
    else
    {
        for (i = 0; i < max_vertices; ++i)
        {
            for (j = 0; j < shader->output_signature.element_count; ++j)
            {
                output = &shader->output_signature.elements[j];
                shader_addline(buffer, "gs_out[%u] = shader_in[%u].reg[%u];\n",
                        output->register_idx, i, output->register_idx);
            }
            shader_addline(buffer, "setup_gs_output(gs_out);\n");
            if (!gl_info->supported[ARB_CLIP_CONTROL])
                shader_glsl_fixup_position(buffer, FALSE);
            shader_addline(buffer, "EmitVertex();\n");
        }
    }
    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_GEOMETRY_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static void shader_glsl_generate_shader_epilogue(const struct wined3d_shader_context *ctx)
{
    const struct shader_glsl_ctx_priv *priv = ctx->backend_data;
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    struct wined3d_string_buffer *buffer = ctx->buffer;
    const struct wined3d_shader *shader = ctx->shader;

    switch (shader->reg_maps.shader_version.type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            shader_glsl_generate_ps_epilogue(gl_info, buffer, shader, priv->cur_ps_args, priv->string_buffers);
            break;
        case WINED3D_SHADER_TYPE_VERTEX:
            shader_glsl_generate_vs_epilogue(gl_info, buffer, shader, priv->cur_vs_args);
            break;
        case WINED3D_SHADER_TYPE_DOMAIN:
            shader_glsl_generate_ds_epilogue(gl_info, buffer, shader, priv->cur_ds_args);
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
        case WINED3D_SHADER_TYPE_COMPUTE:
            break;
        default:
            FIXME("Unhandled shader type %#x.\n", shader->reg_maps.shader_version.type);
            break;
    }
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_compute_shader(const struct wined3d_context_gl *context_gl,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        const struct wined3d_shader *shader)
{
    const struct wined3d_shader_thread_group_size *thread_group_size = &shader->u.cs.thread_group_size;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;
    unsigned int i;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.string_buffers = string_buffers;

    shader_glsl_add_version_declaration(buffer, gl_info);

    shader_glsl_enable_extensions(buffer, gl_info);
    shader_addline(buffer, "#extension GL_ARB_compute_shader : enable\n");

    shader_generate_glsl_declarations(context_gl, buffer, shader, reg_maps, &priv_ctx);

    for (i = 0; i < reg_maps->tgsm_count; ++i)
    {
        if (reg_maps->tgsm[i].size)
            shader_addline(buffer, "shared uint cs_g%u[%u];\n", i, reg_maps->tgsm[i].size);
    }

    shader_addline(buffer, "layout(local_size_x = %u, local_size_y = %u, local_size_z = %u) in;\n",
            thread_group_size->x, thread_group_size->y, thread_group_size->z);

    shader_addline(buffer, "void main()\n{\n");
    shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL);
    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_COMPUTE_SHADER));
    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static GLuint find_glsl_fragment_shader(const struct wined3d_context_gl *context_gl,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        struct wined3d_shader *shader,
        const struct ps_compile_args *args, const struct ps_np2fixup_info **np2fixup_info)
{
    struct glsl_ps_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    struct ps_np2fixup_info *np2fixup;
    UINT i;
    DWORD new_size;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.ps;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for (i = 0; i < shader_data->num_gl_shaders; ++i)
    {
        if (!memcmp(&gl_shaders[i].args, args, sizeof(*args)))
        {
            if (args->np2_fixup)
                *np2fixup_info = &gl_shaders[i].np2fixup;
            return gl_shaders[i].id;
        }
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);
    if (shader_data->shader_array_size == shader_data->num_gl_shaders)
    {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = heap_realloc(shader_data->gl_shaders.ps, new_size * sizeof(*gl_shaders));
        }
        else
        {
            new_array = heap_alloc(sizeof(*gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader_data->gl_shaders.ps = new_array;
        shader_data->shader_array_size = new_size;
        gl_shaders = new_array;
    }

    gl_shaders[shader_data->num_gl_shaders].args = *args;

    np2fixup = &gl_shaders[shader_data->num_gl_shaders].np2fixup;
    memset(np2fixup, 0, sizeof(*np2fixup));
    *np2fixup_info = args->np2_fixup ? np2fixup : NULL;

    string_buffer_clear(buffer);
    ret = shader_glsl_generate_fragment_shader(context_gl, buffer, string_buffers, shader, args, np2fixup);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static inline BOOL vs_args_equal(const struct vs_compile_args *stored, const struct vs_compile_args *new,
        const DWORD use_map)
{
    if ((stored->swizzle_map & use_map) != new->swizzle_map)
        return FALSE;
    if ((stored->clip_enabled) != new->clip_enabled)
        return FALSE;
    if (stored->point_size != new->point_size)
        return FALSE;
    if (stored->per_vertex_point_size != new->per_vertex_point_size)
        return FALSE;
    if (stored->flatshading != new->flatshading)
        return FALSE;
    if (stored->next_shader_type != new->next_shader_type)
        return FALSE;
    if (stored->next_shader_input_count != new->next_shader_input_count)
        return FALSE;
    if (stored->fog_src != new->fog_src)
        return FALSE;
    return !memcmp(stored->interpolation_mode, new->interpolation_mode, sizeof(new->interpolation_mode));
}

static GLuint find_glsl_vertex_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, struct wined3d_shader *shader, const struct vs_compile_args *args)
{
    struct glsl_vs_compiled_shader *gl_shaders, *new_array;
    uint32_t use_map = context_gl->c.stream_info.use_map;
    struct glsl_shader_private *shader_data;
    unsigned int i, new_size;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.vs;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for (i = 0; i < shader_data->num_gl_shaders; ++i)
    {
        if (vs_args_equal(&gl_shaders[i].args, args, use_map))
            return gl_shaders[i].id;
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    if (shader_data->shader_array_size == shader_data->num_gl_shaders)
    {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = heap_realloc(shader_data->gl_shaders.vs, new_size * sizeof(*gl_shaders));
        }
        else
        {
            new_array = heap_alloc(sizeof(*gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader_data->gl_shaders.vs = new_array;
        shader_data->shader_array_size = new_size;
        gl_shaders = new_array;
    }

    gl_shaders[shader_data->num_gl_shaders].args = *args;

    string_buffer_clear(&priv->shader_buffer);
    ret = shader_glsl_generate_vertex_shader(context_gl, priv, shader, args);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static GLuint find_glsl_hull_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, struct wined3d_shader *shader)
{
    struct glsl_hs_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    unsigned int new_size;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.hs;

    if (shader_data->num_gl_shaders > 0)
    {
        assert(shader_data->num_gl_shaders == 1);
        return gl_shaders[0].id;
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    assert(!shader_data->gl_shaders.hs);
    new_size = 1;
    if (!(new_array = heap_alloc(sizeof(*new_array))))
    {
        ERR("Failed to allocate GL shaders array.\n");
        return 0;
    }
    shader_data->gl_shaders.hs = new_array;
    shader_data->shader_array_size = new_size;
    gl_shaders = new_array;

    string_buffer_clear(&priv->shader_buffer);
    ret = shader_glsl_generate_hull_shader(context_gl, priv, shader);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static GLuint find_glsl_domain_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, struct wined3d_shader *shader, const struct ds_compile_args *args)
{
    struct glsl_ds_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    unsigned int i, new_size;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.ds;

    for (i = 0; i < shader_data->num_gl_shaders; ++i)
    {
        if (!memcmp(&gl_shaders[i].args, args, sizeof(*args)))
            return gl_shaders[i].id;
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    if (shader_data->num_gl_shaders)
    {
        new_size = shader_data->shader_array_size + 1;
        new_array = heap_realloc(shader_data->gl_shaders.ds, new_size * sizeof(*new_array));
    }
    else
    {
        new_array = heap_alloc(sizeof(*new_array));
        new_size = 1;
    }

    if (!new_array)
    {
        ERR("Failed to allocate GL shaders array.\n");
        return 0;
    }
    shader_data->gl_shaders.ds = new_array;
    shader_data->shader_array_size = new_size;
    gl_shaders = new_array;

    string_buffer_clear(&priv->shader_buffer);
    ret = shader_glsl_generate_domain_shader(context_gl, priv, shader, args);
    gl_shaders[shader_data->num_gl_shaders].args = *args;
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static GLuint find_glsl_geometry_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, struct wined3d_shader *shader, const struct gs_compile_args *args)
{
    struct glsl_gs_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    unsigned int i, new_size;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.gs;

    for (i = 0; i < shader_data->num_gl_shaders; ++i)
    {
        if (!memcmp(&gl_shaders[i].args, args, sizeof(*args)))
            return gl_shaders[i].id;
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    if (shader_data->num_gl_shaders)
    {
        new_size = shader_data->shader_array_size + 1;
        new_array = heap_realloc(shader_data->gl_shaders.gs, new_size * sizeof(*new_array));
    }
    else
    {
        new_array = heap_alloc(sizeof(*new_array));
        new_size = 1;
    }

    if (!new_array)
    {
        ERR("Failed to allocate GL shaders array.\n");
        return 0;
    }
    shader_data->gl_shaders.gs = new_array;
    shader_data->shader_array_size = new_size;
    gl_shaders = new_array;

    string_buffer_clear(&priv->shader_buffer);
    ret = shader_glsl_generate_geometry_shader(context_gl, priv, shader, args);
    gl_shaders[shader_data->num_gl_shaders].args = *args;
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static const char *shader_glsl_ffp_mcs(enum wined3d_material_color_source mcs, const char *material)
{
    switch (mcs)
    {
        case WINED3D_MCS_MATERIAL:
            return material;
        case WINED3D_MCS_COLOR1:
            return "ffp_attrib_diffuse";
        case WINED3D_MCS_COLOR2:
            return "ffp_attrib_specular";
        default:
            ERR("Invalid material color source %#x.\n", mcs);
            return "<invalid>";
    }
}

static void shader_glsl_ffp_vertex_lighting_footer(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, unsigned int idx, BOOL legacy_lighting)
{
    shader_addline(buffer, "diffuse += clamp(dot(dir, normal), 0.0, 1.0)"
            " * ffp_light[%u].diffuse.xyz * att;\n", idx);
    if (settings->localviewer)
        shader_addline(buffer, "t = dot(normal, normalize(dir - normalize(ec_pos.xyz)));\n");
    else
        shader_addline(buffer, "t = dot(normal, normalize(dir + vec3(0.0, 0.0, -1.0)));\n");
    shader_addline(buffer, "if (dot(dir, normal) > 0.0 && t > 0.0%s) specular +="
            " pow(t, ffp_material.shininess) * ffp_light[%u].specular * att;\n",
            legacy_lighting ? " && ffp_material.shininess > 0.0" : "", idx);
}

static void shader_glsl_ffp_vertex_lighting(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, BOOL legacy_lighting)
{
    const char *diffuse, *specular, *emissive, *ambient;
    unsigned int i, idx;

    if (!settings->lighting)
    {
        shader_addline(buffer, "ffp_varying_diffuse = ffp_attrib_diffuse;\n");
        shader_addline(buffer, "ffp_varying_specular = ffp_attrib_specular;\n");
        return;
    }

    shader_addline(buffer, "vec3 ambient = ffp_light_ambient;\n");
    shader_addline(buffer, "vec3 diffuse = vec3(0.0);\n");
    shader_addline(buffer, "vec4 specular = vec4(0.0);\n");
    shader_addline(buffer, "vec3 dir, dst;\n");
    shader_addline(buffer, "float att, t;\n");

    ambient = shader_glsl_ffp_mcs(settings->ambient_source, "ffp_material.ambient");
    diffuse = shader_glsl_ffp_mcs(settings->diffuse_source, "ffp_material.diffuse");
    specular = shader_glsl_ffp_mcs(settings->specular_source, "ffp_material.specular");
    emissive = shader_glsl_ffp_mcs(settings->emissive_source, "ffp_material.emissive");

    idx = 0;
    for (i = 0; i < settings->point_light_count; ++i, ++idx)
    {
        shader_addline(buffer, "dir = ffp_light[%u].position.xyz - ec_pos.xyz;\n", idx);
        shader_addline(buffer, "dst.z = dot(dir, dir);\n");
        shader_addline(buffer, "dst.y = sqrt(dst.z);\n");
        shader_addline(buffer, "dst.x = 1.0;\n");
        if (legacy_lighting)
        {
            shader_addline(buffer, "dst.y = (ffp_light[%u].range - dst.y) / ffp_light[%u].range;\n", idx, idx);
            shader_addline(buffer, "dst.z = dst.y * dst.y;\n");
            shader_addline(buffer, "if (dst.y > 0.0)\n{\n");
        }
        else
        {
            shader_addline(buffer, "if (dst.y <= ffp_light[%u].range)\n{\n", idx);
        }
        shader_addline(buffer, "att = dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                " ffp_light[%u].l_att, ffp_light[%u].q_att));\n", idx, idx, idx);
        if (!legacy_lighting)
            shader_addline(buffer, "att = 1.0 / att;\n");
        shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz * att;\n", idx);
        if (!settings->normal)
        {
            shader_addline(buffer, "}\n");
            continue;
        }
        shader_addline(buffer, "dir = normalize(dir);\n");
        shader_glsl_ffp_vertex_lighting_footer(buffer, settings, idx, legacy_lighting);
        shader_addline(buffer, "}\n");
    }

    for (i = 0; i < settings->spot_light_count; ++i, ++idx)
    {
        shader_addline(buffer, "dir = ffp_light[%u].position.xyz - ec_pos.xyz;\n", idx);
        shader_addline(buffer, "dst.z = dot(dir, dir);\n");
        shader_addline(buffer, "dst.y = sqrt(dst.z);\n");
        shader_addline(buffer, "dst.x = 1.0;\n");
        if (legacy_lighting)
        {
            shader_addline(buffer, "dst.y = (ffp_light[%u].range - dst.y) / ffp_light[%u].range;\n", idx, idx);
            shader_addline(buffer, "dst.z = dst.y * dst.y;\n");
            shader_addline(buffer, "if (dst.y > 0.0)\n{\n");
        }
        else
        {
            shader_addline(buffer, "if (dst.y <= ffp_light[%u].range)\n{\n", idx);
        }
        shader_addline(buffer, "dir = normalize(dir);\n");
        shader_addline(buffer, "t = dot(-dir, normalize(ffp_light[%u].direction));\n", idx);
        shader_addline(buffer, "if (t > ffp_light[%u].cos_htheta) att = 1.0;\n", idx);
        shader_addline(buffer, "else if (t <= ffp_light[%u].cos_hphi) att = 0.0;\n", idx);
        shader_addline(buffer, "else att = pow((t - ffp_light[%u].cos_hphi)"
                " / (ffp_light[%u].cos_htheta - ffp_light[%u].cos_hphi), ffp_light[%u].falloff);\n",
                idx, idx, idx, idx);
        if (legacy_lighting)
            shader_addline(buffer, "att *= dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                    " ffp_light[%u].l_att, ffp_light[%u].q_att));\n",
                    idx, idx, idx);
        else
            shader_addline(buffer, "att /= dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                    " ffp_light[%u].l_att, ffp_light[%u].q_att));\n",
                    idx, idx, idx);
        shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz * att;\n", idx);
        if (!settings->normal)
        {
            shader_addline(buffer, "}\n");
            continue;
        }
        shader_glsl_ffp_vertex_lighting_footer(buffer, settings, idx, legacy_lighting);
        shader_addline(buffer, "}\n");
    }

    for (i = 0; i < settings->directional_light_count; ++i, ++idx)
    {
        shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz;\n", idx);
        if (!settings->normal)
            continue;
        shader_addline(buffer, "att = 1.0;\n");
        shader_addline(buffer, "dir = normalize(ffp_light[%u].direction.xyz);\n", idx);
        shader_glsl_ffp_vertex_lighting_footer(buffer, settings, idx, legacy_lighting);
    }

    for (i = 0; i < settings->parallel_point_light_count; ++i, ++idx)
    {
        shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz;\n", idx);
        if (!settings->normal)
            continue;
        shader_addline(buffer, "att = 1.0;\n");
        shader_addline(buffer, "dir = normalize(ffp_light[%u].position.xyz);\n", idx);
        shader_glsl_ffp_vertex_lighting_footer(buffer, settings, idx, legacy_lighting);
    }

    shader_addline(buffer, "ffp_varying_diffuse.xyz = %s.xyz * ambient + %s.xyz * diffuse + %s.xyz;\n",
            ambient, diffuse, emissive);
    shader_addline(buffer, "ffp_varying_diffuse.w = %s.w;\n", diffuse);
    shader_addline(buffer, "ffp_varying_specular = %s * specular;\n", specular);
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_ffp_vertex_shader(struct shader_glsl_priv *priv,
        const struct wined3d_ffp_vs_settings *settings, const struct wined3d_gl_info *gl_info)
{
    static const struct attrib_info
    {
        const char type[6];
        const char name[24];
    }
    attrib_info[] =
    {
        {"vec4", "ffp_attrib_position"},        /* WINED3D_FFP_POSITION */
        {"vec4", "ffp_attrib_blendweight"},     /* WINED3D_FFP_BLENDWEIGHT */
        {"vec4", "ffp_attrib_blendindices"},    /* WINED3D_FFP_BLENDINDICES */
        {"vec3", "ffp_attrib_normal"},          /* WINED3D_FFP_NORMAL */
        {"float", "ffp_attrib_psize"},          /* WINED3D_FFP_PSIZE */
        {"vec4", "ffp_attrib_diffuse"},         /* WINED3D_FFP_DIFFUSE */
        {"vec4", "ffp_attrib_specular"},        /* WINED3D_FFP_SPECULAR */
    };
    const BOOL legacy_syntax = needs_legacy_glsl_syntax(gl_info);
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    BOOL output_legacy_fogcoord = legacy_syntax;
    BOOL legacy_lighting = priv->legacy_lighting;
    GLuint shader_obj;
    unsigned int i;

    string_buffer_clear(buffer);

    shader_glsl_add_version_declaration(buffer, gl_info);
    TRACE("settings->vb_indices %#x.\n", settings->vb_indices);
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        shader_addline(buffer,"#extension GL_ARB_uniform_buffer_object : enable\n");

    if (shader_glsl_use_explicit_attrib_location(gl_info))
        shader_addline(buffer, "#extension GL_ARB_explicit_attrib_location : enable\n");

    for (i = 0; i < WINED3D_FFP_ATTRIBS_COUNT; ++i)
    {
        const char *type = i < ARRAY_SIZE(attrib_info) ? attrib_info[i].type : "vec4";

        if (shader_glsl_use_explicit_attrib_location(gl_info))
            shader_addline(buffer, "layout(location = %u) ", i);
        shader_addline(buffer, "%s %s vs_in%u;\n", get_attribute_keyword(gl_info), type, i);
    }
    shader_addline(buffer, "\n");

    if (settings->vb_indices && gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        shader_addline(buffer,"layout(std140) uniform ffp_modelview_ubo\n\
                { \n\
                    mat4 ffp_modelview_matrix[%u];\n\
                };\n", MAX_VERTEX_BLEND_UBO);
    }
    else
    {
        shader_addline(buffer, "uniform mat4 ffp_modelview_matrix[%u];\n", settings->vertexblends + 1);
    }

    shader_addline(buffer, "uniform mat4 ffp_projection_matrix;\n");
    shader_addline(buffer, "uniform mat3 ffp_normal_matrix;\n");
    shader_addline(buffer, "uniform mat4 ffp_texture_matrix[%u];\n", WINED3D_MAX_TEXTURES);

    shader_addline(buffer, "uniform struct\n{\n");
    shader_addline(buffer, "    vec4 emissive;\n");
    shader_addline(buffer, "    vec4 ambient;\n");
    shader_addline(buffer, "    vec4 diffuse;\n");
    shader_addline(buffer, "    vec4 specular;\n");
    shader_addline(buffer, "    float shininess;\n");
    shader_addline(buffer, "} ffp_material;\n");

    shader_addline(buffer, "uniform vec3 ffp_light_ambient;\n");
    shader_addline(buffer, "uniform struct\n{\n");
    shader_addline(buffer, "    vec4 diffuse;\n");
    shader_addline(buffer, "    vec4 specular;\n");
    shader_addline(buffer, "    vec4 ambient;\n");
    shader_addline(buffer, "    vec4 position;\n");
    shader_addline(buffer, "    vec3 direction;\n");
    shader_addline(buffer, "    float range;\n");
    shader_addline(buffer, "    float falloff;\n");
    shader_addline(buffer, "    float c_att;\n");
    shader_addline(buffer, "    float l_att;\n");
    shader_addline(buffer, "    float q_att;\n");
    shader_addline(buffer, "    float cos_htheta;\n");
    shader_addline(buffer, "    float cos_hphi;\n");
    shader_addline(buffer, "} ffp_light[%u];\n", WINED3D_MAX_ACTIVE_LIGHTS);

    if (settings->point_size)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    float size;\n");
        shader_addline(buffer, "    float size_min;\n");
        shader_addline(buffer, "    float size_max;\n");
        shader_addline(buffer, "    float c_att;\n");
        shader_addline(buffer, "    float l_att;\n");
        shader_addline(buffer, "    float q_att;\n");
        shader_addline(buffer, "} ffp_point;\n");
    }

    if (legacy_syntax)
    {
        shader_addline(buffer, "vec4 ffp_varying_diffuse;\n");
        shader_addline(buffer, "vec4 ffp_varying_specular;\n");
        shader_addline(buffer, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        shader_addline(buffer, "float ffp_varying_fogcoord;\n");
    }
    else
    {
        if (settings->clipping)
            shader_addline(buffer, "uniform vec4 clip_planes[%u];\n", gl_info->limits.user_clip_distances);

        declare_out_varying(gl_info, buffer, settings->flatshading, "vec4 ffp_varying_diffuse;\n");
        declare_out_varying(gl_info, buffer, settings->flatshading, "vec4 ffp_varying_specular;\n");
        declare_out_varying(gl_info, buffer, FALSE, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        declare_out_varying(gl_info, buffer, FALSE, "float ffp_varying_fogcoord;\n");
    }

    shader_addline(buffer, "\nvoid main()\n{\n");
    shader_addline(buffer, "float m;\n");
    shader_addline(buffer, "vec3 r;\n");
    if (settings->vb_indices)
        shader_addline(buffer, "int ind;\n");

    for (i = 0; i < ARRAY_SIZE(attrib_info); ++i)
    {
        if (attrib_info[i].name[0])
            shader_addline(buffer, "%s %s = vs_in%u%s;\n", attrib_info[i].type, attrib_info[i].name,
                    i, settings->swizzle_map & (1u << i) ? ".zyxw" : "");
    }
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        unsigned int coord_idx = settings->texgen[i] & 0x0000ffff;
        if ((settings->texgen[i] & 0xffff0000) == WINED3DTSS_TCI_PASSTHRU
                && settings->texcoords & (1u << i))
            shader_addline(buffer, "vec4 ffp_attrib_texcoord%u = vs_in%u;\n", i, coord_idx + WINED3D_FFP_TEXCOORD0);
    }

    shader_addline(buffer, "ffp_attrib_blendweight[%u] = 1.0;\n", settings->vertexblends);

    if (settings->transformed)
    {
        shader_addline(buffer, "vec4 ec_pos = vec4(ffp_attrib_position.xyz, 1.0);\n");
        shader_addline(buffer, "gl_Position = ffp_projection_matrix * ec_pos;\n");
        shader_addline(buffer, "if (ffp_attrib_position.w != 0.0) gl_Position /= ffp_attrib_position.w;\n");
    }
    else
    {
        for (i = 0; i < settings->vertexblends; ++i)
            shader_addline(buffer, "ffp_attrib_blendweight[%u] -= ffp_attrib_blendweight[%u];\n", settings->vertexblends, i);

        shader_addline(buffer, "vec4 ec_pos = vec4(0.0);\n");
        if (settings->vb_indices)
        {
            for (i = 0; i < settings->vertexblends + 1; ++i)
            {
                shader_addline(buffer, "ind = int(ffp_attrib_blendindices[%u] + 0.1);\n", i);
                shader_addline(buffer, "ec_pos += ffp_attrib_blendweight[%u] * "
                        "(ffp_modelview_matrix[ind] * ffp_attrib_position);\n", i);
            }
        }
        else
        {
            for (i = 0; i < settings->vertexblends + 1; ++i)
                shader_addline(buffer, "ec_pos += ffp_attrib_blendweight[%u] * "
                        "(ffp_modelview_matrix[%u] * ffp_attrib_position);\n", i, i);
        }

        shader_addline(buffer, "gl_Position = ffp_projection_matrix * ec_pos;\n");
        if (settings->clipping)
        {
            if (legacy_syntax)
                shader_addline(buffer, "gl_ClipVertex = ec_pos;\n");
            else
                for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
                    shader_addline(buffer, "gl_ClipDistance[%u] = dot(ec_pos, clip_planes[%u]);\n", i, i);
        }
        shader_addline(buffer, "ec_pos /= ec_pos.w;\n");
    }

    shader_addline(buffer, "vec3 normal = vec3(0.0);\n");
    if (settings->normal)
    {
        if (!settings->vertexblends)
        {
            shader_addline(buffer, "normal = ffp_normal_matrix * ffp_attrib_normal;\n");
        }
        else
        {
            for (i = 0; i < settings->vertexblends + 1; ++i)
            {
                if (settings->vb_indices)
                {
                    shader_addline(buffer, "ind = int(ffp_attrib_blendindices[%u] + 0.1);\n", i);
                    shader_addline(buffer, "normal += ffp_attrib_blendweight[%u] * "
                            "(mat3(ffp_modelview_matrix[ind]) * ffp_attrib_normal);\n", i);
                }
                else
                {
                    shader_addline(buffer, "normal += ffp_attrib_blendweight[%u] * "
                            "(mat3(ffp_modelview_matrix[%u]) * ffp_attrib_normal);\n", i, i);
                }
            }
        }

        if (settings->normalize)
            shader_addline(buffer, "normal = normalize(normal);\n");
    }

    shader_glsl_ffp_vertex_lighting(buffer, settings, legacy_lighting);
    if (legacy_syntax)
    {
        shader_addline(buffer, "gl_FrontColor = ffp_varying_diffuse;\n");
        shader_addline(buffer, "gl_FrontSecondaryColor = ffp_varying_specular;\n");
    }
    else
    {
        shader_addline(buffer, "ffp_varying_diffuse = clamp(ffp_varying_diffuse, 0.0, 1.0);\n");
        shader_addline(buffer, "ffp_varying_specular = clamp(ffp_varying_specular, 0.0, 1.0);\n");
    }

    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        BOOL output_legacy_texcoord = legacy_syntax;

        switch (settings->texgen[i] & 0xffff0000)
        {
            case WINED3DTSS_TCI_PASSTHRU:
                if (settings->texcoords & (1u << i))
                    shader_addline(buffer, "ffp_varying_texcoord[%u] = ffp_texture_matrix[%u] * ffp_attrib_texcoord%u;\n",
                            i, i, i);
                else if (shader_glsl_full_ffp_varyings(gl_info))
                    shader_addline(buffer, "ffp_varying_texcoord[%u] = vec4(0.0);\n", i);
                else
                    output_legacy_texcoord = FALSE;
                break;

            case WINED3DTSS_TCI_CAMERASPACENORMAL:
                shader_addline(buffer, "ffp_varying_texcoord[%u] = ffp_texture_matrix[%u] * vec4(normal, 1.0);\n", i, i);
                break;

            case WINED3DTSS_TCI_CAMERASPACEPOSITION:
                shader_addline(buffer, "ffp_varying_texcoord[%u] = ffp_texture_matrix[%u] * ec_pos;\n", i, i);
                break;

            case WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
                shader_addline(buffer, "ffp_varying_texcoord[%u] = ffp_texture_matrix[%u]"
                        " * vec4(reflect(normalize(ec_pos.xyz), normal), 1.0);\n", i, i);
                break;

            case WINED3DTSS_TCI_SPHEREMAP:
                shader_addline(buffer, "r = reflect(normalize(ec_pos.xyz), normal);\n");
                shader_addline(buffer, "m = 2.0 * length(vec3(r.x, r.y, r.z + 1.0));\n");
                shader_addline(buffer, "ffp_varying_texcoord[%u] = ffp_texture_matrix[%u]"
                        " * vec4(r.x / m + 0.5, r.y / m + 0.5, 0.0, 1.0);\n", i, i);
                break;

            default:
                ERR("Unhandled texgen %#x.\n", settings->texgen[i]);
                break;
        }
        if (output_legacy_texcoord)
            shader_addline(buffer, "gl_TexCoord[%u] = ffp_varying_texcoord[%u];\n", i, i);
    }

    switch (settings->fog_mode)
    {
        case WINED3D_FFP_VS_FOG_OFF:
            output_legacy_fogcoord = FALSE;
            break;

        case WINED3D_FFP_VS_FOG_FOGCOORD:
            shader_addline(buffer, "ffp_varying_fogcoord = ffp_attrib_specular.w * 255.0;\n");
            break;

        case WINED3D_FFP_VS_FOG_RANGE:
            shader_addline(buffer, "ffp_varying_fogcoord = length(ec_pos.xyz);\n");
            break;

        case WINED3D_FFP_VS_FOG_DEPTH:
            if (settings->ortho_fog)
            {
                if (gl_info->supported[ARB_CLIP_CONTROL])
                    shader_addline(buffer, "ffp_varying_fogcoord = gl_Position.z;\n");
                else
                    /* Need to undo the [0.0 - 1.0] -> [-1.0 - 1.0] transformation from D3D to GL coordinates. */
                    shader_addline(buffer, "ffp_varying_fogcoord = gl_Position.z * 0.5 + 0.5;\n");
            }
            else if (settings->transformed)
            {
                shader_addline(buffer, "ffp_varying_fogcoord = ec_pos.z;\n");
            }
            else
            {
                shader_addline(buffer, "ffp_varying_fogcoord = abs(ec_pos.z);\n");
            }
            break;

        default:
            ERR("Unhandled fog mode %#x.\n", settings->fog_mode);
            break;
    }
    if (output_legacy_fogcoord)
        shader_addline(buffer, "gl_FogFragCoord = ffp_varying_fogcoord;\n");

    if (settings->point_size)
    {
        shader_addline(buffer, "gl_PointSize = %s / sqrt(ffp_point.c_att"
                " + ffp_point.l_att * length(ec_pos.xyz)"
                " + ffp_point.q_att * dot(ec_pos.xyz, ec_pos.xyz));\n",
                settings->per_vertex_point_size ? "ffp_attrib_psize" : "ffp_point.size");
        shader_addline(buffer, "gl_PointSize = clamp(gl_PointSize, ffp_point.size_min, ffp_point.size_max);\n");
    }

    shader_addline(buffer, "}\n");

    shader_obj = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
    shader_glsl_compile(gl_info, shader_obj, buffer->buffer);

    return shader_obj;
}

static const char *shader_glsl_get_ffp_fragment_op_arg(struct wined3d_string_buffer *buffer,
        DWORD argnum, unsigned int stage, DWORD arg)
{
    const char *ret;

    if (arg == ARG_UNUSED)
        return "<unused arg>";

    switch (arg & WINED3DTA_SELECTMASK)
    {
        case WINED3DTA_DIFFUSE:
            ret = "ffp_varying_diffuse";
            break;

        case WINED3DTA_CURRENT:
            ret = "ret";
            break;

        case WINED3DTA_TEXTURE:
            switch (stage)
            {
                case 0: ret = "tex0"; break;
                case 1: ret = "tex1"; break;
                case 2: ret = "tex2"; break;
                case 3: ret = "tex3"; break;
                case 4: ret = "tex4"; break;
                case 5: ret = "tex5"; break;
                case 6: ret = "tex6"; break;
                case 7: ret = "tex7"; break;
                default:
                    ret = "<invalid texture>";
                    break;
            }
            break;

        case WINED3DTA_TFACTOR:
            ret = "tex_factor";
            break;

        case WINED3DTA_SPECULAR:
            ret = "ffp_varying_specular";
            break;

        case WINED3DTA_TEMP:
            ret = "temp_reg";
            break;

        case WINED3DTA_CONSTANT:
            switch (stage)
            {
                case 0: ret = "tss_const0"; break;
                case 1: ret = "tss_const1"; break;
                case 2: ret = "tss_const2"; break;
                case 3: ret = "tss_const3"; break;
                case 4: ret = "tss_const4"; break;
                case 5: ret = "tss_const5"; break;
                case 6: ret = "tss_const6"; break;
                case 7: ret = "tss_const7"; break;
                default:
                    ret = "<invalid constant>";
                    break;
            }
            break;

        default:
            return "<unhandled arg>";
    }

    if (arg & WINED3DTA_COMPLEMENT)
    {
        shader_addline(buffer, "arg%u = vec4(1.0) - %s;\n", argnum, ret);
        if (argnum == 0)
            ret = "arg0";
        else if (argnum == 1)
            ret = "arg1";
        else if (argnum == 2)
            ret = "arg2";
    }

    if (arg & WINED3DTA_ALPHAREPLICATE)
    {
        shader_addline(buffer, "arg%u = vec4(%s.w);\n", argnum, ret);
        if (argnum == 0)
            ret = "arg0";
        else if (argnum == 1)
            ret = "arg1";
        else if (argnum == 2)
            ret = "arg2";
    }

    return ret;
}

static void shader_glsl_ffp_fragment_op(struct wined3d_string_buffer *buffer, unsigned int stage, BOOL color,
        BOOL alpha, BOOL tmp_dst, DWORD op, DWORD dw_arg0, DWORD dw_arg1, DWORD dw_arg2)
{
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;

    if (color && alpha)
        dstmask = "";
    else if (color)
        dstmask = ".xyz";
    else
        dstmask = ".w";

    dstreg = tmp_dst ? "temp_reg" : "ret";

    arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, dw_arg0);
    arg1 = shader_glsl_get_ffp_fragment_op_arg(buffer, 1, stage, dw_arg1);
    arg2 = shader_glsl_get_ffp_fragment_op_arg(buffer, 2, stage, dw_arg2);

    switch (op)
    {
        case WINED3D_TOP_DISABLE:
            break;

        case WINED3D_TOP_SELECT_ARG1:
            shader_addline(buffer, "%s%s = %s%s;\n", dstreg, dstmask, arg1, dstmask);
            break;

        case WINED3D_TOP_SELECT_ARG2:
            shader_addline(buffer, "%s%s = %s%s;\n", dstreg, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE:
            shader_addline(buffer, "%s%s = %s%s * %s%s;\n", dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE_4X:
            shader_addline(buffer, "%s%s = clamp(%s%s * %s%s * 4.0, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE_2X:
            shader_addline(buffer, "%s%s = clamp(%s%s * %s%s * 2.0, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD:
            shader_addline(buffer, "%s%s = clamp(%s%s + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SIGNED:
            shader_addline(buffer, "%s%s = clamp(%s%s + (%s - vec4(0.5))%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SIGNED_2X:
            shader_addline(buffer, "%s%s = clamp((%s%s + (%s - vec4(0.5))%s) * 2.0, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_SUBTRACT:
            shader_addline(buffer, "%s%s = clamp(%s%s - %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SMOOTH:
            shader_addline(buffer, "%s%s = clamp((vec4(1.0) - %s)%s * %s%s + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1, dstmask);
            break;

        case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
            arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, WINED3DTA_DIFFUSE);
            shader_addline(buffer, "%s%s = mix(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "%s%s = mix(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_FACTOR_ALPHA:
            arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, WINED3DTA_TFACTOR);
            shader_addline(buffer, "%s%s = mix(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
            arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "%s%s = clamp(%s%s * (1.0 - %s.w) + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg2, dstmask, arg0, arg1, dstmask);
            break;

        case WINED3D_TOP_BLEND_CURRENT_ALPHA:
            arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, WINED3DTA_CURRENT);
            shader_addline(buffer, "%s%s = mix(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
            shader_addline(buffer, "%s%s = clamp(%s%s * %s.w + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, arg1, dstmask);
            break;

        case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
            shader_addline(buffer, "%s%s = clamp(%s%s * %s%s + %s.w, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1);
            break;

        case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
            shader_addline(buffer, "%s%s = clamp(%s%s * (1.0 - %s.w) + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, arg1, dstmask);
            break;
        case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
            shader_addline(buffer, "%s%s = clamp((vec4(1.0) - %s)%s * %s%s + %s.w, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1);
            break;

        case WINED3D_TOP_BUMPENVMAP:
        case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
            /* These are handled in the first pass, nothing to do. */
            break;

        case WINED3D_TOP_DOTPRODUCT3:
            shader_addline(buffer, "%s%s = vec4(clamp(dot(%s.xyz - 0.5, %s.xyz - 0.5) * 4.0, 0.0, 1.0))%s;\n",
                    dstreg, dstmask, arg1, arg2, dstmask);
            break;

        case WINED3D_TOP_MULTIPLY_ADD:
            shader_addline(buffer, "%s%s = clamp(%s%s * %s%s + %s%s, 0.0, 1.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg0, dstmask);
            break;

        case WINED3D_TOP_LERP:
            /* MSDN isn't quite right here. */
            shader_addline(buffer, "%s%s = mix(%s%s, %s%s, %s%s);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0, dstmask);
            break;

        default:
            FIXME("Unhandled operation %#x.\n", op);
            break;
    }
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_ffp_fragment_shader(struct shader_glsl_priv *priv,
        const struct ffp_frag_settings *settings, const struct wined3d_context_gl *context_gl)
{
    struct wined3d_string_buffer *tex_reg_name = string_buffer_get(&priv->string_buffers);
    enum wined3d_cmp_func alpha_test_func = settings->alpha_test_func + 1;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    BYTE lum_map = 0, bump_map = 0, tex_map = 0, tss_const_map = 0;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const BOOL legacy_syntax = needs_legacy_glsl_syntax(gl_info);
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    UINT lowest_disabled_stage;
    GLuint shader_id;
    DWORD arg0, arg1, arg2;
    unsigned int stage;

    string_buffer_clear(buffer);

    /* Find out which textures are read */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
            break;

        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE
                || (stage == 0 && settings->color_key_enabled))
            tex_map |= 1u << stage;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (settings->op[stage].tmp_dst)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            tss_const_map |= 1u << stage;

        switch (settings->op[stage].cop)
        {
            case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
                lum_map |= 1u << stage;
                /* fall through */
            case WINED3D_TOP_BUMPENVMAP:
                bump_map |= 1u << stage;
                /* fall through */
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                tex_map |= 1u << stage;
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
            tex_map |= 1u << stage;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            tss_const_map |= 1u << stage;
    }
    lowest_disabled_stage = stage;

    shader_glsl_add_version_declaration(buffer, gl_info);

    if (shader_glsl_use_explicit_attrib_location(gl_info))
        shader_addline(buffer, "#extension GL_ARB_explicit_attrib_location : enable\n");
    if (gl_info->supported[ARB_SHADING_LANGUAGE_420PACK])
        shader_addline(buffer, "#extension GL_ARB_shading_language_420pack : enable\n");
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");

    if (!needs_legacy_glsl_syntax(gl_info))
    {
        shader_addline(buffer, "vec4 ps_out[1];\n");
        if (shader_glsl_use_explicit_attrib_location(gl_info))
            shader_addline(buffer, "layout(location = 0) ");
        shader_addline(buffer, "out vec4 color_out0;\n");
    }

    shader_addline(buffer, "vec4 tmp0, tmp1;\n");
    shader_addline(buffer, "vec4 ret;\n");
    if (tempreg_used || settings->sRGB_write)
        shader_addline(buffer, "vec4 temp_reg = vec4(0.0);\n");
    shader_addline(buffer, "vec4 arg0, arg1, arg2;\n");

    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        const char *sampler_type;

        if (tss_const_map & (1u << stage))
            shader_addline(buffer, "uniform vec4 tss_const%u;\n", stage);

        if (!(tex_map & (1u << stage)))
            continue;

        switch (settings->op[stage].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                sampler_type = "1D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                sampler_type = "2D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                sampler_type = "3D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                sampler_type = "Cube";
                break;
            case WINED3D_GL_RES_TYPE_TEX_RECT:
                sampler_type = "2DRect";
                break;
            default:
                FIXME("Unhandled sampler type %#x.\n", settings->op[stage].tex_type);
                sampler_type = NULL;
                break;
        }
        if (sampler_type)
        {
            if (shader_glsl_use_layout_binding_qualifier(gl_info))
                shader_glsl_append_sampler_binding_qualifier(buffer, &context_gl->c, NULL, stage);
            shader_addline(buffer, "uniform sampler%s ps_sampler%u;\n", sampler_type, stage);
        }

        shader_addline(buffer, "vec4 tex%u;\n", stage);

        if (!(bump_map & (1u << stage)))
            continue;
        shader_addline(buffer, "uniform mat2 bumpenv_mat%u;\n", stage);

        if (!(lum_map & (1u << stage)))
            continue;
        shader_addline(buffer, "uniform float bumpenv_lum_scale%u;\n", stage);
        shader_addline(buffer, "uniform float bumpenv_lum_offset%u;\n", stage);
    }
    if (tfactor_used)
        shader_addline(buffer, "uniform vec4 tex_factor;\n");
    if (settings->color_key_enabled)
        shader_addline(buffer, "uniform vec4 color_key[2];\n");
    shader_addline(buffer, "uniform vec4 specular_enable;\n");

    if (settings->sRGB_write)
    {
        shader_addline(buffer, "const vec4 srgb_const0 = ");
        shader_glsl_append_imm_vec(buffer, &wined3d_srgb_const[0].x, 4, gl_info);
        shader_addline(buffer, ";\n");
        shader_addline(buffer, "const vec4 srgb_const1 = ");
        shader_glsl_append_imm_vec(buffer, &wined3d_srgb_const[1].x, 4, gl_info);
        shader_addline(buffer, ";\n");
    }

    shader_addline(buffer, "uniform struct\n{\n");
    shader_addline(buffer, "    vec4 color;\n");
    shader_addline(buffer, "    float density;\n");
    shader_addline(buffer, "    float end;\n");
    shader_addline(buffer, "    float scale;\n");
    shader_addline(buffer, "} ffp_fog;\n");

    if (alpha_test_func != WINED3D_CMP_ALWAYS)
        shader_addline(buffer, "uniform float alpha_test_ref;\n");

    if (legacy_syntax)
    {
        shader_addline(buffer, "vec4 ffp_varying_diffuse;\n");
        shader_addline(buffer, "vec4 ffp_varying_specular;\n");
        shader_addline(buffer, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        shader_addline(buffer, "vec4 ffp_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        shader_addline(buffer, "float ffp_varying_fogcoord;\n");
    }
    else
    {
        declare_in_varying(gl_info, buffer, settings->flatshading, "vec4 ffp_varying_diffuse;\n");
        declare_in_varying(gl_info, buffer, settings->flatshading, "vec4 ffp_varying_specular;\n");
        declare_in_varying(gl_info, buffer, FALSE, "vec4 ffp_varying_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        shader_addline(buffer, "vec4 ffp_texcoord[%u];\n", WINED3D_MAX_TEXTURES);
        declare_in_varying(gl_info, buffer, FALSE, "float ffp_varying_fogcoord;\n");
    }

    shader_addline(buffer, "void main()\n{\n");

    if (legacy_syntax)
    {
        shader_addline(buffer, "ffp_varying_diffuse = gl_Color;\n");
        shader_addline(buffer, "ffp_varying_specular = gl_SecondaryColor;\n");
    }

    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        if (tex_map & (1u << stage))
        {
            if (settings->pointsprite)
                shader_addline(buffer, "ffp_texcoord[%u] = vec4(gl_PointCoord.xy, 0.0, 0.0);\n", stage);
            else if (settings->texcoords_initialized & (1u << stage))
                shader_addline(buffer, "ffp_texcoord[%u] = %s[%u];\n",
                        stage, legacy_syntax ? "gl_TexCoord" : "ffp_varying_texcoord", stage);
            else
                shader_addline(buffer, "ffp_texcoord[%u] = vec4(0.0);\n", stage);
        }
    }

    if (legacy_syntax && settings->fog != WINED3D_FFP_PS_FOG_OFF)
        shader_addline(buffer, "ffp_varying_fogcoord = gl_FogFragCoord;\n");

    if (lowest_disabled_stage < 7 && settings->emul_clipplanes)
        shader_addline(buffer, "if (any(lessThan(ffp_texcoord[7], vec4(0.0)))) discard;\n");

    /* Generate texture sampling instructions */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES && settings->op[stage].cop != WINED3D_TOP_DISABLE; ++stage)
    {
        const char *texture_function, *coord_mask;
        BOOL proj;

        if (!(tex_map & (1u << stage)))
            continue;

        if (settings->op[stage].projected == WINED3D_PROJECTION_NONE)
        {
            proj = FALSE;
        }
        else if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT4
                || settings->op[stage].projected == WINED3D_PROJECTION_COUNT3)
        {
            proj = TRUE;
        }
        else
        {
            FIXME("Unexpected projection mode %d\n", settings->op[stage].projected);
            proj = TRUE;
        }

        if (settings->op[stage].tex_type == WINED3D_GL_RES_TYPE_TEX_CUBE)
            proj = FALSE;

        switch (settings->op[stage].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                texture_function = "texture1D";
                coord_mask = "x";
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                texture_function = "texture2D";
                coord_mask = "xy";
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                texture_function = "texture3D";
                coord_mask = "xyz";
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                texture_function = "textureCube";
                coord_mask = "xyz";
                break;
            case WINED3D_GL_RES_TYPE_TEX_RECT:
                texture_function = "texture2DRect";
                coord_mask = "xy";
                break;
            default:
                FIXME("Unhandled texture type %#x.\n", settings->op[stage].tex_type);
                texture_function = "";
                coord_mask = "xyzw";
                proj = FALSE;
                break;
        }
        if (!legacy_syntax)
            texture_function = "texture";

        if (stage > 0
                && (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP
                || settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE))
        {
            shader_addline(buffer, "ret.xy = bumpenv_mat%u * tex%u.xy;\n", stage - 1, stage - 1);

            /* With projective textures, texbem only divides the static
             * texture coordinate, not the displacement, so multiply the
             * displacement with the dividing parameter before passing it to
             * TXP. */
            if (settings->op[stage].projected != WINED3D_PROJECTION_NONE)
            {
                if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT4)
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * ffp_texcoord[%u].w) + ffp_texcoord[%u].xy;\n",
                            stage, stage);
                    shader_addline(buffer, "ret.zw = ffp_texcoord[%u].ww;\n", stage);
                }
                else
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * ffp_texcoord[%u].z) + ffp_texcoord[%u].xy;\n",
                            stage, stage);
                    shader_addline(buffer, "ret.zw = ffp_texcoord[%u].zz;\n", stage);
                }
            }
            else
            {
                shader_addline(buffer, "ret = ffp_texcoord[%u] + ret.xyxy;\n", stage);
            }

            shader_addline(buffer, "tex%u = %s%s(ps_sampler%u, ret.%s%s);\n",
                    stage, texture_function, proj ? "Proj" : "", stage, coord_mask, proj ? "w" : "");

            if (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
                shader_addline(buffer, "tex%u *= clamp(tex%u.z * bumpenv_lum_scale%u + bumpenv_lum_offset%u, 0.0, 1.0);\n",
                        stage, stage - 1, stage - 1, stage - 1);
        }
        else if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT3)
        {
            shader_addline(buffer, "tex%u = %s%s(ps_sampler%u, ffp_texcoord[%u].xyz);\n",
                    stage, texture_function, proj ? "Proj" : "", stage, stage);
        }
        else
        {
            shader_addline(buffer, "tex%u = %s%s(ps_sampler%u, ffp_texcoord[%u].%s%s);\n",
                    stage, texture_function, proj ? "Proj" : "", stage, stage, coord_mask, proj ? "w" : "");
        }

        string_buffer_sprintf(tex_reg_name, "tex%u", stage);
        shader_glsl_color_correction_ext(buffer, tex_reg_name->buffer, WINED3DSP_WRITEMASK_ALL,
                settings->op[stage].color_fixup);
    }

    if (settings->color_key_enabled)
        shader_glsl_generate_colour_key_test(buffer, "tex0", "color_key[0]", "color_key[1]");

    shader_addline(buffer, "ret = ffp_varying_diffuse;\n");

    /* Generate the main shader */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        BOOL op_equal;

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
            op_equal = settings->op[stage].aop == settings->op[stage].cop
                    && settings->op[stage].carg0 == settings->op[stage].aarg0
                    && settings->op[stage].carg1 == settings->op[stage].aarg1
                    && settings->op[stage].carg2 == settings->op[stage].aarg2;

        if (settings->op[stage].aop == WINED3D_TOP_DISABLE)
        {
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, FALSE, settings->op[stage].tmp_dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
        }
        else if (op_equal)
        {
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, TRUE, settings->op[stage].tmp_dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
        }
        else if (settings->op[stage].cop != WINED3D_TOP_BUMPENVMAP
                && settings->op[stage].cop != WINED3D_TOP_BUMPENVMAP_LUMINANCE)
        {
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, FALSE, settings->op[stage].tmp_dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
            shader_glsl_ffp_fragment_op(buffer, stage, FALSE, TRUE, settings->op[stage].tmp_dst,
                    settings->op[stage].aop, settings->op[stage].aarg0,
                    settings->op[stage].aarg1, settings->op[stage].aarg2);
        }
    }

    shader_addline(buffer, "%s[0] = ffp_varying_specular * specular_enable + ret;\n",
            get_fragment_output(gl_info));

    if (settings->sRGB_write)
        shader_glsl_generate_srgb_write_correction(buffer, gl_info);

    shader_glsl_generate_fog_code(buffer, gl_info, settings->fog);

    shader_glsl_generate_alpha_test(buffer, gl_info, alpha_test_func);
    if (!needs_legacy_glsl_syntax(gl_info))
        shader_addline(buffer, "color_out0 = ps_out[0];\n");

    shader_addline(buffer, "}\n");

    shader_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    string_buffer_release(&priv->string_buffers, tex_reg_name);
    return shader_id;
}

static struct glsl_ffp_vertex_shader *shader_glsl_find_ffp_vertex_shader(struct shader_glsl_priv *priv,
        const struct wined3d_gl_info *gl_info, const struct wined3d_ffp_vs_settings *settings)
{
    struct glsl_ffp_vertex_shader *shader;
    const struct wine_rb_entry *entry;

    if ((entry = wine_rb_get(&priv->ffp_vertex_shaders, settings)))
        return WINE_RB_ENTRY_VALUE(entry, struct glsl_ffp_vertex_shader, desc.entry);

    if (!(shader = heap_alloc(sizeof(*shader))))
        return NULL;

    shader->desc.settings = *settings;
    shader->id = shader_glsl_generate_ffp_vertex_shader(priv, settings, gl_info);
    list_init(&shader->linked_programs);
    if (wine_rb_put(&priv->ffp_vertex_shaders, &shader->desc.settings, &shader->desc.entry) == -1)
        ERR("Failed to insert ffp vertex shader.\n");

    return shader;
}

static struct glsl_ffp_fragment_shader *shader_glsl_find_ffp_fragment_shader(struct shader_glsl_priv *priv,
        const struct ffp_frag_settings *args, const struct wined3d_context_gl *context_gl)
{
    struct glsl_ffp_fragment_shader *glsl_desc;
    const struct ffp_frag_desc *desc;

    if ((desc = find_ffp_frag_shader(&priv->ffp_fragment_shaders, args)))
        return CONTAINING_RECORD(desc, struct glsl_ffp_fragment_shader, entry);

    if (!(glsl_desc = heap_alloc(sizeof(*glsl_desc))))
        return NULL;

    glsl_desc->entry.settings = *args;
    glsl_desc->id = shader_glsl_generate_ffp_fragment_shader(priv, args, context_gl);
    list_init(&glsl_desc->linked_programs);
    add_ffp_frag_shader(&priv->ffp_fragment_shaders, &glsl_desc->entry);

    return glsl_desc;
}


static void shader_glsl_init_vs_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_vs_program *vs,
        unsigned int vs_c_count)
{
    unsigned int i;
    struct wined3d_string_buffer *name = string_buffer_get(&priv->string_buffers);

    if (priv->consts_ubo && vs_c_count)
    {
        unsigned int base, count;

        vs->vs_c_block_index = GL_EXTCALL(glGetUniformBlockIndex(program_id, "vs_c_ubo"));
        checkGLcall("glGetUniformBlockIndex");
        if (vs->vs_c_block_index == -1)
            ERR("Could not get ubo_vs_c block index.\n");

        wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, WINED3D_SHADER_TYPE_VERTEX,
                &base, &count);
        assert(count >= 1);
        GL_EXTCALL(glUniformBlockBinding(program_id, vs->vs_c_block_index, base));
        checkGLcall("glUniformBlockBinding");
    }
    else if (!priv->consts_ubo)
    {
        for (i = 0; i < min(vs_c_count, priv->max_vs_consts_f); ++i)
        {
            string_buffer_sprintf(name, "vs_c[%u]", i);
            vs->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        }
        if (vs_c_count < priv->max_vs_consts_f)
            memset(&vs->uniform_f_locations[vs_c_count], 0xff, (priv->max_vs_consts_f - vs_c_count) * sizeof(GLuint));
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        string_buffer_sprintf(name, "vs_i[%u]", i);
        vs->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        string_buffer_sprintf(name, "vs_b[%u]", i);
        vs->uniform_b_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    vs->pos_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "pos_fixup"));
    vs->base_vertex_id_location = GL_EXTCALL(glGetUniformLocation(program_id, "base_vertex_id"));

    for (i = 0; i < MAX_VERTEX_BLENDS; ++i)
    {
        string_buffer_sprintf(name, "ffp_modelview_matrix[%u]", i);
        vs->modelview_matrix_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        vs->modelview_block_index = GL_EXTCALL(glGetUniformBlockIndex(program_id, "ffp_modelview_ubo"));
        checkGLcall("glGetUniformBlockIndex");
        if (vs->modelview_block_index != -1)
        {
            unsigned int base, count;

            wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, WINED3D_SHADER_TYPE_VERTEX,
                    &base, &count);
            assert(count >= 2);

            GL_EXTCALL(glUniformBlockBinding(program_id, vs->modelview_block_index, base + 1));
            checkGLcall("glUniformBlockBinding");
        }
    }
    else
    {
        vs->modelview_block_index = -1;
    }

    vs->projection_matrix_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_projection_matrix"));
    vs->normal_matrix_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_normal_matrix"));
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        string_buffer_sprintf(name, "ffp_texture_matrix[%u]", i);
        vs->texture_matrix_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    vs->material_ambient_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_material.ambient"));
    vs->material_diffuse_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_material.diffuse"));
    vs->material_specular_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_material.specular"));
    vs->material_emissive_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_material.emissive"));
    vs->material_shininess_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_material.shininess"));
    vs->light_ambient_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_light_ambient"));
    for (i = 0; i < WINED3D_MAX_ACTIVE_LIGHTS; ++i)
    {
        string_buffer_sprintf(name, "ffp_light[%u].diffuse", i);
        vs->light_location[i].diffuse = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].specular", i);
        vs->light_location[i].specular = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].ambient", i);
        vs->light_location[i].ambient = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].position", i);
        vs->light_location[i].position = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].direction", i);
        vs->light_location[i].direction = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].range", i);
        vs->light_location[i].range = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].falloff", i);
        vs->light_location[i].falloff = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].c_att", i);
        vs->light_location[i].c_att = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].l_att", i);
        vs->light_location[i].l_att = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].q_att", i);
        vs->light_location[i].q_att = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].cos_htheta", i);
        vs->light_location[i].cos_htheta = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "ffp_light[%u].cos_hphi", i);
        vs->light_location[i].cos_hphi = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    vs->pointsize_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.size"));
    vs->pointsize_min_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.size_min"));
    vs->pointsize_max_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.size_max"));
    vs->pointsize_c_att_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.c_att"));
    vs->pointsize_l_att_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.l_att"));
    vs->pointsize_q_att_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_point.q_att"));
    vs->clip_planes_location = GL_EXTCALL(glGetUniformLocation(program_id, "clip_planes"));

    string_buffer_release(&priv->string_buffers, name);
}

static void shader_glsl_init_ds_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_ds_program *ds)
{
    ds->pos_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "pos_fixup"));
}

static void shader_glsl_init_gs_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_gs_program *gs)
{
    gs->pos_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "pos_fixup"));
}

static void shader_glsl_init_ps_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_ps_program *ps, unsigned int ps_c_count)
{
    unsigned int i;
    struct wined3d_string_buffer *name = string_buffer_get(&priv->string_buffers);

    for (i = 0; i < ps_c_count; ++i)
    {
        string_buffer_sprintf(name, "ps_c[%u]", i);
        ps->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    memset(&ps->uniform_f_locations[ps_c_count], 0xff, (WINED3D_MAX_PS_CONSTS_F - ps_c_count) * sizeof(GLuint));

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        string_buffer_sprintf(name, "ps_i[%u]", i);
        ps->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        string_buffer_sprintf(name, "ps_b[%u]", i);
        ps->uniform_b_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        string_buffer_sprintf(name, "bumpenv_mat%u", i);
        ps->bumpenv_mat_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "bumpenv_lum_scale%u", i);
        ps->bumpenv_lum_scale_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "bumpenv_lum_offset%u", i);
        ps->bumpenv_lum_offset_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
        string_buffer_sprintf(name, "tss_const%u", i);
        ps->tss_constant_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    ps->tex_factor_location = GL_EXTCALL(glGetUniformLocation(program_id, "tex_factor"));
    ps->specular_enable_location = GL_EXTCALL(glGetUniformLocation(program_id, "specular_enable"));

    ps->fog_color_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_fog.color"));
    ps->fog_density_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_fog.density"));
    ps->fog_end_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_fog.end"));
    ps->fog_scale_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_fog.scale"));

    ps->alpha_test_ref_location = GL_EXTCALL(glGetUniformLocation(program_id, "alpha_test_ref"));

    ps->np2_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "ps_samplerNP2Fixup"));
    ps->ycorrection_location = GL_EXTCALL(glGetUniformLocation(program_id, "ycorrection"));
    ps->color_key_location = GL_EXTCALL(glGetUniformLocation(program_id, "color_key"));

    string_buffer_release(&priv->string_buffers, name);
}

static HRESULT shader_glsl_compile_compute_shader(struct shader_glsl_priv *priv,
        const struct wined3d_context_gl *context_gl, struct wined3d_shader *shader)
{
    struct glsl_context_data *ctx_data = context_gl->c.shader_backend_data;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    struct glsl_cs_compiled_shader *gl_shaders;
    struct glsl_shader_private *shader_data;
    struct glsl_shader_prog_link *entry;
    GLuint shader_id, program_id;

    if (!(entry = heap_alloc(sizeof(*entry))))
    {
        ERR("Out of memory.\n");
        return E_OUTOFMEMORY;
    }

    if (!(shader->backend_data = heap_alloc_zero(sizeof(*shader_data))))
    {
        ERR("Failed to allocate backend data.\n");
        heap_free(entry);
        return E_OUTOFMEMORY;
    }
    shader_data = shader->backend_data;

    if (!(shader_data->gl_shaders.cs = heap_alloc(sizeof(*gl_shaders))))
    {
        ERR("Failed to allocate GL shader array.\n");
        heap_free(entry);
        heap_free(shader->backend_data);
        shader->backend_data = NULL;
        return E_OUTOFMEMORY;
    }
    shader_data->shader_array_size = 1;
    gl_shaders = shader_data->gl_shaders.cs;

    TRACE("Compiling compute shader %p.\n", shader);

    string_buffer_clear(buffer);
    shader_id = shader_glsl_generate_compute_shader(context_gl, buffer, &priv->string_buffers, shader);
    gl_shaders[shader_data->num_gl_shaders++].id = shader_id;

    program_id = GL_EXTCALL(glCreateProgram());
    TRACE("Created new GLSL shader program %u.\n", program_id);

    entry->id = program_id;
    entry->vs.id = 0;
    entry->hs.id = 0;
    entry->ds.id = 0;
    entry->gs.id = 0;
    entry->ps.id = 0;
    entry->cs.id = shader_id;
    entry->constant_version = 0;
    entry->shader_controlled_clip_distances = 0;
    entry->ps.np2_fixup_info = NULL;
    add_glsl_program_entry(priv, entry);

    TRACE("Attaching GLSL shader object %u to program %u.\n", shader_id, program_id);
    GL_EXTCALL(glAttachShader(program_id, shader_id));
    checkGLcall("glAttachShader");

    list_add_head(&shader->linked_programs, &entry->cs.shader_entry);

    TRACE("Linking GLSL shader program %u.\n", program_id);
    GL_EXTCALL(glLinkProgram(program_id));
    shader_glsl_validate_link(gl_info, program_id);

    GL_EXTCALL(glUseProgram(program_id));
    checkGLcall("glUseProgram");
    shader_glsl_load_program_resources(context_gl, priv, program_id, shader);
    shader_glsl_load_images(gl_info, priv, program_id, &shader->reg_maps);

    entry->constant_update_mask = 0;

    GL_EXTCALL(glUseProgram(ctx_data->glsl_program ? ctx_data->glsl_program->id : 0));
    checkGLcall("glUseProgram");
    return WINED3D_OK;
}

static GLuint find_glsl_compute_shader(const struct wined3d_context_gl *context_gl,
        struct shader_glsl_priv *priv, struct wined3d_shader *shader)
{
    struct glsl_shader_private *shader_data;

    if (!shader->backend_data)
    {
        WARN("Failed to find GLSL program for compute shader %p.\n", shader);
        if (FAILED(shader_glsl_compile_compute_shader(priv, context_gl, shader)))
        {
            ERR("Failed to compile compute shader %p.\n", shader);
            return 0;
        }
    }
    shader_data = shader->backend_data;
    return shader_data->gl_shaders.cs[0].id;
}

/* Context activation is done by the caller. */
static void set_glsl_compute_shader_program(const struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, struct shader_glsl_priv *priv, struct glsl_context_data *ctx_data)
{
    struct glsl_shader_prog_link *entry;
    struct wined3d_shader *shader;
    struct glsl_program_key key;
    GLuint cs_id;

    if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_COMPUTE)))
        return;

    if (!(shader = state->shader[WINED3D_SHADER_TYPE_COMPUTE]))
    {
        WARN("Compute shader is NULL.\n");
        ctx_data->glsl_program = NULL;
        return;
    }

    cs_id = find_glsl_compute_shader(context_gl, priv, shader);
    memset(&key, 0, sizeof(key));
    key.cs_id = cs_id;
    if (!(entry = get_glsl_program_entry(priv, &key)))
        ERR("Failed to find GLSL program for compute shader %p.\n", shader);
    ctx_data->glsl_program = entry;
}

/* Context activation is done by the caller. */
static void set_glsl_shader_program(const struct wined3d_context_gl *context_gl, const struct wined3d_state *state,
        struct shader_glsl_priv *priv, struct glsl_context_data *ctx_data)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_shader *pre_rasterization_shader;
    const struct ps_np2fixup_info *np2fixup_info = NULL;
    struct wined3d_shader *hshader, *dshader, *gshader;
    struct glsl_shader_prog_link *entry = NULL;
    struct wined3d_shader *vshader = NULL;
    struct wined3d_shader *pshader = NULL;
    GLuint reorder_shader_id = 0;
    struct glsl_program_key key;
    GLuint program_id;
    unsigned int i;
    GLuint vs_id = 0;
    GLuint hs_id = 0;
    GLuint ds_id = 0;
    GLuint gs_id = 0;
    GLuint ps_id = 0;
    struct list *ps_list, *vs_list;
    WORD attribs_map;
    struct wined3d_string_buffer *tmp_name;

    if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_VERTEX)) && ctx_data->glsl_program)
    {
        vs_id = ctx_data->glsl_program->vs.id;
        vs_list = &ctx_data->glsl_program->vs.shader_entry;

        if (use_vs(state))
            vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    }
    else if (use_vs(state))
    {
        struct vs_compile_args vs_compile_args;

        vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];

        find_vs_compile_args(state, vshader, context_gl->c.stream_info.swizzle_map, &vs_compile_args, &context_gl->c);
        vs_id = find_glsl_vertex_shader(context_gl, priv, vshader, &vs_compile_args);
        vs_list = &vshader->linked_programs;
    }
    else if (priv->vertex_pipe == &glsl_vertex_pipe)
    {
        struct glsl_ffp_vertex_shader *ffp_shader;
        struct wined3d_ffp_vs_settings settings;

        wined3d_ffp_get_vs_settings(&context_gl->c, state, &settings);
        ffp_shader = shader_glsl_find_ffp_vertex_shader(priv, gl_info, &settings);
        vs_id = ffp_shader->id;
        vs_list = &ffp_shader->linked_programs;
    }

    if (vshader && vshader->reg_maps.constant_float_count > WINED3D_MAX_VS_CONSTS_F
            && !device_is_swvp(context_gl->c.device))
        FIXME("Applying context with SW shader in HW mode.\n");

    hshader = state->shader[WINED3D_SHADER_TYPE_HULL];
    if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_HULL)) && ctx_data->glsl_program)
        hs_id = ctx_data->glsl_program->hs.id;
    else if (hshader)
        hs_id = find_glsl_hull_shader(context_gl, priv, hshader);

    dshader = state->shader[WINED3D_SHADER_TYPE_DOMAIN];
    if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_DOMAIN)) && ctx_data->glsl_program)
    {
        ds_id = ctx_data->glsl_program->ds.id;
    }
    else if (dshader)
    {
        struct ds_compile_args args;

        find_ds_compile_args(state, dshader, &args, &context_gl->c);
        ds_id = find_glsl_domain_shader(context_gl, priv, dshader, &args);
    }

    gshader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];
    if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_GEOMETRY)) && ctx_data->glsl_program)
    {
        gs_id = ctx_data->glsl_program->gs.id;
    }
    else if (gshader)
    {
        struct gs_compile_args args;

        find_gs_compile_args(state, gshader, &args, &context_gl->c);
        gs_id = find_glsl_geometry_shader(context_gl, priv, gshader, &args);
    }

    /* A pixel shader is not used when rasterization is disabled. */
    if (is_rasterization_disabled(gshader))
    {
        ps_id = 0;
        ps_list = NULL;
    }
    else if (!(context_gl->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_PIXEL)) && ctx_data->glsl_program)
    {
        ps_id = ctx_data->glsl_program->ps.id;
        ps_list = &ctx_data->glsl_program->ps.shader_entry;

        if (use_ps(state))
            pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    }
    else if (use_ps(state))
    {
        struct ps_compile_args ps_compile_args;
        pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
        find_ps_compile_args(state, pshader, context_gl->c.stream_info.position_transformed,
                &ps_compile_args, &context_gl->c);
        ps_id = find_glsl_fragment_shader(context_gl, &priv->shader_buffer, &priv->string_buffers,
                pshader, &ps_compile_args, &np2fixup_info);
        ps_list = &pshader->linked_programs;
    }
    else if (priv->fragment_pipe == &glsl_fragment_pipe
            && !(vshader && vshader->reg_maps.shader_version.major >= 4))
    {
        struct glsl_ffp_fragment_shader *ffp_shader;
        struct ffp_frag_settings settings;

        gen_ffp_frag_op(&context_gl->c, state, &settings, FALSE);
        ffp_shader = shader_glsl_find_ffp_fragment_shader(priv, &settings, context_gl);
        ps_id = ffp_shader->id;
        ps_list = &ffp_shader->linked_programs;
    }

    key.vs_id = vs_id;
    key.hs_id = hs_id;
    key.ds_id = ds_id;
    key.gs_id = gs_id;
    key.ps_id = ps_id;
    key.cs_id = 0;
    if ((!vs_id && !hs_id && !ds_id && !gs_id && !ps_id) || (entry = get_glsl_program_entry(priv, &key)))
    {
        ctx_data->glsl_program = entry;
        return;
    }

    /* If we get to this point, then no matching program exists, so we create one */
    program_id = GL_EXTCALL(glCreateProgram());
    TRACE("Created new GLSL shader program %u.\n", program_id);

    /* Create the entry */
    entry = heap_alloc(sizeof(*entry));
    entry->id = program_id;
    entry->vs.id = vs_id;
    entry->hs.id = hs_id;
    entry->ds.id = ds_id;
    entry->gs.id = gs_id;
    entry->ps.id = ps_id;
    entry->cs.id = 0;
    entry->constant_version = 0;
    entry->shader_controlled_clip_distances = 0;
    entry->ps.np2_fixup_info = np2fixup_info;
    /* Add the hash table entry */
    add_glsl_program_entry(priv, entry);

    /* Set the current program */
    ctx_data->glsl_program = entry;

    /* Attach GLSL vshader */
    if (vs_id)
    {
        TRACE("Attaching GLSL shader object %u to program %u.\n", vs_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, vs_id));
        checkGLcall("glAttachShader");

        list_add_head(vs_list, &entry->vs.shader_entry);
    }

    if (vshader)
    {
        attribs_map = vshader->reg_maps.input_registers;
        if (vshader->reg_maps.shader_version.major < 4)
        {
            reorder_shader_id = shader_glsl_generate_vs3_rasterizer_input_setup(priv, vshader, pshader,
                    state->gl_primitive_type == GL_POINTS && vshader->reg_maps.point_size,
                    d3d_info->emulated_flatshading
                    && state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT, gl_info);
            TRACE("Attaching GLSL shader object %u to program %u.\n", reorder_shader_id, program_id);
            GL_EXTCALL(glAttachShader(program_id, reorder_shader_id));
            checkGLcall("glAttachShader");
            /* Flag the reorder function for deletion, it will be freed
             * automatically when the program is destroyed. */
            GL_EXTCALL(glDeleteShader(reorder_shader_id));
        }
    }
    else
    {
        attribs_map = (1u << WINED3D_FFP_ATTRIBS_COUNT) - 1;
    }

    if (!shader_glsl_use_explicit_attrib_location(gl_info))
    {
        /* Bind vertex attributes to a corresponding index number to match
         * the same index numbers as ARB_vertex_programs (makes loading
         * vertex attributes simpler). With this method, we can use the
         * exact same code to load the attributes later for both ARB and
         * GLSL shaders.
         *
         * We have to do this here because we need to know the Program ID
         * in order to make the bindings work, and it has to be done prior
         * to linking the GLSL program. */
        tmp_name = string_buffer_get(&priv->string_buffers);
        for (i = 0; attribs_map; attribs_map >>= 1, ++i)
        {
            if (!(attribs_map & 1))
                continue;

            string_buffer_sprintf(tmp_name, "vs_in%u", i);
            GL_EXTCALL(glBindAttribLocation(program_id, i, tmp_name->buffer));
            if (vshader && vshader->reg_maps.shader_version.major >= 4)
            {
                string_buffer_sprintf(tmp_name, "vs_in_uint%u", i);
                GL_EXTCALL(glBindAttribLocation(program_id, i, tmp_name->buffer));
                string_buffer_sprintf(tmp_name, "vs_in_int%u", i);
                GL_EXTCALL(glBindAttribLocation(program_id, i, tmp_name->buffer));
            }
        }
        checkGLcall("glBindAttribLocation");

        if (!needs_legacy_glsl_syntax(gl_info))
        {
            for (i = 0; i < MAX_RENDER_TARGET_VIEWS; ++i)
            {
                string_buffer_sprintf(tmp_name, "color_out%u", i);
                GL_EXTCALL(glBindFragDataLocation(program_id, i, tmp_name->buffer));
                checkGLcall("glBindFragDataLocation");
            }
        }
        string_buffer_release(&priv->string_buffers, tmp_name);
    }

    if (hshader)
    {
        TRACE("Attaching GLSL tessellation control shader object %u to program %u.\n", hs_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, hs_id));
        checkGLcall("glAttachShader");

        list_add_head(&hshader->linked_programs, &entry->hs.shader_entry);
    }

    if (dshader)
    {
        TRACE("Attaching GLSL tessellation evaluation shader object %u to program %u.\n", ds_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, ds_id));
        checkGLcall("glAttachShader");

        list_add_head(&dshader->linked_programs, &entry->ds.shader_entry);
    }

    if (gshader)
    {
        TRACE("Attaching GLSL geometry shader object %u to program %u.\n", gs_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, gs_id));
        checkGLcall("glAttachShader");

        shader_glsl_init_transform_feedback(context_gl, priv, program_id, gshader);

        list_add_head(&gshader->linked_programs, &entry->gs.shader_entry);
    }

    /* Attach GLSL pshader */
    if (ps_id)
    {
        TRACE("Attaching GLSL shader object %u to program %u.\n", ps_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, ps_id));
        checkGLcall("glAttachShader");

        list_add_head(ps_list, &entry->ps.shader_entry);
    }

    /* Link the program */
    TRACE("Linking GLSL shader program %u.\n", program_id);
    GL_EXTCALL(glLinkProgram(program_id));
    shader_glsl_validate_link(gl_info, program_id);

    shader_glsl_init_vs_uniform_locations(gl_info, priv, program_id, &entry->vs,
            vshader ? vshader->limits->constant_float : 0);
    shader_glsl_init_ds_uniform_locations(gl_info, priv, program_id, &entry->ds);
    shader_glsl_init_gs_uniform_locations(gl_info, priv, program_id, &entry->gs);
    shader_glsl_init_ps_uniform_locations(gl_info, priv, program_id, &entry->ps,
            pshader ? pshader->limits->constant_float : 0);
    checkGLcall("find glsl program uniform locations");

    pre_rasterization_shader = gshader ? gshader : dshader ? dshader : vshader;
    if (pre_rasterization_shader && pre_rasterization_shader->reg_maps.shader_version.major >= 4)
    {
        unsigned int clip_distance_count = wined3d_popcount(pre_rasterization_shader->reg_maps.clip_distance_mask);
        entry->shader_controlled_clip_distances = 1;
        entry->clip_distance_mask = (1u << clip_distance_count) - 1;
    }

    if (needs_legacy_glsl_syntax(gl_info))
    {
        if (pshader && pshader->reg_maps.shader_version.major >= 3
                && pshader->u.ps.declared_in_count > vec4_varyings(3, gl_info))
        {
            TRACE("Shader %d needs vertex color clamping disabled.\n", program_id);
            entry->vs.vertex_color_clamp = GL_FALSE;
        }
        else
        {
            entry->vs.vertex_color_clamp = GL_FIXED_ONLY_ARB;
        }
    }
    else
    {
        /* With core profile we never change vertex_color_clamp from
         * GL_FIXED_ONLY_MODE (which is also the initial value) so we never call
         * glClampColorARB(). */
        entry->vs.vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    /* Set the shader to allow uniform loading on it */
    GL_EXTCALL(glUseProgram(program_id));
    checkGLcall("glUseProgram");

    entry->constant_update_mask = 0;
    if (vshader)
    {
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_F;
        if (vshader->reg_maps.integer_constants)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_I;
        if (vshader->reg_maps.boolean_constants)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_B;
        if (entry->vs.pos_fixup_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_POS_FIXUP;
        if (entry->vs.base_vertex_id_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_BASE_VERTEX_ID;

        shader_glsl_load_program_resources(context_gl, priv, program_id, vshader);
    }
    else
    {
        entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW
                | WINED3D_SHADER_CONST_FFP_PROJ;

        for (i = 0; i < MAX_VERTEX_BLENDS; ++i)
        {
            if (entry->vs.modelview_matrix_location[i] != -1)
            {
                entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_VERTEXBLEND;
                break;
            }
        }

        if (entry->vs.modelview_block_index != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_VERTEXBLEND;

        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (entry->vs.texture_matrix_location[i] != -1)
            {
                entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_TEXMATRIX;
                break;
            }
        }
        if (entry->vs.material_ambient_location != -1 || entry->vs.material_diffuse_location != -1
                || entry->vs.material_specular_location != -1
                || entry->vs.material_emissive_location != -1
                || entry->vs.material_shininess_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MATERIAL;
        if (entry->vs.light_ambient_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_LIGHTS;
    }
    if (entry->vs.clip_planes_location != -1)
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
    if (entry->vs.pointsize_min_location != -1)
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_POINTSIZE;

    if (hshader)
        shader_glsl_load_program_resources(context_gl, priv, program_id, hshader);

    if (dshader)
    {
        if (entry->ds.pos_fixup_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_POS_FIXUP;

        shader_glsl_load_program_resources(context_gl, priv, program_id, dshader);
    }

    if (gshader)
    {
        if (entry->gs.pos_fixup_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_POS_FIXUP;

        shader_glsl_load_program_resources(context_gl, priv, program_id, gshader);
    }

    if (ps_id)
    {
        if (pshader)
        {
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_F;
            if (pshader->reg_maps.integer_constants)
                entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_I;
            if (pshader->reg_maps.boolean_constants)
                entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_B;
            if (entry->ps.ycorrection_location != -1)
                entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_Y_CORR;

            shader_glsl_load_program_resources(context_gl, priv, program_id, pshader);
            shader_glsl_load_images(gl_info, priv, program_id, &pshader->reg_maps);
        }
        else
        {
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PS;

            shader_glsl_load_samplers(&context_gl->c, priv, program_id, NULL);
        }

        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (entry->ps.bumpenv_mat_location[i] != -1)
            {
                entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_BUMP_ENV;
                break;
            }
        }

        if (entry->ps.fog_color_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_FOG;
        if (entry->ps.alpha_test_ref_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_ALPHA_TEST;
        if (entry->ps.np2_fixup_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_NP2_FIXUP;
        if (entry->ps.color_key_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_COLOR_KEY;
    }
}

static void shader_glsl_precompile(void *shader_priv, struct wined3d_shader *shader)
{
    struct wined3d_device *device = shader->device;
    struct wined3d_context *context;

    if (shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_COMPUTE)
    {
        context = context_acquire(device, NULL, 0);
        shader_glsl_compile_compute_shader(shader_priv, wined3d_context_gl(context), shader);
        context_release(context);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_glsl_priv *priv = shader_priv;
    struct glsl_shader_prog_link *glsl_program;
    GLenum current_vertex_color_clamp;
    GLuint program_id, prev_id;

    priv->vertex_pipe->vp_enable(context, !use_vs(state));
    priv->fragment_pipe->fp_enable(context, !use_ps(state));

    prev_id = ctx_data->glsl_program ? ctx_data->glsl_program->id : 0;
    set_glsl_shader_program(context_gl, state, priv, ctx_data);
    glsl_program = ctx_data->glsl_program;

    if (glsl_program)
    {
        program_id = glsl_program->id;
        current_vertex_color_clamp = glsl_program->vs.vertex_color_clamp;
        if (glsl_program->shader_controlled_clip_distances)
            wined3d_context_gl_enable_clip_distances(context_gl, glsl_program->clip_distance_mask);
    }
    else
    {
        program_id = 0;
        current_vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    if (ctx_data->vertex_color_clamp != current_vertex_color_clamp)
    {
        ctx_data->vertex_color_clamp = current_vertex_color_clamp;
        if (gl_info->supported[ARB_COLOR_BUFFER_FLOAT])
        {
            GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, current_vertex_color_clamp));
            checkGLcall("glClampColorARB");
        }
        else
        {
            FIXME("Vertex color clamp needs to be changed, but extension not supported.\n");
        }
    }

    TRACE("Using GLSL program %u.\n", program_id);

    if (prev_id != program_id)
    {
        GL_EXTCALL(glUseProgram(program_id));
        checkGLcall("glUseProgram");

        if (glsl_program)
            context->constant_update_mask |= glsl_program->constant_update_mask;
    }

    context->shader_update_mask |= (1u << WINED3D_SHADER_TYPE_COMPUTE);
}

/* Context activation is done by the caller. */
static void shader_glsl_select_compute(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_glsl_priv *priv = shader_priv;
    GLuint program_id, prev_id;

    prev_id = ctx_data->glsl_program ? ctx_data->glsl_program->id : 0;
    set_glsl_compute_shader_program(context_gl, state, priv, ctx_data);
    program_id = ctx_data->glsl_program ? ctx_data->glsl_program->id : 0;

    TRACE("Using GLSL program %u.\n", program_id);

    if (prev_id != program_id)
    {
        GL_EXTCALL(glUseProgram(program_id));
        checkGLcall("glUseProgram");
    }

    context->shader_update_mask |= (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN);
}

/* "context" is not necessarily the currently active context. */
static void shader_glsl_invalidate_current_program(struct wined3d_context *context)
{
    struct glsl_context_data *ctx_data = context->shader_backend_data;

    ctx_data->glsl_program = NULL;
    context->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN)
            | (1u << WINED3D_SHADER_TYPE_COMPUTE);
}

/* Context activation is done by the caller. */
static void shader_glsl_disable(void *shader_priv, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_glsl_priv *priv = shader_priv;

    shader_glsl_invalidate_current_program(context);
    GL_EXTCALL(glUseProgram(0));
    checkGLcall("glUseProgram");

    priv->vertex_pipe->vp_enable(context, FALSE);
    priv->fragment_pipe->fp_enable(context, FALSE);

    if (needs_legacy_glsl_syntax(gl_info) && gl_info->supported[ARB_COLOR_BUFFER_FLOAT])
    {
        ctx_data->vertex_color_clamp = GL_FIXED_ONLY_ARB;
        GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FIXED_ONLY_ARB));
        checkGLcall("glClampColorARB");
    }
}

static void shader_glsl_invalidate_contexts_program(struct wined3d_device *device,
        const struct glsl_shader_prog_link *program)
{
    const struct glsl_context_data *ctx_data;
    struct wined3d_context *context;
    unsigned int i;

    for (i = 0; i < device->context_count; ++i)
    {
        context = device->contexts[i];
        ctx_data = context->shader_backend_data;

        if (ctx_data->glsl_program == program)
            shader_glsl_invalidate_current_program(context);
    }
}

static void shader_glsl_destroy(struct wined3d_shader *shader)
{
    struct glsl_shader_private *shader_data = shader->backend_data;
    struct wined3d_device *device = shader->device;
    struct shader_glsl_priv *priv = device->shader_priv;
    const struct wined3d_gl_info *gl_info;
    const struct list *linked_programs;
    struct wined3d_context *context;

    if (!shader_data || !shader_data->num_gl_shaders)
    {
        heap_free(shader_data);
        shader->backend_data = NULL;
        return;
    }

    context = context_acquire(device, NULL, 0);
    gl_info = wined3d_context_gl(context)->gl_info;

    TRACE("Deleting linked programs.\n");
    linked_programs = &shader->linked_programs;
    if (linked_programs->next)
    {
        struct glsl_shader_prog_link *entry, *entry2;
        UINT i;

        switch (shader->reg_maps.shader_version.type)
        {
            case WINED3D_SHADER_TYPE_PIXEL:
            {
                struct glsl_ps_compiled_shader *gl_shaders = shader_data->gl_shaders.ps;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting pixel shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.ps);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, ps.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            case WINED3D_SHADER_TYPE_VERTEX:
            {
                struct glsl_vs_compiled_shader *gl_shaders = shader_data->gl_shaders.vs;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting vertex shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.vs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, vs.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            case WINED3D_SHADER_TYPE_HULL:
            {
                struct glsl_hs_compiled_shader *gl_shaders = shader_data->gl_shaders.hs;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting hull shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.hs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, hs.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            case WINED3D_SHADER_TYPE_DOMAIN:
            {
                struct glsl_ds_compiled_shader *gl_shaders = shader_data->gl_shaders.ds;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting domain shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.ds);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, ds.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            case WINED3D_SHADER_TYPE_GEOMETRY:
            {
                struct glsl_gs_compiled_shader *gl_shaders = shader_data->gl_shaders.gs;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting geometry shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.gs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, gs.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            case WINED3D_SHADER_TYPE_COMPUTE:
            {
                struct glsl_cs_compiled_shader *gl_shaders = shader_data->gl_shaders.cs;

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting compute shader %u.\n", gl_shaders[i].id);
                    GL_EXTCALL(glDeleteShader(gl_shaders[i].id));
                    checkGLcall("glDeleteShader");
                }
                heap_free(shader_data->gl_shaders.cs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, cs.shader_entry)
                {
                    shader_glsl_invalidate_contexts_program(device, entry);
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                break;
            }

            default:
                ERR("Unhandled shader type %#x.\n", shader->reg_maps.shader_version.type);
                break;
        }
    }

    heap_free(shader->backend_data);
    shader->backend_data = NULL;

    context_release(context);
}

static int glsl_program_key_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct glsl_program_key *k = key;
    const struct glsl_shader_prog_link *prog = WINE_RB_ENTRY_VALUE(entry,
            const struct glsl_shader_prog_link, program_lookup_entry);

    if (k->vs_id > prog->vs.id) return 1;
    else if (k->vs_id < prog->vs.id) return -1;

    if (k->gs_id > prog->gs.id) return 1;
    else if (k->gs_id < prog->gs.id) return -1;

    if (k->ps_id > prog->ps.id) return 1;
    else if (k->ps_id < prog->ps.id) return -1;

    if (k->hs_id > prog->hs.id) return 1;
    else if (k->hs_id < prog->hs.id) return -1;

    if (k->ds_id > prog->ds.id) return 1;
    else if (k->ds_id < prog->ds.id) return -1;

    if (k->cs_id > prog->cs.id) return 1;
    else if (k->cs_id < prog->cs.id) return -1;

    return 0;
}

static BOOL constant_heap_init(struct constant_heap *heap, unsigned int constant_count)
{
    SIZE_T size = (constant_count + 1) * sizeof(*heap->entries)
            + constant_count * sizeof(*heap->contained)
            + constant_count * sizeof(*heap->positions);
    void *mem;

    if (!(mem = heap_alloc(size)))
    {
        ERR("Failed to allocate memory\n");
        return FALSE;
    }

    heap->entries = mem;
    heap->entries[1].version = 0;
    heap->contained = (BOOL *)(heap->entries + constant_count + 1);
    memset(heap->contained, 0, constant_count * sizeof(*heap->contained));
    heap->positions = (unsigned int *)(heap->contained + constant_count);
    heap->size = 1;

    return TRUE;
}

static void constant_heap_free(struct constant_heap *heap)
{
    heap_free(heap->entries);
}

static HRESULT shader_glsl_alloc(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
        const struct wined3d_fragment_pipe_ops *fragment_pipe)
{
    SIZE_T stack_size;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;
    struct shader_glsl_priv *priv;

    if (!(priv = heap_alloc_zero(sizeof(*priv))))
        return E_OUTOFMEMORY;

    priv->consts_ubo = gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT];
    priv->max_vs_consts_f = min(WINED3D_MAX_VS_CONSTS_F_SWVP, priv->consts_ubo
            ? gl_info->limits.glsl_max_uniform_block_size / sizeof(struct wined3d_vec4)
            : gl_info->limits.glsl_vs_float_constants);

    if (!(device->create_parms.flags & (WINED3DCREATE_SOFTWARE_VERTEXPROCESSING | WINED3DCREATE_MIXED_VERTEXPROCESSING)))
        priv->max_vs_consts_f = min(priv->max_vs_consts_f, WINED3D_MAX_VS_CONSTS_F);

    stack_size = priv->consts_ubo
            ? wined3d_log2i(WINED3D_MAX_PS_CONSTS_F) + 1
            : wined3d_log2i(max(priv->max_vs_consts_f, WINED3D_MAX_PS_CONSTS_F)) + 1;
    TRACE("consts_ubo %#x, max_vs_consts_f %u.\n", priv->consts_ubo, priv->max_vs_consts_f);

    string_buffer_list_init(&priv->string_buffers);

    if (!(vertex_priv = vertex_pipe->vp_alloc(&glsl_shader_backend, priv)))
    {
        ERR("Failed to initialize vertex pipe.\n");
        heap_free(priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&glsl_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        vertex_pipe->vp_free(device, NULL);
        heap_free(priv);
        return E_FAIL;
    }

    if (!string_buffer_init(&priv->shader_buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        goto fail;
    }

    if (!(priv->stack = heap_calloc(stack_size, sizeof(*priv->stack))))
    {
        ERR("Failed to allocate memory.\n");
        goto fail;
    }

    if (!priv->consts_ubo && !constant_heap_init(&priv->vconst_heap, priv->max_vs_consts_f))
    {
        ERR("Failed to initialize vertex shader constant heap\n");
        goto fail;
    }

    if (!constant_heap_init(&priv->pconst_heap, WINED3D_MAX_PS_CONSTS_F))
    {
        ERR("Failed to initialize pixel shader constant heap\n");
        goto fail;
    }

    wine_rb_init(&priv->program_lookup, glsl_program_key_compare);

    priv->next_constant_version = 1;
    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(device->adapter, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;
    priv->legacy_lighting = device->wined3d->flags & WINED3D_LEGACY_FFP_LIGHTING;
    priv->ubo_modelview = -1; /* To be initialized on first usage. */
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        priv->modelview_buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv->modelview_buffer)
                * MAX_VERTEX_BLEND_UBO);
        if (!priv->modelview_buffer)
        {
            ERR("Failed to alloacte modelview buffer.\n");
            goto fail;
        }
    }
    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    priv->ubo_vs_c = -1;

    return WINED3D_OK;

fail:
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    heap_free(priv->stack);
    string_buffer_free(&priv->shader_buffer);
    fragment_pipe->free_private(device, NULL);
    vertex_pipe->vp_free(device, NULL);
    heap_free(priv);
    return E_OUTOFMEMORY;
}

/* Context activation is done by the caller. */
static void shader_glsl_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct shader_glsl_priv *priv = device->shader_priv;

    wine_rb_destroy(&priv->program_lookup, NULL, NULL);
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    heap_free(priv->stack);
    string_buffer_list_cleanup(&priv->string_buffers);
    string_buffer_free(&priv->shader_buffer);
    priv->fragment_pipe->free_private(device, context);
    priv->vertex_pipe->vp_free(device, context);
    if (priv->ubo_modelview != -1)
    {
        const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
        GL_EXTCALL(glDeleteBuffers(1, &priv->ubo_modelview));
        checkGLcall("glDeleteBuffers");
        priv->ubo_modelview = -1;
    }
    HeapFree(GetProcessHeap(), 0, priv->modelview_buffer);

    if (priv->ubo_vs_c != -1)
    {
        const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
        GL_EXTCALL(glDeleteBuffers(1, &priv->ubo_vs_c));
        checkGLcall("glDeleteBuffers");
        priv->ubo_vs_c = -1;
    }
    heap_free(device->shader_priv);
    device->shader_priv = NULL;
}

static BOOL shader_glsl_allocate_context_data(struct wined3d_context *context)
{
    struct glsl_context_data *ctx_data;

    if (!(ctx_data = heap_alloc_zero(sizeof(*ctx_data))))
        return FALSE;
    ctx_data->vertex_color_clamp = GL_FIXED_ONLY_ARB;
    context->shader_backend_data = ctx_data;
    return TRUE;
}

static void shader_glsl_free_context_data(struct wined3d_context *context)
{
    heap_free(context->shader_backend_data);
}

static void shader_glsl_init_context_state(struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    gl_info->gl_ops.gl.p_glEnable(GL_PROGRAM_POINT_SIZE);
    checkGLcall("GL_PROGRAM_POINT_SIZE");
}

static unsigned int shader_glsl_get_shader_model(const struct wined3d_gl_info *gl_info)
{
    BOOL shader_model_4 = gl_info->glsl_version >= MAKEDWORD_VERSION(1, 50)
            && gl_info->supported[ARB_SHADER_BIT_ENCODING]
            && gl_info->supported[ARB_TEXTURE_SWIZZLE];

    if (shader_model_4
            && gl_info->supported[ARB_COMPUTE_SHADER]
            && gl_info->supported[ARB_CULL_DISTANCE]
            && gl_info->supported[ARB_DERIVATIVE_CONTROL]
            && gl_info->supported[ARB_GPU_SHADER5]
            && gl_info->supported[ARB_SHADER_ATOMIC_COUNTERS]
            && gl_info->supported[ARB_SHADER_IMAGE_LOAD_STORE]
            && gl_info->supported[ARB_SHADER_IMAGE_SIZE]
            && gl_info->supported[ARB_SHADING_LANGUAGE_PACKING]
            && gl_info->supported[ARB_TESSELLATION_SHADER]
            && gl_info->supported[ARB_TEXTURE_GATHER]
            && gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
        return 5;

    if (shader_model_4)
        return 4;

    /* Support for texldd and texldl instructions in pixel shaders is required
     * for SM3. */
    if (shader_glsl_has_core_grad(gl_info) || gl_info->supported[ARB_SHADER_TEXTURE_LOD])
        return 3;

    return 2;
}

static void shader_glsl_get_caps(const struct wined3d_adapter *adapter, struct shader_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    unsigned int shader_model = shader_glsl_get_shader_model(gl_info);

    TRACE("Shader model %u.\n", shader_model);

    caps->vs_version = min(wined3d_settings.max_sm_vs, shader_model);
    caps->hs_version = min(wined3d_settings.max_sm_hs, shader_model);
    caps->ds_version = min(wined3d_settings.max_sm_ds, shader_model);
    caps->gs_version = min(wined3d_settings.max_sm_gs, shader_model);
    caps->ps_version = min(wined3d_settings.max_sm_ps, shader_model);
    caps->cs_version = min(wined3d_settings.max_sm_cs, shader_model);

    caps->vs_version = gl_info->supported[ARB_VERTEX_SHADER] ? caps->vs_version : 0;
    caps->ps_version = gl_info->supported[ARB_FRAGMENT_SHADER] ? caps->ps_version : 0;

    caps->vs_uniform_count = min(WINED3D_MAX_VS_CONSTS_F_SWVP,
            gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT]
            ? gl_info->limits.glsl_max_uniform_block_size / sizeof(struct wined3d_vec4)
            : gl_info->limits.glsl_vs_float_constants);
    caps->ps_uniform_count = min(WINED3D_MAX_PS_CONSTS_F, gl_info->limits.glsl_ps_float_constants);
    caps->varying_count = gl_info->limits.glsl_varyings;

    /* FIXME: The following line is card dependent. -8.0 to 8.0 is the
     * Direct3D minimum requirement.
     *
     * Both GL_ARB_fragment_program and GLSL require a "maximum representable magnitude"
     * of colors to be 2^10, and 2^32 for other floats. Should we use 1024 here?
     *
     * The problem is that the refrast clamps temporary results in the shader to
     * [-MaxValue;+MaxValue]. If the card's max value is bigger than the one we advertize here,
     * then applications may miss the clamping behavior. On the other hand, if it is smaller,
     * the shader will generate incorrect results too. Unfortunately, GL deliberately doesn't
     * offer a way to query this.
     */
    if (shader_model >= 4)
        caps->ps_1x_max_value = FLT_MAX;
    else
        caps->ps_1x_max_value = 1024.0f;

    /* Ideally we'd only set caps like sRGB writes here if supported by both
     * the shader backend and the fragment pipe, but we can get called before
     * shader_glsl_alloc(). */
    caps->wined3d_caps = WINED3D_SHADER_CAP_VS_CLIPPING
            | WINED3D_SHADER_CAP_SRGB_WRITE;
    if (needs_interpolation_qualifiers_for_shader_outputs(gl_info))
        caps->wined3d_caps |= WINED3D_SHADER_CAP_OUTPUT_INTERPOLATION;
    if (shader_glsl_full_ffp_varyings(gl_info))
        caps->wined3d_caps |= WINED3D_SHADER_CAP_FULL_FFP_VARYINGS;
}

static BOOL shader_glsl_color_fixup_supported(struct color_fixup_desc fixup)
{
    /* We support everything except YUV conversions. */
    return !is_complex_fixup(fixup);
}

static const SHADER_HANDLER shader_glsl_instruction_handler_table[WINED3DSIH_TABLE_SIZE] =
{
    /* WINED3DSIH_ABS                              */ shader_glsl_map2gl,
    /* WINED3DSIH_ADD                              */ shader_glsl_binop,
    /* WINED3DSIH_AND                              */ shader_glsl_binop,
    /* WINED3DSIH_ATOMIC_AND                       */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_CMP_STORE                 */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_IADD                      */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_IMAX                      */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_IMIN                      */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_OR                        */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_UMAX                      */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_UMIN                      */ shader_glsl_atomic,
    /* WINED3DSIH_ATOMIC_XOR                       */ shader_glsl_atomic,
    /* WINED3DSIH_BEM                              */ shader_glsl_bem,
    /* WINED3DSIH_BFI                              */ shader_glsl_bitwise_op,
    /* WINED3DSIH_BFREV                            */ shader_glsl_map2gl,
    /* WINED3DSIH_BREAK                            */ shader_glsl_break,
    /* WINED3DSIH_BREAKC                           */ shader_glsl_breakc,
    /* WINED3DSIH_BREAKP                           */ shader_glsl_conditional_op,
    /* WINED3DSIH_BUFINFO                          */ shader_glsl_bufinfo,
    /* WINED3DSIH_CALL                             */ shader_glsl_call,
    /* WINED3DSIH_CALLNZ                           */ shader_glsl_callnz,
    /* WINED3DSIH_CASE                             */ shader_glsl_case,
    /* WINED3DSIH_CMP                              */ shader_glsl_conditional_move,
    /* WINED3DSIH_CND                              */ shader_glsl_cnd,
    /* WINED3DSIH_CONTINUE                         */ shader_glsl_continue,
    /* WINED3DSIH_CONTINUEP                        */ shader_glsl_conditional_op,
    /* WINED3DSIH_COUNTBITS                        */ shader_glsl_map2gl,
    /* WINED3DSIH_CRS                              */ shader_glsl_cross,
    /* WINED3DSIH_CUT                              */ shader_glsl_cut,
    /* WINED3DSIH_CUT_STREAM                       */ shader_glsl_cut,
    /* WINED3DSIH_DCL                              */ shader_glsl_nop,
    /* WINED3DSIH_DCL_CONSTANT_BUFFER              */ shader_glsl_nop,
    /* WINED3DSIH_DCL_FUNCTION_BODY                */ NULL,
    /* WINED3DSIH_DCL_FUNCTION_TABLE               */ NULL,
    /* WINED3DSIH_DCL_GLOBAL_FLAGS                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_GS_INSTANCES                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT */ shader_glsl_nop,
    /* WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT */ shader_glsl_nop,
    /* WINED3DSIH_DCL_HS_MAX_TESSFACTOR            */ shader_glsl_nop,
    /* WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER    */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INDEX_RANGE                  */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INDEXABLE_TEMP               */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT                        */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT    */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_PRIMITIVE              */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_PS                     */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_PS_SGV                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_PS_SIV                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_SGV                    */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_SIV                    */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INTERFACE                    */ NULL,
    /* WINED3DSIH_DCL_OUTPUT                       */ shader_glsl_nop,
    /* WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_OUTPUT_SIV                   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_OUTPUT_TOPOLOGY              */ shader_glsl_nop,
    /* WINED3DSIH_DCL_RESOURCE_RAW                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_RESOURCE_STRUCTURED          */ shader_glsl_nop,
    /* WINED3DSIH_DCL_SAMPLER                      */ shader_glsl_nop,
    /* WINED3DSIH_DCL_STREAM                       */ NULL,
    /* WINED3DSIH_DCL_TEMPS                        */ shader_glsl_nop,
    /* WINED3DSIH_DCL_TESSELLATOR_DOMAIN           */ shader_glsl_nop,
    /* WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE */ shader_glsl_nop,
    /* WINED3DSIH_DCL_TESSELLATOR_PARTITIONING     */ shader_glsl_nop,
    /* WINED3DSIH_DCL_TGSM_RAW                     */ shader_glsl_nop,
    /* WINED3DSIH_DCL_TGSM_STRUCTURED              */ shader_glsl_nop,
    /* WINED3DSIH_DCL_THREAD_GROUP                 */ shader_glsl_nop,
    /* WINED3DSIH_DCL_UAV_RAW                      */ shader_glsl_nop,
    /* WINED3DSIH_DCL_UAV_STRUCTURED               */ shader_glsl_nop,
    /* WINED3DSIH_DCL_UAV_TYPED                    */ shader_glsl_nop,
    /* WINED3DSIH_DCL_VERTICES_OUT                 */ shader_glsl_nop,
    /* WINED3DSIH_DEF                              */ shader_glsl_nop,
    /* WINED3DSIH_DEFAULT                          */ shader_glsl_default,
    /* WINED3DSIH_DEFB                             */ shader_glsl_nop,
    /* WINED3DSIH_DEFI                             */ shader_glsl_nop,
    /* WINED3DSIH_DIV                              */ shader_glsl_binop,
    /* WINED3DSIH_DP2                              */ shader_glsl_dot,
    /* WINED3DSIH_DP2ADD                           */ shader_glsl_dp2add,
    /* WINED3DSIH_DP3                              */ shader_glsl_dot,
    /* WINED3DSIH_DP4                              */ shader_glsl_dot,
    /* WINED3DSIH_DST                              */ shader_glsl_dst,
    /* WINED3DSIH_DSX                              */ shader_glsl_map2gl,
    /* WINED3DSIH_DSX_COARSE                       */ shader_glsl_map2gl,
    /* WINED3DSIH_DSX_FINE                         */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY                              */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY_COARSE                       */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY_FINE                         */ shader_glsl_map2gl,
    /* WINED3DSIH_ELSE                             */ shader_glsl_else,
    /* WINED3DSIH_EMIT                             */ shader_glsl_emit,
    /* WINED3DSIH_EMIT_STREAM                      */ shader_glsl_emit,
    /* WINED3DSIH_ENDIF                            */ shader_glsl_end,
    /* WINED3DSIH_ENDLOOP                          */ shader_glsl_end,
    /* WINED3DSIH_ENDREP                           */ shader_glsl_end,
    /* WINED3DSIH_ENDSWITCH                        */ shader_glsl_end,
    /* WINED3DSIH_EQ                               */ shader_glsl_relop,
    /* WINED3DSIH_EVAL_SAMPLE_INDEX                */ shader_glsl_interpolate,
    /* WINED3DSIH_EXP                              */ shader_glsl_scalar_op,
    /* WINED3DSIH_EXPP                             */ shader_glsl_expp,
    /* WINED3DSIH_F16TOF32                         */ shader_glsl_float16,
    /* WINED3DSIH_F32TOF16                         */ shader_glsl_float16,
    /* WINED3DSIH_FCALL                            */ NULL,
    /* WINED3DSIH_FIRSTBIT_HI                      */ shader_glsl_map2gl,
    /* WINED3DSIH_FIRSTBIT_LO                      */ shader_glsl_map2gl,
    /* WINED3DSIH_FIRSTBIT_SHI                     */ shader_glsl_map2gl,
    /* WINED3DSIH_FRC                              */ shader_glsl_map2gl,
    /* WINED3DSIH_FTOI                             */ shader_glsl_to_int,
    /* WINED3DSIH_FTOU                             */ shader_glsl_to_uint,
    /* WINED3DSIH_GATHER4                          */ shader_glsl_gather4,
    /* WINED3DSIH_GATHER4_C                        */ shader_glsl_gather4,
    /* WINED3DSIH_GATHER4_PO                       */ shader_glsl_gather4,
    /* WINED3DSIH_GATHER4_PO_C                     */ shader_glsl_gather4,
    /* WINED3DSIH_GE                               */ shader_glsl_relop,
    /* WINED3DSIH_HS_CONTROL_POINT_PHASE           */ shader_glsl_nop,
    /* WINED3DSIH_HS_DECLS                         */ shader_glsl_nop,
    /* WINED3DSIH_HS_FORK_PHASE                    */ shader_glsl_nop,
    /* WINED3DSIH_HS_JOIN_PHASE                    */ shader_glsl_nop,
    /* WINED3DSIH_IADD                             */ shader_glsl_binop,
    /* WINED3DSIH_IBFE                             */ shader_glsl_bitwise_op,
    /* WINED3DSIH_IEQ                              */ shader_glsl_relop,
    /* WINED3DSIH_IF                               */ shader_glsl_if,
    /* WINED3DSIH_IFC                              */ shader_glsl_ifc,
    /* WINED3DSIH_IGE                              */ shader_glsl_relop,
    /* WINED3DSIH_ILT                              */ shader_glsl_relop,
    /* WINED3DSIH_IMAD                             */ shader_glsl_mad,
    /* WINED3DSIH_IMAX                             */ shader_glsl_map2gl,
    /* WINED3DSIH_IMIN                             */ shader_glsl_map2gl,
    /* WINED3DSIH_IMM_ATOMIC_ALLOC                 */ shader_glsl_uav_counter,
    /* WINED3DSIH_IMM_ATOMIC_AND                   */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_CMP_EXCH              */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_CONSUME               */ shader_glsl_uav_counter,
    /* WINED3DSIH_IMM_ATOMIC_EXCH                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_IADD                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_IMAX                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_IMIN                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_OR                    */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_UMAX                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_UMIN                  */ shader_glsl_atomic,
    /* WINED3DSIH_IMM_ATOMIC_XOR                   */ shader_glsl_atomic,
    /* WINED3DSIH_IMUL                             */ shader_glsl_mul_extended,
    /* WINED3DSIH_INE                              */ shader_glsl_relop,
    /* WINED3DSIH_INEG                             */ shader_glsl_unary_op,
    /* WINED3DSIH_ISHL                             */ shader_glsl_binop,
    /* WINED3DSIH_ISHR                             */ shader_glsl_binop,
    /* WINED3DSIH_ITOF                             */ shader_glsl_to_float,
    /* WINED3DSIH_LABEL                            */ shader_glsl_label,
    /* WINED3DSIH_LD                               */ shader_glsl_ld,
    /* WINED3DSIH_LD2DMS                           */ shader_glsl_ld,
    /* WINED3DSIH_LD_RAW                           */ shader_glsl_ld_raw_structured,
    /* WINED3DSIH_LD_STRUCTURED                    */ shader_glsl_ld_raw_structured,
    /* WINED3DSIH_LD_UAV_TYPED                     */ shader_glsl_ld_uav,
    /* WINED3DSIH_LIT                              */ shader_glsl_lit,
    /* WINED3DSIH_LOD                              */ NULL,
    /* WINED3DSIH_LOG                              */ shader_glsl_scalar_op,
    /* WINED3DSIH_LOGP                             */ shader_glsl_scalar_op,
    /* WINED3DSIH_LOOP                             */ shader_glsl_loop,
    /* WINED3DSIH_LRP                              */ shader_glsl_lrp,
    /* WINED3DSIH_LT                               */ shader_glsl_relop,
    /* WINED3DSIH_M3x2                             */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x3                             */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x4                             */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x3                             */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x4                             */ shader_glsl_mnxn,
    /* WINED3DSIH_MAD                              */ shader_glsl_mad,
    /* WINED3DSIH_MAX                              */ shader_glsl_map2gl,
    /* WINED3DSIH_MIN                              */ shader_glsl_map2gl,
    /* WINED3DSIH_MOV                              */ shader_glsl_mov,
    /* WINED3DSIH_MOVA                             */ shader_glsl_mov,
    /* WINED3DSIH_MOVC                             */ shader_glsl_conditional_move,
    /* WINED3DSIH_MUL                              */ shader_glsl_binop,
    /* WINED3DSIH_NE                               */ shader_glsl_relop,
    /* WINED3DSIH_NOP                              */ shader_glsl_nop,
    /* WINED3DSIH_NOT                              */ shader_glsl_unary_op,
    /* WINED3DSIH_NRM                              */ shader_glsl_nrm,
    /* WINED3DSIH_OR                               */ shader_glsl_binop,
    /* WINED3DSIH_PHASE                            */ shader_glsl_nop,
    /* WINED3DSIH_POW                              */ shader_glsl_pow,
    /* WINED3DSIH_RCP                              */ shader_glsl_scalar_op,
    /* WINED3DSIH_REP                              */ shader_glsl_rep,
    /* WINED3DSIH_RESINFO                          */ shader_glsl_resinfo,
    /* WINED3DSIH_RET                              */ shader_glsl_ret,
    /* WINED3DSIH_RETP                             */ shader_glsl_conditional_op,
    /* WINED3DSIH_ROUND_NE                         */ shader_glsl_map2gl,
    /* WINED3DSIH_ROUND_NI                         */ shader_glsl_map2gl,
    /* WINED3DSIH_ROUND_PI                         */ shader_glsl_map2gl,
    /* WINED3DSIH_ROUND_Z                          */ shader_glsl_map2gl,
    /* WINED3DSIH_RSQ                              */ shader_glsl_scalar_op,
    /* WINED3DSIH_SAMPLE                           */ shader_glsl_sample,
    /* WINED3DSIH_SAMPLE_B                         */ shader_glsl_sample,
    /* WINED3DSIH_SAMPLE_C                         */ shader_glsl_sample_c,
    /* WINED3DSIH_SAMPLE_C_LZ                      */ shader_glsl_sample_c,
    /* WINED3DSIH_SAMPLE_GRAD                      */ shader_glsl_sample,
    /* WINED3DSIH_SAMPLE_INFO                      */ shader_glsl_sample_info,
    /* WINED3DSIH_SAMPLE_LOD                       */ shader_glsl_sample,
    /* WINED3DSIH_SAMPLE_POS                       */ NULL,
    /* WINED3DSIH_SETP                             */ NULL,
    /* WINED3DSIH_SGE                              */ shader_glsl_compare,
    /* WINED3DSIH_SGN                              */ shader_glsl_sgn,
    /* WINED3DSIH_SINCOS                           */ shader_glsl_sincos,
    /* WINED3DSIH_SLT                              */ shader_glsl_compare,
    /* WINED3DSIH_SQRT                             */ shader_glsl_map2gl,
    /* WINED3DSIH_STORE_RAW                        */ shader_glsl_store_raw_structured,
    /* WINED3DSIH_STORE_STRUCTURED                 */ shader_glsl_store_raw_structured,
    /* WINED3DSIH_STORE_UAV_TYPED                  */ shader_glsl_store_uav,
    /* WINED3DSIH_SUB                              */ shader_glsl_binop,
    /* WINED3DSIH_SWAPC                            */ shader_glsl_swapc,
    /* WINED3DSIH_SWITCH                           */ shader_glsl_switch,
    /* WINED3DSIH_SYNC                             */ shader_glsl_sync,
    /* WINED3DSIH_TEX                              */ shader_glsl_tex,
    /* WINED3DSIH_TEXBEM                           */ shader_glsl_texbem,
    /* WINED3DSIH_TEXBEML                          */ shader_glsl_texbem,
    /* WINED3DSIH_TEXCOORD                         */ shader_glsl_texcoord,
    /* WINED3DSIH_TEXDEPTH                         */ shader_glsl_texdepth,
    /* WINED3DSIH_TEXDP3                           */ shader_glsl_texdp3,
    /* WINED3DSIH_TEXDP3TEX                        */ shader_glsl_texdp3tex,
    /* WINED3DSIH_TEXKILL                          */ shader_glsl_texkill,
    /* WINED3DSIH_TEXLDD                           */ shader_glsl_texldd,
    /* WINED3DSIH_TEXLDL                           */ shader_glsl_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH                     */ shader_glsl_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD                       */ shader_glsl_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX                       */ shader_glsl_texm3x2tex,
    /* WINED3DSIH_TEXM3x3                          */ shader_glsl_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF                      */ NULL,
    /* WINED3DSIH_TEXM3x3PAD                       */ shader_glsl_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC                      */ shader_glsl_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX                       */ shader_glsl_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC                     */ shader_glsl_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR                        */ shader_glsl_texreg2ar,
    /* WINED3DSIH_TEXREG2GB                        */ shader_glsl_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB                       */ shader_glsl_texreg2rgb,
    /* WINED3DSIH_UBFE                             */ shader_glsl_bitwise_op,
    /* WINED3DSIH_UDIV                             */ shader_glsl_udiv,
    /* WINED3DSIH_UGE                              */ shader_glsl_relop,
    /* WINED3DSIH_ULT                              */ shader_glsl_relop,
    /* WINED3DSIH_UMAX                             */ shader_glsl_map2gl,
    /* WINED3DSIH_UMIN                             */ shader_glsl_map2gl,
    /* WINED3DSIH_UMUL                             */ shader_glsl_mul_extended,
    /* WINED3DSIH_USHR                             */ shader_glsl_binop,
    /* WINED3DSIH_UTOF                             */ shader_glsl_to_float,
    /* WINED3DSIH_XOR                              */ shader_glsl_binop,
};

static void shader_glsl_handle_instruction(const struct wined3d_shader_instruction *ins) {
    SHADER_HANDLER hw_fct;

    /* Select handler */
    hw_fct = shader_glsl_instruction_handler_table[ins->handler_idx];

    /* Unhandled opcode */
    if (!hw_fct)
    {
        FIXME("Backend can't handle opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
        return;
    }
    hw_fct(ins);

    shader_glsl_add_instruction_modifiers(ins);
}

static BOOL shader_glsl_has_ffp_proj_control(void *shader_priv)
{
    struct shader_glsl_priv *priv = shader_priv;

    return priv->ffp_proj_control;
}

const struct wined3d_shader_backend_ops glsl_shader_backend =
{
    shader_glsl_handle_instruction,
    shader_glsl_precompile,
    shader_glsl_select,
    shader_glsl_select_compute,
    shader_glsl_disable,
    shader_glsl_update_float_vertex_constants,
    shader_glsl_update_float_pixel_constants,
    shader_glsl_load_constants,
    shader_glsl_destroy,
    shader_glsl_alloc,
    shader_glsl_free,
    shader_glsl_allocate_context_data,
    shader_glsl_free_context_data,
    shader_glsl_init_context_state,
    shader_glsl_get_caps,
    shader_glsl_color_fixup_supported,
    shader_glsl_has_ffp_proj_control,
};

static void glsl_vertex_pipe_vp_enable(const struct wined3d_context *context, BOOL enable) {}

static void glsl_vertex_pipe_vp_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->xyzrhw = TRUE;
    caps->emulated_flatshading = !needs_legacy_glsl_syntax(gl_info);
    caps->ffp_generic_attributes = TRUE;
    caps->max_active_lights = WINED3D_MAX_ACTIVE_LIGHTS;
    caps->max_vertex_blend_matrices = MAX_VERTEX_BLENDS;
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        caps->max_vertex_blend_matrix_index = MAX_VERTEX_BLEND_UBO - 1;
    else
        caps->max_vertex_blend_matrix_index = MAX_VERTEX_BLENDS - 1;

    caps->vertex_processing_caps = WINED3DVTXPCAPS_TEXGEN
            | WINED3DVTXPCAPS_MATERIALSOURCE7
            | WINED3DVTXPCAPS_VERTEXFOG
            | WINED3DVTXPCAPS_DIRECTIONALLIGHTS
            | WINED3DVTXPCAPS_POSITIONALLIGHTS
            | WINED3DVTXPCAPS_LOCALVIEWER
            | WINED3DVTXPCAPS_TEXGEN_SPHEREMAP;
    caps->fvf_caps = WINED3DFVFCAPS_PSIZE | 8; /* 8 texture coordinates. */
    caps->max_user_clip_planes = gl_info->limits.user_clip_distances;
    caps->raster_caps = WINED3DPRASTERCAPS_FOGRANGE;
}

static DWORD glsl_vertex_pipe_vp_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        return GL_EXT_EMUL_ARB_MULTITEXTURE;
    return 0;
}

static void *glsl_vertex_pipe_vp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    struct shader_glsl_priv *priv;

    if (shader_backend == &glsl_shader_backend)
    {
        priv = shader_priv;
        wine_rb_init(&priv->ffp_vertex_shaders, wined3d_ffp_vertex_program_key_compare);
        return priv;
    }

    FIXME("GLSL vertex pipe without GLSL shader backend not implemented.\n");

    return NULL;
}

static void shader_glsl_free_ffp_vertex_shader(struct wine_rb_entry *entry, void *param)
{
    struct glsl_ffp_vertex_shader *shader = WINE_RB_ENTRY_VALUE(entry,
            struct glsl_ffp_vertex_shader, desc.entry);
    struct glsl_shader_prog_link *program, *program2;
    struct glsl_ffp_destroy_ctx *ctx = param;
    const struct wined3d_gl_info *gl_info;

    gl_info = ctx->context_gl->gl_info;
    LIST_FOR_EACH_ENTRY_SAFE(program, program2, &shader->linked_programs,
            struct glsl_shader_prog_link, vs.shader_entry)
    {
        delete_glsl_program_entry(ctx->priv, gl_info, program);
    }
    GL_EXTCALL(glDeleteShader(shader->id));
    heap_free(shader);
}

/* Context activation is done by the caller. */
static void glsl_vertex_pipe_vp_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct shader_glsl_priv *priv = device->vertex_priv;
    struct glsl_ffp_destroy_ctx ctx;

    ctx.priv = priv;
    ctx.context_gl = context_gl;
    wine_rb_destroy(&priv->ffp_vertex_shaders, shader_glsl_free_ffp_vertex_shader, &ctx);
}

static void glsl_vertex_pipe_nop(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) {}

static void glsl_vertex_pipe_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_vdecl(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    BOOL specular = !!(context->stream_info.use_map & (1u << WINED3D_FFP_SPECULAR));
    BOOL diffuse = !!(context->stream_info.use_map & (1u << WINED3D_FFP_DIFFUSE));
    BOOL normal = !!(context->stream_info.use_map & (1u << WINED3D_FFP_NORMAL));
    const BOOL legacy_clip_planes = needs_legacy_glsl_syntax(gl_info);
    BOOL transformed = context->stream_info.position_transformed;
    BOOL wasrhw = context->last_was_rhw;
    unsigned int i;

    context->last_was_rhw = transformed;

    /* If the vertex declaration contains a transformed position attribute,
     * the draw uses the fixed function vertex pipeline regardless of any
     * vertex shader set by the application. */
    if (transformed != wasrhw
            || context->stream_info.swizzle_map != context->last_swizzle_map)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;

    context->last_swizzle_map = context->stream_info.swizzle_map;

    if (!use_vs(state))
    {
        if (context->last_was_vshader)
        {
            if (legacy_clip_planes)
                for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
                    clipplane(context, state, STATE_CLIPPLANE(i));
            else
                context->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
        }

        context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_TEXMATRIX;

        /* Because of settings->texcoords, we have to regenerate the vertex
         * shader on a vdecl change if there aren't enough varyings to just
         * always output all the texture coordinates.
         *
         * Likewise, we have to invalidate the shader when using per-vertex
         * colours and diffuse/specular attribute presence changes, or when
         * normal presence changes. */
        if (!shader_glsl_full_ffp_varyings(gl_info) || (state->render_states[WINED3D_RS_COLORVERTEX]
                && (diffuse != context->last_was_diffuse || specular != context->last_was_specular))
                || normal != context->last_was_normal)
            context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;

        if (use_ps(state)
                && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.major == 1
                && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.minor <= 3)
            context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
    }
    else
    {
        if (!context->last_was_vshader)
        {
            /* Vertex shader clipping ignores the view matrix. Update all clip planes. */
            if (legacy_clip_planes)
                for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
                    clipplane(context, state, STATE_CLIPPLANE(i));
            else
                context->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
        }
    }

    context->last_was_vshader = use_vs(state);
    context->last_was_diffuse = diffuse;
    context->last_was_specular = specular;
    context->last_was_normal = normal;
}

static void glsl_vertex_pipe_vs(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
    /* Different vertex shaders potentially require a different vertex attributes setup. */
    if (!isStateDirty(context, STATE_VDECL))
        context_apply_state(context, state, STATE_VDECL);
}

static void glsl_vertex_pipe_hs(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    /* In Direct3D tessellator options (e.g. output primitive type, primitive
     * winding) are defined in Hull Shaders, while in GLSL those are
     * specified in Tessellation Evaluation Shaders. */
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_DOMAIN;

    if (state->shader[WINED3D_SHADER_TYPE_VERTEX])
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_geometry_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    BOOL rasterization_disabled;

    rasterization_disabled = is_rasterization_disabled(state->shader[WINED3D_SHADER_TYPE_GEOMETRY]);
    if (ctx_data->rasterization_disabled != rasterization_disabled)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
    ctx_data->rasterization_disabled = rasterization_disabled;

    if (state->shader[WINED3D_SHADER_TYPE_DOMAIN])
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_DOMAIN;
    else if (state->shader[WINED3D_SHADER_TYPE_VERTEX]
            && state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.shader_version.major >= 4)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_pixel_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    if (state->shader[WINED3D_SHADER_TYPE_GEOMETRY])
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_GEOMETRY;
    else if (state->shader[WINED3D_SHADER_TYPE_DOMAIN])
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_DOMAIN;
    else if (state->shader[WINED3D_SHADER_TYPE_VERTEX]
            && state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.shader_version.major >= 4)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_world(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW
            | WINED3D_SHADER_CONST_FFP_VERTEXBLEND;
}

static void glsl_vertex_pipe_vertexblend(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_VERTEXBLEND;
}

static void glsl_vertex_pipe_view(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int k;

    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW
            | WINED3D_SHADER_CONST_FFP_LIGHTS
            | WINED3D_SHADER_CONST_FFP_VERTEXBLEND;

    if (needs_legacy_glsl_syntax(gl_info))
    {
        for (k = 0; k < gl_info->limits.user_clip_distances; ++k)
        {
            if (!isStateDirty(context, STATE_CLIPPLANE(k)))
                clipplane(context, state, STATE_CLIPPLANE(k));
        }
    }
    else
    {
        context->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
    }
}

static void glsl_vertex_pipe_projection(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    /* Table fog behavior depends on the projection matrix. */
    if (state->render_states[WINED3D_RS_FOGENABLE]
            && state->render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PROJ;
}

static void glsl_vertex_pipe_viewport(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION)))
        glsl_vertex_pipe_projection(context, state, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_POINTSCALEENABLE))
            && state->render_states[WINED3D_RS_POINTSCALEENABLE])
        context->constant_update_mask |= WINED3D_SHADER_CONST_VS_POINTSIZE;
    context->constant_update_mask |= WINED3D_SHADER_CONST_POS_FIXUP;
}

static void glsl_vertex_pipe_texmatrix(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_TEXMATRIX;
}

static void glsl_vertex_pipe_texmatrix_np2(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    DWORD sampler = state_id - STATE_SAMPLER(0);
    const struct wined3d_texture *texture = state->textures[sampler];
    BOOL np2;

    if (!texture)
        return;

    if (sampler >= WINED3D_MAX_TEXTURES)
        return;

    if ((np2 = !(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
            || context->lastWasPow2Texture & (1u << sampler))
    {
        if (np2)
            context->lastWasPow2Texture |= 1u << sampler;
        else
            context->lastWasPow2Texture &= ~(1u << sampler);

        context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_TEXMATRIX;
    }
}

static void glsl_vertex_pipe_material(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MATERIAL;
}

static void glsl_vertex_pipe_light(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_LIGHTS;
}

static void glsl_vertex_pipe_pointsize(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_VS_POINTSIZE;
}

static void glsl_vertex_pipe_pointscale(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    if (!use_vs(state))
        context->constant_update_mask |= WINED3D_SHADER_CONST_VS_POINTSIZE;
}

static void glsl_vertex_pointsprite_core(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    static unsigned int once;

    if (state->gl_primitive_type == GL_POINTS && !state->render_states[WINED3D_RS_POINTSPRITEENABLE] && !once++)
        FIXME("Non-point sprite points not supported in core profile.\n");
}

static void glsl_vertex_pipe_shademode(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_clip_plane(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    UINT index = state_id - STATE_CLIPPLANE(0);

    if (index >= gl_info->limits.user_clip_distances)
        return;

    context->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
}

static const struct wined3d_state_entry_template glsl_vertex_pipe_vp_states[] =
{
    {STATE_VDECL,                                                {STATE_VDECL,                                                glsl_vertex_pipe_vdecl }, WINED3D_GL_EXT_NONE          },
    {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   glsl_vertex_pipe_vs    }, WINED3D_GL_EXT_NONE          },
    {STATE_SHADER(WINED3D_SHADER_TYPE_HULL),                     {STATE_SHADER(WINED3D_SHADER_TYPE_HULL),                     glsl_vertex_pipe_hs    }, WINED3D_GL_EXT_NONE          },
    {STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY),                 {STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY),                 glsl_vertex_pipe_geometry_shader}, WINED3D_GL_EXT_NONE },
    {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    glsl_vertex_pipe_pixel_shader}, WINED3D_GL_EXT_NONE    },
    {STATE_MATERIAL,                                             {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    glsl_vertex_pipe_material}, WINED3D_GL_EXT_NONE        },
    /* Clip planes */
    {STATE_CLIPPLANE(0),                                         {STATE_CLIPPLANE(0),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(0),                                         {STATE_CLIPPLANE(0),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(1),                                         {STATE_CLIPPLANE(1),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(1),                                         {STATE_CLIPPLANE(1),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(2),                                         {STATE_CLIPPLANE(2),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(2),                                         {STATE_CLIPPLANE(2),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(3),                                         {STATE_CLIPPLANE(3),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(3),                                         {STATE_CLIPPLANE(3),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(4),                                         {STATE_CLIPPLANE(4),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(4),                                         {STATE_CLIPPLANE(4),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(5),                                         {STATE_CLIPPLANE(5),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(5),                                         {STATE_CLIPPLANE(5),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(6),                                         {STATE_CLIPPLANE(6),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(6),                                         {STATE_CLIPPLANE(6),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(7),                                         {STATE_CLIPPLANE(7),                                         glsl_vertex_pipe_clip_plane}, WINED3D_GLSL_130         },
    {STATE_CLIPPLANE(7),                                         {STATE_CLIPPLANE(7),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    /* Lights */
    {STATE_LIGHT_TYPE,                                           {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(0),                                       {STATE_ACTIVELIGHT(0),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(1),                                       {STATE_ACTIVELIGHT(1),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(2),                                       {STATE_ACTIVELIGHT(2),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(3),                                       {STATE_ACTIVELIGHT(3),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(4),                                       {STATE_ACTIVELIGHT(4),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(5),                                       {STATE_ACTIVELIGHT(5),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(6),                                       {STATE_ACTIVELIGHT(6),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_ACTIVELIGHT(7),                                       {STATE_ACTIVELIGHT(7),                                       glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    /* Viewport */
    {STATE_VIEWPORT,                                             {STATE_VIEWPORT,                                             glsl_vertex_pipe_viewport}, WINED3D_GL_EXT_NONE        },
    /* Transform states */
    {STATE_TRANSFORM(WINED3D_TS_VIEW),                           {STATE_TRANSFORM(WINED3D_TS_VIEW),                           glsl_vertex_pipe_view  }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_PROJECTION),                     {STATE_TRANSFORM(WINED3D_TS_PROJECTION),                     glsl_vertex_pipe_projection}, WINED3D_GL_EXT_NONE      },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE0),                       {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE1),                       {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE2),                       {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE3),                       {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE4),                       {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE5),                       {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE6),                       {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_TEXTURE7),                       {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)),                glsl_vertex_pipe_world }, WINED3D_GL_EXT_NONE          },
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(1)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(1)),                glsl_vertex_pipe_vertexblend }, WINED3D_GL_EXT_NONE    },
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(2)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(2)),                glsl_vertex_pipe_vertexblend }, WINED3D_GL_EXT_NONE    },
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(3)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(3)),                glsl_vertex_pipe_vertexblend }, WINED3D_GL_EXT_NONE    },
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(4)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(4)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(5)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(5)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(6)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(6)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(7)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(7)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(8)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(8)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(9)),                {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(9)),              glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(10)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(10)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(11)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(11)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(12)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(12)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(13)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(13)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(14)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(14)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(15)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(15)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(16)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(16)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(17)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(17)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(18)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(18)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(19)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(19)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(20)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(20)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(21)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(21)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(22)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(22)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(23)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(23)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(24)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(24)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(25)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(25)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(26)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(26)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(27)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(27)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(28)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(28)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(29)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(29)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(30)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(30)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(31)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(31)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(32)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(32)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(33)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(33)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(34)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(34)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(35)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(35)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(36)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(36)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(37)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(37)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(38)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(38)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(39)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(39)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(40)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(40)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(41)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(41)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(42)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(42)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(43)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(43)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(44)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(44)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(45)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(45)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(46)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(46)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(47)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(47)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(48)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(48)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(49)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(49)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(50)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(50)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(51)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(51)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(52)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(52)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(53)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(53)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(54)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(54)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(55)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(55)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(56)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(56)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(57)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(57)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(58)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(58)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(59)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(59)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(60)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(60)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(61)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(61)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(62)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(62)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(63)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(63)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(64)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(64)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(65)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(65)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(66)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(66)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(67)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(67)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(68)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(68)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(69)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(69)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(70)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(70)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(71)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(71)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(72)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(72)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(73)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(73)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(74)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(74)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(75)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(75)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(76)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(76)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(77)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(77)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(78)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(78)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(79)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(79)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(80)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(80)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(81)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(81)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(82)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(82)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(83)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(83)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(84)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(84)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(85)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(85)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(86)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(86)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(87)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(87)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(88)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(88)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(89)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(89)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(90)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(90)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(91)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(91)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(92)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(92)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(93)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(93)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(94)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(94)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(95)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(95)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(96)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(96)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(97)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(97)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(98)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(98)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(99)),               {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(99)),             glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(100)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(100)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(101)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(101)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(102)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(102)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(103)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(103)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(104)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(104)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(105)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(105)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(106)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(106)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(107)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(107)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(108)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(108)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(109)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(109)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(110)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(110)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(111)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(111)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(112)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(112)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(113)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(113)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(114)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(114)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(115)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(115)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(116)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(116)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(117)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(117)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(118)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(118)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(119)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(119)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(120)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(120)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(121)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(121)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(122)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(122)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(123)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(123)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(124)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(124)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(125)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(125)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(126)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(126)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(127)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(127)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(128)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(128)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(129)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(129)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(130)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(130)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(131)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(131)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(132)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(132)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(133)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(133)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(134)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(134)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(135)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(135)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(136)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(136)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(137)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(137)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(138)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(138)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(139)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(139)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(140)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(140)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(141)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(141)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(142)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(142)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(143)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(143)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(144)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(144)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(145)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(145)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(146)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(146)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(147)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(147)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(148)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(148)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(149)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(149)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(150)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(150)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(151)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(151)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(152)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(152)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(153)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(153)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(154)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(154)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(155)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(155)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(156)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(156)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(157)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(157)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(158)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(158)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(159)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(159)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(160)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(160)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(161)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(161)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(162)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(162)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(163)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(163)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(164)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(164)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(165)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(165)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(166)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(166)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(167)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(167)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(168)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(168)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(169)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(169)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(170)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(170)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(171)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(171)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(172)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(172)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(173)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(173)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(174)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(174)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(175)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(175)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(176)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(176)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(177)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(177)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(178)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(178)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(179)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(179)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(180)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(180)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(181)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(181)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(182)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(182)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(183)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(183)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(184)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(184)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(185)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(185)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(186)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(186)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(187)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(187)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(188)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(188)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(189)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(189)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(190)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(190)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(191)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(191)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(192)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(192)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(193)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(193)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(194)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(194)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(195)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(195)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(196)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(196)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(197)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(197)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(198)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(198)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(199)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(199)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(200)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(200)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(201)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(201)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(202)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(202)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(203)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(203)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(204)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(204)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(205)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(205)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(206)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(206)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(207)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(207)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(208)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(208)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(209)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(209)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(210)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(210)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(211)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(211)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(212)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(212)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(213)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(213)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(214)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(214)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(215)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(215)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(216)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(216)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(217)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(217)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(218)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(218)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(219)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(219)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(220)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(220)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(221)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(221)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(222)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(222)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(223)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(223)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(224)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(224)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(225)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(225)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(226)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(226)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(227)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(227)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(228)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(228)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(229)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(229)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(230)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(230)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(231)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(231)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(232)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(232)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(233)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(233)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(234)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(234)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(235)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(235)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(236)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(236)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(237)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(237)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(238)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(238)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(239)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(239)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(240)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(240)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(241)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(241)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(242)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(242)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(243)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(243)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(244)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(244)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(245)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(245)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(246)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(246)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(247)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(247)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(248)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(248)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(249)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(249)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(250)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(250)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(251)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(251)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(252)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(252)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(253)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(253)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(254)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(254)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)),              {STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)),            glsl_vertex_pipe_vertexblend }, ARB_UNIFORM_BUFFER_OBJECT},
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_vertex_pipe_texmatrix}, WINED3D_GL_EXT_NONE       },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXCOORD_INDEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    /* Fog */
    {STATE_RENDER(WINED3D_RS_FOGENABLE),                         {STATE_RENDER(WINED3D_RS_FOGENABLE),                         glsl_vertex_pipe_shader}, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),                      {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),                     {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_RANGEFOGENABLE),                    {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_CLIPPING),                          {STATE_RENDER(WINED3D_RS_CLIPPING),                          state_clipping         }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE),                   {STATE_RENDER(WINED3D_RS_CLIPPING),                          NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_LIGHTING),                          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_AMBIENT),                           {STATE_RENDER(WINED3D_RS_AMBIENT),                           glsl_vertex_pipe_light }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_COLORVERTEX),                       {STATE_RENDER(WINED3D_RS_COLORVERTEX),                       glsl_vertex_pipe_shader}, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_LOCALVIEWER),                       {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_NORMALIZENORMALS),                  {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_DIFFUSEMATERIALSOURCE),             {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_SPECULARMATERIALSOURCE),            {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_AMBIENTMATERIALSOURCE),             {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_EMISSIVEMATERIALSOURCE),            {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_VERTEXBLEND),                       {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_POINTSIZE),                         {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),                     NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),                     {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),                     glsl_vertex_pipe_pointsize}, WINED3D_GL_EXT_NONE       },
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 state_pointsprite      }, ARB_POINT_SPRITE             },
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 state_pointsprite_w    }, WINED3D_GL_LEGACY_CONTEXT    },
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 glsl_vertex_pointsprite_core}, WINED3D_GL_EXT_NONE     },
    {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),                  {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),                  glsl_vertex_pipe_pointscale}, WINED3D_GL_EXT_NONE      },
    {STATE_RENDER(WINED3D_RS_POINTSCALE_A),                      {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),                  NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_POINTSCALE_B),                      {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),                  NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_POINTSCALE_C),                      {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),                  NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),                     {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),                     NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_TWEENFACTOR),                       {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_INDEXEDVERTEXBLENDENABLE),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   NULL                   }, WINED3D_GL_EXT_NONE          },
    /* NP2 texture matrix fixups. They are not needed if
     * GL_ARB_texture_non_power_of_two is supported. Otherwise, register
     * glsl_vertex_pipe_texmatrix(), which takes care of updating the texture
     * matrix. */
    {STATE_SAMPLER(0),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(0),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(0),                                           {STATE_SAMPLER(0),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(1),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(1),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(1),                                           {STATE_SAMPLER(1),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(2),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(2),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(2),                                           {STATE_SAMPLER(2),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(3),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(3),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(3),                                           {STATE_SAMPLER(3),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(4),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(4),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(4),                                           {STATE_SAMPLER(4),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(5),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(5),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(5),                                           {STATE_SAMPLER(5),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(6),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(6),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(6),                                           {STATE_SAMPLER(6),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_SAMPLER(7),                                           {0,                                                          NULL                   }, ARB_TEXTURE_NON_POWER_OF_TWO },
    {STATE_SAMPLER(7),                                           {0,                                                          NULL                   }, WINED3D_GL_NORMALIZED_TEXRECT},
    {STATE_SAMPLER(7),                                           {STATE_SAMPLER(7),                                           glsl_vertex_pipe_texmatrix_np2}, WINED3D_GL_EXT_NONE   },
    {STATE_POINT_ENABLE,                                         {STATE_POINT_ENABLE,                                         glsl_vertex_pipe_shader}, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_SHADEMODE),                         {STATE_RENDER(WINED3D_RS_SHADEMODE),                         glsl_vertex_pipe_shademode}, WINED3D_GLSL_130          },
    {STATE_RENDER(WINED3D_RS_SHADEMODE),                         {STATE_RENDER(WINED3D_RS_SHADEMODE),                         glsl_vertex_pipe_nop   }, WINED3D_GL_EXT_NONE          },
    {0 /* Terminate */,                                          {0,                                                          NULL                   }, WINED3D_GL_EXT_NONE          },
};

/* TODO:
 *   - Implement vertex tweening. */
const struct wined3d_vertex_pipe_ops glsl_vertex_pipe =
{
    glsl_vertex_pipe_vp_enable,
    glsl_vertex_pipe_vp_get_caps,
    glsl_vertex_pipe_vp_get_emul_mask,
    glsl_vertex_pipe_vp_alloc,
    glsl_vertex_pipe_vp_free,
    glsl_vertex_pipe_vp_states,
};

static void glsl_fragment_pipe_enable(const struct wined3d_context *context, BOOL enable)
{
    /* Nothing to do. */
}

static void glsl_fragment_pipe_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->wined3d_caps = WINED3D_FRAGMENT_CAP_PROJ_CONTROL
            | WINED3D_FRAGMENT_CAP_SRGB_WRITE
            | WINED3D_FRAGMENT_CAP_COLOR_KEY;
    caps->PrimitiveMiscCaps = WINED3DPMISCCAPS_TSSARGTEMP
            | WINED3DPMISCCAPS_PERSTAGECONSTANT;
    caps->TextureOpCaps = WINED3DTEXOPCAPS_DISABLE
            | WINED3DTEXOPCAPS_SELECTARG1
            | WINED3DTEXOPCAPS_SELECTARG2
            | WINED3DTEXOPCAPS_MODULATE4X
            | WINED3DTEXOPCAPS_MODULATE2X
            | WINED3DTEXOPCAPS_MODULATE
            | WINED3DTEXOPCAPS_ADDSIGNED2X
            | WINED3DTEXOPCAPS_ADDSIGNED
            | WINED3DTEXOPCAPS_ADD
            | WINED3DTEXOPCAPS_SUBTRACT
            | WINED3DTEXOPCAPS_ADDSMOOTH
            | WINED3DTEXOPCAPS_BLENDCURRENTALPHA
            | WINED3DTEXOPCAPS_BLENDFACTORALPHA
            | WINED3DTEXOPCAPS_BLENDTEXTUREALPHA
            | WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA
            | WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM
            | WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR
            | WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA
            | WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA
            | WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR
            | WINED3DTEXOPCAPS_DOTPRODUCT3
            | WINED3DTEXOPCAPS_MULTIPLYADD
            | WINED3DTEXOPCAPS_LERP
            | WINED3DTEXOPCAPS_BUMPENVMAP
            | WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE;
    caps->MaxTextureBlendStages = WINED3D_MAX_TEXTURES;
    caps->MaxSimultaneousTextures = min(gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL], WINED3D_MAX_TEXTURES);
}

static DWORD glsl_fragment_pipe_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        return GL_EXT_EMUL_ARB_MULTITEXTURE;
    return 0;
}

static void *glsl_fragment_pipe_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    struct shader_glsl_priv *priv;

    if (shader_backend == &glsl_shader_backend)
    {
        priv = shader_priv;
        wine_rb_init(&priv->ffp_fragment_shaders, wined3d_ffp_frag_program_key_compare);
        return priv;
    }

    FIXME("GLSL fragment pipe without GLSL shader backend not implemented.\n");

    return NULL;
}

static void shader_glsl_free_ffp_fragment_shader(struct wine_rb_entry *entry, void *param)
{
    struct glsl_ffp_fragment_shader *shader = WINE_RB_ENTRY_VALUE(entry,
            struct glsl_ffp_fragment_shader, entry.entry);
    struct glsl_shader_prog_link *program, *program2;
    struct glsl_ffp_destroy_ctx *ctx = param;
    const struct wined3d_gl_info *gl_info;

    gl_info = ctx->context_gl->gl_info;
    LIST_FOR_EACH_ENTRY_SAFE(program, program2, &shader->linked_programs,
            struct glsl_shader_prog_link, ps.shader_entry)
    {
        delete_glsl_program_entry(ctx->priv, gl_info, program);
    }
    GL_EXTCALL(glDeleteShader(shader->id));
    heap_free(shader);
}

/* Context activation is done by the caller. */
static void glsl_fragment_pipe_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct shader_glsl_priv *priv = device->fragment_priv;
    struct glsl_ffp_destroy_ctx ctx;

    ctx.priv = priv;
    ctx.context_gl = context_gl;
    wine_rb_destroy(&priv->ffp_fragment_shaders, shader_glsl_free_ffp_fragment_shader, &ctx);
}

static void glsl_fragment_pipe_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->last_was_pshader = use_ps(state);

    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

static void glsl_fragment_pipe_fogparams(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_FOG;
}

static void glsl_fragment_pipe_fog(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    BOOL use_vshader = use_vs(state);
    enum fogsource new_source;
    DWORD fogstart = state->render_states[WINED3D_RS_FOGSTART];
    DWORD fogend = state->render_states[WINED3D_RS_FOGEND];

    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;

    if (!state->render_states[WINED3D_RS_FOGENABLE])
        return;

    if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
    {
        if (use_vshader)
            new_source = FOGSOURCE_VS;
        else if (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE || context->stream_info.position_transformed)
            new_source = FOGSOURCE_COORD;
        else
            new_source = FOGSOURCE_FFP;
    }
    else
    {
        new_source = FOGSOURCE_FFP;
    }

    if (new_source != context->fog_source || fogstart == fogend)
    {
        context->fog_source = new_source;
        context->constant_update_mask |= WINED3D_SHADER_CONST_PS_FOG;
    }
}

static void glsl_fragment_pipe_vdecl(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* Because of settings->texcoords_initialized and args->texcoords_initialized. */
    if (!shader_glsl_full_ffp_varyings(gl_info))
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;

    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_FOGENABLE)))
        glsl_fragment_pipe_fog(context, state, state_id);
}

static void glsl_fragment_pipe_vs(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* Because of settings->texcoords_initialized and args->texcoords_initialized. */
    if (!shader_glsl_full_ffp_varyings(gl_info))
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

static void glsl_fragment_pipe_tex_transform(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

static void glsl_fragment_pipe_invalidate_constants(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PS;
}

static void glsl_fragment_pipe_alpha_test_func(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    GLint func = wined3d_gl_compare_func(state->render_states[WINED3D_RS_ALPHAFUNC]);
    float ref = wined3d_alpha_ref(state);

    if (func)
    {
        gl_info->gl_ops.gl.p_glAlphaFunc(func, ref);
        checkGLcall("glAlphaFunc");
    }
}

static void glsl_fragment_pipe_core_alpha_test(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

static void glsl_fragment_pipe_alpha_test(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_ALPHATESTENABLE])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable(GL_ALPHA_TEST)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }
}

static void glsl_fragment_pipe_core_alpha_test_ref(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_ALPHA_TEST;
}

static void glsl_fragment_pipe_color_key(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_COLOR_KEY;
}

static void glsl_fragment_pipe_shademode(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

static const struct wined3d_state_entry_template glsl_fragment_pipe_state_template[] =
{
    {STATE_VDECL,                                               {STATE_VDECL,                                                glsl_fragment_pipe_vdecl               }, WINED3D_GL_EXT_NONE },
    {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                  {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   glsl_fragment_pipe_vs                  }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),                    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),                     glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),               {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG1),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG2),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG0),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),             {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                   {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    glsl_fragment_pipe_shader              }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                        {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                         glsl_fragment_pipe_alpha_test_func     }, WINED3D_GL_LEGACY_CONTEXT},
    {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                        {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_ALPHAREF),                         {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                         NULL                                   }, WINED3D_GL_LEGACY_CONTEXT},
    {STATE_RENDER(WINED3D_RS_ALPHAREF),                         {STATE_RENDER(WINED3D_RS_ALPHAREF),                          glsl_fragment_pipe_core_alpha_test_ref }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                  {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   glsl_fragment_pipe_alpha_test          }, WINED3D_GL_LEGACY_CONTEXT},
    {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                  {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   glsl_fragment_pipe_core_alpha_test     }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_COLORKEYENABLE),                   {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_COLOR_KEY,                                           { STATE_COLOR_KEY,                                           glsl_fragment_pipe_color_key           }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGENABLE),                        {STATE_RENDER(WINED3D_RS_FOGENABLE),                         glsl_fragment_pipe_fog                 }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),                     {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),                    {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGSTART),                         {STATE_RENDER(WINED3D_RS_FOGSTART),                          glsl_fragment_pipe_fogparams           }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGEND),                           {STATE_RENDER(WINED3D_RS_FOGSTART),                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                  {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                   state_srgbwrite                        }, ARB_FRAMEBUFFER_SRGB},
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                  {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),                    NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGCOLOR),                         {STATE_RENDER(WINED3D_RS_FOGCOLOR),                          glsl_fragment_pipe_fogparams           }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGDENSITY),                       {STATE_RENDER(WINED3D_RS_FOGDENSITY),                        glsl_fragment_pipe_fogparams           }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 glsl_fragment_pipe_shader              }, ARB_POINT_SPRITE    },
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 glsl_fragment_pipe_shader              }, WINED3D_GL_VERSION_2_0},
    {STATE_TEXTURESTAGE(0,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(0, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(1, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(2, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(3, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(4, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(5, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(6, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_CONSTANT),               {STATE_TEXTURESTAGE(7, WINED3D_TSS_CONSTANT),                glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                   {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_POINT_ENABLE,                                        {STATE_POINT_ENABLE,                                         glsl_fragment_pipe_shader              }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_SHADEMODE),                        {STATE_RENDER(WINED3D_RS_SHADEMODE),                         glsl_fragment_pipe_shademode           }, WINED3D_GLSL_130    },
    {STATE_RENDER(WINED3D_RS_SHADEMODE),                        {STATE_RENDER(WINED3D_RS_SHADEMODE),                         state_shademode                        }, WINED3D_GL_EXT_NONE },
    {0 /* Terminate */,                                         {0,                                                          0                                      }, WINED3D_GL_EXT_NONE },
};

static BOOL glsl_fragment_pipe_alloc_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void glsl_fragment_pipe_free_context_data(struct wined3d_context *context)
{
}

const struct wined3d_fragment_pipe_ops glsl_fragment_pipe =
{
    glsl_fragment_pipe_enable,
    glsl_fragment_pipe_get_caps,
    glsl_fragment_pipe_get_emul_mask,
    glsl_fragment_pipe_alloc,
    glsl_fragment_pipe_free,
    glsl_fragment_pipe_alloc_context_data,
    glsl_fragment_pipe_free_context_data,
    shader_glsl_color_fixup_supported,
    glsl_fragment_pipe_state_template,
};

struct glsl_blitter_args
{
    GLenum texture_type;
    struct color_fixup_desc fixup;
    unsigned short use_colour_key;
};

struct glsl_blitter_program
{
    struct wine_rb_entry entry;
    struct glsl_blitter_args args;
    GLuint id;
};

struct wined3d_glsl_blitter
{
    struct wined3d_blitter blitter;
    struct wined3d_string_buffer_list string_buffers;
    struct wine_rb_tree programs;
    GLuint palette_texture;
};

static int glsl_blitter_args_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct glsl_blitter_args *a = key;
    const struct glsl_blitter_args *b = &WINE_RB_ENTRY_VALUE(entry, const struct glsl_blitter_program, entry)->args;

    return memcmp(a, b, sizeof(*a));
}

/* Context activation is done by the caller. */
static void glsl_free_blitter_program(struct wine_rb_entry *entry, void *ctx)
{
    struct glsl_blitter_program *program = WINE_RB_ENTRY_VALUE(entry, struct glsl_blitter_program, entry);
    struct wined3d_context_gl *context_gl = ctx;
    const struct wined3d_gl_info *gl_info;

    gl_info = context_gl->gl_info;
    GL_EXTCALL(glDeleteProgram(program->id));
    checkGLcall("glDeleteProgram()");
    heap_free(program);
}

/* Context activation is done by the caller. */
static void glsl_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_glsl_blitter *glsl_blitter;
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    glsl_blitter = CONTAINING_RECORD(blitter, struct wined3d_glsl_blitter, blitter);

    if (glsl_blitter->palette_texture)
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &glsl_blitter->palette_texture);

    wine_rb_destroy(&glsl_blitter->programs, glsl_free_blitter_program, context_gl);
    string_buffer_list_cleanup(&glsl_blitter->string_buffers);

    heap_free(glsl_blitter);
}

static void glsl_blitter_generate_p8_shader(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct glsl_blitter_args *args,
        const char *output, const char *tex_type, const char *swizzle)
{
    shader_addline(buffer, "uniform sampler1D sampler_palette;\n");
    shader_addline(buffer, "\nvoid main()\n{\n");
    /* The alpha-component contains the palette index. */
    shader_addline(buffer, "    float index = texture%s(sampler, out_texcoord.%s).%c;\n",
            needs_legacy_glsl_syntax(gl_info) ? tex_type : "", swizzle,
            gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] ? 'w' : 'x');
    /* Scale the index by 255/256 and add a bias of 0.5 in order to sample in
     * the middle. */
    shader_addline(buffer, "    index = (index * 255.0 + 0.5) / 256.0;\n");
    shader_addline(buffer, "    %s = texture%s(sampler_palette, index);\n",
            output, needs_legacy_glsl_syntax(gl_info) ? "1D" : "");
    if (args->use_colour_key)
        shader_glsl_generate_colour_key_test(buffer, output, "colour_key.low", "colour_key.high");
    shader_addline(buffer, "}\n");
}

static void gen_packed_yuv_read(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct glsl_blitter_args *args,
        const char *tex_type)
{
    enum complex_fixup complex_fixup = get_complex_fixup(args->fixup);
    char chroma, luminance;
    const char *tex;

    /* The YUY2 and UYVY formats contain two pixels packed into a 32 bit
     * macropixel, giving effectively 16 bits per pixel. The color consists of
     * a luminance(Y) and two chroma(U and V) values. Each macropixel has two
     * luminance values, one for each single pixel it contains, and one U and
     * one V value shared between both pixels.
     *
     * The data is loaded into an A8L8 texture. With YUY2, the luminance
     * component contains the luminance and alpha the chroma. With UYVY it is
     * vice versa. Thus take the format into account when generating the read
     * swizzles
     *
     * Reading the Y value is straightforward - just sample the texture. The
     * hardware takes care of filtering in the horizontal and vertical
     * direction.
     *
     * Reading the U and V values is harder. We have to avoid filtering
     * horizontally, because that would mix the U and V values of one pixel or
     * two adjacent pixels.  Thus floor the texture coordinate and add 0.5 to
     * get an unfiltered read, regardless of the filtering setting. Vertical
     * filtering works automatically though - the U and V values of two rows
     * are mixed nicely.
     *
     * Apart of avoiding filtering issues, the code has to know which value it
     * just read, and where it can find the other one. To determine this, it
     * checks if it sampled an even or odd pixel, and shifts the 2nd read
     * accordingly.
     *
     * Handling horizontal filtering of U and V values requires reading a 2nd
     * pair of pixels, extracting U and V and mixing them. This is not
     * implemented yet.
     *
     * An alternative implementation idea is to load the texture as A8R8G8B8
     * texture, with width / 2. This way one read gives all 3 values, finding
     * U and V is easy in an unfiltered situation. Finding the luminance on
     * the other hand requires finding out if it is an odd or even pixel. The
     * real drawback of this approach is filtering. This would have to be
     * emulated completely in the shader, reading up two 2 packed pixels in up
     * to 2 rows and interpolating both horizontally and vertically. Beyond
     * that it would require adjustments to the texture handling code to deal
     * with the width scaling. */

    if (complex_fixup == COMPLEX_FIXUP_UYVY)
    {
        chroma = 'x';
        luminance = gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] ? 'w' : 'y';
    }
    else
    {
        chroma = gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] ? 'w' : 'y';
        luminance = 'x';
    }

    tex = needs_legacy_glsl_syntax(gl_info) ? tex_type : "";

    /* First we have to read the chroma values. This means we need at least
     * two pixels (no filtering), or 4 pixels (with filtering). To get the
     * unmodified chroma, we have to rid ourselves of the filtering when we
     * sample the texture. */
    shader_addline(buffer, "    texcoord.xy = out_texcoord.xy;\n");
    /* We must not allow filtering between pixel x and x+1, this would mix U
     * and V. Vertical filtering is ok. However, bear in mind that the pixel
     * center is at 0.5, so add 0.5. */
    shader_addline(buffer, "    texcoord.x = (floor(texcoord.x * size.x) + 0.5) / size.x;\n");
    shader_addline(buffer, "    luminance = texture%s(sampler, texcoord.xy).%c;\n", tex, chroma);

    /* Multiply the x coordinate by 0.5 and get the fraction. This gives 0.25
     * and 0.75 for the even and odd pixels respectively. */
    /* Put the value into either of the chroma values. */
    shader_addline(buffer, "    bool even = fract(texcoord.x * size.x * 0.5) < 0.5;\n");
    shader_addline(buffer, "    if (even)\n");
    shader_addline(buffer, "        chroma.y = luminance;\n");
    shader_addline(buffer, "    else\n");
    shader_addline(buffer, "        chroma.x = luminance;\n");

    /* Sample pixel 2. If we read an even pixel, sample the pixel right to the
     * current one. Otherwise, sample the left pixel. */
    shader_addline(buffer, "    texcoord.x += even ? 1.0 / size.x : -1.0 / size.x;\n");
    shader_addline(buffer, "    luminance = texture%s(sampler, texcoord.xy).%c;\n", tex, chroma);

    /* Put the value into the other chroma. */
    shader_addline(buffer, "    if (even)\n");
    shader_addline(buffer, "        chroma.x = luminance;\n");
    shader_addline(buffer, "    else\n");
    shader_addline(buffer, "        chroma.y = luminance;\n");

    /* TODO: If filtering is enabled, sample a 2nd pair of pixels left or right of
     * the current one and lerp the two U and V values. */

    /* This gives the correctly filtered luminance value. */
    shader_addline(buffer, "    luminance = texture%s(sampler, out_texcoord.xy).%c;\n", tex, luminance);
}

static void gen_yv12_read(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const char *tex_type)
{
    char component = gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] ? 'w' : 'x';
    const char *tex = needs_legacy_glsl_syntax(gl_info) ? tex_type : "";

    /* YV12 surfaces contain a WxH sized luminance plane, followed by a
     * (W/2)x(H/2) V and a (W/2)x(H/2) U plane, each with 8 bit per pixel. So
     * the effective bitdepth is 12 bits per pixel. Since the U and V planes
     * have only half the pitch of the luminance plane, the packing into the
     * gl texture is a bit unfortunate. If the whole texture is interpreted as
     * luminance data it looks approximately like this:
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
     * kinda tricky to read. */

    /* First sample the chroma values. */
    shader_addline(buffer, "    texcoord.xy = out_texcoord.xy;\n");
    /* The chroma planes have only half the width. */
    shader_addline(buffer, "    texcoord.x *= 0.5;\n");

    /* The first value is between 2/3 and 5/6 of the texture's height, so
     * scale+bias the coordinate. Also read the right side of the image when
     * reading odd lines.
     *
     * Don't forget to clamp the y values in into the range, otherwise we'll
     * get filtering bleeding. */

    /* Read odd lines from the right side (add 0.5 to the x coordinate). */
    shader_addline(buffer, "    if (fract(floor(texcoord.y * size.y) * 0.5 + 1.0 / 6.0) >= 0.5)\n");
    shader_addline(buffer, "        texcoord.x += 0.5;\n");

    /* Clamp, keep the half pixel origin in mind. */
    shader_addline(buffer, "    texcoord.y = clamp(2.0 / 3.0 + texcoord.y / 6.0, "
            "2.0 / 3.0 + 0.5 / size.y, 5.0 / 6.0 - 0.5 / size.y);\n");

    shader_addline(buffer, "    chroma.x = texture%s(sampler, texcoord.xy).%c;\n", tex, component);

    /* The other chroma value is 1/6th of the texture lower, from 5/6th to
     * 6/6th No need to clamp because we're just reusing the already clamped
     * value from above. */
    shader_addline(buffer, "    texcoord.y += 1.0 / 6.0;\n");
    shader_addline(buffer, "    chroma.y = texture%s(sampler, texcoord.xy).%c;\n", tex, component);

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so
     * scale the y coordinate.  Clamp the y coordinate to prevent the chroma
     * values from bleeding into the sampled luminance values due to
     * filtering. */
    shader_addline(buffer, "    texcoord.xy = out_texcoord.xy;\n");
    /* Multiply the y coordinate by 2/3 and clamp it. */
    shader_addline(buffer, "    texcoord.y = min(texcoord.y * 2.0 / 3.0, 2.0 / 3.0 - 0.5 / size.y);\n");
    shader_addline(buffer, "    luminance = texture%s(sampler, texcoord.xy).%c;\n", tex, component);
}

static void gen_nv12_read(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const char *tex_type)
{
    char component = gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] ? 'w' : 'x';
    const char *tex = needs_legacy_glsl_syntax(gl_info) ? tex_type : "";

    /* NV12 surfaces contain a WxH sized luminance plane, followed by a
     * (W/2)x(H/2) sized plane where each component is an UV pair. So the
     * effective bitdepth is 12 bits per pixel. If the whole texture is
     * interpreted as luminance data it looks approximately like this:
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
     *        +----------------------------------+---- */

    /* First sample the chroma values. */
    shader_addline(buffer, "    texcoord.xy = out_texcoord.xy;\n");
    /* We only have half the number of chroma pixels. */
    shader_addline(buffer, "    texcoord.x *= 0.5;\n");
    shader_addline(buffer, "    texcoord.y = (texcoord.y + 2.0) / 3.0;\n");

    /* We must not allow filtering horizontally, this would mix U and V.
     * Vertical filtering is ok. However, bear in mind that the pixel center
     * is at 0.5, so add 0.5. */

    /* Convert to non-normalised coordinates so we can find the individual
     * pixel. */
    shader_addline(buffer, "    texcoord.x = floor(texcoord.x * size.x);\n");
    /* Multiply by 2 since chroma components are stored in UV pixel pairs, add
     * 0.5 to hit the center of the pixel. Then convert back to normalised
     * coordinates. */
    shader_addline(buffer, "    texcoord.x = (texcoord.x * 2.0 + 0.5) / size.x;\n");
    /* Clamp, keep the half pixel origin in mind. */
    shader_addline(buffer, "    texcoord.y = max(texcoord.y, 2.0 / 3.0 + 0.5 / size.y);\n");

    shader_addline(buffer, "    chroma.y = texture%s(sampler, texcoord.xy).%c;\n", tex, component);
    /* Add 1.0 / size.x to sample the adjacent texel. */
    shader_addline(buffer, "    texcoord.x += 1.0 / size.x;\n");
    shader_addline(buffer, "    chroma.x = texture%s(sampler, texcoord.xy).%c;\n", tex, component);

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so
     * scale the y coordinate. Clamp the y coordinate to prevent the chroma
     * values from bleeding into the sampled luminance values due to
     * filtering. */
    shader_addline(buffer, "    texcoord.xy = out_texcoord.xy;\n");
    /* Multiply the y coordinate by 2/3 and clamp it. */
    shader_addline(buffer, "    texcoord.y = min(texcoord.y * 2.0 / 3.0, 2.0 / 3.0 - 0.5 / size.y);\n");
    shader_addline(buffer, "    luminance = texture%s(sampler, texcoord.xy).%c;\n", tex, component);
}

static void glsl_blitter_generate_yuv_shader(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct glsl_blitter_args *args,
        const char *output, const char *tex_type, const char *swizzle)
{
    enum complex_fixup complex_fixup = get_complex_fixup(args->fixup);

    shader_addline(buffer, "const vec4 yuv_coef = vec4(1.403, -0.344, -0.714, 1.770);\n");
    shader_addline(buffer, "float luminance;\n");
    shader_addline(buffer, "vec2 texcoord;\n");
    shader_addline(buffer, "vec2 chroma;\n");
    shader_addline(buffer, "uniform vec2 size;\n");

    shader_addline(buffer, "\nvoid main()\n{\n");

    switch (complex_fixup)
    {
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YUY2:
            gen_packed_yuv_read(buffer, gl_info, args, tex_type);
            break;

        case COMPLEX_FIXUP_YV12:
            gen_yv12_read(buffer, gl_info, tex_type);
            break;

        case COMPLEX_FIXUP_NV12:
            gen_nv12_read(buffer, gl_info, tex_type);
            break;

        default:
            FIXME("Unsupported fixup %#x.\n", complex_fixup);
            string_buffer_free(buffer);
            return;
    }

    /* Calculate the final result. Formula is taken from
     * http://www.fourcc.org/fccyvrgb.php. Note that the chroma
     * ranges from -0.5 to 0.5. */
    shader_addline(buffer, "\n    chroma.xy -= 0.5;\n");

    shader_addline(buffer, "    %s.x = luminance + chroma.x * yuv_coef.x;\n", output);
    shader_addline(buffer, "    %s.y = luminance + chroma.y * yuv_coef.y + chroma.x * yuv_coef.z;\n", output);
    shader_addline(buffer, "    %s.z = luminance + chroma.y * yuv_coef.w;\n", output);
    if (args->use_colour_key)
        shader_glsl_generate_colour_key_test(buffer, output, "colour_key.low", "colour_key.high");

    shader_addline(buffer, "}\n");
}

static void glsl_blitter_generate_plain_shader(struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const struct glsl_blitter_args *args,
        const char *output, const char *tex_type, const char *swizzle)
{
    shader_addline(buffer, "\nvoid main()\n{\n");
    shader_addline(buffer, "    %s = texture%s(sampler, out_texcoord.%s);\n",
            output, needs_legacy_glsl_syntax(gl_info) ? tex_type : "", swizzle);
    shader_glsl_color_correction_ext(buffer, output, WINED3DSP_WRITEMASK_ALL, args->fixup);
    if (args->use_colour_key)
        shader_glsl_generate_colour_key_test(buffer, output, "colour_key.low", "colour_key.high");
    shader_addline(buffer, "}\n");
}

/* Context activation is done by the caller. */
static GLuint glsl_blitter_generate_program(struct wined3d_glsl_blitter *blitter,
        const struct wined3d_gl_info *gl_info, const struct glsl_blitter_args *args)
{
    static const struct
    {
        GLenum texture_target;
        const char texture_type[7];
        const char texcoord_swizzle[4];
    }
    texture_data[] =
    {
        {GL_TEXTURE_2D, "2D", "xy"},
        {GL_TEXTURE_CUBE_MAP, "Cube", "xyz"},
        {GL_TEXTURE_RECTANGLE_ARB, "2DRect", "xy"},
    };
    static const char vshader_main[] =
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(pos, 0.0, 1.0);\n"
        "    out_texcoord = texcoord;\n"
        "}\n";
    enum complex_fixup complex_fixup = get_complex_fixup(args->fixup);
    struct wined3d_string_buffer *buffer, *output;
    GLuint program, vshader_id, fshader_id;
    const char *tex_type, *swizzle, *ptr;
    unsigned int i;
    GLint loc;

    for (i = 0; i < ARRAY_SIZE(texture_data); ++i)
    {
        if (args->texture_type == texture_data[i].texture_target)
        {
            tex_type = texture_data[i].texture_type;
            swizzle = texture_data[i].texcoord_swizzle;
            break;
        }
    }
    if (i == ARRAY_SIZE(texture_data))
    {
        FIXME("Unsupported texture type %#x.\n", args->texture_type);
        return 0;
    }

    program = GL_EXTCALL(glCreateProgram());

    vshader_id = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));

    buffer = string_buffer_get(&blitter->string_buffers);
    shader_glsl_add_version_declaration(buffer, gl_info);
    shader_addline(buffer, "%s vec2 pos;\n", get_attribute_keyword(gl_info));
    shader_addline(buffer, "%s vec3 texcoord;\n", get_attribute_keyword(gl_info));
    declare_out_varying(gl_info, buffer, FALSE, "vec3 out_texcoord;\n");
    shader_addline(buffer, vshader_main);

    ptr = buffer->buffer;
    GL_EXTCALL(glShaderSource(vshader_id, 1, &ptr, NULL));
    GL_EXTCALL(glAttachShader(program, vshader_id));
    GL_EXTCALL(glDeleteShader(vshader_id));

    fshader_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));

    string_buffer_clear(buffer);
    shader_glsl_add_version_declaration(buffer, gl_info);
    shader_addline(buffer, "uniform sampler%s sampler;\n", tex_type);
    if (args->use_colour_key)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    vec4 low, high;\n");
        shader_addline(buffer, "} colour_key;\n");
    }
    declare_in_varying(gl_info, buffer, FALSE, "vec3 out_texcoord;\n");
    /* TODO: Declare the out variable with the correct type (and put it in the
     * blitter args). */
    if (!needs_legacy_glsl_syntax(gl_info))
        shader_addline(buffer, "out vec4 ps_out[1];\n");

    output = string_buffer_get(&blitter->string_buffers);
    string_buffer_sprintf(output, "%s[0]", get_fragment_output(gl_info));

    switch (complex_fixup)
    {
        case COMPLEX_FIXUP_P8:
            glsl_blitter_generate_p8_shader(buffer, gl_info, args, output->buffer, tex_type, swizzle);
            break;
        case COMPLEX_FIXUP_YUY2:
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YV12:
        case COMPLEX_FIXUP_NV12:
            glsl_blitter_generate_yuv_shader(buffer, gl_info, args, output->buffer, tex_type, swizzle);
            break;
        case COMPLEX_FIXUP_NONE:
            glsl_blitter_generate_plain_shader(buffer, gl_info, args, output->buffer, tex_type, swizzle);
    }

    string_buffer_release(&blitter->string_buffers, output);

    ptr = buffer->buffer;
    GL_EXTCALL(glShaderSource(fshader_id, 1, &ptr, NULL));
    string_buffer_release(&blitter->string_buffers, buffer);
    GL_EXTCALL(glAttachShader(program, fshader_id));
    GL_EXTCALL(glDeleteShader(fshader_id));

    GL_EXTCALL(glBindAttribLocation(program, 0, "pos"));
    GL_EXTCALL(glBindAttribLocation(program, 1, "texcoord"));

    if (!needs_legacy_glsl_syntax(gl_info))
        GL_EXTCALL(glBindFragDataLocation(program, 0, "ps_out"));

    GL_EXTCALL(glCompileShader(vshader_id));
    print_glsl_info_log(gl_info, vshader_id, FALSE);
    GL_EXTCALL(glCompileShader(fshader_id));
    print_glsl_info_log(gl_info, fshader_id, FALSE);
    GL_EXTCALL(glLinkProgram(program));
    shader_glsl_validate_link(gl_info, program);

    GL_EXTCALL(glUseProgram(program));
    loc = GL_EXTCALL(glGetUniformLocation(program, "sampler"));
    GL_EXTCALL(glUniform1i(loc, 0));
    if (complex_fixup == COMPLEX_FIXUP_P8)
    {
        loc = GL_EXTCALL(glGetUniformLocation(program, "sampler_palette"));
        GL_EXTCALL(glUniform1i(loc, 1));
    }

    return program;
}

/* Context activation is done by the caller. */
static void glsl_blitter_upload_palette(struct wined3d_glsl_blitter *blitter,
        struct wined3d_context_gl *context_gl, const struct wined3d_texture_gl *texture_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_palette *palette;

    palette = texture_gl->t.swapchain ? texture_gl->t.swapchain->palette : NULL;

    if (!blitter->palette_texture)
        gl_info->gl_ops.gl.p_glGenTextures(1, &blitter->palette_texture);

    wined3d_context_gl_active_texture(context_gl, gl_info, 1);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, blitter->palette_texture);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    if (palette)
    {
        gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, palette->colors);
    }
    else
    {
        static const DWORD black;

        FIXME("P8 texture loaded without a palette.\n");
        gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 1, 0, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, &black);
    }

    wined3d_context_gl_active_texture(context_gl, gl_info, 0);
}

/* Context activation is done by the caller. */
static struct glsl_blitter_program *glsl_blitter_get_program(struct wined3d_glsl_blitter *blitter,
        struct wined3d_context_gl *context_gl, const struct wined3d_texture_gl *texture_gl, BOOL use_colour_key)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct glsl_blitter_program *program;
    struct glsl_blitter_args args;
    struct wine_rb_entry *entry;

    memset(&args, 0, sizeof(args));
    args.texture_type = texture_gl->target;
    args.fixup = texture_gl->t.resource.format->color_fixup;
    args.use_colour_key = use_colour_key;

    if ((entry = wine_rb_get(&blitter->programs, &args)))
        return WINE_RB_ENTRY_VALUE(entry, struct glsl_blitter_program, entry);

    if (!(program = heap_alloc(sizeof(*program))))
    {
        ERR("Failed to allocate blitter program memory.\n");
        return NULL;
    }

    program->args = args;
    if (!(program->id = glsl_blitter_generate_program(blitter, gl_info, &args)))
    {
        WARN("Failed to generate blitter program.\n");
        heap_free(program);
        return NULL;
    }

    if (wine_rb_put(&blitter->programs, &program->args, &program->entry) == -1)
    {
        ERR("Failed to store blitter program.\n");
        GL_EXTCALL(glDeleteProgram(program->id));
        heap_free(program);
        return NULL;
    }

    return program;
}

static BOOL glsl_blitter_supported(enum wined3d_blit_op blit_op, const struct wined3d_context *context,
        const struct wined3d_texture_gl *src_texture, DWORD src_location,
        const struct wined3d_texture_gl *dst_texture, DWORD dst_location)
{
    const struct wined3d_resource *src_resource = &src_texture->t.resource;
    const struct wined3d_resource *dst_resource = &dst_texture->t.resource;
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;
    BOOL decompress;

    if (blit_op == WINED3D_BLIT_OP_RAW_BLIT && dst_format->id == src_format->id)
    {
        if (dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    }

    if (blit_op != WINED3D_BLIT_OP_COLOR_BLIT && blit_op != WINED3D_BLIT_OP_COLOR_BLIT_CKEY
            && blit_op != WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST)
    {
        TRACE("Unsupported blit_op %#x.\n", blit_op);
        return FALSE;
    }

    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return FALSE;

    if (src_texture->target == GL_TEXTURE_2D_MULTISAMPLE
            || src_texture->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
    {
        TRACE("Multi-sample source textures not supported.\n");
        return FALSE;
    }

    /* We don't necessarily want to blit from resources without
     * WINED3D_RESOURCE_ACCESS_GPU, but that may be the only way to decompress
     * compressed textures. */
    decompress = (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
            && !(dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED);
    if (!decompress && !(src_resource->access & dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        TRACE("Source or destination resource does not have GPU access.\n");
        return FALSE;
    }

    if (!is_identity_fixup(dst_format->color_fixup)
            && (dst_format->id != src_format->id || dst_location != WINED3D_LOCATION_DRAWABLE))
    {
        TRACE("Destination fixups are not supported.\n");
        return FALSE;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && !((dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
            || (dst_resource->bind_flags & WINED3D_BIND_RENDER_TARGET)))
    {
        TRACE("Destination texture is not FBO attachable.\n");
        return FALSE;
    }

    TRACE("Returning supported.\n");
    return TRUE;
}

static DWORD glsl_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_texture_gl *dst_texture_gl = wined3d_texture_gl(dst_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_device *device = dst_texture->resource.device;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture *staging_texture = NULL;
    struct wined3d_glsl_blitter *glsl_blitter;
    struct wined3d_color_key alpha_test_key;
    struct glsl_blitter_program *program;
    struct wined3d_blitter *next;
    unsigned int src_level;
    GLint location;
    RECT s, d;

    TRACE("blitter %p, op %#x, context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_rect %s, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, colour_key %p, filter %s.\n",
            blitter, op, context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            wine_dbgstr_rect(src_rect), dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location),
            wine_dbgstr_rect(dst_rect), colour_key, debug_d3dtexturefiltertype(filter));

    if (!glsl_blitter_supported(op, context, src_texture_gl, src_location, dst_texture_gl, dst_location))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter);
    }

    glsl_blitter = CONTAINING_RECORD(blitter, struct wined3d_glsl_blitter, blitter);

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

    wined3d_context_gl_apply_blit_state(context_gl, device);

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
        const struct wined3d_format *f = src_texture->resource.format;

        alpha_test_key.color_space_low_value = 0;
        alpha_test_key.color_space_high_value = ~(((1u << f->alpha_size) - 1) << f->alpha_offset);
        colour_key = &alpha_test_key;
    }
    else if (op != WINED3D_BLIT_OP_COLOR_BLIT_CKEY)
    {
        colour_key = NULL;
    }

    if (!(program = glsl_blitter_get_program(glsl_blitter, context_gl, src_texture_gl, !!colour_key)))
    {
        ERR("Failed to get blitter program.\n");
        return dst_location;
    }
    GL_EXTCALL(glUseProgram(program->id));
    switch (get_complex_fixup(program->args.fixup))
    {
        case COMPLEX_FIXUP_P8:
            glsl_blitter_upload_palette(glsl_blitter, context_gl, src_texture_gl);
            break;

        case COMPLEX_FIXUP_YUY2:
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YV12:
        case COMPLEX_FIXUP_NV12:
            src_level = src_sub_resource_idx % src_texture->level_count;
            location = GL_EXTCALL(glGetUniformLocation(program->id, "size"));
            GL_EXTCALL(glUniform2f(location, wined3d_texture_get_level_pow2_width(src_texture, src_level),
                    wined3d_texture_get_level_pow2_height(src_texture, src_level)));
            break;

        default:
            break;
    }
    if (colour_key)
    {
        struct wined3d_color float_key[2];

        wined3d_format_get_float_color_key(src_texture->resource.format, colour_key, float_key);
        location = GL_EXTCALL(glGetUniformLocation(program->id, "colour_key.low"));
        GL_EXTCALL(glUniform4fv(location, 1, &float_key[0].r));
        location = GL_EXTCALL(glGetUniformLocation(program->id, "colour_key.high"));
        GL_EXTCALL(glUniform4fv(location, 1, &float_key[1].r));
    }
    wined3d_context_gl_draw_shaded_quad(context_gl, src_texture_gl, src_sub_resource_idx, src_rect, dst_rect, filter);
    GL_EXTCALL(glUseProgram(0));

    if (dst_texture->swapchain && (dst_texture->swapchain->front_buffer == dst_texture))
        gl_info->gl_ops.gl.p_glFlush();

    if (staging_texture)
        wined3d_texture_decref(staging_texture);

    return dst_location;
}

static void glsl_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
                clear_rects, draw_rect, flags, color, depth, stencil);
}

static const struct wined3d_blitter_ops glsl_blitter_ops =
{
    glsl_blitter_destroy,
    glsl_blitter_clear,
    glsl_blitter_blit,
};

struct wined3d_blitter *wined3d_glsl_blitter_create(struct wined3d_blitter **next,
        const struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_glsl_blitter *blitter;

    if (device->shader_backend != &glsl_shader_backend)
        return NULL;

    if (!gl_info->supported[ARB_VERTEX_SHADER] || !gl_info->supported[ARB_FRAGMENT_SHADER])
        return NULL;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
    {
        ERR("Failed to allocate blitter.\n");
        return NULL;
    }

    TRACE("Created blitter %p.\n", blitter);

    blitter->blitter.ops = &glsl_blitter_ops;
    blitter->blitter.next = *next;
    string_buffer_list_init(&blitter->string_buffers);
    wine_rb_init(&blitter->programs, glsl_blitter_args_compare);
    blitter->palette_texture = 0;
    *next = &blitter->blitter;

    return *next;
}