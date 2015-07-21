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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINED3D_GLSL_SAMPLE_PROJECTED   0x1
#define WINED3D_GLSL_SAMPLE_NPOT        0x2
#define WINED3D_GLSL_SAMPLE_LOD         0x4
#define WINED3D_GLSL_SAMPLE_GRAD        0x8

struct glsl_dst_param
{
    char reg_name[150];
    char mask_str[6];
};

struct glsl_src_param
{
    char reg_name[150];
    char param_str[200];
};

struct glsl_sample_function
{
    struct wined3d_string_buffer *name;
    DWORD coord_mask;
    enum wined3d_data_type data_type;
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
struct shader_glsl_priv {
    struct wined3d_string_buffer shader_buffer;
    struct wined3d_string_buffer_list string_buffers;
    struct wine_rb_tree program_lookup;
    struct constant_heap vconst_heap;
    struct constant_heap pconst_heap;
    unsigned char *stack;
    GLuint depth_blt_program_full[WINED3D_GL_RES_TYPE_COUNT];
    GLuint depth_blt_program_masked[WINED3D_GL_RES_TYPE_COUNT];
    UINT next_constant_version;

    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct fragment_pipeline *fragment_pipe;
    struct wine_rb_tree ffp_vertex_shaders;
    struct wine_rb_tree ffp_fragment_shaders;
    BOOL ffp_proj_control;
    BOOL legacy_lighting;
};

struct glsl_vs_program
{
    struct list shader_entry;
    GLuint id;
    GLenum vertex_color_clamp;
    GLint *uniform_f_locations;
    GLint uniform_i_locations[MAX_CONST_I];
    GLint uniform_b_locations[MAX_CONST_B];
    GLint pos_fixup_location;

    GLint modelview_matrix_location[MAX_VERTEX_BLENDS];
    GLint projection_matrix_location;
    GLint normal_matrix_location;
    GLint texture_matrix_location[MAX_TEXTURES];
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
    } light_location[MAX_ACTIVE_LIGHTS];
    GLint pointsize_location;
    GLint pointsize_min_location;
    GLint pointsize_max_location;
    GLint pointsize_c_att_location;
    GLint pointsize_l_att_location;
    GLint pointsize_q_att_location;
};

struct glsl_gs_program
{
    struct list shader_entry;
    GLuint id;
};

struct glsl_ps_program
{
    struct list shader_entry;
    GLuint id;
    GLint *uniform_f_locations;
    GLint uniform_i_locations[MAX_CONST_I];
    GLint uniform_b_locations[MAX_CONST_B];
    GLint bumpenv_mat_location[MAX_TEXTURES];
    GLint bumpenv_lum_scale_location[MAX_TEXTURES];
    GLint bumpenv_lum_offset_location[MAX_TEXTURES];
    GLint tss_constant_location[MAX_TEXTURES];
    GLint tex_factor_location;
    GLint specular_enable_location;
    GLint fog_color_location;
    GLint fog_density_location;
    GLint fog_end_location;
    GLint fog_scale_location;
    GLint ycorrection_location;
    GLint np2_fixup_location;
    GLint color_key_location;
    const struct ps_np2fixup_info *np2_fixup_info;
};

/* Struct to maintain data about a linked GLSL program */
struct glsl_shader_prog_link
{
    struct wine_rb_entry program_lookup_entry;
    struct glsl_vs_program vs;
    struct glsl_gs_program gs;
    struct glsl_ps_program ps;
    GLuint id;
    DWORD constant_update_mask;
    UINT constant_version;
};

struct glsl_program_key
{
    GLuint vs_id;
    GLuint gs_id;
    GLuint ps_id;
};

struct shader_glsl_ctx_priv {
    const struct vs_compile_args    *cur_vs_args;
    const struct ps_compile_args    *cur_ps_args;
    struct ps_np2fixup_info         *cur_np2fixup_info;
    struct wined3d_string_buffer_list *string_buffers;
};

struct glsl_context_data
{
    struct glsl_shader_prog_link *glsl_program;
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

struct glsl_gs_compiled_shader
{
    GLuint id;
};

struct glsl_shader_private
{
    union
    {
        struct glsl_vs_compiled_shader *vs;
        struct glsl_gs_compiled_shader *gs;
        struct glsl_ps_compiled_shader *ps;
    } gl_shaders;
    UINT num_gl_shaders, shader_array_size;
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
    const struct wined3d_gl_info *gl_info;
};

static const char *debug_gl_shader_type(GLenum type)
{
    switch (type)
    {
#define WINED3D_TO_STR(u) case u: return #u
        WINED3D_TO_STR(GL_VERTEX_SHADER);
        WINED3D_TO_STR(GL_GEOMETRY_SHADER);
        WINED3D_TO_STR(GL_FRAGMENT_SHADER);
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

        case WINED3D_SHADER_TYPE_GEOMETRY:
            return "gs";

        case WINED3D_SHADER_TYPE_PIXEL:
            return "ps";

        default:
            FIXME("Unhandled shader type %#x.\n", type);
            return "unknown";
    }
}

static const char *shader_glsl_get_version(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_version *version)
{
    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        return "#version 150";
    else if (gl_info->glsl_version >= MAKEDWORD_VERSION(1, 30) && version && version->major >= 4)
        return "#version 130";
    else
        return "#version 120";
}

static void shader_glsl_append_imm_vec4(struct wined3d_string_buffer *buffer, const float *values)
{
    char str[4][17];

    wined3d_ftoa(values[0], str[0]);
    wined3d_ftoa(values[1], str[1]);
    wined3d_ftoa(values[2], str[2]);
    wined3d_ftoa(values[3], str[3]);
    shader_addline(buffer, "vec4(%s, %s, %s, %s)", str[0], str[1], str[2], str[3]);
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
static void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLuint id, BOOL program)
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

        log = HeapAlloc(GetProcessHeap(), 0, length);
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
        HeapFree(GetProcessHeap(), 0, log);
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
    shaders = HeapAlloc(GetProcessHeap(), 0, shader_count * sizeof(*shaders));
    if (!shaders)
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
            HeapFree(GetProcessHeap(), 0, source);

            source = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tmp);
            if (!source)
            {
                ERR("Failed to allocate %d bytes for shader source.\n", tmp);
                HeapFree(GetProcessHeap(), 0, shaders);
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

    HeapFree(GetProcessHeap(), 0, source);
    HeapFree(GetProcessHeap(), 0, shaders);
}

/* Context activation is done by the caller. */
static void shader_glsl_validate_link(const struct wined3d_gl_info *gl_info, GLuint program)
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

/* Context activation is done by the caller. */
static void shader_glsl_load_samplers(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, const DWORD *tex_unit_map, GLuint program_id)
{
    unsigned int mapped_unit;
    struct wined3d_string_buffer *sampler_name = string_buffer_get(&priv->string_buffers);
    const char *prefix;
    unsigned int i, j;
    GLint name_loc;

    static const struct
    {
        enum wined3d_shader_type type;
        unsigned int base_idx;
        unsigned int count;
    }
    sampler_info[] =
    {
        {WINED3D_SHADER_TYPE_PIXEL,     0,                      MAX_FRAGMENT_SAMPLERS},
        {WINED3D_SHADER_TYPE_VERTEX,    MAX_FRAGMENT_SAMPLERS,  MAX_VERTEX_SAMPLERS},
    };

    for (i = 0; i < ARRAY_SIZE(sampler_info); ++i)
    {
        prefix = shader_glsl_get_prefix(sampler_info[i].type);

        for (j = 0; j < sampler_info[i].count; ++j)
        {
            string_buffer_sprintf(sampler_name, "%s_sampler%u", prefix, j);
            name_loc = GL_EXTCALL(glGetUniformLocation(program_id, sampler_name->buffer));
            if (name_loc == -1)
                continue;

            mapped_unit = tex_unit_map[sampler_info[i].base_idx + j];
            if (mapped_unit == WINED3D_UNMAPPED_STAGE || mapped_unit >= gl_info->limits.combined_samplers)
            {
                ERR("Trying to load sampler %s on unsupported unit %u.\n", sampler_name->buffer, mapped_unit);
                continue;
            }

            TRACE("Loading sampler %s on unit %u.\n", sampler_name->buffer, mapped_unit);
            GL_EXTCALL(glUniform1i(name_loc, mapped_unit));
        }
    }
    checkGLcall("glUniform1i");
    string_buffer_release(&priv->string_buffers, sampler_name);
}

/* Context activation is done by the caller. */
static inline void walk_constant_heap(const struct wined3d_gl_info *gl_info, const float *constants,
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
        GL_EXTCALL(glUniform4fv(constant_locations[start], end - start + 1, &constants[start * 4]));
    checkGLcall("walk_constant_heap()");
}

/* Context activation is done by the caller. */
static inline void apply_clamped_constant(const struct wined3d_gl_info *gl_info, GLint location, const GLfloat *data)
{
    GLfloat clamped_constant[4];

    if (location == -1) return;

    clamped_constant[0] = data[0] < -1.0f ? -1.0f : data[0] > 1.0f ? 1.0f : data[0];
    clamped_constant[1] = data[1] < -1.0f ? -1.0f : data[1] > 1.0f ? 1.0f : data[1];
    clamped_constant[2] = data[2] < -1.0f ? -1.0f : data[2] > 1.0f ? 1.0f : data[2];
    clamped_constant[3] = data[3] < -1.0f ? -1.0f : data[3] > 1.0f ? 1.0f : data[3];

    GL_EXTCALL(glUniform4fv(location, 1, clamped_constant));
}

/* Context activation is done by the caller. */
static inline void walk_constant_heap_clamped(const struct wined3d_gl_info *gl_info, const float *constants,
        const GLint *constant_locations, const struct constant_heap *heap, unsigned char *stack, DWORD version)
{
    int stack_idx = 0;
    unsigned int heap_idx = 1;
    unsigned int idx;

    if (heap->entries[heap_idx].version <= version) return;

    idx = heap->entries[heap_idx].idx;
    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx * 4]);
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
                    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx * 4]);

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
                    apply_clamped_constant(gl_info, constant_locations[idx], &constants[idx * 4]);

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

/* Context activation is done by the caller. */
static void shader_glsl_load_constantsF(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        const float *constants, const GLint *constant_locations, const struct constant_heap *heap,
        unsigned char *stack, UINT version)
{
    const struct wined3d_shader_lconst *lconst;

    /* 1.X pshaders have the constants clamped to [-1;1] implicitly. */
    if (shader->reg_maps.shader_version.major == 1
            && shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
        walk_constant_heap_clamped(gl_info, constants, constant_locations, heap, stack, version);
    else
        walk_constant_heap(gl_info, constants, constant_locations, heap, stack, version);

    if (!shader->load_local_constsF)
    {
        TRACE("No need to load local float constants for this shader\n");
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
static void shader_glsl_load_constantsI(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        const GLint locations[MAX_CONST_I], const int *constants, WORD constants_set)
{
    unsigned int i;
    struct list* ptr;

    for (i = 0; constants_set; constants_set >>= 1, ++i)
    {
        if (!(constants_set & 1)) continue;

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4iv(locations[i], 1, &constants[i * 4]));
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
        const GLint locations[MAX_CONST_B], const BOOL *constants, WORD constants_set)
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
    GLfloat np2fixup_constants[4 * MAX_FRAGMENT_SAMPLERS];
    UINT fixup = ps->np2_fixup_info->active;
    UINT i;

    for (i = 0; fixup; fixup >>= 1, ++i)
    {
        const struct wined3d_texture *tex = state->textures[i];
        unsigned char idx = ps->np2_fixup_info->idx[i];
        GLfloat *tex_dim = &np2fixup_constants[(idx >> 1) * 4];

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

    GL_EXTCALL(glUniform4fv(ps->np2_fixup_location, ps->np2_fixup_info->num_consts, np2fixup_constants));
}

/* Taken and adapted from Mesa. */
static BOOL invert_matrix_3d(struct wined3d_matrix *out, const struct wined3d_matrix *in)
{
    float pos, neg, t, det;
    struct wined3d_matrix temp;

    /* Calculate the determinant of upper left 3x3 submatrix and
     * determine if the matrix is singular. */
    pos = neg = 0.0f;
    t =  in->_11 * in->_22 * in->_33;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t =  in->_21 * in->_32 * in->_13;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;
    t =  in->_31 * in->_12 * in->_23;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t = -in->_31 * in->_22 * in->_13;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;
    t = -in->_21 * in->_12 * in->_33;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t = -in->_11 * in->_32 * in->_23;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    det = pos + neg;

    if (fabsf(det) < 1e-25f)
        return FALSE;

    det = 1.0f / det;
    temp._11 =  (in->_22 * in->_33 - in->_32 * in->_23) * det;
    temp._12 = -(in->_12 * in->_33 - in->_32 * in->_13) * det;
    temp._13 =  (in->_12 * in->_23 - in->_22 * in->_13) * det;
    temp._21 = -(in->_21 * in->_33 - in->_31 * in->_23) * det;
    temp._22 =  (in->_11 * in->_33 - in->_31 * in->_13) * det;
    temp._23 = -(in->_11 * in->_23 - in->_21 * in->_13) * det;
    temp._31 =  (in->_21 * in->_32 - in->_31 * in->_22) * det;
    temp._32 = -(in->_11 * in->_32 - in->_31 * in->_12) * det;
    temp._33 =  (in->_11 * in->_22 - in->_21 * in->_12) * det;

    *out = temp;
    return TRUE;
}

static void swap_rows(float **a, float **b)
{
    float *tmp = *a;

    *a = *b;
    *b = tmp;
}

static BOOL invert_matrix(struct wined3d_matrix *out, struct wined3d_matrix *m)
{
    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;

    r0 = wtmp[0];
    r1 = wtmp[1];
    r2 = wtmp[2];
    r3 = wtmp[3];

    r0[0] = m->_11;
    r0[1] = m->_12;
    r0[2] = m->_13;
    r0[3] = m->_14;
    r0[4] = 1.0f;
    r0[5] = r0[6] = r0[7] = 0.0f;

    r1[0] = m->_21;
    r1[1] = m->_22;
    r1[2] = m->_23;
    r1[3] = m->_24;
    r1[5] = 1.0f;
    r1[4] = r1[6] = r1[7] = 0.0f;

    r2[0] = m->_31;
    r2[1] = m->_32;
    r2[2] = m->_33;
    r2[3] = m->_34;
    r2[6] = 1.0f;
    r2[4] = r2[5] = r2[7] = 0.0f;

    r3[0] = m->_41;
    r3[1] = m->_42;
    r3[2] = m->_43;
    r3[3] = m->_44;
    r3[7] = 1.0f;
    r3[4] = r3[5] = r3[6] = 0.0f;

    /* Choose pivot - or die. */
    if (fabsf(r3[0]) > fabsf(r2[0]))
        swap_rows(&r3, &r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        swap_rows(&r2, &r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        swap_rows(&r1, &r0);
    if (r0[0] == 0.0f)
        return FALSE;

    /* Eliminate first variable. */
    m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
    s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
    s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
    s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0f)
    {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0f)
    {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0f)
    {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0f)
    {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* Choose pivot - or die. */
    if (fabsf(r3[1]) > fabsf(r2[1]))
        swap_rows(&r3, &r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        swap_rows(&r2, &r1);
    if (r1[1] == 0.0f)
        return FALSE;

    /* Eliminate second variable. */
    m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
    s = r1[4];
    if (s != 0.0f)
    {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (s != 0.0f)
    {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (s != 0.0f)
    {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (s != 0.0f)
    {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* Choose pivot - or die. */
    if (fabsf(r3[2]) > fabsf(r2[2]))
        swap_rows(&r3, &r2);
    if (r2[2] == 0.0f)
        return FALSE;

    /* Eliminate third variable. */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3];
    r3[4] -= m3 * r2[4];
    r3[5] -= m3 * r2[5];
    r3[6] -= m3 * r2[6];
    r3[7] -= m3 * r2[7];

    /* Last check. */
    if (r3[3] == 0.0f)
        return FALSE;

    /* Back substitute row 3. */
    s = 1.0f / r3[3];
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    /* Back substitute row 2. */
    m2 = r2[3];
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2);
    r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2);
    r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1;
    r1[5] -= r3[5] * m1;
    r1[6] -= r3[6] * m1;
    r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0;
    r0[5] -= r3[5] * m0;
    r0[6] -= r3[6] * m0;
    r0[7] -= r3[7] * m0;

    /* Back substitute row 1. */
    m1 = r1[2];
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1);
    r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1);
    r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0;
    r0[5] -= r2[5] * m0;
    r0[6] -= r2[6] * m0;
    r0[7] -= r2[7] * m0;

    /* Back substitute row 0. */
    m0 = r0[1];
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0);
    r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0);
    r0[7] = s * (r0[7] - r1[7] * m0);

    out->_11 = r0[4];
    out->_12 = r0[5];
    out->_13 = r0[6];
    out->_14 = r0[7];
    out->_21 = r1[4];
    out->_22 = r1[5];
    out->_23 = r1[6];
    out->_24 = r1[7];
    out->_31 = r2[4];
    out->_32 = r2[5];
    out->_33 = r2[6];
    out->_34 = r2[7];
    out->_41 = r3[4];
    out->_42 = r3[5];
    out->_43 = r3[6];
    out->_44 = r3[7];

    return TRUE;
}

static void shader_glsl_ffp_vertex_normalmatrix_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    float mat[3 * 3];
    struct wined3d_matrix mv;
    unsigned int i, j;

    if (prog->vs.normal_matrix_location == -1)
        return;

    get_modelview_matrix(context, state, 0, &mv);
    if (context->swapchain->device->wined3d->flags & WINED3D_LEGACY_FFP_LIGHTING)
        invert_matrix_3d(&mv, &mv);
    else
        invert_matrix(&mv, &mv);
    /* Tests show that singular modelview matrices are used unchanged as normal
     * matrices on D3D3 and older. There seems to be no clearly consistent
     * behavior on newer D3D versions so always follow older ddraw behavior. */
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            mat[i * 3 + j] = (&mv._11)[j * 4 + i];

    GL_EXTCALL(glUniformMatrix3fv(prog->vs.normal_matrix_location, 1, FALSE, mat));
    checkGLcall("glUniformMatrix3fv");
}

static void shader_glsl_ffp_vertex_texmatrix_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, unsigned int tex, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_matrix mat;

    if (tex >= MAX_TEXTURES)
        return;
    if (prog->vs.texture_matrix_location[tex] == -1)
        return;

    get_texture_matrix(context, state, tex, &mat);
    GL_EXTCALL(glUniformMatrix4fv(prog->vs.texture_matrix_location[tex], 1, FALSE, &mat._11));
    checkGLcall("glUniformMatrix4fv");
}

static void shader_glsl_ffp_vertex_material_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

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

static void shader_glsl_ffp_vertex_lightambient_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    float col[4];

    D3DCOLORTOGLFLOAT4(state->render_states[WINED3D_RS_AMBIENT], col);
    GL_EXTCALL(glUniform3fv(prog->vs.light_ambient_location, 1, col));
    checkGLcall("glUniform3fv");
}

static void multiply_vector_matrix(struct wined3d_vec4 *dest, const struct wined3d_vec4 *src1,
        const struct wined3d_matrix *src2)
{
    struct wined3d_vec4 temp;

    temp.x = (src1->x * src2->_11) + (src1->y * src2->_21) + (src1->z * src2->_31) + (src1->w * src2->_41);
    temp.y = (src1->x * src2->_12) + (src1->y * src2->_22) + (src1->z * src2->_32) + (src1->w * src2->_42);
    temp.z = (src1->x * src2->_13) + (src1->y * src2->_23) + (src1->z * src2->_33) + (src1->w * src2->_43);
    temp.w = (src1->x * src2->_14) + (src1->y * src2->_24) + (src1->z * src2->_34) + (src1->w * src2->_44);

    *dest = temp;
}

static void shader_glsl_ffp_vertex_light_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, unsigned int light, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_light_info *light_info = state->lights[light];
    struct wined3d_vec4 vec4;
    const struct wined3d_matrix *view = &state->transforms[WINED3D_TS_VIEW];

    if (!light_info)
        return;

    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].diffuse, 1, &light_info->OriginalParms.diffuse.r));
    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].specular, 1, &light_info->OriginalParms.specular.r));
    GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].ambient, 1, &light_info->OriginalParms.ambient.r));

    switch (light_info->OriginalParms.type)
    {
        case WINED3D_LIGHT_POINT:
            multiply_vector_matrix(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].range, light_info->OriginalParms.range));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].c_att, light_info->OriginalParms.attenuation0));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].l_att, light_info->OriginalParms.attenuation1));
            GL_EXTCALL(glUniform1f(prog->vs.light_location[light].q_att, light_info->OriginalParms.attenuation2));
            break;

        case WINED3D_LIGHT_SPOT:
            multiply_vector_matrix(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));

            multiply_vector_matrix(&vec4, &light_info->direction, view);
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
            multiply_vector_matrix(&vec4, &light_info->direction, view);
            GL_EXTCALL(glUniform3fv(prog->vs.light_location[light].direction, 1, &vec4.x));
            break;

        case WINED3D_LIGHT_PARALLELPOINT:
            multiply_vector_matrix(&vec4, &light_info->position, view);
            GL_EXTCALL(glUniform4fv(prog->vs.light_location[light].position, 1, &vec4.x));
            break;

        default:
            FIXME("Unrecognized light type %#x.\n", light_info->OriginalParms.type);
    }
    checkGLcall("setting FFP lights uniforms");
}

static void shader_glsl_pointsize_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    float min, max;
    float size, att[3];

    get_pointsize_minmax(context, state, &min, &max);

    GL_EXTCALL(glUniform1f(prog->vs.pointsize_min_location, min));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_max_location, max));
    checkGLcall("glUniform1f");

    get_pointsize(context, state, &size, att);

    GL_EXTCALL(glUniform1f(prog->vs.pointsize_location, size));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_c_att_location, att[0]));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_l_att_location, att[1]));
    checkGLcall("glUniform1f");
    GL_EXTCALL(glUniform1f(prog->vs.pointsize_q_att_location, att[2]));
    checkGLcall("glUniform1f");
}

static void shader_glsl_load_fog_uniform(const struct wined3d_context *context,
        const struct wined3d_state *state, struct glsl_shader_prog_link *prog)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    float start, end, scale;
    union
    {
        DWORD d;
        float f;
    } tmpvalue;
    float col[4];

    D3DCOLORTOGLFLOAT4(state->render_states[WINED3D_RS_FOGCOLOR], col);
    GL_EXTCALL(glUniform4fv(prog->ps.fog_color_location, 1, col));
    tmpvalue.d = state->render_states[WINED3D_RS_FOGDENSITY];
    GL_EXTCALL(glUniform1f(prog->ps.fog_density_location, tmpvalue.f));
    get_fog_start_end(context, state, &start, &end);
    scale = 1.0f / (end - start);
    GL_EXTCALL(glUniform1f(prog->ps.fog_end_location, end));
    GL_EXTCALL(glUniform1f(prog->ps.fog_scale_location, scale));
    checkGLcall("fog emulation uniforms");
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_color_key_constant(const struct glsl_ps_program *ps,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    struct wined3d_color float_key;
    const struct wined3d_texture *texture = state->textures[0];

    wined3d_format_convert_color_to_float(texture->resource.format, NULL,
            texture->async.src_blt_color_key.color_space_high_value, &float_key);
    GL_EXTCALL(glUniform4fv(ps->color_key_location, 1, &float_key.r));
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_constants(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    const struct glsl_context_data *ctx_data = context->shader_backend_data;
    const struct wined3d_shader *vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    const struct wined3d_shader *pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct shader_glsl_priv *priv = shader_priv;
    float position_fixup[4];
    DWORD update_mask = 0;

    struct glsl_shader_prog_link *prog = ctx_data->glsl_program;
    UINT constant_version;
    int i;

    if (!prog) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    constant_version = prog->constant_version;
    update_mask = context->constant_update_mask & prog->constant_update_mask;

    if (update_mask & WINED3D_SHADER_CONST_VS_F)
        shader_glsl_load_constantsF(vshader, gl_info, state->vs_consts_f,
                prog->vs.uniform_f_locations, &priv->vconst_heap, priv->stack, constant_version);

    if (update_mask & WINED3D_SHADER_CONST_VS_I)
        shader_glsl_load_constantsI(vshader, gl_info, prog->vs.uniform_i_locations, state->vs_consts_i,
                vshader->reg_maps.integer_constants);

    if (update_mask & WINED3D_SHADER_CONST_VS_B)
        shader_glsl_load_constantsB(vshader, gl_info, prog->vs.uniform_b_locations, state->vs_consts_b,
                vshader->reg_maps.boolean_constants);

    if (update_mask & WINED3D_SHADER_CONST_VS_POINTSIZE)
        shader_glsl_pointsize_uniform(context, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_VS_POS_FIXUP)
    {
        shader_get_position_fixup(context, state, position_fixup);
        GL_EXTCALL(glUniform4fv(prog->vs.pos_fixup_location, 1, position_fixup));
        checkGLcall("glUniform4fv");
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_MODELVIEW)
    {
        struct wined3d_matrix mat;

        get_modelview_matrix(context, state, 0, &mat);
        GL_EXTCALL(glUniformMatrix4fv(prog->vs.modelview_matrix_location[0], 1, FALSE, &mat._11));
        checkGLcall("glUniformMatrix4fv");

        shader_glsl_ffp_vertex_normalmatrix_uniform(context, state, prog);
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_VERTEXBLEND)
    {
        struct wined3d_matrix mat;

        for (i = 1; i < MAX_VERTEX_BLENDS; ++i)
        {
            if (prog->vs.modelview_matrix_location[i] == -1)
                break;

            get_modelview_matrix(context, state, i, &mat);
            GL_EXTCALL(glUniformMatrix4fv(prog->vs.modelview_matrix_location[i], 1, FALSE, &mat._11));
            checkGLcall("glUniformMatrix4fv");
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
        for (i = 0; i < MAX_TEXTURES; ++i)
            shader_glsl_ffp_vertex_texmatrix_uniform(context, state, i, prog);
    }

    if (update_mask & WINED3D_SHADER_CONST_FFP_MATERIAL)
        shader_glsl_ffp_vertex_material_uniform(context, state, prog);

    if (update_mask & WINED3D_SHADER_CONST_FFP_LIGHTS)
    {
        shader_glsl_ffp_vertex_lightambient_uniform(context, state, prog);
        for (i = 0; i < MAX_ACTIVE_LIGHTS; ++i)
            shader_glsl_ffp_vertex_light_uniform(context, state, i, prog);
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_F)
        shader_glsl_load_constantsF(pshader, gl_info, state->ps_consts_f,
                prog->ps.uniform_f_locations, &priv->pconst_heap, priv->stack, constant_version);

    if (update_mask & WINED3D_SHADER_CONST_PS_I)
        shader_glsl_load_constantsI(pshader, gl_info, prog->ps.uniform_i_locations, state->ps_consts_i,
                pshader->reg_maps.integer_constants);

    if (update_mask & WINED3D_SHADER_CONST_PS_B)
        shader_glsl_load_constantsB(pshader, gl_info, prog->ps.uniform_b_locations, state->ps_consts_b,
                pshader->reg_maps.boolean_constants);

    if (update_mask & WINED3D_SHADER_CONST_PS_BUMP_ENV)
    {
        for (i = 0; i < MAX_TEXTURES; ++i)
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
        float correction_params[] =
        {
            /* position is window relative, not viewport relative */
            context->render_offscreen ? 0.0f : (float)context->current_rt->resource.height,
            context->render_offscreen ? 1.0f : -1.0f,
            0.0f,
            0.0f,
        };

        GL_EXTCALL(glUniform4fv(prog->ps.ycorrection_location, 1, correction_params));
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_NP2_FIXUP)
        shader_glsl_load_np2fixup_constants(&prog->ps, gl_info, state);
    if (update_mask & WINED3D_SHADER_CONST_FFP_COLOR_KEY)
        shader_glsl_load_color_key_constant(&prog->ps, gl_info, state);

    if (update_mask & WINED3D_SHADER_CONST_FFP_PS)
    {
        float col[4];

        if (prog->ps.tex_factor_location != -1)
        {
            D3DCOLORTOGLFLOAT4(state->render_states[WINED3D_RS_TEXTUREFACTOR], col);
            GL_EXTCALL(glUniform4fv(prog->ps.tex_factor_location, 1, col));
        }

        if (state->render_states[WINED3D_RS_SPECULARENABLE])
            GL_EXTCALL(glUniform4f(prog->ps.specular_enable_location, 1.0f, 1.0f, 1.0f, 0.0f));
        else
            GL_EXTCALL(glUniform4f(prog->ps.specular_enable_location, 0.0f, 0.0f, 0.0f, 0.0f));

        for (i = 0; i < MAX_TEXTURES; ++i)
        {
            if (prog->ps.tss_constant_location[i] == -1)
                continue;

            D3DCOLORTOGLFLOAT4(state->texture_states[i][WINED3D_TSS_CONSTANT], col);
            GL_EXTCALL(glUniform4fv(prog->ps.tss_constant_location[i], 1, col));
        }

        checkGLcall("fixed function uniforms");
    }

    if (update_mask & WINED3D_SHADER_CONST_PS_FOG)
        shader_glsl_load_fog_uniform(context, state, prog);

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

    for (i = start; i < count + start; ++i)
    {
        update_heap_entry(heap, i, priv->next_constant_version);
    }

    for (i = 0; i < device->context_count; ++i)
    {
        device->contexts[i]->constant_update_mask |= WINED3D_SHADER_CONST_VS_F;
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

    for (i = 0; i < device->context_count; ++i)
    {
        device->contexts[i]->constant_update_mask |= WINED3D_SHADER_CONST_PS_F;
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

/** Generate the variable & register declarations for the GLSL output target */
static void shader_generate_glsl_declarations(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, const struct wined3d_shader *shader,
        const struct wined3d_shader_reg_maps *reg_maps, const struct shader_glsl_ctx_priv *ctx_priv)
{
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
#if defined(STAGING_CSMT)
    const struct vs_compile_args *vs_args = ctx_priv->cur_vs_args;
    const struct ps_compile_args *ps_args = ctx_priv->cur_ps_args;
    const struct wined3d_gl_info *gl_info = context->gl_info;
#else  /* STAGING_CSMT */
    const struct wined3d_state *state = &shader->device->state;
    const struct vs_compile_args *vs_args = ctx_priv->cur_vs_args;
    const struct ps_compile_args *ps_args = ctx_priv->cur_ps_args;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_fb_state *fb = &shader->device->fb;
#endif /* STAGING_CSMT */
    unsigned int i, extra_constants_needed = 0;
    const struct wined3d_shader_lconst *lconst;
    const char *prefix;
    DWORD map;

    prefix = shader_glsl_get_prefix(version->type);

    /* Prototype the subroutines */
    for (i = 0, map = reg_maps->labels; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "void subroutine%u();\n", i);
    }

    /* Declare the constants (aka uniforms) */
    if (shader->limits->constant_float > 0)
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
                if(ctx_priv->cur_vs_args->clip_enabled)
                {
                    max_constantsF -= gl_info->limits.clipplanes;
                }
                max_constantsF -= count_bits(reg_maps->integer_constants);
                /* Strictly speaking a bool only uses one scalar, but the nvidia(Linux) compiler doesn't pack them properly,
                 * so each scalar requires a full vec4. We could work around this by packing the booleans ourselves, but
                 * for now take this into account when calculating the number of available constants
                 */
                max_constantsF -= count_bits(reg_maps->boolean_constants);
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
                max_constantsF = gl_info->limits.glsl_vs_float_constants;
            }
        }
        max_constantsF = min(shader->limits->constant_float, max_constantsF);
        shader_addline(buffer, "uniform vec4 %s_c[%u];\n", prefix, max_constantsF);
    }

    /* Always declare the full set of constants, the compiler can remove the
     * unused ones because d3d doesn't (yet) support indirect int and bool
     * constant addressing. This avoids problems if the app uses e.g. i0 and i9. */
    if (shader->limits->constant_int > 0 && reg_maps->integer_constants)
        shader_addline(buffer, "uniform ivec4 %s_i[%u];\n", prefix, shader->limits->constant_int);

    if (shader->limits->constant_bool > 0 && reg_maps->boolean_constants)
        shader_addline(buffer, "uniform bool %s_b[%u];\n", prefix, shader->limits->constant_bool);

    for (i = 0; i < WINED3D_MAX_CBS; ++i)
    {
        if (reg_maps->cb_sizes[i])
            shader_addline(buffer, "layout(std140) uniform block_%s_cb%u { vec4 %s_cb%u[%u]; };\n",
                    prefix, i, prefix, i, reg_maps->cb_sizes[i]);
    }

    /* Declare texture samplers */
    for (i = 0; i < reg_maps->sampler_map.count; ++i)
    {
        struct wined3d_shader_sampler_map_entry *entry;
        BOOL shadow_sampler, tex_rect;
        const char *sampler_type;

        entry = &reg_maps->sampler_map.entries[i];

        if (entry->resource_idx >= ARRAY_SIZE(reg_maps->resource_info))
        {
            ERR("Invalid resource index %u.\n", entry->resource_idx);
            continue;
        }

        shadow_sampler = version->type == WINED3D_SHADER_TYPE_PIXEL && (ps_args->shadow & (1 << entry->sampler_idx));
        switch (reg_maps->resource_info[entry->resource_idx].type)
        {
            case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
                if (shadow_sampler)
                    sampler_type = "sampler1DShadow";
                else
                    sampler_type = "sampler1D";
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                tex_rect = version->type == WINED3D_SHADER_TYPE_PIXEL
                        && (ps_args->np2_fixup & (1 << entry->resource_idx))
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
                    FIXME("Unsupported Cube shadow sampler.\n");
                sampler_type = "samplerCube";
                break;

            default:
                sampler_type = "unsupported_sampler";
                FIXME("Unhandled resource type %#x.\n", reg_maps->resource_info[i].type);
                break;
        }
        shader_addline(buffer, "uniform %s %s_sampler%u;\n", sampler_type, prefix, entry->bind_idx);
    }

    /* Declare uniforms for NP2 texcoord fixup:
     * This is NOT done inside the loop that declares the texture samplers
     * since the NP2 fixup code is currently only used for the GeforceFX
     * series and when forcing the ARB_npot extension off. Modern cards just
     * skip the code anyway, so put it inside a separate loop. */
    if (version->type == WINED3D_SHADER_TYPE_PIXEL && ps_args->np2_fixup)
    {
        struct ps_np2fixup_info *fixup = ctx_priv->cur_np2fixup_info;
        UINT cur = 0;

        /* NP2/RECT textures in OpenGL use texcoords in the range [0,width]x[0,height]
         * while D3D has them in the (normalized) [0,1]x[0,1] range.
         * samplerNP2Fixup stores texture dimensions and is updated through
         * shader_glsl_load_np2fixup_constants when the sampler changes. */

        for (i = 0; i < shader->limits->sampler; ++i)
        {
            if (!reg_maps->resource_info[i].type || !(ps_args->np2_fixup & (1 << i)))
                continue;

            if (reg_maps->resource_info[i].type != WINED3D_SHADER_RESOURCE_TEXTURE_2D)
            {
                FIXME("Non-2D texture is flagged for NP2 texcoord fixup.\n");
                continue;
            }

            fixup->idx[i] = cur++;
        }

        fixup->num_consts = (cur + 1) >> 1;
        fixup->active = ps_args->np2_fixup;
        shader_addline(buffer, "uniform vec4 %s_samplerNP2Fixup[%u];\n", prefix, fixup->num_consts);
    }

    /* Declare address variables */
    for (i = 0, map = reg_maps->address; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "ivec4 A%u;\n", i);
    }

    /* Declare texture coordinate temporaries and initialize them */
    for (i = 0, map = reg_maps->texcoord; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "vec4 T%u = gl_TexCoord[%u];\n", i, i);
    }

    if (version->type == WINED3D_SHADER_TYPE_VERTEX)
    {
        for (i = 0; i < shader->input_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *e = &shader->input_signature.elements[i];
            if (e->sysval_semantic == WINED3D_SV_INSTANCEID)
                shader_addline(buffer, "vec4 %s_in%u = vec4(intBitsToFloat(gl_InstanceID), 0.0, 0.0, 0.0);\n",
                        prefix, e->register_idx);
            else
                shader_addline(buffer, "attribute vec4 %s_in%u;\n", prefix, e->register_idx);
        }

        if (vs_args->point_size && !vs_args->per_vertex_point_size)
        {
            shader_addline(buffer, "uniform struct\n{\n");
            shader_addline(buffer, "    float size;\n");
            shader_addline(buffer, "    float size_min;\n");
            shader_addline(buffer, "    float size_max;\n");
            shader_addline(buffer, "} ffp_point;\n");
        }

        shader_addline(buffer, "uniform vec4 posFixup;\n");
        shader_addline(buffer, "void order_ps_input(in vec4[%u]);\n", shader->limits->packed_output);
    }
    else if (version->type == WINED3D_SHADER_TYPE_GEOMETRY)
    {
        shader_addline(buffer, "varying in vec4 gs_in[][%u];\n", shader->limits->packed_input);
    }
    else if (version->type == WINED3D_SHADER_TYPE_PIXEL)
    {
        if (version->major < 3 || ps_args->vp_mode != vertexshader)
        {
            shader_addline(buffer, "uniform struct\n{\n");
            shader_addline(buffer, "    vec4 color;\n");
            shader_addline(buffer, "    float density;\n");
            shader_addline(buffer, "    float end;\n");
            shader_addline(buffer, "    float scale;\n");
            shader_addline(buffer, "} ffp_fog;\n");
        }

        if (version->major >= 3)
        {
            UINT in_count = min(vec4_varyings(version->major, gl_info), shader->limits->packed_input);

#if defined(STAGING_CSMT)
            if (ps_args->vp_mode == vertexshader)
#else  /* STAGING_CSMT */
            if (use_vs(state))
#endif /* STAGING_CSMT */
                shader_addline(buffer, "varying vec4 %s_link[%u];\n", prefix, in_count);
            shader_addline(buffer, "vec4 %s_in[%u];\n", prefix, in_count);
        }

        for (i = 0, map = reg_maps->bumpmat; map; map >>= 1, ++i)
        {
            if (!(map & 1))
                continue;

            shader_addline(buffer, "uniform mat2 bumpenv_mat%u;\n", i);

            if (reg_maps->luminanceparams & (1 << i))
            {
                shader_addline(buffer, "uniform float bumpenv_lum_scale%u;\n", i);
                shader_addline(buffer, "uniform float bumpenv_lum_offset%u;\n", i);
                extra_constants_needed++;
            }

            extra_constants_needed++;
        }

        if (ps_args->srgb_correction)
        {
            shader_addline(buffer, "const vec4 srgb_const0 = ");
            shader_glsl_append_imm_vec4(buffer, wined3d_srgb_const0);
            shader_addline(buffer, ";\n");
            shader_addline(buffer, "const vec4 srgb_const1 = ");
            shader_glsl_append_imm_vec4(buffer, wined3d_srgb_const1);
            shader_addline(buffer, ";\n");
        }
        if (reg_maps->vpos || reg_maps->usesdsy)
        {
            if (shader->limits->constant_float + extra_constants_needed
                    + 1 < gl_info->limits.glsl_ps_float_constants)
            {
                shader_addline(buffer, "uniform vec4 ycorrection;\n");
                extra_constants_needed++;
            }
            else
            {
#if defined(STAGING_CSMT)
                /* This happens because we do not have proper tracking of the
                 * constant registers that are actually used, only the max
                 * limit of the shader version.
                 *
                 * FIXME 2: This is wrong, there's no need to do this. Get rid of
                 * it and just create the uniform.
                 */
                FIXME("Cannot find a free uniform for vpos correction params\n");
#else  /* STAGING_CSMT */
                float ycorrection[] =
                {
                    context->render_offscreen ? 0.0f : fb->render_targets[0]->height,
                    context->render_offscreen ? 1.0f : -1.0f,
                    0.0f,
                    0.0f,
                };

                /* This happens because we do not have proper tracking of the
                 * constant registers that are actually used, only the max
                 * limit of the shader version. */
                FIXME("Cannot find a free uniform for vpos correction params\n");
                shader_addline(buffer, "const vec4 ycorrection = ");
                shader_glsl_append_imm_vec4(buffer, ycorrection);
                shader_addline(buffer, ";\n");
#endif /* STAGING_CSMT */
            }
            shader_addline(buffer, "vec4 vpos;\n");
        }
    }

    /* Declare output register temporaries */
    if (shader->limits->packed_output)
        shader_addline(buffer, "vec4 %s_out[%u];\n", prefix, shader->limits->packed_output);

    /* Declare temporary variables */
    for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "vec4 R%u;\n", i);
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
            shader_glsl_append_imm_vec4(buffer, (const float *)lconst->value);
            shader_addline(buffer, ";\n");
        }
    }

    /* Start the main program. */
    shader_addline(buffer, "void main()\n{\n");

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
    if (version->type == WINED3D_SHADER_TYPE_PIXEL && reg_maps->vpos)
    {
        if (shader->device->wined3d->flags & WINED3D_PIXEL_CENTER_INTEGER)
            shader_addline(buffer,
                    "vpos = floor(vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1));\n");
        else
            shader_addline(buffer,
                    "vpos = vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1);\n");
    }
}

/*****************************************************************************
 * Functions to generate GLSL strings from DirectX Shader bytecode begin here.
 *
 * For more information, see http://wiki.winehq.org/DirectX-Shaders
 ****************************************************************************/

/* Prototypes */
static void shader_glsl_add_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, struct glsl_src_param *glsl_src);

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
    out_str[0] = 0;

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

/** Writes the GLSL variable name that corresponds to the register that the
 * DX opcode parameter is trying to access */
static void shader_glsl_get_register_name(const struct wined3d_shader_register *reg,
        char *register_name, BOOL *is_color, const struct wined3d_shader_instruction *ins)
{
    /* oPos, oFog and oPts in D3D */
    static const char * const hwrastout_reg_names[] = {"vs_out[10]", "vs_out[11].x", "vs_out[11].y"};

    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    const char *prefix = shader_glsl_get_prefix(version->type);
    struct glsl_src_param rel_param0, rel_param1;
    char imm_str[4][17];

    if (reg->idx[0].offset != ~0U && reg->idx[0].rel_addr)
        shader_glsl_add_src_param(ins, reg->idx[0].rel_addr, WINED3DSP_WRITEMASK_0, &rel_param0);
    if (reg->idx[1].offset != ~0U && reg->idx[1].rel_addr)
        shader_glsl_add_src_param(ins, reg->idx[1].rel_addr, WINED3DSP_WRITEMASK_0, &rel_param1);
    *is_color = FALSE;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            sprintf(register_name, "R%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_INPUT:
            /* vertex shaders */
            if (version->type == WINED3D_SHADER_TYPE_VERTEX)
            {
                struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
                if (priv->cur_vs_args->swizzle_map & (1 << reg->idx[0].offset))
                    *is_color = TRUE;
                sprintf(register_name, "%s_in%u", prefix, reg->idx[0].offset);
                break;
            }

            if (version->type == WINED3D_SHADER_TYPE_GEOMETRY)
            {
                if (reg->idx[0].rel_addr)
                {
                    if (reg->idx[1].rel_addr)
                        sprintf(register_name, "gs_in[%s + %u][%s + %u]",
                                rel_param0.param_str, reg->idx[0].offset, rel_param1.param_str, reg->idx[1].offset);
                    else
                        sprintf(register_name, "gs_in[%s + %u][%u]",
                                rel_param0.param_str, reg->idx[0].offset, reg->idx[1].offset);
                }
                else if (reg->idx[1].rel_addr)
                    sprintf(register_name, "gs_in[%u][%s + %u]",
                            reg->idx[0].offset, rel_param1.param_str, reg->idx[1].offset);
                else
                    sprintf(register_name, "gs_in[%u][%u]", reg->idx[0].offset, reg->idx[1].offset);
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
                        if (shader->u.ps.declared_in_count > in_count)
                        {
                            sprintf(register_name,
                                    "((%s + %u) > %u ? (%s + %u) > %u ? gl_SecondaryColor : gl_Color : %s_in[%s + %u])",
                                    rel_param0.param_str, idx, in_count - 1, rel_param0.param_str, idx, in_count,
                                    prefix, rel_param0.param_str, idx);
                        }
                        else
                        {
                            sprintf(register_name, "%s_in[%s + %u]", prefix, rel_param0.param_str, idx);
                        }
                    }
                    else
                    {
                        if (shader->u.ps.declared_in_count > in_count)
                        {
                            sprintf(register_name, "((%s) > %u ? (%s) > %u ? gl_SecondaryColor : gl_Color : %s_in[%s])",
                                    rel_param0.param_str, in_count - 1, rel_param0.param_str, in_count,
                                    prefix, rel_param0.param_str);
                        }
                        else
                        {
                            sprintf(register_name, "%s_in[%s]", prefix, rel_param0.param_str);
                        }
                    }
                }
                else
                {
                    if (idx == in_count) sprintf(register_name, "gl_Color");
                    else if (idx == in_count + 1) sprintf(register_name, "gl_SecondaryColor");
                    else sprintf(register_name, "%s_in[%u]", prefix, idx);
                }
            }
            else
            {
                if (!reg->idx[0].offset)
                    strcpy(register_name, "gl_Color");
                else
                    strcpy(register_name, "gl_SecondaryColor");
                break;
            }
            break;

        case WINED3DSPR_CONST:
            {
                /* Relative addressing */
                if (reg->idx[0].rel_addr)
                {
                    if (reg->idx[0].offset)
                        sprintf(register_name, "%s_c[%s + %u]", prefix, rel_param0.param_str, reg->idx[0].offset);
                    else
                        sprintf(register_name, "%s_c[%s]", prefix, rel_param0.param_str);
                }
                else
                {
                    if (shader_constant_is_local(shader, reg->idx[0].offset))
                        sprintf(register_name, "%s_lc%u", prefix, reg->idx[0].offset);
                    else
                        sprintf(register_name, "%s_c[%u]", prefix, reg->idx[0].offset);
                }
            }
            break;

        case WINED3DSPR_CONSTINT:
            sprintf(register_name, "%s_i[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_CONSTBOOL:
            sprintf(register_name, "%s_b[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
            if (version->type == WINED3D_SHADER_TYPE_PIXEL)
                sprintf(register_name, "T%u", reg->idx[0].offset);
            else
                sprintf(register_name, "A%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_LOOP:
            sprintf(register_name, "aL%u", ins->ctx->loop_state->current_reg - 1);
            break;

        case WINED3DSPR_SAMPLER:
            sprintf(register_name, "%s_sampler%u", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_COLOROUT:
            if (reg->idx[0].offset >= gl_info->limits.buffers)
                WARN("Write to render target %u, only %d supported.\n",
                        reg->idx[0].offset, gl_info->limits.buffers);

            sprintf(register_name, "gl_FragData[%u]", reg->idx[0].offset);
            break;

        case WINED3DSPR_RASTOUT:
            sprintf(register_name, "%s", hwrastout_reg_names[reg->idx[0].offset]);
            break;

        case WINED3DSPR_DEPTHOUT:
            sprintf(register_name, "gl_FragDepth");
            break;

        case WINED3DSPR_ATTROUT:
            if (!reg->idx[0].offset)
                sprintf(register_name, "%s_out[8]", prefix);
            else
                sprintf(register_name, "%s_out[9]", prefix);
            break;

        case WINED3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0: WINED3DSPR_OUTPUT */
            sprintf(register_name, "%s_out[%u]", prefix, reg->idx[0].offset);
            break;

        case WINED3DSPR_MISCTYPE:
            if (!reg->idx[0].offset)
            {
                /* vPos */
                sprintf(register_name, "vpos");
            }
            else if (reg->idx[0].offset == 1)
            {
                /* Note that gl_FrontFacing is a bool, while vFace is
                 * a float for which the sign determines front/back */
                sprintf(register_name, "(gl_FrontFacing ? 1.0 : -1.0)");
            }
            else
            {
                FIXME("Unhandled misctype register %u.\n", reg->idx[0].offset);
                sprintf(register_name, "unrecognized_register");
            }
            break;

        case WINED3DSPR_IMMCONST:
            switch (reg->immconst_type)
            {
                case WINED3D_IMMCONST_SCALAR:
                    switch (reg->data_type)
                    {
                        case WINED3D_DATA_FLOAT:
                            wined3d_ftoa(*(const float *)reg->immconst_data, register_name);
                            break;
                        case WINED3D_DATA_INT:
                            sprintf(register_name, "%#x", reg->immconst_data[0]);
                            break;
                        case WINED3D_DATA_RESOURCE:
                        case WINED3D_DATA_SAMPLER:
                        case WINED3D_DATA_UINT:
                            sprintf(register_name, "%#xu", reg->immconst_data[0]);
                            break;
                        default:
                            sprintf(register_name, "<unhandled data type %#x>", reg->data_type);
                            break;
                    }
                    break;

                case WINED3D_IMMCONST_VEC4:
                    switch (reg->data_type)
                    {
                        case WINED3D_DATA_FLOAT:
                            wined3d_ftoa(*(const float *)&reg->immconst_data[0], imm_str[0]);
                            wined3d_ftoa(*(const float *)&reg->immconst_data[1], imm_str[1]);
                            wined3d_ftoa(*(const float *)&reg->immconst_data[2], imm_str[2]);
                            wined3d_ftoa(*(const float *)&reg->immconst_data[3], imm_str[3]);
                            sprintf(register_name, "vec4(%s, %s, %s, %s)",
                                    imm_str[0], imm_str[1], imm_str[2], imm_str[3]);
                            break;
                        case WINED3D_DATA_INT:
                            sprintf(register_name, "ivec4(%#x, %#x, %#x, %#x)",
                                    reg->immconst_data[0], reg->immconst_data[1],
                                    reg->immconst_data[2], reg->immconst_data[3]);
                            break;
                        case WINED3D_DATA_RESOURCE:
                        case WINED3D_DATA_SAMPLER:
                        case WINED3D_DATA_UINT:
                            sprintf(register_name, "uvec4(%#xu, %#xu, %#xu, %#xu)",
                                    reg->immconst_data[0], reg->immconst_data[1],
                                    reg->immconst_data[2], reg->immconst_data[3]);
                            break;
                        default:
                            sprintf(register_name, "<unhandled data type %#x>", reg->data_type);
                            break;
                    }
                    break;

                default:
                    FIXME("Unhandled immconst type %#x\n", reg->immconst_type);
                    sprintf(register_name, "<unhandled_immconst_type %#x>", reg->immconst_type);
            }
            break;

        case WINED3DSPR_CONSTBUFFER:
            if (reg->idx[1].rel_addr)
                sprintf(register_name, "%s_cb%u[%s + %u]",
                        prefix, reg->idx[0].offset, rel_param1.param_str, reg->idx[1].offset);
            else
                sprintf(register_name, "%s_cb%u[%u]", prefix, reg->idx[0].offset, reg->idx[1].offset);
            break;

        case WINED3DSPR_PRIMID:
            sprintf(register_name, "uint(gl_PrimitiveIDIn)");
            break;

        default:
            FIXME("Unhandled register type %#x.\n", reg->type);
            sprintf(register_name, "unrecognized_register");
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

static unsigned int shader_glsl_get_write_mask_size(DWORD write_mask) {
    unsigned int size = 0;

    if (write_mask & WINED3DSP_WRITEMASK_0) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_1) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_2) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_3) ++size;

    return size;
}

static void shader_glsl_swizzle_to_str(const DWORD swizzle, BOOL fixup, DWORD mask, char *str)
{
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";

    *str++ = '.';
    /* swizzle bits fields: wwzzyyxx */
    if (mask & WINED3DSP_WRITEMASK_0) *str++ = swizzle_chars[swizzle & 0x03];
    if (mask & WINED3DSP_WRITEMASK_1) *str++ = swizzle_chars[(swizzle >> 2) & 0x03];
    if (mask & WINED3DSP_WRITEMASK_2) *str++ = swizzle_chars[(swizzle >> 4) & 0x03];
    if (mask & WINED3DSP_WRITEMASK_3) *str++ = swizzle_chars[(swizzle >> 6) & 0x03];
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

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static void shader_glsl_add_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, struct glsl_src_param *glsl_src)
{
    BOOL is_color = FALSE;
    char swizzle_str[6];

    glsl_src->reg_name[0] = '\0';
    glsl_src->param_str[0] = '\0';
    swizzle_str[0] = '\0';

    shader_glsl_get_register_name(&wined3d_src->reg, glsl_src->reg_name, &is_color, ins);
    shader_glsl_get_swizzle(wined3d_src, is_color, mask, swizzle_str);

    if (wined3d_src->reg.type == WINED3DSPR_IMMCONST || wined3d_src->reg.type == WINED3DSPR_PRIMID)
    {
        shader_glsl_gen_modifier(wined3d_src->modifiers, glsl_src->reg_name, swizzle_str, glsl_src->param_str);
    }
    else
    {
        char reg_name[200];

        switch (wined3d_src->reg.data_type)
        {
            case WINED3D_DATA_FLOAT:
                sprintf(reg_name, "%s", glsl_src->reg_name);
                break;
            case WINED3D_DATA_INT:
                sprintf(reg_name, "floatBitsToInt(%s)", glsl_src->reg_name);
                break;
            case WINED3D_DATA_RESOURCE:
            case WINED3D_DATA_SAMPLER:
            case WINED3D_DATA_UINT:
                sprintf(reg_name, "floatBitsToUint(%s)", glsl_src->reg_name);
                break;
            default:
                FIXME("Unhandled data type %#x.\n", wined3d_src->reg.data_type);
                sprintf(reg_name, "%s", glsl_src->reg_name);
                break;
        }

        shader_glsl_gen_modifier(wined3d_src->modifiers, reg_name, swizzle_str, glsl_src->param_str);
    }
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static DWORD shader_glsl_add_dst_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_dst_param *wined3d_dst, struct glsl_dst_param *glsl_dst)
{
    BOOL is_color = FALSE;

    glsl_dst->mask_str[0] = '\0';
    glsl_dst->reg_name[0] = '\0';

    shader_glsl_get_register_name(&wined3d_dst->reg, glsl_dst->reg_name, &is_color, ins);
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

static void shader_glsl_get_sample_function(const struct wined3d_shader_context *ctx,
        DWORD resource_idx, DWORD flags, struct glsl_sample_function *sample_function)
{
    static const struct
    {
        unsigned int coord_size;
        const char *type_part;
    }
    resource_types[] =
    {
        {0, ""},     /* WINED3D_SHADER_RESOURCE_NONE */
        {1, ""},     /* WINED3D_SHADER_RESOURCE_BUFFER */
        {1, "1D"},   /* WINED3D_SHADER_RESOURCE_TEXTURE_1D */
        {2, "2D"},   /* WINED3D_SHADER_RESOURCE_TEXTURE_2D */
        {2, ""},     /* WINED3D_SHADER_RESOURCE_TEXTURE_2DMS */
        {3, "3D"},   /* WINED3D_SHADER_RESOURCE_TEXTURE_3D */
        {3, "Cube"}, /* WINED3D_SHADER_RESOURCE_TEXTURE_CUBE */
        {2, ""},     /* WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY */
        {3, ""},     /* WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY */
        {3, ""},     /* WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY */
    };
    struct shader_glsl_ctx_priv *priv = ctx->backend_data;
    enum wined3d_shader_resource_type resource_type = ctx->reg_maps->resource_info[resource_idx].type;
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    BOOL shadow = ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            && (priv->cur_ps_args->shadow & (1 << resource_idx));
    BOOL projected = flags & WINED3D_GLSL_SAMPLE_PROJECTED;
    BOOL texrect = flags & WINED3D_GLSL_SAMPLE_NPOT && gl_info->supported[ARB_TEXTURE_RECTANGLE];
    BOOL lod = flags & WINED3D_GLSL_SAMPLE_LOD;
    BOOL grad = flags & WINED3D_GLSL_SAMPLE_GRAD;
    const char *base = "texture", *type_part = "", *suffix = "";
    unsigned int coord_size;

    sample_function->data_type = ctx->reg_maps->resource_info[resource_idx].data_type;

    if (resource_type >= ARRAY_SIZE(resource_types))
    {
        ERR("Unexpected resource type %#x.\n", resource_type);
        resource_type = WINED3D_SHADER_RESOURCE_TEXTURE_2D;
    }

    /* Note that there's no such thing as a projected cube texture. */
    if (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_CUBE)
        projected = FALSE;

    if (shadow)
        base = "shadow";

    type_part = resource_types[resource_type].type_part;
    if (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2D && texrect)
        type_part = "2DRect";
    if (!type_part[0])
        FIXME("Unhandled resource type %#x.\n", resource_type);

    if (!lod && grad && !gl_info->supported[EXT_GPU_SHADER4])
    {
        if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
            suffix = "ARB";
        else
            FIXME("Unsupported grad function.\n");
    }

    sample_function->name = string_buffer_get(priv->string_buffers);
    string_buffer_sprintf(sample_function->name, "%s%s%s%s%s", base, type_part, projected ? "Proj" : "",
            lod ? "Lod" : grad ? "Grad" : "", suffix);

    coord_size = resource_types[resource_type].coord_size;
    if (shadow)
        ++coord_size;
    sample_function->coord_mask = (1 << coord_size) - 1;
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
    char reg_name[256];
    BOOL is_color;

    shader_glsl_get_register_name(&ins->dst[0].reg, reg_name, &is_color, ins);
    shader_glsl_color_correction_ext(ins->ctx->buffer, reg_name, ins->dst[0].write_mask, fixup);
}

static void PRINTF_ATTR(8, 9) shader_glsl_gen_sample_code(const struct wined3d_shader_instruction *ins,
        DWORD sampler, const struct glsl_sample_function *sample_function, DWORD swizzle,
        const char *dx, const char *dy, const char *bias, const char *coord_reg_fmt, ...)
{
    const struct wined3d_shader_version *version = &ins->ctx->reg_maps->shader_version;
    char dst_swizzle[6];
    struct color_fixup_desc fixup;
    BOOL np2_fixup = FALSE;
    va_list args;
    int ret;

    shader_glsl_swizzle_to_str(swizzle, FALSE, ins->dst[0].write_mask, dst_swizzle);

    /* FIXME: We currently don't support fixups for vertex shaders or anything
     * above SM3. Note that for SM4+ the sampler index doesn't have to match
     * the resource index. */
    if (version->type == WINED3D_SHADER_TYPE_PIXEL && version->major < 4)
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        fixup = priv->cur_ps_args->color_fixup[sampler];

        if (priv->cur_ps_args->np2_fixup & (1 << sampler))
            np2_fixup = TRUE;
    }
    else
    {
        fixup = COLOR_FIXUP_IDENTITY;
    }

    shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &ins->dst[0], sample_function->data_type);

    shader_addline(ins->ctx->buffer, "%s(%s_sampler%u, ",
            sample_function->name->buffer, shader_glsl_get_prefix(version->type), sampler);

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
        const unsigned char idx = priv->cur_np2fixup_info->idx[sampler];

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
        shader_addline(ins->ctx->buffer, ", %s, %s)", dx, dy);
    else if (bias)
        shader_addline(ins->ctx->buffer, ", %s)", bias);
    else
        shader_addline(ins->ctx->buffer, ")");

    shader_addline(ins->ctx->buffer, "%s);\n", dst_swizzle);

    if (!is_identity_fixup(fixup))
        shader_glsl_color_correction(ins, fixup);
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
        case WINED3DSIH_MUL:  op = "*";  break;
        case WINED3DSIH_OR:   op = "|";  break;
        case WINED3DSIH_SUB:  op = "-";  break;
        case WINED3DSIH_USHR: op = ">>"; break;
        case WINED3DSIH_XOR:  op = "^";  break;
        default:
            op = "<unhandled operator>";
            FIXME("Opcode %#x not yet handled in GLSL\n", ins->handler_idx);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
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
            case WINED3DSIH_GE:  op = "greaterThanEqual"; break;
            case WINED3DSIH_IGE: op = "greaterThanEqual"; break;
            case WINED3DSIH_UGE: op = "greaterThanEqual"; break;
            case WINED3DSIH_LT:  op = "lessThan"; break;
            case WINED3DSIH_NE:  op = "notEqual"; break;
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
            case WINED3DSIH_GE:  op = ">="; break;
            case WINED3DSIH_IGE: op = ">="; break;
            case WINED3DSIH_UGE: op = ">="; break;
            case WINED3DSIH_LT:  op = "<"; break;
            case WINED3DSIH_NE:  op = "!="; break;
            default:
                op = "<unhandled operator>";
                ERR("Unhandled opcode %#x.\n", ins->handler_idx);
                break;
        }

        shader_addline(buffer, "%s %s %s ? 0xffffffffu : 0u);\n",
                src0_param.param_str, op, src1_param.param_str);
    }
}

static void shader_glsl_imul(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD write_mask;

    /* If we have ARB_gpu_shader5 or GLSL 4.0, we can use imulExtended(). If
     * not, we can emulate it. */
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
            shader_addline(buffer, "tmp0%s = %s / %s;\n",
                    dst_mask, src0_param.param_str, src1_param.param_str);

            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1], ins->dst[1].reg.data_type);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "%s %% %s));\n", src0_param.param_str, src1_param.param_str);

            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0], ins->dst[0].reg.data_type);
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
    else if(ins->handler_idx == WINED3DSIH_MOVA)
    {
        /* We need to *round* to the nearest int here. */
        unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);

        if (gl_info->supported[EXT_GPU_SHADER4])
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
    unsigned int dst_size = 0;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    /* dp4 works on vec4, dp3 on vec3, etc. */
    if (ins->handler_idx == WINED3DSIH_DP4)
        src_write_mask = WINED3DSP_WRITEMASK_ALL;
    else if (ins->handler_idx == WINED3DSIH_DP3)
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    else
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;

    shader_glsl_add_src_param(ins, &ins->src[0], src_write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_write_mask, &src1_param);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(dot(%s, %s)));\n", dst_size, src0_param.param_str, src1_param.param_str);
    } else {
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
    shader_addline(ins->ctx->buffer, "EndPrimitive();\n");
}

/* Process the WINED3DSIO_POW instruction in GLSL (dst = |src0|^src1)
 * Src0 and src1 are scalars. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_pow(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    if (dst_size > 1)
    {
        shader_addline(buffer, "vec%u(%s == 0.0 ? 1.0 : pow(abs(%s), %s)));\n",
                dst_size, src1_param.param_str, src0_param.param_str, src1_param.param_str);
    }
    else
    {
        shader_addline(buffer, "%s == 0.0 ? 1.0 : pow(abs(%s), %s));\n",
                src1_param.param_str, src0_param.param_str, src1_param.param_str);
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
        case WINED3DSIH_MIN: instruction = "min"; break;
        case WINED3DSIH_MAX: instruction = "max"; break;
        case WINED3DSIH_ABS: instruction = "abs"; break;
        case WINED3DSIH_FRC: instruction = "fract"; break;
        case WINED3DSIH_DSX: instruction = "dFdx"; break;
        case WINED3DSIH_DSY: instruction = "ycorrection.y * dFdy"; break;
        case WINED3DSIH_ROUND_NI: instruction = "floor"; break;
        case WINED3DSIH_SQRT: instruction = "sqrt"; break;
        default: instruction = "";
            FIXME("Opcode %#x not yet handled in GLSL\n", ins->handler_idx);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, ins);

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

    shader_addline(buffer, "tmp0.x = dot(%s, %s);\n",
            src_param.param_str, src_param.param_str);
    shader_glsl_append_dst(buffer, ins);

    if (mask_size > 1)
    {
        shader_addline(buffer, "tmp0.x == 0.0 ? vec%u(0.0) : (%s * inversesqrt(tmp0.x)));\n",
                mask_size, src_param.param_str);
    }
    else
    {
        shader_addline(buffer, "tmp0.x == 0.0 ? 0.0 : (%s * inversesqrt(tmp0.x)));\n",
                src_param.param_str);
    }
}

static void shader_glsl_scalar_op(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    const char *prefix, *suffix;
    unsigned int dst_size;
    DWORD dst_write_mask;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src0_param);

    switch (ins->handler_idx)
    {
        case WINED3DSIH_EXP:
        case WINED3DSIH_EXPP:
            prefix = "exp2(";
            suffix = ")";
            break;

        case WINED3DSIH_LOG:
        case WINED3DSIH_LOGP:
            prefix = "log2(abs(";
            suffix = "))";
            break;

        case WINED3DSIH_RCP:
            prefix = "1.0 / ";
            suffix = "";
            break;

        case WINED3DSIH_RSQ:
            prefix = "inversesqrt(abs(";
            suffix = "))";
            break;

        default:
            prefix = "";
            suffix = "";
            FIXME("Unhandled instruction %#x.\n", ins->handler_idx);
            break;
    }

    if (dst_size > 1)
        shader_addline(buffer, "vec%u(%s%s%s));\n", dst_size, prefix, src0_param.param_str, suffix);
    else
        shader_addline(buffer, "%s%s%s);\n", prefix, src0_param.param_str, suffix);
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

static void shader_glsl_to_int(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    unsigned int mask_size;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);

    if (mask_size > 1)
        shader_addline(buffer, "ivec%u(%s));\n", mask_size, src_param.param_str);
    else
        shader_addline(buffer, "int(%s));\n", src_param.param_str);
}

static void shader_glsl_to_float(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    unsigned int mask_size;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);

    if (mask_size > 1)
        shader_addline(buffer, "vec%u(%s));\n", mask_size, src_param.param_str);
    else
        shader_addline(buffer, "float(%s));\n", src_param.param_str);
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
                FIXME("Can't handle opcode %#x\n", ins->handler_idx);
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
                FIXME("Can't handle opcode %#x\n", ins->handler_idx);
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
            if (((ins->src[0].swizzle >> (2 * j)) & 0x3) == i)
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
    struct wined3d_shader_loop_state *loop_state = ins->ctx->loop_state;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_lconst *constant;
    struct glsl_src_param src1_param;
    const DWORD *control_values = NULL;

    if (ins->ctx->reg_maps->shader_version.major < 4)
    {
        shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_ALL, &src1_param);

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
                        loop_state->current_depth, loop_control.start,
                        loop_state->current_depth, loop_control.count, loop_control.step, loop_control.start,
                        loop_state->current_depth, loop_control.step);
            }
            else if (loop_control.step < 0)
            {
                shader_addline(buffer, "for (aL%u = %u; aL%u > (%u * %d + %u); aL%u += %d)\n{\n",
                        loop_state->current_depth, loop_control.start,
                        loop_state->current_depth, loop_control.count, loop_control.step, loop_control.start,
                        loop_state->current_depth, loop_control.step);
            }
            else
            {
                shader_addline(buffer, "for (aL%u = %u, tmpInt%u = 0; tmpInt%u < %u; tmpInt%u++)\n{\n",
                        loop_state->current_depth, loop_control.start, loop_state->current_depth,
                        loop_state->current_depth, loop_control.count,
                        loop_state->current_depth);
            }
        }
        else
        {
            shader_addline(buffer, "for (tmpInt%u = 0, aL%u = %s.y; tmpInt%u < %s.x; tmpInt%u++, aL%u += %s.z)\n{\n",
                    loop_state->current_depth, loop_state->current_reg,
                    src1_param.reg_name, loop_state->current_depth, src1_param.reg_name,
                    loop_state->current_depth, loop_state->current_reg, src1_param.reg_name);
        }

        ++loop_state->current_reg;
    }
    else
    {
        shader_addline(buffer, "for (;;)\n{\n");
    }

    ++loop_state->current_depth;
}

static void shader_glsl_end(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_loop_state *loop_state = ins->ctx->loop_state;

    shader_addline(ins->ctx->buffer, "}\n");

    if (ins->handler_idx == WINED3DSIH_ENDLOOP)
    {
        --loop_state->current_depth;
        --loop_state->current_reg;
    }

    if (ins->handler_idx == WINED3DSIH_ENDREP)
    {
        --loop_state->current_depth;
    }
}

static void shader_glsl_rep(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader *shader = ins->ctx->shader;
    struct wined3d_shader_loop_state *loop_state = ins->ctx->loop_state;
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
                loop_state->current_depth, loop_state->current_depth,
                control_values[0], loop_state->current_depth);
    }
    else
    {
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
        shader_addline(ins->ctx->buffer, "for (tmpInt%d = 0; tmpInt%d < %s; tmpInt%d++) {\n",
                loop_state->current_depth, loop_state->current_depth,
                src0_param.param_str, loop_state->current_depth);
    }

    ++loop_state->current_depth;
}

static void shader_glsl_if(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(ins->ctx->buffer, "if (bool(%s)) {\n", src0_param.param_str);
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
    shader_addline(ins->ctx->buffer, "EmitVertex();\n");
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

static void shader_glsl_breakp(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src_param);
    shader_addline(ins->ctx->buffer, "if (bool(%s)) break;\n", src_param.param_str);
}

static void shader_glsl_label(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "}\n");
    shader_addline(ins->ctx->buffer, "void subroutine%u()\n{\n", ins->src[0].reg.idx[0].offset);
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
    /* No-op. The closing } is written when a new function is started, and at the end of the shader. This
     * function only suppresses the unhandled instruction warning
     */
}

/*********************************************
 * Pixel Shader Specific Code begins here
 ********************************************/
static void shader_glsl_tex(const struct wined3d_shader_instruction *ins)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    struct glsl_sample_function sample_function;
    DWORD sample_flags = 0;
    DWORD resource_idx;
    DWORD mask = 0, swizzle;
    const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0))
        resource_idx = ins->dst[0].reg.idx[0].offset;
    else
        resource_idx = ins->src[1].reg.idx[0].offset;

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD flags = (priv->cur_ps_args->tex_transform >> resource_idx * WINED3D_PSARGS_TEXTRANSFORM_SHIFT)
                & WINED3D_PSARGS_TEXTRANSFORM_MASK;
        enum wined3d_shader_resource_type resource_type = ins->ctx->reg_maps->resource_info[resource_idx].type;

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
                && ins->ctx->reg_maps->resource_info[resource_idx].type != WINED3D_SHADER_RESOURCE_TEXTURE_CUBE)
        {
            /* ps 2.0 texldp instruction always divides by the fourth component. */
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            mask = WINED3DSP_WRITEMASK_3;
        }
    }

    if (priv->cur_ps_args->np2_fixup & (1 << resource_idx))
        sample_flags |= WINED3D_GLSL_SAMPLE_NPOT;

    shader_glsl_get_sample_function(ins->ctx, resource_idx, sample_flags, &sample_function);
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
        shader_glsl_gen_sample_code(ins, resource_idx, &sample_function, swizzle, NULL, NULL, NULL,
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
                    "%s", coord_param.param_str);
        } else {
            shader_glsl_gen_sample_code(ins, resource_idx, &sample_function, swizzle, NULL, NULL, NULL,
                    "%s", coord_param.param_str);
        }
    }
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_texldd(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, dx_param, dy_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_GRAD;
    struct glsl_sample_function sample_function;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;
    const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4])
    {
        FIXME("texldd used, but not supported by hardware. Falling back to regular tex\n");
        shader_glsl_tex(ins);
        return;
    }

    sampler_idx = ins->src[1].reg.idx[0].offset;
    if (priv->cur_ps_args->np2_fixup & (1 << sampler_idx))
        sample_flags |= WINED3D_GLSL_SAMPLE_NPOT;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[2], sample_function.coord_mask, &dx_param);
    shader_glsl_add_src_param(ins, &ins->src[3], sample_function.coord_mask, &dy_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, dx_param.param_str, dy_param.param_str, NULL,
                                "%s", coord_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

static void shader_glsl_texldl(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, lod_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_LOD;
    struct glsl_sample_function sample_function;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;
    const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;

    sampler_idx = ins->src[1].reg.idx[0].offset;
    if (ins->ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            && priv->cur_ps_args->np2_fixup & (1 << sampler_idx))
        sample_flags |= WINED3D_GLSL_SAMPLE_NPOT;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &lod_param);

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4]
            && ins->ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
    {
        /* Plain GLSL only supports Lod sampling functions in vertex shaders.
         * However, the NVIDIA drivers allow them in fragment shaders as well,
         * even without the appropriate extension. */
        WARN("Using %s in fragment shader.\n", sample_function.name->buffer);
    }
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, lod_param.param_str,
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

static void shader_glsl_sample(const struct wined3d_shader_instruction *ins)
{
    struct glsl_sample_function sample_function;
    struct glsl_src_param coord_param;
    unsigned int sampler_idx;

    shader_glsl_get_sample_function(ins->ctx, ins->src[1].reg.idx[0].offset, 0, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    sampler_idx = shader_glsl_find_sampler(&ins->ctx->reg_maps->sampler_map,
            ins->src[1].reg.idx[0].offset, ins->src[2].reg.idx[0].offset);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE,
            NULL, NULL, NULL, "%s", coord_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
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
        shader_addline(buffer, "clamp(gl_TexCoord[%u], 0.0, 1.0)%s);\n",
                ins->dst[0].reg.idx[0].offset, dst_mask);
    }
    else
    {
        enum wined3d_shader_src_modifier src_mod = ins->src[0].modifiers;
        DWORD reg = ins->src[0].reg.idx[0].offset;
        char dst_swizzle[6];

        shader_glsl_get_swizzle(&ins->src[0], FALSE, write_mask, dst_swizzle);

        if (src_mod == WINED3DSPSM_DZ)
        {
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
            struct glsl_src_param div_param;

            shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_2, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        }
        else if (src_mod == WINED3DSPSM_DW)
        {
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
            struct glsl_src_param div_param;

            shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        } else {
            shader_addline(buffer, "gl_TexCoord[%u]%s);\n", reg, dst_swizzle);
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
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    mask_size = shader_glsl_get_write_mask_size(sample_function.coord_mask);

    switch(mask_size)
    {
        case 1:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    "dot(gl_TexCoord[%u].xyz, %s)", sampler_idx, src0_param.param_str);
            break;

        case 2:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    "vec2(dot(gl_TexCoord[%u].xyz, %s), 0.0)", sampler_idx, src0_param.param_str);
            break;

        case 3:
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
                    "vec3(dot(gl_TexCoord[%u].xyz, %s), 0.0, 0.0)", sampler_idx, src0_param.param_str);
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

    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xy");
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
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xyz");
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
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);
    shader_glsl_write_mask_to_str(sample_function.coord_mask, coord_mask);

    /* Sample the texture */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE,
            NULL, NULL, NULL, "tmp0%s", coord_mask);
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
    shader_addline(buffer, "tmp1.xyz = normalize(vec3(gl_TexCoord[%u].w, gl_TexCoord[%u].w, gl_TexCoord[%u].w));\n",
            tex_mx->texcoord_w[0], tex_mx->texcoord_w[1], reg);
    shader_addline(buffer, "tmp0.xyz = -reflect(tmp1.xyz, normalize(tmp0.xyz));\n");

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);
    shader_glsl_write_mask_to_str(sample_function.coord_mask, coord_mask);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE,
            NULL, NULL, NULL, "tmp0%s", coord_mask);
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
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
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

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
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
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "%s.wx", src0_param.reg_name);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
static void shader_glsl_texreg2gb(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "%s.yz", src0_param.reg_name);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
static void shader_glsl_texreg2rgb(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    struct glsl_sample_function sample_function;
    struct glsl_src_param src0_param;

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &src0_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "%s", src0_param.param_str);
    shader_glsl_release_sample_function(ins->ctx, &sample_function);
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
static void shader_glsl_texkill(const struct wined3d_shader_instruction *ins)
{
    if (ins->ctx->reg_maps->shader_version.major >= 4)
    {
        struct glsl_src_param src_param;

        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src_param);
        shader_addline(ins->ctx->buffer, "if (bool(floatBitsToUint(%s))) discard;\n", src_param.param_str);
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
        const struct ps_compile_args *args)
{
    unsigned int i;

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &input_signature->elements[i];
        const char *semantic_name;
        UINT semantic_idx;
        char reg_mask[6];

        /* Unused */
        if (!(reg_maps->input_registers & (1 << input->register_idx)))
            continue;

        semantic_name = input->semantic_name;
        semantic_idx = input->semantic_idx;
        shader_glsl_write_mask_to_str(input->mask, reg_mask);

        if (args->vp_mode == vertexshader)
        {
            if (input->sysval_semantic == WINED3D_SV_POSITION)
                shader_addline(buffer, "ps_in[%u]%s = vpos%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
            else if (args->pointsprite && shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
                shader_addline(buffer, "ps_in[%u] = vec4(gl_PointCoord.xy, 0.0, 0.0);\n", input->register_idx);
            else
                shader_addline(buffer, "ps_in[%u]%s = ps_link[%u]%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask,
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
        {
            if (semantic_idx < 8 && args->vp_mode == pretransformed)
                shader_addline(buffer, "ps_in[%u]%s = gl_TexCoord[%u]%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, semantic_idx, reg_mask);
            else
                shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
        {
            if (!semantic_idx)
                shader_addline(buffer, "ps_in[%u]%s = vec4(gl_Color)%s;\n",
                        shader->u.ps.input_reg_map[input->register_idx], reg_mask, reg_mask);
            else if (semantic_idx == 1)
                shader_addline(buffer, "ps_in[%u]%s = vec4(gl_SecondaryColor)%s;\n",
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

/*********************************************
 * Vertex Shader Specific Code begins here
 ********************************************/

static void add_glsl_program_entry(struct shader_glsl_priv *priv, struct glsl_shader_prog_link *entry)
{
    struct glsl_program_key key;

    key.vs_id = entry->vs.id;
    key.gs_id = entry->gs.id;
    key.ps_id = entry->ps.id;

    if (wine_rb_put(&priv->program_lookup, &key, &entry->program_lookup_entry) == -1)
    {
        ERR("Failed to insert program entry.\n");
    }
}

static struct glsl_shader_prog_link *get_glsl_program_entry(const struct shader_glsl_priv *priv,
        GLuint vs_id, GLuint gs_id, GLuint ps_id)
{
    struct wine_rb_entry *entry;
    struct glsl_program_key key;

    key.vs_id = vs_id;
    key.gs_id = gs_id;
    key.ps_id = ps_id;

    entry = wine_rb_get(&priv->program_lookup, &key);
    return entry ? WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry) : NULL;
}

/* Context activation is done by the caller. */
static void delete_glsl_program_entry(struct shader_glsl_priv *priv, const struct wined3d_gl_info *gl_info,
        struct glsl_shader_prog_link *entry)
{
    struct glsl_program_key key;

    key.vs_id = entry->vs.id;
    key.gs_id = entry->gs.id;
    key.ps_id = entry->ps.id;
    wine_rb_remove(&priv->program_lookup, &key);

    GL_EXTCALL(glDeleteProgram(entry->id));
    if (entry->vs.id)
        list_remove(&entry->vs.shader_entry);
    if (entry->gs.id)
        list_remove(&entry->gs.shader_entry);
    if (entry->ps.id)
        list_remove(&entry->ps.shader_entry);
    HeapFree(GetProcessHeap(), 0, entry->vs.uniform_f_locations);
    HeapFree(GetProcessHeap(), 0, entry->ps.uniform_f_locations);
    HeapFree(GetProcessHeap(), 0, entry);
}

static void handle_ps3_input(struct shader_glsl_priv *priv,
        const struct wined3d_gl_info *gl_info, const DWORD *map,
        const struct wined3d_shader_signature *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps_in,
        const struct wined3d_shader_signature *output_signature,
        const struct wined3d_shader_reg_maps *reg_maps_out)
{
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    unsigned int i, j;
    DWORD *set;
    DWORD in_idx;
    unsigned int in_count = vec4_varyings(3, gl_info);
    char reg_mask[6];
    struct wined3d_string_buffer *destination = string_buffer_get(&priv->string_buffers);

    set = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*set) * (in_count + 2));

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &input_signature->elements[i];

        if (!(reg_maps_in->input_registers & (1 << input->register_idx)))
            continue;

        in_idx = map[input->register_idx];
        /* Declared, but not read register */
        if (in_idx == ~0u)
            continue;
        if (in_idx >= (in_count + 2))
        {
            FIXME("More input varyings declared than supported, expect issues.\n");
            continue;
        }

        if (in_idx == in_count)
            string_buffer_sprintf(destination, "gl_FrontColor");
        else if (in_idx == in_count + 1)
            string_buffer_sprintf(destination, "gl_FrontSecondaryColor");
        else
            string_buffer_sprintf(destination, "ps_link[%u]", in_idx);

        if (!set[in_idx])
            set[in_idx] = ~0u;

        for (j = 0; j < output_signature->element_count; ++j)
        {
            const struct wined3d_shader_signature_element *output = &output_signature->elements[j];
            DWORD mask;

            if (!(reg_maps_out->output_registers & (1 << output->register_idx))
                    || input->semantic_idx != output->semantic_idx
                    || strcmp(input->semantic_name, output->semantic_name)
                    || !(mask = input->mask & output->mask))
                continue;

            if (set[in_idx] == ~0u)
                set[in_idx] = mask;
            else
                set[in_idx] |= mask;
            shader_glsl_write_mask_to_str(mask, reg_mask);

            shader_addline(buffer, "%s%s = vs_out[%u]%s;\n",
                    destination->buffer, reg_mask, output->register_idx, reg_mask);
        }
    }

    for (i = 0; i < in_count + 2; ++i)
    {
        unsigned int size;

        if (!set[i] || set[i] == WINED3DSP_WRITEMASK_ALL)
            continue;

        if (set[i] == ~0U) set[i] = 0;

        size = 0;
        if (!(set[i] & WINED3DSP_WRITEMASK_0)) reg_mask[size++] = 'x';
        if (!(set[i] & WINED3DSP_WRITEMASK_1)) reg_mask[size++] = 'y';
        if (!(set[i] & WINED3DSP_WRITEMASK_2)) reg_mask[size++] = 'z';
        if (!(set[i] & WINED3DSP_WRITEMASK_3)) reg_mask[size++] = 'w';
        reg_mask[size] = '\0';

        if (i == in_count)
            string_buffer_sprintf(destination, "gl_FrontColor");
        else if (i == in_count + 1)
            string_buffer_sprintf(destination, "gl_FrontSecondaryColor");
        else
            string_buffer_sprintf(destination, "ps_link[%u]", i);

        if (size == 1) shader_addline(buffer, "%s.%s = 0.0;\n", destination->buffer, reg_mask);
        else shader_addline(buffer, "%s.%s = vec%u(0.0);\n", destination->buffer, reg_mask, size);
    }

    HeapFree(GetProcessHeap(), 0, set);
    string_buffer_release(&priv->string_buffers, destination);
}

/* Context activation is done by the caller. */
static GLuint generate_param_reorder_function(struct shader_glsl_priv *priv,
        const struct wined3d_shader *vs, const struct wined3d_shader *ps,
        BOOL per_vertex_point_size, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    GLuint ret = 0;
    DWORD ps_major = ps ? ps->reg_maps.shader_version.major : 0;
    unsigned int i;
    const char *semantic_name;
    UINT semantic_idx;
    char reg_mask[6];

    string_buffer_clear(buffer);

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, &vs->reg_maps.shader_version));

    if (per_vertex_point_size)
    {
        shader_addline(buffer, "uniform struct\n{\n");
        shader_addline(buffer, "    float size_min;\n");
        shader_addline(buffer, "    float size_max;\n");
        shader_addline(buffer, "} ffp_point;\n");
    }

    if (ps_major < 3)
    {
        shader_addline(buffer, "void order_ps_input(in vec4 vs_out[%u])\n{\n", vs->limits->packed_output);

        for (i = 0; i < vs->output_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *output = &vs->output_signature.elements[i];
            DWORD write_mask;

            if (!(vs->reg_maps.output_registers & (1 << output->register_idx)))
                continue;

            semantic_name = output->semantic_name;
            semantic_idx = output->semantic_idx;
            write_mask = output->mask;
            shader_glsl_write_mask_to_str(write_mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
            {
                if (!semantic_idx)
                    shader_addline(buffer, "gl_FrontColor%s = vs_out[%u]%s;\n",
                            reg_mask, output->register_idx, reg_mask);
                else if (semantic_idx == 1)
                    shader_addline(buffer, "gl_FrontSecondaryColor%s = vs_out[%u]%s;\n",
                            reg_mask, output->register_idx, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_POSITION) && !semantic_idx)
            {
                shader_addline(buffer, "gl_Position%s = vs_out[%u]%s;\n",
                        reg_mask, output->register_idx, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
            {
                if (semantic_idx < 8)
                {
                    if (!(gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W) || ps_major > 0)
                        write_mask |= WINED3DSP_WRITEMASK_3;

                    shader_addline(buffer, "gl_TexCoord[%u]%s = vs_out[%u]%s;\n",
                            semantic_idx, reg_mask, output->register_idx, reg_mask);
                    if (!(write_mask & WINED3DSP_WRITEMASK_3))
                        shader_addline(buffer, "gl_TexCoord[%u].w = 1.0;\n", semantic_idx);
                }
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE) && per_vertex_point_size)
            {
                shader_addline(buffer, "gl_PointSize = clamp(vs_out[%u].%c, ffp_point.size_min, ffp_point.size_max);\n",
                        output->register_idx, reg_mask[1]);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_FOG))
            {
                shader_addline(buffer, "gl_FogFragCoord = clamp(vs_out[%u].%c, 0.0, 1.0);\n",
                        output->register_idx, reg_mask[1]);
            }
        }
        shader_addline(buffer, "}\n");
    }
    else
    {
        UINT in_count = min(vec4_varyings(ps_major, gl_info), ps->limits->packed_input);
        /* This one is tricky: a 3.0 pixel shader reads from a 3.0 vertex shader */
        shader_addline(buffer, "varying vec4 ps_link[%u];\n", in_count);
        shader_addline(buffer, "void order_ps_input(in vec4 vs_out[%u])\n{\n", vs->limits->packed_output);

        /* First, sort out position and point size. Those are not passed to the pixel shader */
        for (i = 0; i < vs->output_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *output = &vs->output_signature.elements[i];

            if (!(vs->reg_maps.output_registers & (1 << output->register_idx)))
                continue;

            semantic_name = output->semantic_name;
            semantic_idx = output->semantic_idx;
            shader_glsl_write_mask_to_str(output->mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_POSITION) && !semantic_idx)
            {
                shader_addline(buffer, "gl_Position%s = vs_out[%u]%s;\n",
                        reg_mask, output->register_idx, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE) && per_vertex_point_size)
            {
                shader_addline(buffer, "gl_PointSize = clamp(vs_out[%u].%c, ffp_point.size_min, ffp_point.size_max);\n",
                        output->register_idx, reg_mask[1]);
            }
        }

        /* Then, fix the pixel shader input */
        handle_ps3_input(priv, gl_info, ps->u.ps.input_reg_map, &ps->input_signature,
                &ps->reg_maps, &vs->output_signature, &vs->reg_maps);

        shader_addline(buffer, "}\n");
    }

    ret = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
    checkGLcall("glCreateShader(GL_VERTEX_SHADER)");
    shader_glsl_compile(gl_info, ret, buffer->buffer);

    return ret;
}

static void shader_glsl_generate_srgb_write_correction(struct wined3d_string_buffer *buffer)
{
    shader_addline(buffer, "tmp0.xyz = pow(gl_FragData[0].xyz, vec3(srgb_const0.x));\n");
    shader_addline(buffer, "tmp0.xyz = tmp0.xyz * vec3(srgb_const0.y) - vec3(srgb_const0.z);\n");
    shader_addline(buffer, "tmp1.xyz = gl_FragData[0].xyz * vec3(srgb_const0.w);\n");
    shader_addline(buffer, "bvec3 srgb_compare = lessThan(gl_FragData[0].xyz, vec3(srgb_const1.x));\n");
    shader_addline(buffer, "gl_FragData[0].xyz = mix(tmp0.xyz, tmp1.xyz, vec3(srgb_compare));\n");
    shader_addline(buffer, "gl_FragData[0] = clamp(gl_FragData[0], 0.0, 1.0);\n");
}

static void shader_glsl_generate_fog_code(struct wined3d_string_buffer *buffer, enum wined3d_ffp_ps_fog_mode mode)
{
    switch (mode)
    {
        case WINED3D_FFP_PS_FOG_OFF:
            return;

        case WINED3D_FFP_PS_FOG_LINEAR:
            shader_addline(buffer, "float fog = (ffp_fog.end - gl_FogFragCoord) * ffp_fog.scale;\n");
            break;

        case WINED3D_FFP_PS_FOG_EXP:
            shader_addline(buffer, "float fog = exp(-ffp_fog.density * gl_FogFragCoord);\n");
            break;

        case WINED3D_FFP_PS_FOG_EXP2:
            shader_addline(buffer, "float fog = exp(-ffp_fog.density * ffp_fog.density"
                    " * gl_FogFragCoord * gl_FogFragCoord);\n");
            break;

        default:
            ERR("Invalid fog mode %#x.\n", mode);
            return;
    }

    shader_addline(buffer, "gl_FragData[0].xyz = mix(ffp_fog.color.xyz, gl_FragData[0].xyz,"
            " clamp(fog, 0.0, 1.0));\n");
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_pshader(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        const struct wined3d_shader *shader,
        const struct ps_compile_args *args, struct ps_np2fixup_info *np2fixup_info)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader object and assign it as the shader->prgId */
    GLuint shader_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.cur_np2fixup_info = np2fixup_info;
    priv_ctx.string_buffers = string_buffers;

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, &reg_maps->shader_version));

    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
        shader_addline(buffer, "#extension GL_ARB_shader_texture_lod : enable\n");
    /* The spec says that it doesn't have to be explicitly enabled, but the
     * nvidia drivers write a warning if we don't do so. */
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        shader_addline(buffer, "#extension GL_ARB_uniform_buffer_object : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    /* Base Declarations */
    shader_generate_glsl_declarations(context, buffer, shader, reg_maps, &priv_ctx);

    /* Pack 3.0 inputs */
    if (reg_maps->shader_version.major >= 3)
        shader_glsl_input_pack(shader, buffer, &shader->input_signature, reg_maps, args);

    /* Base Shader Body */
    shader_generate_main(shader, buffer, reg_maps, function, &priv_ctx);

    /* Pixel shaders < 2.0 place the resulting color in R0 implicitly */
    if (reg_maps->shader_version.major < 2)
    {
        /* Some older cards like GeforceFX ones don't support multiple buffers, so also not gl_FragData */
        shader_addline(buffer, "gl_FragData[0] = R0;\n");
    }

    if (args->srgb_correction)
        shader_glsl_generate_srgb_write_correction(buffer);

    /* SM < 3 does not replace the fog stage. */
    if (reg_maps->shader_version.major < 3)
        shader_glsl_generate_fog_code(buffer, args->fog);

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_vshader(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        const struct wined3d_shader *shader,
        const struct vs_compile_args *args)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader program and assign it as the shader->prgId */
    GLuint shader_id = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, &reg_maps->shader_version));

    if (gl_info->supported[ARB_DRAW_INSTANCED])
        shader_addline(buffer, "#extension GL_ARB_draw_instanced : enable\n");
    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        shader_addline(buffer, "#extension GL_ARB_uniform_buffer_object : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;
    priv_ctx.string_buffers = string_buffers;

    /* Base Declarations */
    shader_generate_glsl_declarations(context, buffer, shader, reg_maps, &priv_ctx);

    /* Base Shader Body */
    shader_generate_main(shader, buffer, reg_maps, function, &priv_ctx);

    /* Unpack outputs */
    shader_addline(buffer, "order_ps_input(vs_out);\n");

    /* The D3DRS_FOGTABLEMODE render state defines if the shader-generated fog coord is used
     * or if the fragment depth is used. If the fragment depth is used(FOGTABLEMODE != NONE),
     * the fog frag coord is thrown away. If the fog frag coord is used, but not written by
     * the shader, it is set to 0.0(fully fogged, since start = 1.0, end = 0.0)
     */
    if (args->fog_src == VS_FOG_Z)
        shader_addline(buffer, "gl_FogFragCoord = gl_Position.z;\n");
    else if (!reg_maps->fog)
        shader_addline(buffer, "gl_FogFragCoord = 0.0;\n");

    /* We always store the clipplanes without y inversion */
    if (args->clip_enabled)
        shader_addline(buffer, "gl_ClipVertex = gl_Position;\n");

    if (args->point_size && !args->per_vertex_point_size)
        shader_addline(buffer, "gl_PointSize = clamp(ffp_point.size, ffp_point.size_min, ffp_point.size_max);\n");

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad.
     */
    shader_addline(buffer, "gl_Position.y = gl_Position.y * posFixup.y;\n");
    shader_addline(buffer, "gl_Position.xy += posFixup.zw * gl_Position.ww;\n");

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     *
     * Basically we want (in homogeneous coordinates) z = z * 2 - 1. However, shaders are run
     * before the homogeneous divide, so we have to take the w into account: z = ((z / w) * 2 - 1) * w,
     * which is the same as z = z * 2 - w.
     */
    shader_addline(buffer, "gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_geometry_shader(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        const struct wined3d_shader *shader)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;
    GLuint shader_id;

    shader_id = GL_EXTCALL(glCreateShader(GL_GEOMETRY_SHADER));

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, &reg_maps->shader_version));

    if (gl_info->supported[ARB_GEOMETRY_SHADER4])
        shader_addline(buffer, "#extension GL_ARB_geometry_shader4 : enable\n");
    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        shader_addline(buffer, "#extension GL_ARB_uniform_buffer_object : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.string_buffers = string_buffers;
    shader_generate_glsl_declarations(context, buffer, shader, reg_maps, &priv_ctx);
    shader_generate_main(shader, buffer, reg_maps, function, &priv_ctx);
    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static GLuint find_glsl_pshader(const struct wined3d_context *context,
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
        shader->backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
        if (!shader->backend_data)
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
    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader_data->gl_shaders.ps,
                    new_size * sizeof(*gl_shaders));
        }
        else
        {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*gl_shaders));
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

    pixelshader_update_resource_types(shader, args->tex_types);

    string_buffer_clear(buffer);
    ret = shader_glsl_generate_pshader(context, buffer, string_buffers, shader, args, np2fixup);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static inline BOOL vs_args_equal(const struct vs_compile_args *stored, const struct vs_compile_args *new,
        const DWORD use_map)
{
    if((stored->swizzle_map & use_map) != new->swizzle_map) return FALSE;
    if((stored->clip_enabled) != new->clip_enabled) return FALSE;
    if (stored->point_size != new->point_size)
        return FALSE;
    if (stored->per_vertex_point_size != new->per_vertex_point_size)
        return FALSE;
    return stored->fog_src == new->fog_src;
}

static GLuint find_glsl_vshader(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        struct wined3d_shader *shader,
        const struct vs_compile_args *args)
{
    UINT i;
    DWORD new_size;
    DWORD use_map = context->stream_info.use_map;
    struct glsl_vs_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    GLuint ret;

    if (!shader->backend_data)
    {
        shader->backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
        if (!shader->backend_data)
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

    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader_data->gl_shaders.vs,
                    new_size * sizeof(*gl_shaders));
        }
        else
        {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*gl_shaders));
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

    string_buffer_clear(buffer);
    ret = shader_glsl_generate_vshader(context, buffer, string_buffers, shader, args);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static GLuint find_glsl_geometry_shader(const struct wined3d_context *context,
        struct wined3d_string_buffer *buffer, struct wined3d_string_buffer_list *string_buffers,
        struct wined3d_shader *shader)
{
    struct glsl_gs_compiled_shader *gl_shaders;
    struct glsl_shader_private *shader_data;
    GLuint ret;

    if (!shader->backend_data)
    {
        if (!(shader->backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data))))
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->backend_data;
    gl_shaders = shader_data->gl_shaders.gs;

    if (shader_data->num_gl_shaders)
        return gl_shaders[0].id;

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    if (!(shader_data->gl_shaders.gs = HeapAlloc(GetProcessHeap(), 0, sizeof(*gl_shaders))))
    {
        ERR("Failed to allocate GL shader array.\n");
        return 0;
    }
    shader_data->shader_array_size = 1;
    gl_shaders = shader_data->gl_shaders.gs;

    string_buffer_clear(buffer);
    ret = shader_glsl_generate_geometry_shader(context, buffer, string_buffers, shader);
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

static void shader_glsl_ffp_vertex_lighting(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, BOOL legacy_lighting)
{
    const char *diffuse, *specular, *emissive, *ambient;
    enum wined3d_light_type light_type;
    unsigned int i;

    if (!settings->lighting)
    {
        shader_addline(buffer, "gl_FrontColor = ffp_attrib_diffuse;\n");
        shader_addline(buffer, "gl_FrontSecondaryColor = ffp_attrib_specular;\n");
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

    for (i = 0; i < MAX_ACTIVE_LIGHTS; ++i)
    {
        light_type = (settings->light_type >> WINED3D_FFP_LIGHT_TYPE_SHIFT(i)) & WINED3D_FFP_LIGHT_TYPE_MASK;
        switch (light_type)
        {
            case WINED3D_LIGHT_POINT:
                shader_addline(buffer, "dir = ffp_light[%u].position.xyz - ec_pos.xyz;\n", i);
                shader_addline(buffer, "dst.z = dot(dir, dir);\n");
                shader_addline(buffer, "dst.y = sqrt(dst.z);\n");
                shader_addline(buffer, "dst.x = 1.0;\n");
                if (legacy_lighting)
                {
                    shader_addline(buffer, "dst.y = (ffp_light[%u].range - dst.y) / ffp_light[%u].range;\n", i, i);
                    shader_addline(buffer, "dst.z = dst.y * dst.y;\n");
                }
                else
                {
                    shader_addline(buffer, "if (dst.y <= ffp_light[%u].range)\n{\n", i);
                }
                shader_addline(buffer, "att = dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                        " ffp_light[%u].l_att, ffp_light[%u].q_att));\n", i, i, i);
                if (!legacy_lighting)
                    shader_addline(buffer, "att = 1.0 / att;\n");
                shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz * att;\n", i);
                if (!settings->normal)
                {
                    if (!legacy_lighting)
                        shader_addline(buffer, "}\n");
                    break;
                }
                shader_addline(buffer, "dir = normalize(dir);\n");
                shader_addline(buffer, "diffuse += (clamp(dot(dir, normal), 0.0, 1.0)"
                        " * ffp_light[%u].diffuse.xyz) * att;\n", i);
                if (settings->localviewer)
                    shader_addline(buffer, "t = dot(normal, normalize(dir - normalize(ec_pos.xyz)));\n");
                else
                    shader_addline(buffer, "t = dot(normal, normalize(dir + vec3(0.0, 0.0, -1.0)));\n");
                shader_addline(buffer, "if (t > 0.0) specular += (pow(t, ffp_material.shininess)"
                        " * ffp_light[%u].specular) * att;\n", i);
                if (!legacy_lighting)
                    shader_addline(buffer, "}\n");
                break;

            case WINED3D_LIGHT_SPOT:
                shader_addline(buffer, "dir = ffp_light[%u].position.xyz - ec_pos.xyz;\n", i);
                shader_addline(buffer, "dst.z = dot(dir, dir);\n");
                shader_addline(buffer, "dst.y = sqrt(dst.z);\n");
                shader_addline(buffer, "dst.x = 1.0;\n");
                if (legacy_lighting)
                {
                    shader_addline(buffer, "dst.y = (ffp_light[%u].range - dst.y) / ffp_light[%u].range;\n", i, i);
                    shader_addline(buffer, "dst.z = dst.y * dst.y;\n");
                }
                else
                {
                    shader_addline(buffer, "if (dst.y <= ffp_light[%u].range)\n{\n", i);
                }
                shader_addline(buffer, "dir = normalize(dir);\n");
                shader_addline(buffer, "t = dot(-dir, normalize(ffp_light[%u].direction));\n", i);
                shader_addline(buffer, "if (t > ffp_light[%u].cos_htheta) att = 1.0;\n", i);
                shader_addline(buffer, "else if (t <= ffp_light[%u].cos_hphi) att = 0.0;\n", i);
                shader_addline(buffer, "else att = pow((t - ffp_light[%u].cos_hphi)"
                        " / (ffp_light[%u].cos_htheta - ffp_light[%u].cos_hphi), ffp_light[%u].falloff);\n",
                        i, i, i, i);
                if (legacy_lighting)
                    shader_addline(buffer, "att *= dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                            " ffp_light[%u].l_att, ffp_light[%u].q_att));\n",
                            i, i, i);
                else
                    shader_addline(buffer, "att /= dot(dst.xyz, vec3(ffp_light[%u].c_att,"
                            " ffp_light[%u].l_att, ffp_light[%u].q_att));\n",
                            i, i, i);
                shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz * att;\n", i);
                if (!settings->normal)
                {
                    if (!legacy_lighting)
                        shader_addline(buffer, "}\n");
                    break;
                }
                shader_addline(buffer, "diffuse += (clamp(dot(dir, normal), 0.0, 1.0)"
                        " * ffp_light[%u].diffuse.xyz) * att;\n", i);
                if (settings->localviewer)
                    shader_addline(buffer, "t = dot(normal, normalize(dir - normalize(ec_pos.xyz)));\n");
                else
                    shader_addline(buffer, "t = dot(normal, normalize(dir + vec3(0.0, 0.0, -1.0)));\n");
                shader_addline(buffer, "if (t > 0.0) specular += (pow(t, ffp_material.shininess)"
                        " * ffp_light[%u].specular) * att;\n", i);
                if (!legacy_lighting)
                    shader_addline(buffer, "}\n");
                break;

            case WINED3D_LIGHT_DIRECTIONAL:
                shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz;\n", i);
                if (!settings->normal)
                    break;
                shader_addline(buffer, "dir = normalize(ffp_light[%u].direction.xyz);\n", i);
                shader_addline(buffer, "diffuse += clamp(dot(dir, normal), 0.0, 1.0)"
                        " * ffp_light[%u].diffuse.xyz;\n", i);
                /* TODO: In the non-local viewer case the halfvector is constant
                 * and could be precomputed and stored in a uniform. */
                if (settings->localviewer)
                    shader_addline(buffer, "t = dot(normal, normalize(dir - normalize(ec_pos.xyz)));\n");
                else
                    shader_addline(buffer, "t = dot(normal, normalize(dir + vec3(0.0, 0.0, -1.0)));\n");
                shader_addline(buffer, "if (t > 0.0) specular += pow(t, ffp_material.shininess)"
                        " * ffp_light[%u].specular;\n", i);
                break;

            case WINED3D_LIGHT_PARALLELPOINT:
                shader_addline(buffer, "ambient += ffp_light[%u].ambient.xyz;\n", i);
                if (!settings->normal)
                    break;
                shader_addline(buffer, "dir = normalize(ffp_light[%u].position.xyz);\n", i);
                shader_addline(buffer, "diffuse += clamp(dot(dir, normal), 0.0, 1.0)"
                        " * ffp_light[%u].diffuse.xyz;\n", i);
                shader_addline(buffer, "t = dot(normal, normalize(dir - normalize(ec_pos.xyz)));\n");
                shader_addline(buffer, "if (t > 0.0) specular += pow(t, ffp_material.shininess)"
                        " * ffp_light[%u].specular;\n", i);
                break;

            default:
                if (light_type)
                    FIXME("Unhandled light type %#x.\n", light_type);
                continue;
        }
    }

    shader_addline(buffer, "gl_FrontColor.xyz = %s.xyz * ambient + %s.xyz * diffuse + %s.xyz;\n",
            ambient, diffuse, emissive);
    shader_addline(buffer, "gl_FrontColor.w = %s.w;\n", diffuse);
    shader_addline(buffer, "gl_FrontSecondaryColor = %s * specular;\n", specular);
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_ffp_vertex_shader(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, const struct wined3d_gl_info *gl_info,
        BOOL legacy_lighting)
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
        /* TODO: Indexed vertex blending */
        {"float", ""},                          /* WINED3D_FFP_BLENDINDICES */
        {"vec3", "ffp_attrib_normal"},          /* WINED3D_FFP_NORMAL */
        {"float", "ffp_attrib_psize"},          /* WINED3D_FFP_PSIZE */
        {"vec4", "ffp_attrib_diffuse"},         /* WINED3D_FFP_DIFFUSE */
        {"vec4", "ffp_attrib_specular"},        /* WINED3D_FFP_SPECULAR */
    };
    GLuint shader_obj;
    unsigned int i;

    string_buffer_clear(buffer);

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, NULL));

    for (i = 0; i < WINED3D_FFP_ATTRIBS_COUNT; ++i)
    {
        const char *type = i < ARRAY_SIZE(attrib_info) ? attrib_info[i].type : "vec4";

        shader_addline(buffer, "attribute %s vs_in%u;\n", type, i);
    }
    shader_addline(buffer, "\n");

    shader_addline(buffer, "uniform mat4 ffp_modelview_matrix[%u];\n", MAX_VERTEX_BLENDS);
    shader_addline(buffer, "uniform mat4 ffp_projection_matrix;\n");
    shader_addline(buffer, "uniform mat3 ffp_normal_matrix;\n");
    shader_addline(buffer, "uniform mat4 ffp_texture_matrix[%u];\n", MAX_TEXTURES);

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
    shader_addline(buffer, "} ffp_light[%u];\n", MAX_ACTIVE_LIGHTS);

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

    shader_addline(buffer, "\nvoid main()\n{\n");
    shader_addline(buffer, "float m;\n");
    shader_addline(buffer, "vec3 r;\n");

    for (i = 0; i < ARRAY_SIZE(attrib_info); ++i)
    {
        if (attrib_info[i].name[0])
            shader_addline(buffer, "%s %s = vs_in%u;\n",
                    attrib_info[i].type, attrib_info[i].name, i);
    }
    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        unsigned int coord_idx = settings->texgen[i] & 0x0000ffff;
        if ((settings->texgen[i] & 0xffff0000) == WINED3DTSS_TCI_PASSTHRU)
        {
            shader_addline(buffer, "vec4 ffp_attrib_texcoord%u = vs_in%u;\n", i, coord_idx + WINED3D_FFP_TEXCOORD0);
        }
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
        for (i = 0; i < settings->vertexblends + 1; ++i)
            shader_addline(buffer, "ec_pos += ffp_attrib_blendweight[%u] * (ffp_modelview_matrix[%u] * ffp_attrib_position);\n", i, i);

        shader_addline(buffer, "gl_Position = ffp_projection_matrix * ec_pos;\n");
        if (settings->clipping)
            shader_addline(buffer, "gl_ClipVertex = ec_pos;\n");
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
                shader_addline(buffer, "normal += ffp_attrib_blendweight[%u] * (mat3(ffp_modelview_matrix[%u]) * ffp_attrib_normal);\n", i, i);
        }

        if (settings->normalize)
            shader_addline(buffer, "normal = normalize(normal);\n");
    }

    shader_glsl_ffp_vertex_lighting(buffer, settings, legacy_lighting);

    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        switch (settings->texgen[i] & 0xffff0000)
        {
            case WINED3DTSS_TCI_PASSTHRU:
                if (settings->texcoords & (1 << i))
                    shader_addline(buffer, "gl_TexCoord[%u] = ffp_texture_matrix[%u] * ffp_attrib_texcoord%u;\n",
                            i, i, i);
                break;

            case WINED3DTSS_TCI_CAMERASPACENORMAL:
                shader_addline(buffer, "gl_TexCoord[%u] = ffp_texture_matrix[%u] * vec4(normal, 1.0);\n", i, i);
                break;

            case WINED3DTSS_TCI_CAMERASPACEPOSITION:
                shader_addline(buffer, "gl_TexCoord[%u] = ffp_texture_matrix[%u] * ec_pos;\n", i, i);
                break;

            case WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
                shader_addline(buffer, "gl_TexCoord[%u] = ffp_texture_matrix[%u]"
                        " * vec4(reflect(normalize(ec_pos.xyz), normal), 1.0);\n", i, i);
                break;

            case WINED3DTSS_TCI_SPHEREMAP:
                shader_addline(buffer, "r = reflect(normalize(ec_pos.xyz), normal);\n");
                shader_addline(buffer, "m = 2.0 * length(vec3(r.x, r.y, r.z + 1.0));\n");
                shader_addline(buffer, "gl_TexCoord[%u] = ffp_texture_matrix[%u]"
                        " * vec4(r.x / m + 0.5, r.y / m + 0.5, 0.0, 1.0);\n", i, i);
                break;

            default:
                ERR("Unhandled texgen %#x.\n", settings->texgen[i]);
                break;
        }
    }

    switch (settings->fog_mode)
    {
        case WINED3D_FFP_VS_FOG_OFF:
            break;

        case WINED3D_FFP_VS_FOG_FOGCOORD:
            shader_addline(buffer, "gl_FogFragCoord = ffp_attrib_specular.w * 255.0;\n");
            break;

        case WINED3D_FFP_VS_FOG_RANGE:
            shader_addline(buffer, "gl_FogFragCoord = length(ec_pos.xyz);\n");
            break;

        case WINED3D_FFP_VS_FOG_DEPTH:
            if (settings->ortho_fog)
                /* Need to undo the [0.0 - 1.0] -> [-1.0 - 1.0] transformation from D3D to GL coordinates. */
                shader_addline(buffer, "gl_FogFragCoord = gl_Position.z * 0.5 + 0.5;\n");
            else if (settings->transformed)
                shader_addline(buffer, "gl_FogFragCoord = ec_pos.z;\n");
            else
                shader_addline(buffer, "gl_FogFragCoord = abs(ec_pos.z);\n");
            break;

        default:
            ERR("Unhandled fog mode %#x.\n", settings->fog_mode);
            break;
    }

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
            ret = "gl_Color";
            break;

        case WINED3DTA_CURRENT:
            if (!stage)
                ret = "gl_Color";
            else
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
            ret = "gl_SecondaryColor";
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
        BOOL alpha, DWORD dst, DWORD op, DWORD dw_arg0, DWORD dw_arg1, DWORD dw_arg2)
{
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;

    if (color && alpha)
        dstmask = "";
    else if (color)
        dstmask = ".xyz";
    else
        dstmask = ".w";

    if (dst == tempreg)
        dstreg = "temp_reg";
    else
        dstreg = "ret";

    arg0 = shader_glsl_get_ffp_fragment_op_arg(buffer, 0, stage, dw_arg0);
    arg1 = shader_glsl_get_ffp_fragment_op_arg(buffer, 1, stage, dw_arg1);
    arg2 = shader_glsl_get_ffp_fragment_op_arg(buffer, 2, stage, dw_arg2);

    switch (op)
    {
        case WINED3D_TOP_DISABLE:
            if (!stage)
                shader_addline(buffer, "%s%s = gl_Color%s;\n", dstreg, dstmask, dstmask);
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
        const struct ffp_frag_settings *settings, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_string_buffer *buffer = &priv->shader_buffer;
    BYTE lum_map = 0, bump_map = 0, tex_map = 0, tss_const_map = 0;
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    const char *final_combiner_src = "ret";
    UINT lowest_disabled_stage;
    GLuint shader_id;
    DWORD arg0, arg1, arg2;
    unsigned int stage;
    struct wined3d_string_buffer *tex_reg_name = string_buffer_get(&priv->string_buffers);

    string_buffer_clear(buffer);

    /* Find out which textures are read */
    for (stage = 0; stage < MAX_TEXTURES; ++stage)
    {
        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
            break;

        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE
                || (stage == 0 && settings->color_key_enabled))
            tex_map |= 1 << stage;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (settings->op[stage].dst == tempreg)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            tss_const_map |= 1 << stage;

        switch (settings->op[stage].cop)
        {
            case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
                lum_map |= 1 << stage;
                /* fall through */
            case WINED3D_TOP_BUMPENVMAP:
                bump_map |= 1 << stage;
                /* fall through */
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                tex_map |= 1 << stage;
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
            tex_map |= 1 << stage;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            tss_const_map |= 1 << stage;
    }
    lowest_disabled_stage = stage;

    shader_addline(buffer, "%s\n", shader_glsl_get_version(gl_info, NULL));

    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");

    shader_addline(buffer, "vec4 tmp0, tmp1;\n");
    shader_addline(buffer, "vec4 ret;\n");
    if (tempreg_used || settings->sRGB_write)
        shader_addline(buffer, "vec4 temp_reg = vec4(0.0);\n");
    shader_addline(buffer, "vec4 arg0, arg1, arg2;\n");

    for (stage = 0; stage < MAX_TEXTURES; ++stage)
    {
        if (tss_const_map & (1 << stage))
            shader_addline(buffer, "uniform vec4 tss_const%u;\n", stage);

        if (!(tex_map & (1 << stage)))
            continue;

        switch (settings->op[stage].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                shader_addline(buffer, "uniform sampler1D ps_sampler%u;\n", stage);
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                shader_addline(buffer, "uniform sampler2D ps_sampler%u;\n", stage);
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                shader_addline(buffer, "uniform sampler3D ps_sampler%u;\n", stage);
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                shader_addline(buffer, "uniform samplerCube ps_sampler%u;\n", stage);
                break;
            case WINED3D_GL_RES_TYPE_TEX_RECT:
                shader_addline(buffer, "uniform sampler2DRect ps_sampler%u;\n", stage);
                break;
            default:
                FIXME("Unhandled sampler type %#x.\n", settings->op[stage].tex_type);
                break;
        }

        shader_addline(buffer, "vec4 tex%u;\n", stage);

        if (!(bump_map & (1 << stage)))
            continue;
        shader_addline(buffer, "uniform mat2 bumpenv_mat%u;\n", stage);

        if (!(lum_map & (1 << stage)))
            continue;
        shader_addline(buffer, "uniform float bumpenv_lum_scale%u;\n", stage);
        shader_addline(buffer, "uniform float bumpenv_lum_offset%u;\n", stage);
    }
    if (tfactor_used)
        shader_addline(buffer, "uniform vec4 tex_factor;\n");
    if (settings->color_key_enabled)
        shader_addline(buffer, "uniform vec4 color_key;\n");
    shader_addline(buffer, "uniform vec4 specular_enable;\n");

    if (settings->sRGB_write)
    {
        shader_addline(buffer, "const vec4 srgb_const0 = ");
        shader_glsl_append_imm_vec4(buffer, wined3d_srgb_const0);
        shader_addline(buffer, ";\n");
        shader_addline(buffer, "const vec4 srgb_const1 = ");
        shader_glsl_append_imm_vec4(buffer, wined3d_srgb_const1);
        shader_addline(buffer, ";\n");
    }

    shader_addline(buffer, "uniform struct\n{\n");
    shader_addline(buffer, "    vec4 color;\n");
    shader_addline(buffer, "    float density;\n");
    shader_addline(buffer, "    float end;\n");
    shader_addline(buffer, "    float scale;\n");
    shader_addline(buffer, "} ffp_fog;\n");

    shader_addline(buffer, "void main()\n{\n");

    if (lowest_disabled_stage < 7 && settings->emul_clipplanes)
        shader_addline(buffer, "if (any(lessThan(gl_TexCoord[7], vec4(0.0)))) discard;\n");

    /* Generate texture sampling instructions) */
    for (stage = 0; stage < MAX_TEXTURES && settings->op[stage].cop != WINED3D_TOP_DISABLE; ++stage)
    {
        const char *texture_function, *coord_mask;
        BOOL proj;

        if (!(tex_map & (1 << stage)))
            continue;

        if (settings->op[stage].projected == proj_none)
        {
            proj = FALSE;
        }
        else if (settings->op[stage].projected == proj_count4
                || settings->op[stage].projected == proj_count3)
        {
            proj = TRUE;
        }
        else
        {
            FIXME("Unexpected projection mode %d\n", settings->op[stage].projected);
            proj = TRUE;
        }

        switch (settings->op[stage].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                if (proj)
                {
                    texture_function = "texture1DProj";
                    coord_mask = "xw";
                }
                else
                {
                    texture_function = "texture1D";
                    coord_mask = "x";
                }
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                if (proj)
                {
                    texture_function = "texture2DProj";
                    coord_mask = "xyw";
                }
                else
                {
                    texture_function = "texture2D";
                    coord_mask = "xy";
                }
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                if (proj)
                {
                    texture_function = "texture3DProj";
                    coord_mask = "xyzw";
                }
                else
                {
                    texture_function = "texture3D";
                    coord_mask = "xyz";
                }
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                texture_function = "textureCube";
                coord_mask = "xyz";
                break;
            case WINED3D_GL_RES_TYPE_TEX_RECT:
                if (proj)
                {
                    texture_function = "texture2DRectProj";
                    coord_mask = "xyw";
                }
                else
                {
                    texture_function = "texture2DRect";
                    coord_mask = "xy";
                }
                break;
            default:
                FIXME("Unhandled texture type %#x.\n", settings->op[stage].tex_type);
                texture_function = "";
                coord_mask = "xyzw";
                break;
        }

        if (stage > 0
                && (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP
                || settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE))
        {
            shader_addline(buffer, "ret.xy = bumpenv_mat%u * tex%u.xy;\n", stage - 1, stage - 1);

            /* With projective textures, texbem only divides the static
             * texture coord, not the displacement, so multiply the
             * displacement with the dividing parameter before passing it to
             * TXP. */
            if (settings->op[stage].projected != proj_none)
            {
                if (settings->op[stage].projected == proj_count4)
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * gl_TexCoord[%u].w) + gl_TexCoord[%u].xy;\n",
                            stage, stage);
                    shader_addline(buffer, "ret.zw = gl_TexCoord[%u].ww;\n", stage);
                }
                else
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * gl_TexCoord[%u].z) + gl_TexCoord[%u].xy;\n",
                            stage, stage);
                    shader_addline(buffer, "ret.zw = gl_TexCoord[%u].zz;\n", stage);
                }
            }
            else
            {
                shader_addline(buffer, "ret = gl_TexCoord[%u] + ret.xyxy;\n", stage);
            }

            shader_addline(buffer, "tex%u = %s(ps_sampler%u, ret.%s);\n",
                    stage, texture_function, stage, coord_mask);

            if (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
                shader_addline(buffer, "tex%u *= clamp(tex%u.z * bumpenv_lum_scale%u + bumpenv_lum_offset%u, 0.0, 1.0);\n",
                        stage, stage - 1, stage - 1, stage - 1);
        }
        else if (settings->op[stage].projected == proj_count3)
        {
            shader_addline(buffer, "tex%u = %s(ps_sampler%u, gl_TexCoord[%u].xyz);\n",
                    stage, texture_function, stage, stage);
        }
        else
        {
            shader_addline(buffer, "tex%u = %s(ps_sampler%u, gl_TexCoord[%u].%s);\n",
                    stage, texture_function, stage, stage, coord_mask);
        }

        string_buffer_sprintf(tex_reg_name, "tex%u", stage);
        shader_glsl_color_correction_ext(buffer, tex_reg_name->buffer, WINED3DSP_WRITEMASK_ALL,
                settings->op[stage].color_fixup);
    }

    if (settings->color_key_enabled)
        shader_addline(buffer, "if (all(equal(tex0, color_key))) discard;\n");

    /* Generate the main shader */
    for (stage = 0; stage < MAX_TEXTURES; ++stage)
    {
        BOOL op_equal;

        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
        {
            if (!stage)
                final_combiner_src = "gl_Color";
            break;
        }

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
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
            if (!stage)
                shader_addline(buffer, "ret.w = gl_Color.w;\n");
        }
        else if (op_equal)
        {
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, TRUE, settings->op[stage].dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
        }
        else
        {
            shader_glsl_ffp_fragment_op(buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                    settings->op[stage].cop, settings->op[stage].carg0,
                    settings->op[stage].carg1, settings->op[stage].carg2);
            shader_glsl_ffp_fragment_op(buffer, stage, FALSE, TRUE, settings->op[stage].dst,
                    settings->op[stage].aop, settings->op[stage].aarg0,
                    settings->op[stage].aarg1, settings->op[stage].aarg2);
        }
    }

    shader_addline(buffer, "gl_FragData[0] = gl_SecondaryColor * specular_enable + %s;\n", final_combiner_src);

    if (settings->sRGB_write)
        shader_glsl_generate_srgb_write_correction(buffer);

    shader_glsl_generate_fog_code(buffer, settings->fog);

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

    if (!(shader = HeapAlloc(GetProcessHeap(), 0, sizeof(*shader))))
        return NULL;

    shader->desc.settings = *settings;
    shader->id = shader_glsl_generate_ffp_vertex_shader(&priv->shader_buffer, settings, gl_info, priv->legacy_lighting);
    list_init(&shader->linked_programs);
    if (wine_rb_put(&priv->ffp_vertex_shaders, &shader->desc.settings, &shader->desc.entry) == -1)
        ERR("Failed to insert ffp vertex shader.\n");

    return shader;
}

static struct glsl_ffp_fragment_shader *shader_glsl_find_ffp_fragment_shader(struct shader_glsl_priv *priv,
        const struct wined3d_gl_info *gl_info, const struct ffp_frag_settings *args)
{
    struct glsl_ffp_fragment_shader *glsl_desc;
    const struct ffp_frag_desc *desc;

    if ((desc = find_ffp_frag_shader(&priv->ffp_fragment_shaders, args)))
        return CONTAINING_RECORD(desc, struct glsl_ffp_fragment_shader, entry);

    if (!(glsl_desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*glsl_desc))))
        return NULL;

    glsl_desc->entry.settings = *args;
    glsl_desc->id = shader_glsl_generate_ffp_fragment_shader(priv, args, gl_info);
    list_init(&glsl_desc->linked_programs);
    add_ffp_frag_shader(&priv->ffp_fragment_shaders, &glsl_desc->entry);

    return glsl_desc;
}


static void shader_glsl_init_vs_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_vs_program *vs, unsigned int vs_c_count)
{
    unsigned int i;
    struct wined3d_string_buffer *name = string_buffer_get(&priv->string_buffers);

    vs->uniform_f_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLuint) * gl_info->limits.glsl_vs_float_constants);
    for (i = 0; i < vs_c_count; ++i)
    {
        string_buffer_sprintf(name, "vs_c[%u]", i);
        vs->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    memset(&vs->uniform_f_locations[vs_c_count], 0xff,
            (gl_info->limits.glsl_vs_float_constants - vs_c_count) * sizeof(GLuint));

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        string_buffer_sprintf(name, "vs_i[%u]", i);
        vs->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < MAX_CONST_B; ++i)
    {
        string_buffer_sprintf(name, "vs_b[%u]", i);
        vs->uniform_b_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    vs->pos_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "posFixup"));

    for (i = 0; i < MAX_VERTEX_BLENDS; ++i)
    {
        string_buffer_sprintf(name, "ffp_modelview_matrix[%u]", i);
        vs->modelview_matrix_location[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    vs->projection_matrix_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_projection_matrix"));
    vs->normal_matrix_location = GL_EXTCALL(glGetUniformLocation(program_id, "ffp_normal_matrix"));
    for (i = 0; i < MAX_TEXTURES; ++i)
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
    for (i = 0; i < MAX_ACTIVE_LIGHTS; ++i)
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

    string_buffer_release(&priv->string_buffers, name);
}

static void shader_glsl_init_ps_uniform_locations(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id, struct glsl_ps_program *ps, unsigned int ps_c_count)
{
    unsigned int i;
    struct wined3d_string_buffer *name = string_buffer_get(&priv->string_buffers);

    ps->uniform_f_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLuint) * gl_info->limits.glsl_ps_float_constants);
    for (i = 0; i < ps_c_count; ++i)
    {
        string_buffer_sprintf(name, "ps_c[%u]", i);
        ps->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }
    memset(&ps->uniform_f_locations[ps_c_count], 0xff,
            (gl_info->limits.glsl_ps_float_constants - ps_c_count) * sizeof(GLuint));

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        string_buffer_sprintf(name, "ps_i[%u]", i);
        ps->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < MAX_CONST_B; ++i)
    {
        string_buffer_sprintf(name, "ps_b[%u]", i);
        ps->uniform_b_locations[i] = GL_EXTCALL(glGetUniformLocation(program_id, name->buffer));
    }

    for (i = 0; i < MAX_TEXTURES; ++i)
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

    ps->np2_fixup_location = GL_EXTCALL(glGetUniformLocation(program_id, "ps_samplerNP2Fixup"));
    ps->ycorrection_location = GL_EXTCALL(glGetUniformLocation(program_id, "ycorrection"));
    ps->color_key_location = GL_EXTCALL(glGetUniformLocation(program_id, "color_key"));

    string_buffer_release(&priv->string_buffers, name);
}

static void shader_glsl_init_uniform_block_bindings(const struct wined3d_gl_info *gl_info,
        struct shader_glsl_priv *priv, GLuint program_id,
        const struct wined3d_shader_reg_maps *reg_maps, unsigned int base, unsigned int count)
{
    const char *prefix = shader_glsl_get_prefix(reg_maps->shader_version.type);
    GLuint block_idx;
    unsigned int i;
    struct wined3d_string_buffer *name = string_buffer_get(&priv->string_buffers);

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
static void set_glsl_shader_program(const struct wined3d_context *context, const struct wined3d_state *state,
        struct shader_glsl_priv *priv, struct glsl_context_data *ctx_data)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct ps_np2fixup_info *np2fixup_info = NULL;
    struct glsl_shader_prog_link *entry = NULL;
    struct wined3d_shader *vshader = NULL;
    struct wined3d_shader *gshader = NULL;
    struct wined3d_shader *pshader = NULL;
    GLuint program_id = 0;
    GLuint reorder_shader_id = 0;
    unsigned int i;
    GLuint vs_id = 0;
    GLuint gs_id = 0;
    GLuint ps_id = 0;
    struct list *ps_list = NULL, *vs_list = NULL;
    WORD attribs_map;
    struct wined3d_string_buffer *tmp_name;

    if (!(context->shader_update_mask & (1 << WINED3D_SHADER_TYPE_VERTEX)) && ctx_data->glsl_program)
    {
        vs_id = ctx_data->glsl_program->vs.id;
        vs_list = &ctx_data->glsl_program->vs.shader_entry;

        if (use_vs(state))
        {
            vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
            gshader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];

            if (!(context->shader_update_mask & (1 << WINED3D_SHADER_TYPE_GEOMETRY))
                    && ctx_data->glsl_program->gs.id)
                gs_id = ctx_data->glsl_program->gs.id;
            else if (gshader)
                gs_id = find_glsl_geometry_shader(context, &priv->shader_buffer, &priv->string_buffers, gshader);
        }
    }
    else if (use_vs(state))
    {
        struct vs_compile_args vs_compile_args;
        vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];

        find_vs_compile_args(state, vshader, context->stream_info.swizzle_map, &vs_compile_args);
        vs_id = find_glsl_vshader(context, &priv->shader_buffer, &priv->string_buffers, vshader, &vs_compile_args);
        vs_list = &vshader->linked_programs;

        if ((gshader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]))
            gs_id = find_glsl_geometry_shader(context, &priv->shader_buffer, &priv->string_buffers, gshader);
    }
    else if (priv->vertex_pipe == &glsl_vertex_pipe)
    {
        struct glsl_ffp_vertex_shader *ffp_shader;
        struct wined3d_ffp_vs_settings settings;

        wined3d_ffp_get_vs_settings(state, &context->stream_info, &settings);
        ffp_shader = shader_glsl_find_ffp_vertex_shader(priv, gl_info, &settings);
        vs_id = ffp_shader->id;
        vs_list = &ffp_shader->linked_programs;
    }

    if (!(context->shader_update_mask & (1 << WINED3D_SHADER_TYPE_PIXEL)) && ctx_data->glsl_program)
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
        find_ps_compile_args(state, pshader, context->stream_info.position_transformed, &ps_compile_args, gl_info);
        ps_id = find_glsl_pshader(context, &priv->shader_buffer, &priv->string_buffers,
                pshader, &ps_compile_args, &np2fixup_info);
        ps_list = &pshader->linked_programs;
    }
    else if (priv->fragment_pipe == &glsl_fragment_pipe)
    {
        struct glsl_ffp_fragment_shader *ffp_shader;
        struct ffp_frag_settings settings;

        gen_ffp_frag_op(context, state, &settings, FALSE);
        ffp_shader = shader_glsl_find_ffp_fragment_shader(priv, gl_info, &settings);
        ps_id = ffp_shader->id;
        ps_list = &ffp_shader->linked_programs;
    }

    if ((!vs_id && !gs_id && !ps_id) || (entry = get_glsl_program_entry(priv, vs_id, gs_id, ps_id)))
    {
        ctx_data->glsl_program = entry;
        return;
    }

    /* If we get to this point, then no matching program exists, so we create one */
    program_id = GL_EXTCALL(glCreateProgram());
    TRACE("Created new GLSL shader program %u.\n", program_id);

    /* Create the entry */
    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(struct glsl_shader_prog_link));
    entry->id = program_id;
    entry->vs.id = vs_id;
    entry->gs.id = gs_id;
    entry->ps.id = ps_id;
    entry->constant_version = 0;
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
        reorder_shader_id = generate_param_reorder_function(priv, vshader, pshader,
                state->gl_primitive_type == GL_POINTS && vshader->reg_maps.point_size, gl_info);
        TRACE("Attaching GLSL shader object %u to program %u.\n", reorder_shader_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, reorder_shader_id));
        checkGLcall("glAttachShader");
        /* Flag the reorder function for deletion, then it will be freed automatically when the program
         * is destroyed
         */
        GL_EXTCALL(glDeleteShader(reorder_shader_id));
    }
    else
    {
        attribs_map = (1 << WINED3D_FFP_ATTRIBS_COUNT) - 1;
    }

    /* Bind vertex attributes to a corresponding index number to match
     * the same index numbers as ARB_vertex_programs (makes loading
     * vertex attributes simpler).  With this method, we can use the
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
    }
    checkGLcall("glBindAttribLocation");
    string_buffer_release(&priv->string_buffers, tmp_name);

    if (gshader)
    {
        TRACE("Attaching GLSL geometry shader object %u to program %u.\n", gs_id, program_id);
        GL_EXTCALL(glAttachShader(program_id, gs_id));
        checkGLcall("glAttachShader");

        TRACE("input type %s, output type %s, vertices out %u.\n",
                debug_d3dprimitivetype(gshader->u.gs.input_type),
                debug_d3dprimitivetype(gshader->u.gs.output_type),
                gshader->u.gs.vertices_out);
        GL_EXTCALL(glProgramParameteriARB(program_id, GL_GEOMETRY_INPUT_TYPE_ARB,
                gl_primitive_type_from_d3d(gshader->u.gs.input_type)));
        GL_EXTCALL(glProgramParameteriARB(program_id, GL_GEOMETRY_OUTPUT_TYPE_ARB,
                gl_primitive_type_from_d3d(gshader->u.gs.output_type)));
        GL_EXTCALL(glProgramParameteriARB(program_id, GL_GEOMETRY_VERTICES_OUT_ARB,
                gshader->u.gs.vertices_out));
        checkGLcall("glProgramParameteriARB");

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
            vshader ? min(vshader->limits->constant_float, gl_info->limits.glsl_vs_float_constants) : 0);
    shader_glsl_init_ps_uniform_locations(gl_info, priv, program_id, &entry->ps,
            pshader ? min(pshader->limits->constant_float, gl_info->limits.glsl_ps_float_constants) : 0);
    checkGLcall("Find glsl program uniform locations");

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

    /* Set the shader to allow uniform loading on it */
    GL_EXTCALL(glUseProgram(program_id));
    checkGLcall("glUseProgram");

    /* Load the vertex and pixel samplers now. The function that finds the mappings makes sure
     * that it stays the same for each vertexshader-pixelshader pair(=linked glsl program). If
     * a pshader with fixed function pipeline is used there are no vertex samplers, and if a
     * vertex shader with fixed function pixel processing is used we make sure that the card
     * supports enough samplers to allow the max number of vertex samplers with all possible
     * fixed function fragment processing setups. So once the program is linked these samplers
     * won't change. */
    shader_glsl_load_samplers(gl_info, priv, context->tex_unit_map, program_id);

    entry->constant_update_mask = 0;
    if (vshader)
    {
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_F;
        if (vshader->reg_maps.integer_constants)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_I;
        if (vshader->reg_maps.boolean_constants)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_B;
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_POS_FIXUP;

        shader_glsl_init_uniform_block_bindings(gl_info, priv, program_id, &vshader->reg_maps,
                0, gl_info->limits.vertex_uniform_blocks);
    }
    else
    {
        entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW
                | WINED3D_SHADER_CONST_FFP_PROJ;

        for (i = 1; i < MAX_VERTEX_BLENDS; ++i)
        {
            if (entry->vs.modelview_matrix_location[i] != -1)
            {
                entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_VERTEXBLEND;
                break;
            }
        }

        for (i = 0; i < MAX_TEXTURES; ++i)
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
    if (entry->vs.pointsize_min_location != -1)
        entry->constant_update_mask |= WINED3D_SHADER_CONST_VS_POINTSIZE;

    if (gshader)
        shader_glsl_init_uniform_block_bindings(gl_info, priv, program_id, &gshader->reg_maps,
                gl_info->limits.vertex_uniform_blocks, gl_info->limits.geometry_uniform_blocks);

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

            shader_glsl_init_uniform_block_bindings(gl_info, priv, program_id, &pshader->reg_maps,
                    gl_info->limits.vertex_uniform_blocks + gl_info->limits.geometry_uniform_blocks,
                    gl_info->limits.fragment_uniform_blocks);
        }
        else
        {
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PS;
        }

        for (i = 0; i < MAX_TEXTURES; ++i)
        {
            if (entry->ps.bumpenv_mat_location[i] != -1)
            {
                entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_BUMP_ENV;
                break;
            }
        }

        if (entry->ps.fog_color_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_FOG;
        if (entry->ps.np2_fixup_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_PS_NP2_FIXUP;
        if (entry->ps.color_key_location != -1)
            entry->constant_update_mask |= WINED3D_SHADER_CONST_FFP_COLOR_KEY;
    }
}

/* Context activation is done by the caller. */
static GLuint create_glsl_blt_shader(const struct wined3d_gl_info *gl_info, enum wined3d_gl_resource_type tex_type,
        BOOL masked)
{
    GLuint program_id;
    GLuint vshader_id, pshader_id;
    const char *blt_pshader;

    static const char blt_vshader[] =
        "#version 120\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "}\n";

    static const char * const blt_pshaders_full[WINED3D_GL_RES_TYPE_COUNT] =
    {
        /* WINED3D_GL_RES_TYPE_TEX_1D */
        NULL,
        /* WINED3D_GL_RES_TYPE_TEX_2D */
        "#version 120\n"
        "uniform sampler2D sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_TEX_3D */
        NULL,
        /* WINED3D_GL_RES_TYPE_TEX_CUBE */
        "#version 120\n"
        "uniform samplerCube sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = textureCube(sampler, gl_TexCoord[0].xyz).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_TEX_RECT */
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2DRect(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_BUFFER */
        NULL,
        /* WINED3D_GL_RES_TYPE_RB */
        NULL,
    };

    static const char * const blt_pshaders_masked[WINED3D_GL_RES_TYPE_COUNT] =
    {
        /* WINED3D_GL_RES_TYPE_TEX_1D */
        NULL,
        /* WINED3D_GL_RES_TYPE_TEX_2D */
        "#version 120\n"
        "uniform sampler2D sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_TEX_3D */
        NULL,
        /* WINED3D_GL_RES_TYPE_TEX_CUBE */
        "#version 120\n"
        "uniform samplerCube sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = textureCube(sampler, gl_TexCoord[0].xyz).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_TEX_RECT */
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = texture2DRect(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* WINED3D_GL_RES_TYPE_BUFFER */
        NULL,
        /* WINED3D_GL_RES_TYPE_RB */
        NULL,
    };

    blt_pshader = masked ? blt_pshaders_masked[tex_type] : blt_pshaders_full[tex_type];
    if (!blt_pshader)
    {
        FIXME("tex_type %#x not supported\n", tex_type);
        return 0;
    }

    vshader_id = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
    shader_glsl_compile(gl_info, vshader_id, blt_vshader);

    pshader_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));
    shader_glsl_compile(gl_info, pshader_id, blt_pshader);

    program_id = GL_EXTCALL(glCreateProgram());
    GL_EXTCALL(glAttachShader(program_id, vshader_id));
    GL_EXTCALL(glAttachShader(program_id, pshader_id));
    GL_EXTCALL(glLinkProgram(program_id));

    shader_glsl_validate_link(gl_info, program_id);

    /* Once linked we can mark the shaders for deletion. They will be deleted once the program
     * is destroyed
     */
    GL_EXTCALL(glDeleteShader(vshader_id));
    GL_EXTCALL(glDeleteShader(pshader_id));
    return program_id;
}

/* Context activation is done by the caller. */
static void shader_glsl_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct glsl_context_data *ctx_data = context->shader_backend_data;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct shader_glsl_priv *priv = shader_priv;
    GLuint program_id = 0, prev_id = 0;
    GLenum old_vertex_color_clamp, current_vertex_color_clamp;

    priv->vertex_pipe->vp_enable(gl_info, !use_vs(state));
    priv->fragment_pipe->enable_extension(gl_info, !use_ps(state));

    if (ctx_data->glsl_program)
    {
        prev_id = ctx_data->glsl_program->id;
        old_vertex_color_clamp = ctx_data->glsl_program->vs.vertex_color_clamp;
    }
    else
    {
        prev_id = 0;
        old_vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    set_glsl_shader_program(context, state, priv, ctx_data);

    if (ctx_data->glsl_program)
    {
        program_id = ctx_data->glsl_program->id;
        current_vertex_color_clamp = ctx_data->glsl_program->vs.vertex_color_clamp;
    }
    else
    {
        program_id = 0;
        current_vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    if (old_vertex_color_clamp != current_vertex_color_clamp)
    {
        if (gl_info->supported[ARB_COLOR_BUFFER_FLOAT])
        {
            GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, current_vertex_color_clamp));
            checkGLcall("glClampColorARB");
        }
        else
        {
            FIXME("vertex color clamp needs to be changed, but extension not supported.\n");
        }
    }

    TRACE("Using GLSL program %u.\n", program_id);

    if (prev_id != program_id)
    {
        GL_EXTCALL(glUseProgram(program_id));
        checkGLcall("glUseProgram");

        if (program_id)
            context->constant_update_mask |= ctx_data->glsl_program->constant_update_mask;
    }
}

/* "context" is not necessarily the currently active context. */
static void shader_glsl_invalidate_current_program(struct wined3d_context *context)
{
    struct glsl_context_data *ctx_data = context->shader_backend_data;

    ctx_data->glsl_program = NULL;
    context->shader_update_mask = (1 << WINED3D_SHADER_TYPE_PIXEL)
            | (1 << WINED3D_SHADER_TYPE_VERTEX)
            | (1 << WINED3D_SHADER_TYPE_GEOMETRY);
}

/* Context activation is done by the caller. */
static void shader_glsl_disable(void *shader_priv, struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct shader_glsl_priv *priv = shader_priv;

    shader_glsl_invalidate_current_program(context);
    GL_EXTCALL(glUseProgram(0));
    checkGLcall("glUseProgram");

    priv->vertex_pipe->vp_enable(gl_info, FALSE);
    priv->fragment_pipe->enable_extension(gl_info, FALSE);

    if (gl_info->supported[ARB_COLOR_BUFFER_FLOAT])
    {
        GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FIXED_ONLY_ARB));
        checkGLcall("glClampColorARB");
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_select_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info,
        enum wined3d_gl_resource_type tex_type, const SIZE *ds_mask_size)
{
    BOOL masked = ds_mask_size->cx && ds_mask_size->cy;
    struct shader_glsl_priv *priv = shader_priv;
    GLuint *blt_program;
    GLint loc;

    blt_program = masked ? &priv->depth_blt_program_masked[tex_type] : &priv->depth_blt_program_full[tex_type];
    if (!*blt_program)
    {
        *blt_program = create_glsl_blt_shader(gl_info, tex_type, masked);
        loc = GL_EXTCALL(glGetUniformLocation(*blt_program, "sampler"));
        GL_EXTCALL(glUseProgram(*blt_program));
        GL_EXTCALL(glUniform1i(loc, 0));
    }
    else
    {
        GL_EXTCALL(glUseProgram(*blt_program));
    }

    if (masked)
    {
        loc = GL_EXTCALL(glGetUniformLocation(*blt_program, "mask"));
        GL_EXTCALL(glUniform4f(loc, 0.0f, 0.0f, (float)ds_mask_size->cx, (float)ds_mask_size->cy));
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_deselect_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info)
{
    const struct glsl_context_data *ctx_data = context_get_current()->shader_backend_data;
    GLuint program_id;

    program_id = ctx_data->glsl_program ? ctx_data->glsl_program->id : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);

    GL_EXTCALL(glUseProgram(program_id));
    checkGLcall("glUseProgram");
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
        HeapFree(GetProcessHeap(), 0, shader_data);
        shader->backend_data = NULL;
        return;
    }

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

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
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.ps);

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
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.vs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, vs.shader_entry)
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
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.gs);

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, gs.shader_entry)
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

    HeapFree(GetProcessHeap(), 0, shader->backend_data);
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

    return 0;
}

static BOOL constant_heap_init(struct constant_heap *heap, unsigned int constant_count)
{
    SIZE_T size = (constant_count + 1) * sizeof(*heap->entries)
            + constant_count * sizeof(*heap->contained)
            + constant_count * sizeof(*heap->positions);
    void *mem = HeapAlloc(GetProcessHeap(), 0, size);

    if (!mem)
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
    HeapFree(GetProcessHeap(), 0, heap->entries);
}

static const struct wine_rb_functions wined3d_glsl_program_rb_functions =
{
    wined3d_rb_alloc,
    wined3d_rb_realloc,
    wined3d_rb_free,
    glsl_program_key_compare,
};

static HRESULT shader_glsl_alloc(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
        const struct fragment_pipeline *fragment_pipe)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct shader_glsl_priv *priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_glsl_priv));
    SIZE_T stack_size = wined3d_log2i(max(gl_info->limits.glsl_vs_float_constants,
            gl_info->limits.glsl_ps_float_constants)) + 1;
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;

    string_buffer_list_init(&priv->string_buffers);

    if (!(vertex_priv = vertex_pipe->vp_alloc(&glsl_shader_backend, priv)))
    {
        ERR("Failed to initialize vertex pipe.\n");
        HeapFree(GetProcessHeap(), 0, priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&glsl_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        vertex_pipe->vp_free(device);
        HeapFree(GetProcessHeap(), 0, priv);
        return E_FAIL;
    }

    if (!string_buffer_init(&priv->shader_buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        goto fail;
    }

    priv->stack = HeapAlloc(GetProcessHeap(), 0, stack_size * sizeof(*priv->stack));
    if (!priv->stack)
    {
        ERR("Failed to allocate memory.\n");
        goto fail;
    }

    if (!constant_heap_init(&priv->vconst_heap, gl_info->limits.glsl_vs_float_constants))
    {
        ERR("Failed to initialize vertex shader constant heap\n");
        goto fail;
    }

    if (!constant_heap_init(&priv->pconst_heap, gl_info->limits.glsl_ps_float_constants))
    {
        ERR("Failed to initialize pixel shader constant heap\n");
        goto fail;
    }

    if (wine_rb_init(&priv->program_lookup, &wined3d_glsl_program_rb_functions) == -1)
    {
        ERR("Failed to initialize rbtree.\n");
        goto fail;
    }

    priv->next_constant_version = 1;
    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(gl_info, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;
    priv->legacy_lighting = device->wined3d->flags & WINED3D_LEGACY_FFP_LIGHTING;

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;

fail:
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    string_buffer_free(&priv->shader_buffer);
    fragment_pipe->free_private(device);
    vertex_pipe->vp_free(device);
    HeapFree(GetProcessHeap(), 0, priv);
    return E_OUTOFMEMORY;
}

/* Context activation is done by the caller. */
static void shader_glsl_free(struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct shader_glsl_priv *priv = device->shader_priv;
    int i;

    for (i = 0; i < WINED3D_GL_RES_TYPE_COUNT; ++i)
    {
        if (priv->depth_blt_program_full[i])
        {
            GL_EXTCALL(glDeleteProgram(priv->depth_blt_program_full[i]));
        }
        if (priv->depth_blt_program_masked[i])
        {
            GL_EXTCALL(glDeleteProgram(priv->depth_blt_program_masked[i]));
        }
    }

    wine_rb_destroy(&priv->program_lookup, NULL, NULL);
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    string_buffer_list_cleanup(&priv->string_buffers);
    string_buffer_free(&priv->shader_buffer);
    priv->fragment_pipe->free_private(device);
    priv->vertex_pipe->vp_free(device);

    HeapFree(GetProcessHeap(), 0, device->shader_priv);
    device->shader_priv = NULL;
}

static BOOL shader_glsl_allocate_context_data(struct wined3d_context *context)
{
    return !!(context->shader_backend_data = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, sizeof(struct glsl_context_data)));
}

static void shader_glsl_free_context_data(struct wined3d_context *context)
{
    HeapFree(GetProcessHeap(), 0, context->shader_backend_data);
}

static void shader_glsl_init_context_state(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    gl_info->gl_ops.gl.p_glEnable(GL_PROGRAM_POINT_SIZE);
    checkGLcall("GL_PROGRAM_POINT_SIZE");
}

static void shader_glsl_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *caps)
{
    UINT shader_model;

    if (gl_info->supported[EXT_GPU_SHADER4] && gl_info->supported[ARB_SHADER_BIT_ENCODING]
            && gl_info->supported[ARB_GEOMETRY_SHADER4] && gl_info->glsl_version >= MAKEDWORD_VERSION(1, 50)
            && gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX] && gl_info->supported[ARB_DRAW_INSTANCED]
            && gl_info->supported[ARB_TEXTURE_RG] && gl_info->supported[ARB_SAMPLER_OBJECTS])
        shader_model = 4;
    /* ARB_shader_texture_lod or EXT_gpu_shader4 is required for the SM3
     * texldd and texldl instructions. */
    else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD] || gl_info->supported[EXT_GPU_SHADER4])
        shader_model = 3;
    else
        shader_model = 2;
    TRACE("Shader model %u.\n", shader_model);

    caps->vs_version = min(wined3d_settings.max_sm_vs, shader_model);
    caps->gs_version = min(wined3d_settings.max_sm_gs, shader_model);
    caps->ps_version = min(wined3d_settings.max_sm_ps, shader_model);

    caps->vs_uniform_count = gl_info->limits.glsl_vs_float_constants;
    caps->ps_uniform_count = gl_info->limits.glsl_ps_float_constants;

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
}

static BOOL shader_glsl_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* We support everything except YUV conversions. */
    if (!is_complex_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

static const SHADER_HANDLER shader_glsl_instruction_handler_table[WINED3DSIH_TABLE_SIZE] =
{
    /* WINED3DSIH_ABS                   */ shader_glsl_map2gl,
    /* WINED3DSIH_ADD                   */ shader_glsl_binop,
    /* WINED3DSIH_AND                   */ shader_glsl_binop,
    /* WINED3DSIH_BEM                   */ shader_glsl_bem,
    /* WINED3DSIH_BREAK                 */ shader_glsl_break,
    /* WINED3DSIH_BREAKC                */ shader_glsl_breakc,
    /* WINED3DSIH_BREAKP                */ shader_glsl_breakp,
    /* WINED3DSIH_CALL                  */ shader_glsl_call,
    /* WINED3DSIH_CALLNZ                */ shader_glsl_callnz,
    /* WINED3DSIH_CMP                   */ shader_glsl_conditional_move,
    /* WINED3DSIH_CND                   */ shader_glsl_cnd,
    /* WINED3DSIH_CRS                   */ shader_glsl_cross,
    /* WINED3DSIH_CUT                   */ shader_glsl_cut,
    /* WINED3DSIH_DCL                   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_CONSTANT_BUFFER   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_INPUT_PRIMITIVE   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_OUTPUT_TOPOLOGY   */ shader_glsl_nop,
    /* WINED3DSIH_DCL_VERTICES_OUT      */ shader_glsl_nop,
    /* WINED3DSIH_DEF                   */ shader_glsl_nop,
    /* WINED3DSIH_DEFB                  */ shader_glsl_nop,
    /* WINED3DSIH_DEFI                  */ shader_glsl_nop,
    /* WINED3DSIH_DIV                   */ shader_glsl_binop,
    /* WINED3DSIH_DP2                   */ shader_glsl_dot,
    /* WINED3DSIH_DP2ADD                */ shader_glsl_dp2add,
    /* WINED3DSIH_DP3                   */ shader_glsl_dot,
    /* WINED3DSIH_DP4                   */ shader_glsl_dot,
    /* WINED3DSIH_DST                   */ shader_glsl_dst,
    /* WINED3DSIH_DSX                   */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY                   */ shader_glsl_map2gl,
    /* WINED3DSIH_ELSE                  */ shader_glsl_else,
    /* WINED3DSIH_EMIT                  */ shader_glsl_emit,
    /* WINED3DSIH_ENDIF                 */ shader_glsl_end,
    /* WINED3DSIH_ENDLOOP               */ shader_glsl_end,
    /* WINED3DSIH_ENDREP                */ shader_glsl_end,
    /* WINED3DSIH_EQ                    */ shader_glsl_relop,
    /* WINED3DSIH_EXP                   */ shader_glsl_scalar_op,
    /* WINED3DSIH_EXPP                  */ shader_glsl_expp,
    /* WINED3DSIH_FRC                   */ shader_glsl_map2gl,
    /* WINED3DSIH_FTOI                  */ shader_glsl_to_int,
    /* WINED3DSIH_GE                    */ shader_glsl_relop,
    /* WINED3DSIH_IADD                  */ shader_glsl_binop,
    /* WINED3DSIH_IEQ                   */ NULL,
    /* WINED3DSIH_IF                    */ shader_glsl_if,
    /* WINED3DSIH_IFC                   */ shader_glsl_ifc,
    /* WINED3DSIH_IGE                   */ shader_glsl_relop,
    /* WINED3DSIH_IMUL                  */ shader_glsl_imul,
    /* WINED3DSIH_ISHL                  */ shader_glsl_binop,
    /* WINED3DSIH_ITOF                  */ shader_glsl_to_float,
    /* WINED3DSIH_LABEL                 */ shader_glsl_label,
    /* WINED3DSIH_LD                    */ NULL,
    /* WINED3DSIH_LIT                   */ shader_glsl_lit,
    /* WINED3DSIH_LOG                   */ shader_glsl_scalar_op,
    /* WINED3DSIH_LOGP                  */ shader_glsl_scalar_op,
    /* WINED3DSIH_LOOP                  */ shader_glsl_loop,
    /* WINED3DSIH_LRP                   */ shader_glsl_lrp,
    /* WINED3DSIH_LT                    */ shader_glsl_relop,
    /* WINED3DSIH_M3x2                  */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x3                  */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x4                  */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x3                  */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x4                  */ shader_glsl_mnxn,
    /* WINED3DSIH_MAD                   */ shader_glsl_mad,
    /* WINED3DSIH_MAX                   */ shader_glsl_map2gl,
    /* WINED3DSIH_MIN                   */ shader_glsl_map2gl,
    /* WINED3DSIH_MOV                   */ shader_glsl_mov,
    /* WINED3DSIH_MOVA                  */ shader_glsl_mov,
    /* WINED3DSIH_MOVC                  */ shader_glsl_conditional_move,
    /* WINED3DSIH_MUL                   */ shader_glsl_binop,
    /* WINED3DSIH_NE                    */ shader_glsl_relop,
    /* WINED3DSIH_NOP                   */ shader_glsl_nop,
    /* WINED3DSIH_NRM                   */ shader_glsl_nrm,
    /* WINED3DSIH_OR                    */ shader_glsl_binop,
    /* WINED3DSIH_PHASE                 */ shader_glsl_nop,
    /* WINED3DSIH_POW                   */ shader_glsl_pow,
    /* WINED3DSIH_RCP                   */ shader_glsl_scalar_op,
    /* WINED3DSIH_REP                   */ shader_glsl_rep,
    /* WINED3DSIH_RET                   */ shader_glsl_ret,
    /* WINED3DSIH_ROUND_NI              */ shader_glsl_map2gl,
    /* WINED3DSIH_RSQ                   */ shader_glsl_scalar_op,
    /* WINED3DSIH_SAMPLE                */ shader_glsl_sample,
    /* WINED3DSIH_SAMPLE_GRAD           */ NULL,
    /* WINED3DSIH_SAMPLE_LOD            */ NULL,
    /* WINED3DSIH_SETP                  */ NULL,
    /* WINED3DSIH_SGE                   */ shader_glsl_compare,
    /* WINED3DSIH_SGN                   */ shader_glsl_sgn,
    /* WINED3DSIH_SINCOS                */ shader_glsl_sincos,
    /* WINED3DSIH_SLT                   */ shader_glsl_compare,
    /* WINED3DSIH_SQRT                  */ shader_glsl_map2gl,
    /* WINED3DSIH_SUB                   */ shader_glsl_binop,
    /* WINED3DSIH_TEX                   */ shader_glsl_tex,
    /* WINED3DSIH_TEXBEM                */ shader_glsl_texbem,
    /* WINED3DSIH_TEXBEML               */ shader_glsl_texbem,
    /* WINED3DSIH_TEXCOORD              */ shader_glsl_texcoord,
    /* WINED3DSIH_TEXDEPTH              */ shader_glsl_texdepth,
    /* WINED3DSIH_TEXDP3                */ shader_glsl_texdp3,
    /* WINED3DSIH_TEXDP3TEX             */ shader_glsl_texdp3tex,
    /* WINED3DSIH_TEXKILL               */ shader_glsl_texkill,
    /* WINED3DSIH_TEXLDD                */ shader_glsl_texldd,
    /* WINED3DSIH_TEXLDL                */ shader_glsl_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH          */ shader_glsl_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD            */ shader_glsl_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX            */ shader_glsl_texm3x2tex,
    /* WINED3DSIH_TEXM3x3               */ shader_glsl_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF           */ NULL,
    /* WINED3DSIH_TEXM3x3PAD            */ shader_glsl_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC           */ shader_glsl_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX            */ shader_glsl_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC          */ shader_glsl_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR             */ shader_glsl_texreg2ar,
    /* WINED3DSIH_TEXREG2GB             */ shader_glsl_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB            */ shader_glsl_texreg2rgb,
    /* WINED3DSIH_UDIV                  */ shader_glsl_udiv,
    /* WINED3DSIH_UGE                   */ shader_glsl_relop,
    /* WINED3DSIH_USHR                  */ shader_glsl_binop,
    /* WINED3DSIH_UTOF                  */ shader_glsl_to_float,
    /* WINED3DSIH_XOR                   */ shader_glsl_binop,
};

static void shader_glsl_handle_instruction(const struct wined3d_shader_instruction *ins) {
    SHADER_HANDLER hw_fct;

    /* Select handler */
    hw_fct = shader_glsl_instruction_handler_table[ins->handler_idx];

    /* Unhandled opcode */
    if (!hw_fct)
    {
        FIXME("Backend can't handle opcode %#x\n", ins->handler_idx);
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
    shader_glsl_select,
    shader_glsl_disable,
    shader_glsl_select_depth_blt,
    shader_glsl_deselect_depth_blt,
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

static void glsl_vertex_pipe_vp_enable(const struct wined3d_gl_info *gl_info, BOOL enable) {}

static void glsl_vertex_pipe_vp_get_caps(const struct wined3d_gl_info *gl_info, struct wined3d_vertex_caps *caps)
{
    caps->xyzrhw = TRUE;
    caps->ffp_generic_attributes = TRUE;
    caps->max_active_lights = MAX_ACTIVE_LIGHTS;
    caps->max_vertex_blend_matrices = MAX_VERTEX_BLENDS;
    caps->max_vertex_blend_matrix_index = 0;
    caps->vertex_processing_caps = WINED3DVTXPCAPS_TEXGEN
            | WINED3DVTXPCAPS_MATERIALSOURCE7
            | WINED3DVTXPCAPS_VERTEXFOG
            | WINED3DVTXPCAPS_DIRECTIONALLIGHTS
            | WINED3DVTXPCAPS_POSITIONALLIGHTS
            | WINED3DVTXPCAPS_LOCALVIEWER
            | WINED3DVTXPCAPS_TEXGEN_SPHEREMAP;
    caps->fvf_caps = WINED3DFVFCAPS_PSIZE | 8; /* 8 texture coordinates. */
    caps->max_user_clip_planes = gl_info->limits.clipplanes;
    caps->raster_caps = WINED3DPRASTERCAPS_FOGRANGE;
}

static void *glsl_vertex_pipe_vp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    struct shader_glsl_priv *priv;

    if (shader_backend == &glsl_shader_backend)
    {
        priv = shader_priv;

        if (wine_rb_init(&priv->ffp_vertex_shaders, &wined3d_ffp_vertex_program_rb_functions) == -1)
        {
            ERR("Failed to initialize rbtree.\n");
            return NULL;
        }

        return priv;
    }

    FIXME("GLSL vertex pipe without GLSL shader backend not implemented.\n");

    return NULL;
}

static void shader_glsl_free_ffp_vertex_shader(struct wine_rb_entry *entry, void *context)
{
    struct glsl_ffp_vertex_shader *shader = WINE_RB_ENTRY_VALUE(entry,
            struct glsl_ffp_vertex_shader, desc.entry);
    struct glsl_shader_prog_link *program, *program2;
    struct glsl_ffp_destroy_ctx *ctx = context;

    LIST_FOR_EACH_ENTRY_SAFE(program, program2, &shader->linked_programs,
            struct glsl_shader_prog_link, vs.shader_entry)
    {
        delete_glsl_program_entry(ctx->priv, ctx->gl_info, program);
    }
    ctx->gl_info->gl_ops.ext.p_glDeleteShader(shader->id);
    HeapFree(GetProcessHeap(), 0, shader);
}

/* Context activation is done by the caller. */
static void glsl_vertex_pipe_vp_free(struct wined3d_device *device)
{
    struct shader_glsl_priv *priv = device->vertex_priv;
    struct glsl_ffp_destroy_ctx ctx;

    ctx.priv = priv;
    ctx.gl_info = &device->adapter->gl_info;
    wine_rb_destroy(&priv->ffp_vertex_shaders, shader_glsl_free_ffp_vertex_shader, &ctx);
}

static void glsl_vertex_pipe_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_VERTEX;
}

static void glsl_vertex_pipe_vdecl(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    BOOL transformed = context->stream_info.position_transformed;
    BOOL wasrhw = context->last_was_rhw;
    unsigned int i;

    context->last_was_rhw = transformed;

    /* If the vertex declaration contains a transformed position attribute,
     * the draw uses the fixed function vertex pipeline regardless of any
     * vertex shader set by the application. */
    if (transformed != wasrhw)
        context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_VERTEX;

    if (!use_vs(state))
    {
        if (context->last_was_vshader)
        {
            for (i = 0; i < gl_info->limits.clipplanes; ++i)
                clipplane(context, state, STATE_CLIPPLANE(i));
        }

        context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_TEXMATRIX;

        /* Because of settings->texcoords, we have to always regenerate the
         * vertex shader on a vdecl change.
         * TODO: Just always output all the texcoords when there are enough
         * varyings available to drop the dependency. */
        context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_VERTEX;

        if (use_ps(state)
                && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.major == 1
                && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.minor <= 3)
            context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_PIXEL;
    }
    else
    {
        if (!context->last_was_vshader)
        {
            /* Vertex shader clipping ignores the view matrix. Update all clipplanes. */
            for (i = 0; i < gl_info->limits.clipplanes; ++i)
                clipplane(context, state, STATE_CLIPPLANE(i));
        }
    }

    context->last_was_vshader = use_vs(state);
}

static void glsl_vertex_pipe_vs(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_VERTEX;
    /* Different vertex shaders potentially require a different vertex attributes setup. */
    if (!isStateDirty(context, STATE_VDECL))
        context_apply_state(context, state, STATE_VDECL);
}

static void glsl_vertex_pipe_world(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW;
}

static void glsl_vertex_pipe_vertexblend(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_VERTEXBLEND;
}

static void glsl_vertex_pipe_view(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int k;

    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_MODELVIEW
            | WINED3D_SHADER_CONST_FFP_LIGHTS
            | WINED3D_SHADER_CONST_FFP_VERTEXBLEND;

    for (k = 0; k < gl_info->limits.clipplanes; ++k)
    {
        if (!isStateDirty(context, STATE_CLIPPLANE(k)))
            clipplane(context, state, STATE_CLIPPLANE(k));
    }
}

static void glsl_vertex_pipe_projection(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    /* Table fog behavior depends on the projection matrix. */
    if (state->render_states[WINED3D_RS_FOGENABLE]
            && state->render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
        context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_VERTEX;
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
    context->constant_update_mask |= WINED3D_SHADER_CONST_VS_POS_FIXUP;
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

    if (sampler >= MAX_TEXTURES)
        return;

    if ((np2 = !(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
            || context->lastWasPow2Texture & (1 << sampler))
    {
        if (np2)
            context->lastWasPow2Texture |= 1 << sampler;
        else
            context->lastWasPow2Texture &= ~(1 << sampler);

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

static const struct StateEntryTemplate glsl_vertex_pipe_vp_states[] =
{
    {STATE_VDECL,                                                {STATE_VDECL,                                                glsl_vertex_pipe_vdecl }, WINED3D_GL_EXT_NONE          },
    {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),                   glsl_vertex_pipe_vs    }, WINED3D_GL_EXT_NONE          },
    {STATE_MATERIAL,                                             {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    NULL                   }, WINED3D_GL_EXT_NONE          },
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    glsl_vertex_pipe_material}, WINED3D_GL_EXT_NONE        },
    /* Clip planes */
    {STATE_CLIPPLANE(0),                                         {STATE_CLIPPLANE(0),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(1),                                         {STATE_CLIPPLANE(1),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(2),                                         {STATE_CLIPPLANE(2),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(3),                                         {STATE_CLIPPLANE(3),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(4),                                         {STATE_CLIPPLANE(4),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(5),                                         {STATE_CLIPPLANE(5),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(6),                                         {STATE_CLIPPLANE(6),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(7),                                         {STATE_CLIPPLANE(7),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(8),                                         {STATE_CLIPPLANE(8),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(9),                                         {STATE_CLIPPLANE(9),                                         clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(10),                                        {STATE_CLIPPLANE(10),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(11),                                        {STATE_CLIPPLANE(11),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(12),                                        {STATE_CLIPPLANE(12),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(13),                                        {STATE_CLIPPLANE(13),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(14),                                        {STATE_CLIPPLANE(14),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(15),                                        {STATE_CLIPPLANE(15),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(16),                                        {STATE_CLIPPLANE(16),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(17),                                        {STATE_CLIPPLANE(17),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(18),                                        {STATE_CLIPPLANE(18),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(19),                                        {STATE_CLIPPLANE(19),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(20),                                        {STATE_CLIPPLANE(20),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(21),                                        {STATE_CLIPPLANE(21),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(22),                                        {STATE_CLIPPLANE(22),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(23),                                        {STATE_CLIPPLANE(23),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(24),                                        {STATE_CLIPPLANE(24),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(25),                                        {STATE_CLIPPLANE(25),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(26),                                        {STATE_CLIPPLANE(26),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(27),                                        {STATE_CLIPPLANE(27),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(28),                                        {STATE_CLIPPLANE(28),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(29),                                        {STATE_CLIPPLANE(29),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(30),                                        {STATE_CLIPPLANE(30),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
    {STATE_CLIPPLANE(31),                                        {STATE_CLIPPLANE(31),                                        clipplane              }, WINED3D_GL_EXT_NONE          },
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
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),                 state_pointsprite_w    }, WINED3D_GL_EXT_NONE          },
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
    {0 /* Terminate */,                                          {0,                                                          NULL                   }, WINED3D_GL_EXT_NONE          },
};

/* TODO:
 *   - This currently depends on GL fixed function functions to set things
 *     like light parameters. Ideally we'd use regular uniforms for that.
 *   - In part because of the previous point, much of this is modelled after
 *     GL fixed function, and has much of the same limitations. For example,
 *     D3D spot lights are slightly different from GL spot lights.
 *   - We can now implement drawing transformed vertices using the GLSL pipe,
 *     instead of using the immediate mode fallback.
 *   - Similarly, we don't need the fallback for certain combinations of
 *     material sources anymore.
 *   - Implement vertex tweening.
 *   - Handle WINED3D_TSS_TEXCOORD_INDEX in the shader, instead of duplicating
 *     attribute arrays in load_tex_coords().
 *   - Per-vertex point sizes. */
const struct wined3d_vertex_pipe_ops glsl_vertex_pipe =
{
    glsl_vertex_pipe_vp_enable,
    glsl_vertex_pipe_vp_get_caps,
    glsl_vertex_pipe_vp_alloc,
    glsl_vertex_pipe_vp_free,
    glsl_vertex_pipe_vp_states,
};

static void glsl_fragment_pipe_enable(const struct wined3d_gl_info *gl_info, BOOL enable)
{
    /* Nothing to do. */
}

static void glsl_fragment_pipe_get_caps(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps)
{
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
    caps->MaxTextureBlendStages = 8;
    caps->MaxSimultaneousTextures = min(gl_info->limits.fragment_samplers, 8);
}

static void *glsl_fragment_pipe_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    struct shader_glsl_priv *priv;

    if (shader_backend == &glsl_shader_backend)
    {
        priv = shader_priv;

        if (wine_rb_init(&priv->ffp_fragment_shaders, &wined3d_ffp_frag_program_rb_functions) == -1)
        {
            ERR("Failed to initialize rbtree.\n");
            return NULL;
        }

        return priv;
    }

    FIXME("GLSL fragment pipe without GLSL shader backend not implemented.\n");

    return NULL;
}

static void shader_glsl_free_ffp_fragment_shader(struct wine_rb_entry *entry, void *context)
{
    struct glsl_ffp_fragment_shader *shader = WINE_RB_ENTRY_VALUE(entry,
            struct glsl_ffp_fragment_shader, entry.entry);
    struct glsl_shader_prog_link *program, *program2;
    struct glsl_ffp_destroy_ctx *ctx = context;

    LIST_FOR_EACH_ENTRY_SAFE(program, program2, &shader->linked_programs,
            struct glsl_shader_prog_link, ps.shader_entry)
    {
        delete_glsl_program_entry(ctx->priv, ctx->gl_info, program);
    }
    ctx->gl_info->gl_ops.ext.p_glDeleteShader(shader->id);
    HeapFree(GetProcessHeap(), 0, shader);
}

/* Context activation is done by the caller. */
static void glsl_fragment_pipe_free(struct wined3d_device *device)
{
    struct shader_glsl_priv *priv = device->fragment_priv;
    struct glsl_ffp_destroy_ctx ctx;

    ctx.priv = priv;
    ctx.gl_info = &device->adapter->gl_info;
    wine_rb_destroy(&priv->ffp_fragment_shaders, shader_glsl_free_ffp_fragment_shader, &ctx);
}

static void glsl_fragment_pipe_shader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->last_was_pshader = use_ps(state);

    context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_PIXEL;
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

    context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_PIXEL;

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
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_FOGENABLE)))
        glsl_fragment_pipe_fog(context, state, state_id);
}

static void glsl_fragment_pipe_tex_transform(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1 << WINED3D_SHADER_TYPE_PIXEL;
}

static void glsl_fragment_pipe_invalidate_constants(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PS;
}

static void glsl_fragment_pipe_alpha_test(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
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

    ref = ((float)state->render_states[WINED3D_RS_ALPHAREF]) / 255.0f;
    glParm = wined3d_gl_compare_func(state->render_states[WINED3D_RS_ALPHAFUNC]);

    if (glParm)
    {
        gl_info->gl_ops.gl.p_glAlphaFunc(glParm, ref);
        checkGLcall("glAlphaFunc");
    }
}

static void glsl_fragment_pipe_color_key(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_COLOR_KEY;
}

static const struct StateEntryTemplate glsl_fragment_pipe_state_template[] =
{
    {STATE_VDECL,                                               {STATE_VDECL,                                                glsl_fragment_pipe_vdecl               }, WINED3D_GL_EXT_NONE },
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
    {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                        {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_ALPHAREF),                         {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                  {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),                   glsl_fragment_pipe_alpha_test          }, WINED3D_GL_EXT_NONE },
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
    {0 /* Terminate */,                                         {0,                                                          0                                      }, WINED3D_GL_EXT_NONE },
};

static BOOL glsl_fragment_pipe_alloc_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void glsl_fragment_pipe_free_context_data(struct wined3d_context *context)
{
}

const struct fragment_pipeline glsl_fragment_pipe =
{
    glsl_fragment_pipe_enable,
    glsl_fragment_pipe_get_caps,
    glsl_fragment_pipe_alloc,
    glsl_fragment_pipe_free,
    glsl_fragment_pipe_alloc_context_data,
    glsl_fragment_pipe_free_context_data,
    shader_glsl_color_fixup_supported,
    glsl_fragment_pipe_state_template,
};
