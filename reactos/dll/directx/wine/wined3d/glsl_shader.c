/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green
 * Copyright 2006-2007 Henri Verbeet
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

/*
 * D3D shader asm has swizzles on source parameters, and write masks for
 * destination parameters. GLSL uses swizzles for both. The result of this is
 * that for example "mov dst.xw, src.zyxw" becomes "dst.xw = src.zw" in GLSL.
 * Ie, to generate a proper GLSL source swizzle, we need to take the D3D write
 * mask for the destination parameter into account.
 */

#include <config.h>
#include <wine/port.h>

//#include <limits.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINED3D_GLSL_SAMPLE_PROJECTED   0x1
#define WINED3D_GLSL_SAMPLE_RECT        0x2
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
    const char *name;
    DWORD coord_mask;
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
    unsigned int *positions;
    unsigned int size;
};

/* GLSL shader private data */
struct shader_glsl_priv {
    struct wined3d_shader_buffer shader_buffer;
    struct wine_rb_tree program_lookup;
    struct glsl_shader_prog_link *glsl_program;
    struct constant_heap vconst_heap;
    struct constant_heap pconst_heap;
    unsigned char *stack;
    GLhandleARB depth_blt_program_full[tex_type_count];
    GLhandleARB depth_blt_program_masked[tex_type_count];
    UINT next_constant_version;

    const struct fragment_pipeline *fragment_pipe;
    struct wine_rb_tree ffp_fragment_shaders;
    BOOL ffp_proj_control;
};

struct glsl_vs_program
{
    struct list shader_entry;
    GLhandleARB id;
    GLenum vertex_color_clamp;
    GLint *uniform_f_locations;
    GLint uniform_i_locations[MAX_CONST_I];
    GLint pos_fixup_location;
};

struct glsl_gs_program
{
    struct list shader_entry;
    GLhandleARB id;
};

struct glsl_ps_program
{
    struct list shader_entry;
    GLhandleARB id;
    GLint *uniform_f_locations;
    GLint uniform_i_locations[MAX_CONST_I];
    GLint bumpenv_mat_location[MAX_TEXTURES];
    GLint bumpenv_lum_scale_location[MAX_TEXTURES];
    GLint bumpenv_lum_offset_location[MAX_TEXTURES];
    GLint tex_factor_location;
    GLint specular_enable_location;
    GLint ycorrection_location;
    GLint np2_fixup_location;
    const struct ps_np2fixup_info *np2_fixup_info;
};

/* Struct to maintain data about a linked GLSL program */
struct glsl_shader_prog_link
{
    struct wine_rb_entry program_lookup_entry;
    struct glsl_vs_program vs;
    struct glsl_gs_program gs;
    struct glsl_ps_program ps;
    GLhandleARB programId;
    UINT constant_version;
};

struct glsl_program_key
{
    GLhandleARB vs_id;
    GLhandleARB gs_id;
    GLhandleARB ps_id;
};

struct shader_glsl_ctx_priv {
    const struct vs_compile_args    *cur_vs_args;
    const struct ps_compile_args    *cur_ps_args;
    struct ps_np2fixup_info         *cur_np2fixup_info;
};

struct glsl_ps_compiled_shader
{
    struct ps_compile_args          args;
    struct ps_np2fixup_info         np2fixup;
    GLhandleARB                     prgId;
};

struct glsl_vs_compiled_shader
{
    struct vs_compile_args          args;
    GLhandleARB                     prgId;
};

struct glsl_gs_compiled_shader
{
    GLhandleARB id;
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

struct glsl_ffp_fragment_shader
{
    struct ffp_frag_desc entry;
    GLhandleARB id;
    struct list linked_programs;
};

static const char *debug_gl_shader_type(GLenum type)
{
    switch (type)
    {
#define WINED3D_TO_STR(u) case u: return #u
        WINED3D_TO_STR(GL_VERTEX_SHADER_ARB);
        WINED3D_TO_STR(GL_GEOMETRY_SHADER_ARB);
        WINED3D_TO_STR(GL_FRAGMENT_SHADER_ARB);
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

/* Extract a line from the info log.
 * Note that this modifies the source string. */
static char *get_info_log_line(char **ptr)
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

/* Context activation is done by the caller. */
static void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLhandleARB obj)
{
    int infologLength = 0;
    char *infoLog;

    if (!WARN_ON(d3d_shader) && !FIXME_ON(d3d_shader))
        return;

    GL_EXTCALL(glGetObjectParameterivARB(obj,
               GL_OBJECT_INFO_LOG_LENGTH_ARB,
               &infologLength));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (infologLength > 1)
    {
        char *ptr, *line;

        infoLog = HeapAlloc(GetProcessHeap(), 0, infologLength);
        /* The info log is supposed to be zero-terminated, but at least some
         * versions of fglrx don't terminate the string properly. The reported
         * length does include the terminator, so explicitly set it to zero
         * here. */
        infoLog[infologLength - 1] = 0;
        GL_EXTCALL(glGetInfoLogARB(obj, infologLength, NULL, infoLog));

        ptr = infoLog;
        if (gl_info->quirks & WINED3D_QUIRK_INFO_LOG_SPAM)
        {
            WARN("Info log received from GLSL shader #%u:\n", obj);
            while ((line = get_info_log_line(&ptr))) WARN("    %s\n", line);
        }
        else
        {
            FIXME("Info log received from GLSL shader #%u:\n", obj);
            while ((line = get_info_log_line(&ptr))) FIXME("    %s\n", line);
        }
        HeapFree(GetProcessHeap(), 0, infoLog);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_compile(const struct wined3d_gl_info *gl_info, GLhandleARB shader, const char *src)
{
    TRACE("Compiling shader object %u.\n", shader);
    GL_EXTCALL(glShaderSourceARB(shader, 1, &src, NULL));
    checkGLcall("glShaderSourceARB");
    GL_EXTCALL(glCompileShaderARB(shader));
    checkGLcall("glCompileShaderARB");
    print_glsl_info_log(gl_info, shader);
}

/* Context activation is done by the caller. */
static void shader_glsl_dump_program_source(const struct wined3d_gl_info *gl_info, GLhandleARB program)
{
    GLint i, object_count, source_size = -1;
    GLhandleARB *objects;
    char *source = NULL;

    GL_EXTCALL(glGetObjectParameterivARB(program, GL_OBJECT_ATTACHED_OBJECTS_ARB, &object_count));
    objects = HeapAlloc(GetProcessHeap(), 0, object_count * sizeof(*objects));
    if (!objects)
    {
        ERR("Failed to allocate object array memory.\n");
        return;
    }

    GL_EXTCALL(glGetAttachedObjectsARB(program, object_count, NULL, objects));
    for (i = 0; i < object_count; ++i)
    {
        char *ptr, *line;
        GLint tmp;

        GL_EXTCALL(glGetObjectParameterivARB(objects[i], GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &tmp));

        if (source_size < tmp)
        {
            HeapFree(GetProcessHeap(), 0, source);

            source = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tmp);
            if (!source)
            {
                ERR("Failed to allocate %d bytes for shader source.\n", tmp);
                HeapFree(GetProcessHeap(), 0, objects);
                return;
            }
            source_size = tmp;
        }

        FIXME("Object %u:\n", objects[i]);
        GL_EXTCALL(glGetObjectParameterivARB(objects[i], GL_OBJECT_SUBTYPE_ARB, &tmp));
        FIXME("    GL_OBJECT_SUBTYPE_ARB: %s.\n", debug_gl_shader_type(tmp));
        GL_EXTCALL(glGetObjectParameterivARB(objects[i], GL_OBJECT_COMPILE_STATUS_ARB, &tmp));
        FIXME("    GL_OBJECT_COMPILE_STATUS_ARB: %d.\n", tmp);
        FIXME("\n");

        ptr = source;
        GL_EXTCALL(glGetShaderSourceARB(objects[i], source_size, NULL, source));
        while ((line = get_info_log_line(&ptr))) FIXME("    %s\n", line);
        FIXME("\n");
    }

    HeapFree(GetProcessHeap(), 0, source);
    HeapFree(GetProcessHeap(), 0, objects);
}

/* Context activation is done by the caller. */
static void shader_glsl_validate_link(const struct wined3d_gl_info *gl_info, GLhandleARB program)
{
    GLint tmp;

    if (!TRACE_ON(d3d_shader) && !FIXME_ON(d3d_shader)) return;

    GL_EXTCALL(glGetObjectParameterivARB(program, GL_OBJECT_TYPE_ARB, &tmp));
    if (tmp == GL_PROGRAM_OBJECT_ARB)
    {
        GL_EXTCALL(glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &tmp));
        if (!tmp)
        {
            FIXME("Program %u link status invalid.\n", program);
            shader_glsl_dump_program_source(gl_info, program);
        }
    }

    print_glsl_info_log(gl_info, program);
}

/* Context activation is done by the caller. */
static void shader_glsl_load_psamplers(const struct wined3d_gl_info *gl_info,
        const DWORD *tex_unit_map, GLhandleARB programId)
{
    GLint name_loc;
    char sampler_name[20];
    unsigned int i;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        snprintf(sampler_name, sizeof(sampler_name), "ps_sampler%u", i);
        name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
        if (name_loc != -1) {
            DWORD mapped_unit = tex_unit_map[i];
            if (mapped_unit != WINED3D_UNMAPPED_STAGE && mapped_unit < gl_info->limits.fragment_samplers)
            {
                TRACE("Loading %s for texture %d\n", sampler_name, mapped_unit);
                GL_EXTCALL(glUniform1iARB(name_loc, mapped_unit));
                checkGLcall("glUniform1iARB");
            } else {
                ERR("Trying to load sampler %s on unsupported unit %d\n", sampler_name, mapped_unit);
            }
        }
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_load_vsamplers(const struct wined3d_gl_info *gl_info,
        const DWORD *tex_unit_map, GLhandleARB programId)
{
    GLint name_loc;
    char sampler_name[20];
    unsigned int i;

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i)
    {
        snprintf(sampler_name, sizeof(sampler_name), "vs_sampler%u", i);
        name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
        if (name_loc != -1) {
            DWORD mapped_unit = tex_unit_map[MAX_FRAGMENT_SAMPLERS + i];
            if (mapped_unit != WINED3D_UNMAPPED_STAGE && mapped_unit < gl_info->limits.combined_samplers)
            {
                TRACE("Loading %s for texture %d\n", sampler_name, mapped_unit);
                GL_EXTCALL(glUniform1iARB(name_loc, mapped_unit));
                checkGLcall("glUniform1iARB");
            } else {
                ERR("Trying to load sampler %s on unsupported unit %d\n", sampler_name, mapped_unit);
            }
        }
    }
}

/* Context activation is done by the caller. */
static inline void walk_constant_heap(const struct wined3d_gl_info *gl_info, const float *constants,
        const GLint *constant_locations, const struct constant_heap *heap, unsigned char *stack, DWORD version)
{
    int stack_idx = 0;
    unsigned int heap_idx = 1;
    unsigned int idx;

    if (heap->entries[heap_idx].version <= version) return;

    idx = heap->entries[heap_idx].idx;
    if (constant_locations[idx] != -1) GL_EXTCALL(glUniform4fvARB(constant_locations[idx], 1, &constants[idx * 4]));
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
                        GL_EXTCALL(glUniform4fvARB(constant_locations[idx], 1, &constants[idx * 4]));

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
                        GL_EXTCALL(glUniform4fvARB(constant_locations[idx], 1, &constants[idx * 4]));

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

    GL_EXTCALL(glUniform4fvARB(location, 1, clamped_constant));
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
        GLint location = constant_locations[lconst->idx];
        /* We found this uniform name in the program - go ahead and send the data */
        if (location != -1) GL_EXTCALL(glUniform4fvARB(location, 1, (const GLfloat *)lconst->value));
    }
    checkGLcall("glUniform4fvARB()");
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

        TRACE_(d3d_constants)("Loading constants %u: %i, %i, %i, %i\n",
                i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4ivARB(locations[i], 1, &constants[i*4]));
        checkGLcall("glUniform4ivARB");
    }

    /* Load immediate constants */
    ptr = list_head(&shader->constantsI);
    while (ptr)
    {
        const struct wined3d_shader_lconst *lconst = LIST_ENTRY(ptr, const struct wined3d_shader_lconst, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i, %i, %i, %i\n", idx,
            values[0], values[1], values[2], values[3]);

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4ivARB(locations[idx], 1, values));
        checkGLcall("glUniform4ivARB");
        ptr = list_next(&shader->constantsI, ptr);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_load_constantsB(const struct wined3d_shader *shader, const struct wined3d_gl_info *gl_info,
        GLhandleARB programId, const BOOL *constants, WORD constants_set)
{
    GLint tmp_loc;
    unsigned int i;
    char tmp_name[10];
    const char *prefix;
    struct list* ptr;

    prefix = shader_glsl_get_prefix(shader->reg_maps.shader_version.type);

    /* TODO: Benchmark and see if it would be beneficial to store the
     * locations of the constants to avoid looking up each time */
    for (i = 0; constants_set; constants_set >>= 1, ++i)
    {
        if (!(constants_set & 1)) continue;

        TRACE_(d3d_constants)("Loading constants %i: %i;\n", i, constants[i]);

        /* TODO: Benchmark and see if it would be beneficial to store the
         * locations of the constants to avoid looking up each time */
        snprintf(tmp_name, sizeof(tmp_name), "%s_b[%i]", prefix, i);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1)
        {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, &constants[i]));
            checkGLcall("glUniform1ivARB");
        }
    }

    /* Load immediate constants */
    ptr = list_head(&shader->constantsB);
    while (ptr)
    {
        const struct wined3d_shader_lconst *lconst = LIST_ENTRY(ptr, const struct wined3d_shader_lconst, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i\n", idx, values[0]);

        snprintf(tmp_name, sizeof(tmp_name), "%s_b[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform1ivARB");
        }
        ptr = list_next(&shader->constantsB, ptr);
    }
}

static void reset_program_constant_version(struct wine_rb_entry *entry, void *context)
{
    WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry)->constant_version = 0;
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_np2fixup_constants(void *shader_priv,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    struct shader_glsl_priv *glsl_priv = shader_priv;
    const struct glsl_shader_prog_link *prog = glsl_priv->glsl_program;

    /* No GLSL program set - nothing to do. */
    if (!prog) return;

    /* NP2 texcoord fixup is (currently) only done for pixelshaders. */
    if (!use_ps(state)) return;

    if (prog->ps.np2_fixup_info && prog->ps.np2_fixup_location != -1)
    {
        UINT i;
        UINT fixup = prog->ps.np2_fixup_info->active;
        GLfloat np2fixup_constants[4 * MAX_FRAGMENT_SAMPLERS];

        for (i = 0; fixup; fixup >>= 1, ++i)
        {
            const struct wined3d_texture *tex = state->textures[i];
            const unsigned char idx = prog->ps.np2_fixup_info->idx[i];
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

        GL_EXTCALL(glUniform4fvARB(prog->ps.np2_fixup_location,
                prog->ps.np2_fixup_info->num_consts, np2fixup_constants));
    }
}

/* Context activation is done by the caller (state handler). */
static void shader_glsl_load_constants(const struct wined3d_context *context,
        BOOL usePixelShader, BOOL useVertexShader)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_device *device = context->swapchain->device;
    struct wined3d_stateblock *stateBlock = device->stateBlock;
    const struct wined3d_state *state = &stateBlock->state;
    struct shader_glsl_priv *priv = device->shader_priv;
    float position_fixup[4];

    GLhandleARB programId;
    struct glsl_shader_prog_link *prog = priv->glsl_program;
    UINT constant_version;
    int i;

    if (!prog) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    programId = prog->programId;
    constant_version = prog->constant_version;

    if (useVertexShader)
    {
        const struct wined3d_shader *vshader = state->vertex_shader;

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, state->vs_consts_f,
                prog->vs.uniform_f_locations, &priv->vconst_heap, priv->stack, constant_version);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, prog->vs.uniform_i_locations, state->vs_consts_i,
                stateBlock->changed.vertexShaderConstantsI & vshader->reg_maps.integer_constants);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, state->vs_consts_b,
                stateBlock->changed.vertexShaderConstantsB & vshader->reg_maps.boolean_constants);

        /* Upload the position fixup params */
        shader_get_position_fixup(context, state, position_fixup);
        GL_EXTCALL(glUniform4fvARB(prog->vs.pos_fixup_location, 1, position_fixup));
        checkGLcall("glUniform4fvARB");
    }

    if (usePixelShader)
    {
        const struct wined3d_shader *pshader = state->pixel_shader;

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, state->ps_consts_f,
                prog->ps.uniform_f_locations, &priv->pconst_heap, priv->stack, constant_version);

        /* Load DirectX 9 integer constants/uniforms for pixel shader */
        shader_glsl_load_constantsI(pshader, gl_info, prog->ps.uniform_i_locations, state->ps_consts_i,
                stateBlock->changed.pixelShaderConstantsI & pshader->reg_maps.integer_constants);

        /* Load DirectX 9 boolean constants/uniforms for pixel shader */
        shader_glsl_load_constantsB(pshader, gl_info, programId, state->ps_consts_b,
                stateBlock->changed.pixelShaderConstantsB & pshader->reg_maps.boolean_constants);

        /* Upload the environment bump map matrix if needed. The needsbumpmat
         * member specifies the texture stage to load the matrix from. It
         * can't be 0 for a valid texbem instruction. */
        for (i = 0; i < MAX_TEXTURES; ++i)
        {
            const float *data;

            if (prog->ps.bumpenv_mat_location[i] == -1)
                continue;

            data = (const float *)&state->texture_states[i][WINED3D_TSS_BUMPENV_MAT00];
            GL_EXTCALL(glUniformMatrix2fvARB(prog->ps.bumpenv_mat_location[i], 1, 0, data));
            checkGLcall("glUniformMatrix2fvARB");

            /* texbeml needs the luminance scale and offset too. If texbeml
             * is used, needsbumpmat is set too, so we can check that in the
             * needsbumpmat check. */
            if (prog->ps.bumpenv_lum_scale_location[i] != -1)
            {
                const GLfloat *scale = (const GLfloat *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LSCALE];
                const GLfloat *offset = (const GLfloat *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LOFFSET];

                GL_EXTCALL(glUniform1fvARB(prog->ps.bumpenv_lum_scale_location[i], 1, scale));
                checkGLcall("glUniform1fvARB");
                GL_EXTCALL(glUniform1fvARB(prog->ps.bumpenv_lum_offset_location[i], 1, offset));
                checkGLcall("glUniform1fvARB");
            }
        }

        if (prog->ps.ycorrection_location != -1)
        {
            float correction_params[4];

            if (context->render_offscreen)
            {
                correction_params[0] = 0.0f;
                correction_params[1] = 1.0f;
            } else {
                /* position is window relative, not viewport relative */
                correction_params[0] = (float) context->current_rt->resource.height;
                correction_params[1] = -1.0f;
            }
            GL_EXTCALL(glUniform4fvARB(prog->ps.ycorrection_location, 1, correction_params));
        }
    }
    else if (priv->fragment_pipe == &glsl_fragment_pipe)
    {
        float col[4];

        for (i = 0; i < MAX_TEXTURES; ++i)
        {
            GL_EXTCALL(glUniformMatrix2fvARB(prog->ps.bumpenv_mat_location[i], 1, 0,
                        (const float *)&state->texture_states[i][WINED3D_TSS_BUMPENV_MAT00]));
            GL_EXTCALL(glUniform1fARB(prog->ps.bumpenv_lum_scale_location[i],
                        *(const float *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LSCALE]));
            GL_EXTCALL(glUniform1fARB(prog->ps.bumpenv_lum_offset_location[i],
                        *(const float *)&state->texture_states[i][WINED3D_TSS_BUMPENV_LOFFSET]));
        }

        D3DCOLORTOGLFLOAT4(state->render_states[WINED3D_RS_TEXTUREFACTOR], col);
        GL_EXTCALL(glUniform4fARB(prog->ps.tex_factor_location, col[0], col[1], col[2], col[3]));

        if (state->render_states[WINED3D_RS_SPECULARENABLE])
            GL_EXTCALL(glUniform4fARB(prog->ps.specular_enable_location, 1.0f, 1.0f, 1.0f, 0.0f));
        else
            GL_EXTCALL(glUniform4fARB(prog->ps.specular_enable_location, 0.0f, 0.0f, 0.0f, 0.0f));

        checkGLcall("fixed function uniforms");
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

static void update_heap_entry(const struct constant_heap *heap, unsigned int idx,
        unsigned int heap_idx, DWORD new_version)
{
    struct constant_entry *entries = heap->entries;
    unsigned int *positions = heap->positions;
    unsigned int parent_idx;

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
        if (!device->stateBlock->changed.vertexShaderConstantsF[i])
            update_heap_entry(heap, i, heap->size++, priv->next_constant_version);
        else
            update_heap_entry(heap, i, heap->positions[i], priv->next_constant_version);
    }
}

static void shader_glsl_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count)
{
    struct shader_glsl_priv *priv = device->shader_priv;
    struct constant_heap *heap = &priv->pconst_heap;
    UINT i;

    for (i = start; i < count + start; ++i)
    {
        if (!device->stateBlock->changed.pixelShaderConstantsF[i])
            update_heap_entry(heap, i, heap->size++, priv->next_constant_version);
        else
            update_heap_entry(heap, i, heap->positions[i], priv->next_constant_version);
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
        struct wined3d_shader_buffer *buffer, const struct wined3d_shader *shader,
        const struct wined3d_shader_reg_maps *reg_maps, const struct shader_glsl_ctx_priv *ctx_priv)
{
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_state *state = &shader->device->stateBlock->state;
    const struct ps_compile_args *ps_args = ctx_priv->cur_ps_args;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_fb_state *fb = &shader->device->fb;
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
    if (shader->limits.constant_float > 0)
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

                if (max_constantsF < shader->limits.constant_float)
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
        max_constantsF = min(shader->limits.constant_float, max_constantsF);
        shader_addline(buffer, "uniform vec4 %s_c[%u];\n", prefix, max_constantsF);
    }

    /* Always declare the full set of constants, the compiler can remove the
     * unused ones because d3d doesn't (yet) support indirect int and bool
     * constant addressing. This avoids problems if the app uses e.g. i0 and i9. */
    if (shader->limits.constant_int > 0 && reg_maps->integer_constants)
        shader_addline(buffer, "uniform ivec4 %s_i[%u];\n", prefix, shader->limits.constant_int);

    if (shader->limits.constant_bool > 0 && reg_maps->boolean_constants)
        shader_addline(buffer, "uniform bool %s_b[%u];\n", prefix, shader->limits.constant_bool);

    for (i = 0; i < WINED3D_MAX_CBS; ++i)
    {
        if (reg_maps->cb_sizes[i])
            shader_addline(buffer, "uniform vec4 %s_cb%u[%u];\n", prefix, i, reg_maps->cb_sizes[i]);
    }

    /* Declare texture samplers */
    for (i = 0; i < shader->limits.sampler; ++i)
    {
        if (reg_maps->sampler_type[i])
        {
            BOOL shadow_sampler = version->type == WINED3D_SHADER_TYPE_PIXEL && (ps_args->shadow & (1 << i));
            const struct wined3d_texture *texture;

            switch (reg_maps->sampler_type[i])
            {
                case WINED3DSTT_1D:
                    if (shadow_sampler)
                        shader_addline(buffer, "uniform sampler1DShadow %s_sampler%u;\n", prefix, i);
                    else
                        shader_addline(buffer, "uniform sampler1D %s_sampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_2D:
                    texture = state->textures[i];
                    if (shadow_sampler)
                    {
                        if (texture && texture->target == GL_TEXTURE_RECTANGLE_ARB)
                            shader_addline(buffer, "uniform sampler2DRectShadow %s_sampler%u;\n", prefix, i);
                        else
                            shader_addline(buffer, "uniform sampler2DShadow %s_sampler%u;\n", prefix, i);
                    }
                    else
                    {
                        if (texture && texture->target == GL_TEXTURE_RECTANGLE_ARB)
                            shader_addline(buffer, "uniform sampler2DRect %s_sampler%u;\n", prefix, i);
                        else
                            shader_addline(buffer, "uniform sampler2D %s_sampler%u;\n", prefix, i);
                    }
                    break;
                case WINED3DSTT_CUBE:
                    if (shadow_sampler)
                        FIXME("Unsupported Cube shadow sampler.\n");
                    shader_addline(buffer, "uniform samplerCube %s_sampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_VOLUME:
                    if (shadow_sampler)
                        FIXME("Unsupported 3D shadow sampler.\n");
                    shader_addline(buffer, "uniform sampler3D %s_sampler%u;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %s_sampler%u;\n", prefix, i);
                    FIXME("Unrecognized sampler type: %#x\n", reg_maps->sampler_type[i]);
                    break;
            }
        }
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

        for (i = 0; i < shader->limits.sampler; ++i)
        {
            if (reg_maps->sampler_type[i])
            {
                if (!(ps_args->np2_fixup & (1 << i))) continue;

                if (WINED3DSTT_2D != reg_maps->sampler_type[i]) {
                    FIXME("Non-2D texture is flagged for NP2 texcoord fixup.\n");
                    continue;
                }

                fixup->idx[i] = cur++;
            }
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
        /* Declare attributes. */
        for (i = 0, map = reg_maps->input_registers; map; map >>= 1, ++i)
        {
            if (map & 1)
                shader_addline(buffer, "attribute vec4 %s_in%u;\n", prefix, i);
        }

        shader_addline(buffer, "uniform vec4 posFixup;\n");
        shader_addline(buffer, "void order_ps_input(in vec4[%u]);\n", shader->limits.packed_output);
    }
    else if (version->type == WINED3D_SHADER_TYPE_GEOMETRY)
    {
        shader_addline(buffer, "varying in vec4 gs_in[][%u];\n", shader->limits.packed_input);
    }
    else if (version->type == WINED3D_SHADER_TYPE_PIXEL)
    {
        if (version->major >= 3)
        {
            UINT in_count = min(vec4_varyings(version->major, gl_info), shader->limits.packed_input);

            if (use_vs(state))
                shader_addline(buffer, "varying vec4 %s_in[%u];\n", prefix, in_count);
            else
                /* TODO: Write a replacement shader for the fixed function
                 * vertex pipeline, so this isn't needed. For fixed function
                 * vertex processing + 3.0 pixel shader we need a separate
                 * function in the pixel shader that reads the fixed function
                 * color into the packed input registers. */
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
            shader_addline(buffer, "const vec4 srgb_const0 = vec4(%.8e, %.8e, %.8e, %.8e);\n",
                    srgb_pow, srgb_mul_high, srgb_sub_high, srgb_mul_low);
            shader_addline(buffer, "const vec4 srgb_const1 = vec4(%.8e, 0.0, 0.0, 0.0);\n",
                    srgb_cmp);
        }
        if (reg_maps->vpos || reg_maps->usesdsy)
        {
            if (shader->limits.constant_float + extra_constants_needed
                    + 1 < gl_info->limits.glsl_ps_float_constants)
            {
                shader_addline(buffer, "uniform vec4 ycorrection;\n");
                extra_constants_needed++;
            }
            else
            {
                /* This happens because we do not have proper tracking of the constant registers that are
                 * actually used, only the max limit of the shader version
                 */
                FIXME("Cannot find a free uniform for vpos correction params\n");
                shader_addline(buffer, "const vec4 ycorrection = vec4(%f, %f, 0.0, 0.0);\n",
                        context->render_offscreen ? 0.0f : fb->render_targets[0]->resource.height,
                        context->render_offscreen ? 1.0f : -1.0f);
            }
            shader_addline(buffer, "vec4 vpos;\n");
        }
    }

    /* Declare output register temporaries */
    if (shader->limits.packed_output)
        shader_addline(buffer, "vec4 %s_out[%u];\n", prefix, shader->limits.packed_output);

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

    /* Local constants use a different name so they can be loaded once at shader link time
     * They can't be hardcoded into the shader text via LC = {x, y, z, w}; because the
     * float -> string conversion can cause precision loss.
     */
    if (!shader->load_local_constsF)
    {
        LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
        {
            shader_addline(buffer, "uniform vec4 %s_lc%u;\n", prefix, lconst->idx);
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
        shader_addline(buffer,
                "vpos = floor(vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1));\n");
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
                            sprintf(register_name, "%.8e", *(const float *)reg->immconst_data);
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
                            sprintf(register_name, "vec4(%.8e, %.8e, %.8e, %.8e)",
                                    *(const float *)&reg->immconst_data[0], *(const float *)&reg->immconst_data[1],
                                    *(const float *)&reg->immconst_data[2], *(const float *)&reg->immconst_data[3]);
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
        char param_str[200];

        shader_glsl_gen_modifier(wined3d_src->modifiers, glsl_src->reg_name, swizzle_str, param_str);

        switch (wined3d_src->reg.data_type)
        {
            case WINED3D_DATA_FLOAT:
                sprintf(glsl_src->param_str, "%s", param_str);
                break;
            case WINED3D_DATA_INT:
                sprintf(glsl_src->param_str, "floatBitsToInt(%s)", param_str);
                break;
            case WINED3D_DATA_RESOURCE:
            case WINED3D_DATA_SAMPLER:
            case WINED3D_DATA_UINT:
                sprintf(glsl_src->param_str, "floatBitsToUint(%s)", param_str);
                break;
            default:
                FIXME("Unhandled data type %#x.\n", wined3d_src->reg.data_type);
                sprintf(glsl_src->param_str, "%s", param_str);
                break;
        }
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
static DWORD shader_glsl_append_dst_ext(struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader_instruction *ins, const struct wined3d_shader_dst_param *dst)
{
    struct glsl_dst_param glsl_dst;
    DWORD mask;

    if ((mask = shader_glsl_add_dst_param(ins, dst, &glsl_dst)))
    {
        switch (dst->reg.data_type)
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
                FIXME("Unhandled data type %#x.\n", dst->reg.data_type);
                shader_addline(buffer, "%s%s = %s(",
                        glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);
                break;
        }
    }

    return mask;
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst(struct wined3d_shader_buffer *buffer, const struct wined3d_shader_instruction *ins)
{
    return shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0]);
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
        DWORD sampler_idx, DWORD flags, struct glsl_sample_function *sample_function)
{
    enum wined3d_sampler_texture_type sampler_type = ctx->reg_maps->sampler_type[sampler_idx];
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    BOOL shadow = ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
            && (((const struct shader_glsl_ctx_priv *)ctx->backend_data)->cur_ps_args->shadow & (1 << sampler_idx));
    BOOL projected = flags & WINED3D_GLSL_SAMPLE_PROJECTED;
    BOOL texrect = flags & WINED3D_GLSL_SAMPLE_RECT;
    BOOL lod = flags & WINED3D_GLSL_SAMPLE_LOD;
    BOOL grad = flags & WINED3D_GLSL_SAMPLE_GRAD;

    /* Note that there's no such thing as a projected cube texture. */
    switch(sampler_type) {
        case WINED3DSTT_1D:
            if (shadow)
            {
                if (lod)
                {
                    sample_function->name = projected ? "shadow1DProjLod" : "shadow1DLod";
                }
                else if (grad)
                {
                    if (gl_info->supported[EXT_GPU_SHADER4])
                        sample_function->name = projected ? "shadow1DProjGrad" : "shadow1DGrad";
                    else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                        sample_function->name = projected ? "shadow1DProjGradARB" : "shadow1DGradARB";
                    else
                    {
                        FIXME("Unsupported 1D shadow grad function.\n");
                        sample_function->name = "unsupported1DGrad";
                    }
                }
                else
                {
                    sample_function->name = projected ? "shadow1DProj" : "shadow1D";
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;
            }
            else
            {
                if (lod)
                {
                    sample_function->name = projected ? "texture1DProjLod" : "texture1DLod";
                }
                else if (grad)
                {
                    if (gl_info->supported[EXT_GPU_SHADER4])
                        sample_function->name = projected ? "texture1DProjGrad" : "texture1DGrad";
                    else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                        sample_function->name = projected ? "texture1DProjGradARB" : "texture1DGradARB";
                    else
                    {
                        FIXME("Unsupported 1D grad function.\n");
                        sample_function->name = "unsupported1DGrad";
                    }
                }
                else
                {
                    sample_function->name = projected ? "texture1DProj" : "texture1D";
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0;
            }
            break;

        case WINED3DSTT_2D:
            if (shadow)
            {
                if (texrect)
                {
                    if (lod)
                    {
                        sample_function->name = projected ? "shadow2DRectProjLod" : "shadow2DRectLod";
                    }
                    else if (grad)
                    {
                        if (gl_info->supported[EXT_GPU_SHADER4])
                            sample_function->name = projected ? "shadow2DRectProjGrad" : "shadow2DRectGrad";
                        else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                            sample_function->name = projected ? "shadow2DRectProjGradARB" : "shadow2DRectGradARB";
                        else
                        {
                            FIXME("Unsupported RECT shadow grad function.\n");
                            sample_function->name = "unsupported2DRectGrad";
                        }
                    }
                    else
                    {
                        sample_function->name = projected ? "shadow2DRectProj" : "shadow2DRect";
                    }
                }
                else
                {
                    if (lod)
                    {
                        sample_function->name = projected ? "shadow2DProjLod" : "shadow2DLod";
                    }
                    else if (grad)
                    {
                        if (gl_info->supported[EXT_GPU_SHADER4])
                            sample_function->name = projected ? "shadow2DProjGrad" : "shadow2DGrad";
                        else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                            sample_function->name = projected ? "shadow2DProjGradARB" : "shadow2DGradARB";
                        else
                        {
                            FIXME("Unsupported 2D shadow grad function.\n");
                            sample_function->name = "unsupported2DGrad";
                        }
                    }
                    else
                    {
                        sample_function->name = projected ? "shadow2DProj" : "shadow2D";
                    }
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            }
            else
            {
                if (texrect)
                {
                    if (lod)
                    {
                        sample_function->name = projected ? "texture2DRectProjLod" : "texture2DRectLod";
                    }
                    else if (grad)
                    {
                        if (gl_info->supported[EXT_GPU_SHADER4])
                            sample_function->name = projected ? "texture2DRectProjGrad" : "texture2DRectGrad";
                        else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                            sample_function->name = projected ? "texture2DRectProjGradARB" : "texture2DRectGradARB";
                        else
                        {
                            FIXME("Unsupported RECT grad function.\n");
                            sample_function->name = "unsupported2DRectGrad";
                        }
                    }
                    else
                    {
                        sample_function->name = projected ? "texture2DRectProj" : "texture2DRect";
                    }
                }
                else
                {
                    if (lod)
                    {
                        sample_function->name = projected ? "texture2DProjLod" : "texture2DLod";
                    }
                    else if (grad)
                    {
                        if (gl_info->supported[EXT_GPU_SHADER4])
                            sample_function->name = projected ? "texture2DProjGrad" : "texture2DGrad";
                        else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                            sample_function->name = projected ? "texture2DProjGradARB" : "texture2DGradARB";
                        else
                        {
                            FIXME("Unsupported 2D grad function.\n");
                            sample_function->name = "unsupported2DGrad";
                        }
                    }
                    else
                    {
                        sample_function->name = projected ? "texture2DProj" : "texture2D";
                    }
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;
            }
            break;

        case WINED3DSTT_CUBE:
            if (shadow)
            {
                FIXME("Unsupported Cube shadow function.\n");
                sample_function->name = "unsupportedCubeShadow";
                sample_function->coord_mask = 0;
            }
            else
            {
                if (lod)
                {
                    sample_function->name = "textureCubeLod";
                }
                else if (grad)
                {
                    if (gl_info->supported[EXT_GPU_SHADER4])
                        sample_function->name = "textureCubeGrad";
                    else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                        sample_function->name = "textureCubeGradARB";
                    else
                    {
                        FIXME("Unsupported Cube grad function.\n");
                        sample_function->name = "unsupportedCubeGrad";
                    }
                }
                else
                {
                    sample_function->name = "textureCube";
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            }
            break;

        case WINED3DSTT_VOLUME:
            if (shadow)
            {
                FIXME("Unsupported 3D shadow function.\n");
                sample_function->name = "unsupported3DShadow";
                sample_function->coord_mask = 0;
            }
            else
            {
                if (lod)
                {
                    sample_function->name = projected ? "texture3DProjLod" : "texture3DLod";
                }
                else  if (grad)
                {
                    if (gl_info->supported[EXT_GPU_SHADER4])
                        sample_function->name = projected ? "texture3DProjGrad" : "texture3DGrad";
                    else if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
                        sample_function->name = projected ? "texture3DProjGradARB" : "texture3DGradARB";
                    else
                    {
                        FIXME("Unsupported 3D grad function.\n");
                        sample_function->name = "unsupported3DGrad";
                    }
                }
                else
                {
                    sample_function->name = projected ? "texture3DProj" : "texture3D";
                }
                sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            }
            break;

        default:
            sample_function->name = "";
            sample_function->coord_mask = 0;
            FIXME("Unrecognized sampler type: %#x;\n", sampler_type);
            break;
    }
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

static void shader_glsl_color_correction_ext(struct wined3d_shader_buffer *buffer,
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

    shader_glsl_swizzle_to_str(swizzle, FALSE, ins->dst[0].write_mask, dst_swizzle);

    if (version->type == WINED3D_SHADER_TYPE_PIXEL)
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        fixup = priv->cur_ps_args->color_fixup[sampler];

        if(priv->cur_ps_args->np2_fixup & (1 << sampler)) {
            if(bias) {
                FIXME("Biased sampling from NP2 textures is unsupported\n");
            } else {
                np2_fixup = TRUE;
            }
        }
    }
    else
    {
        fixup = COLOR_FIXUP_IDENTITY; /* FIXME: Vshader color fixup */
    }

    shader_glsl_append_dst(ins->ctx->buffer, ins);

    shader_addline(ins->ctx->buffer, "%s(%s_sampler%u, ",
            sample_function->name, shader_glsl_get_prefix(version->type), sampler);

    va_start(args, coord_reg_fmt);
    shader_vaddline(ins->ctx->buffer, coord_reg_fmt, args);
    va_end(args);

    if(bias) {
        shader_addline(ins->ctx->buffer, ", %s)%s);\n", bias, dst_swizzle);
    } else {
        if (np2_fixup) {
            const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
            const unsigned char idx = priv->cur_np2fixup_info->idx[sampler];

            shader_addline(ins->ctx->buffer, " * ps_samplerNP2Fixup[%u].%s)%s);\n", idx >> 1,
                           (idx % 2) ? "zw" : "xy", dst_swizzle);
        } else if(dx && dy) {
            shader_addline(ins->ctx->buffer, ", %s, %s)%s);\n", dx, dy, dst_swizzle);
        } else {
            shader_addline(ins->ctx->buffer, ")%s);\n", dst_swizzle);
        }
    }

    if(!is_identity_fixup(fixup)) {
        shader_glsl_color_correction(ins, fixup);
    }
}

/*****************************************************************************
 * Begin processing individual instruction opcodes
 ****************************************************************************/

static void shader_glsl_binop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
        case WINED3DSIH_MUL:  op = "*";  break;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
            case WINED3DSIH_LT:  op = "lessThan"; break;
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
            case WINED3DSIH_LT:  op = "<"; break;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD write_mask;

    /* If we have ARB_gpu_shader5 or GLSL 4.0, we can use imulExtended(). If
     * not, we can emulate it. */
    if (ins->dst[0].reg.type != WINED3DSPR_NULL)
        FIXME("64-bit integer multiplies not implemented.\n");

    if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1]);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);

        shader_addline(ins->ctx->buffer, "%s * %s);\n",
                src0_param.param_str, src1_param.param_str);
    }
}

static void shader_glsl_udiv(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1]);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "%s %% %s));\n", src0_param.param_str, src1_param.param_str);

            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0]);
            shader_addline(buffer, "tmp0%s);\n", dst_mask);
        }
        else
        {
            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0]);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_addline(buffer, "%s / %s);\n", src0_param.param_str, src1_param.param_str);
        }
    }
    else if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1]);
        shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_addline(buffer, "%s %% %s);\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Process the WINED3DSIO_MOV opcode using GLSL (dst = src) */
static void shader_glsl_mov(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    struct glsl_src_param src1_param;
    DWORD dst_write_mask, src_write_mask;
    unsigned int dst_size = 0;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    /* dp3 works on vec3, dp4 on vec4 */
    if (ins->handler_idx == WINED3DSIH_DP4)
    {
        src_write_mask = WINED3DSP_WRITEMASK_ALL;
    } else {
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    }

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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

/* Process the WINED3DSIO_LOG instruction in GLSL (dst = log2(|src0|))
 * Src0 is a scalar. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_log(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);

    if (dst_size > 1)
    {
        shader_addline(buffer, "vec%u(log2(abs(%s))));\n",
                dst_size, src0_param.param_str);
    }
    else
    {
        shader_addline(buffer, "log2(abs(%s)));\n",
                src0_param.param_str);
    }
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
static void shader_glsl_map2gl(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
        case WINED3DSIH_EXP: instruction = "exp2"; break;
        case WINED3DSIH_DSX: instruction = "dFdx"; break;
        case WINED3DSIH_DSY: instruction = "ycorrection.y * dFdy"; break;
        case WINED3DSIH_ROUND_NI: instruction = "floor"; break;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct glsl_src_param src_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src_param);

    if (ins->ctx->reg_maps->shader_version.major < 2)
    {
        char dst_mask[6];

        shader_addline(ins->ctx->buffer, "tmp0.x = exp2(floor(%s));\n", src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.y = %s - floor(%s);\n", src_param.param_str, src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.z = exp2(%s);\n", src_param.param_str);
        shader_addline(ins->ctx->buffer, "tmp0.w = 1.0;\n");

        shader_glsl_append_dst(ins->ctx->buffer, ins);
        shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
        shader_addline(ins->ctx->buffer, "tmp0%s);\n", dst_mask);
    } else {
        DWORD write_mask;
        unsigned int mask_size;

        write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
        mask_size = shader_glsl_get_write_mask_size(write_mask);

        if (mask_size > 1) {
            shader_addline(ins->ctx->buffer, "vec%d(exp2(%s)));\n", mask_size, src_param.param_str);
        } else {
            shader_addline(ins->ctx->buffer, "exp2(%s));\n", src_param.param_str);
        }
    }
}

static void shader_glsl_to_int(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

/** Process the RCP (reciprocal or inverse) opcode in GLSL (dst = 1 / src) */
static void shader_glsl_rcp(const struct wined3d_shader_instruction *ins)
{
    struct glsl_src_param src_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1)
    {
        shader_addline(ins->ctx->buffer, "vec%u(1.0 / %s));\n",
                mask_size, src_param.param_str);
    }
    else
    {
        shader_addline(ins->ctx->buffer, "1.0 / %s);\n",
                src_param.param_str);
    }
}

static void shader_glsl_rsq(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1)
    {
        shader_addline(buffer, "vec%u(inversesqrt(abs(%s))));\n",
                mask_size, src_param.param_str);
    }
    else
    {
        shader_addline(buffer, "inversesqrt(abs(%s)));\n",
                src_param.param_str);
    }
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
        else if (!(write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &dst)))
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

        /* Fun: The D3DSI_COISSUE flag changes the semantic of the cnd instruction for < 1.4 shaders */
        if (ins->coissue)
        {
            shader_addline(ins->ctx->buffer, "%s /* COISSUE! */);\n", src1_param.param_str);
        } else {
            shader_addline(ins->ctx->buffer, "%s > 0.5 ? %s : %s);\n",
                    src0_param.param_str, src1_param.param_str, src2_param.param_str);
        }
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1]);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_addline(buffer, "cos(%s));\n", src0_param.param_str);

            shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0]);
            shader_addline(buffer, "tmp0%s);\n", dst_mask);
        }
        else
        {
            write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[0]);
            shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
            shader_addline(buffer, "sin(%s));\n", src0_param.param_str);
        }
    }
    else if (ins->dst[1].reg.type != WINED3DSPR_NULL)
    {
        write_mask = shader_glsl_append_dst_ext(buffer, ins, &ins->dst[1]);
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    shader_addline(ins->ctx->buffer, "if (%s) {\n", src0_param.param_str);
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
    const struct wined3d_shader *shader = ins->ctx->shader;
    struct wined3d_device *device = shader->device;
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    struct glsl_sample_function sample_function;
    const struct wined3d_texture *texture;
    DWORD sample_flags = 0;
    DWORD sampler_idx;
    DWORD mask = 0, swizzle;

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0))
        sampler_idx = ins->dst[0].reg.idx[0].offset;
    else
        sampler_idx = ins->src[1].reg.idx[0].offset;
    texture = device->stateBlock->state.textures[sampler_idx];

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        DWORD flags = (priv->cur_ps_args->tex_transform >> sampler_idx * WINED3D_PSARGS_TEXTRANSFORM_SHIFT)
                & WINED3D_PSARGS_TEXTRANSFORM_MASK;
        enum wined3d_sampler_texture_type sampler_type = ins->ctx->reg_maps->sampler_type[sampler_idx];

        /* Projected cube textures don't make a lot of sense, the resulting coordinates stay the same. */
        if (flags & WINED3D_PSARGS_PROJECTED && sampler_type != WINED3DSTT_CUBE)
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
    } else {
        if (ins->flags & WINED3DSI_TEXLD_PROJECT)
        {
            /* ps 2.0 texldp instruction always divides by the fourth component. */
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            mask = WINED3DSP_WRITEMASK_3;
        }
    }

    if (texture && texture->target == GL_TEXTURE_RECTANGLE_ARB)
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    mask |= sample_function.coord_mask;

    if (shader_version < WINED3D_SHADER_VERSION(2,0)) swizzle = WINED3DSP_NOSWIZZLE;
    else swizzle = ins->src[1].swizzle;

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        char coord_mask[6];
        shader_glsl_write_mask_to_str(mask, coord_mask);
        shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, NULL,
                "T%u%s", sampler_idx, coord_mask);
    }
    else
    {
        struct glsl_src_param coord_param;
        shader_glsl_add_src_param(ins, &ins->src[0], mask, &coord_param);
        if (ins->flags & WINED3DSI_TEXLD_BIAS)
        {
            struct glsl_src_param bias;
            shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &bias);
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, bias.param_str,
                    "%s", coord_param.param_str);
        } else {
            shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, NULL,
                    "%s", coord_param.param_str);
        }
    }
}

static void shader_glsl_texldd(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader *shader = ins->ctx->shader;
    struct wined3d_device *device = shader->device;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, dx_param, dy_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_GRAD;
    struct glsl_sample_function sample_function;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;
    const struct wined3d_texture *texture;

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4])
    {
        FIXME("texldd used, but not supported by hardware. Falling back to regular tex\n");
        shader_glsl_tex(ins);
        return;
    }

    sampler_idx = ins->src[1].reg.idx[0].offset;
    texture = device->stateBlock->state.textures[sampler_idx];
    if (texture && texture->target == GL_TEXTURE_RECTANGLE_ARB)
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[2], sample_function.coord_mask, &dx_param);
    shader_glsl_add_src_param(ins, &ins->src[3], sample_function.coord_mask, &dy_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, dx_param.param_str, dy_param.param_str, NULL,
                                "%s", coord_param.param_str);
}

static void shader_glsl_texldl(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader *shader = ins->ctx->shader;
    struct wined3d_device *device = shader->device;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct glsl_src_param coord_param, lod_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_LOD;
    struct glsl_sample_function sample_function;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;
    const struct wined3d_texture *texture;

    sampler_idx = ins->src[1].reg.idx[0].offset;
    texture = device->stateBlock->state.textures[sampler_idx];
    if (texture && texture->target == GL_TEXTURE_RECTANGLE_ARB)
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &lod_param);

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4]
            && ins->ctx->reg_maps->shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
    {
        /* Plain GLSL only supports Lod sampling functions in vertex shaders.
         * However, the NVIDIA drivers allow them in fragment shaders as well,
         * even without the appropriate extension. */
        WARN("Using %s in fragment shader.\n", sample_function.name);
    }
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, NULL, NULL, lod_param.param_str,
            "%s", coord_param.param_str);
}

static void shader_glsl_texcoord(const struct wined3d_shader_instruction *ins)
{
    /* FIXME: Make this work for more than just 2D textures */
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
static void shader_glsl_texm3x3pad(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct glsl_sample_function sample_function;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct glsl_src_param src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.y = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xy");
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

    tex_mx->current_row = 0;
}

/* Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void shader_glsl_texm3x3vspec(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
static void shader_glsl_texkill(const struct wined3d_shader_instruction *ins)
{
    struct glsl_dst_param dst_param;

    /* The argument is a destination parameter, and no writemasks are allowed */
    shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);
    if (ins->ctx->reg_maps->shader_version.major >= 2)
    {
        /* 2.0 shaders compare all 4 components in texkill */
        shader_addline(ins->ctx->buffer, "if (any(lessThan(%s.xyzw, vec4(0.0)))) discard;\n", dst_param.reg_name);
    } else {
        /* 1.X shaders only compare the first 3 components, probably due to the nature of the texkill
         * instruction as a tex* instruction, and phase, which kills all a / w components. Even if all
         * 4 components are defined, only the first 3 are used
         */
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

static void shader_glsl_input_pack(const struct wined3d_shader *shader, struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader_signature_element *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps,
        enum vertexprocessing_mode vertexprocessing)
{
    WORD map = reg_maps->input_registers;
    unsigned int i;

    for (i = 0; map; map >>= 1, ++i)
    {
        const char *semantic_name;
        UINT semantic_idx;
        char reg_mask[6];

        /* Unused */
        if (!(map & 1)) continue;

        semantic_name = input_signature[i].semantic_name;
        semantic_idx = input_signature[i].semantic_idx;
        shader_glsl_write_mask_to_str(input_signature[i].mask, reg_mask);

        if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
        {
            if (semantic_idx < 8 && vertexprocessing == pretransformed)
                shader_addline(buffer, "ps_in[%u]%s = gl_TexCoord[%u]%s;\n",
                        shader->u.ps.input_reg_map[i], reg_mask, semantic_idx, reg_mask);
            else
                shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        shader->u.ps.input_reg_map[i], reg_mask, reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
        {
            if (!semantic_idx)
                shader_addline(buffer, "ps_in[%u]%s = vec4(gl_Color)%s;\n",
                        shader->u.ps.input_reg_map[i], reg_mask, reg_mask);
            else if (semantic_idx == 1)
                shader_addline(buffer, "ps_in[%u]%s = vec4(gl_SecondaryColor)%s;\n",
                        shader->u.ps.input_reg_map[i], reg_mask, reg_mask);
            else
                shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        shader->u.ps.input_reg_map[i], reg_mask, reg_mask);
        }
        else
        {
            shader_addline(buffer, "ps_in[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                    shader->u.ps.input_reg_map[i], reg_mask, reg_mask);
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
        GLhandleARB vs_id, GLhandleARB gs_id, GLhandleARB ps_id)
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

    GL_EXTCALL(glDeleteObjectARB(entry->programId));
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

static void handle_ps3_input(struct wined3d_shader_buffer *buffer,
        const struct wined3d_gl_info *gl_info, const DWORD *map,
        const struct wined3d_shader_signature_element *input_signature,
        const struct wined3d_shader_reg_maps *reg_maps_in,
        const struct wined3d_shader_signature_element *output_signature,
        const struct wined3d_shader_reg_maps *reg_maps_out)
{
    unsigned int i, j;
    const char *semantic_name_in;
    UINT semantic_idx_in;
    DWORD *set;
    DWORD in_idx;
    unsigned int in_count = vec4_varyings(3, gl_info);
    char reg_mask[6];
    char destination[50];
    WORD input_map, output_map;

    set = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*set) * (in_count + 2));

    input_map = reg_maps_in->input_registers;
    for (i = 0; input_map; input_map >>= 1, ++i)
    {
        if (!(input_map & 1)) continue;

        in_idx = map[i];
        /* Declared, but not read register */
        if (in_idx == ~0U) continue;
        if (in_idx >= (in_count + 2))
        {
            FIXME("More input varyings declared than supported, expect issues.\n");
            continue;
        }

        if (in_idx == in_count)
            sprintf(destination, "gl_FrontColor");
        else if (in_idx == in_count + 1)
            sprintf(destination, "gl_FrontSecondaryColor");
        else
            sprintf(destination, "ps_in[%u]", in_idx);

        semantic_name_in = input_signature[i].semantic_name;
        semantic_idx_in = input_signature[i].semantic_idx;
        set[in_idx] = ~0U;

        output_map = reg_maps_out->output_registers;
        for (j = 0; output_map; output_map >>= 1, ++j)
        {
            DWORD mask;

            if (!(output_map & 1)
                    || semantic_idx_in != output_signature[j].semantic_idx
                    || strcmp(semantic_name_in, output_signature[j].semantic_name)
                    || !(mask = input_signature[i].mask & output_signature[j].mask))
                continue;

            set[in_idx] = mask;
            shader_glsl_write_mask_to_str(mask, reg_mask);

            shader_addline(buffer, "%s%s = vs_out[%u]%s;\n",
                    destination, reg_mask, j, reg_mask);
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
            sprintf(destination, "gl_FrontColor");
        else if (i == in_count + 1)
            sprintf(destination, "gl_FrontSecondaryColor");
        else
            sprintf(destination, "ps_in[%u]", i);

        if (size == 1) shader_addline(buffer, "%s.%s = 0.0;\n", destination, reg_mask);
        else shader_addline(buffer, "%s.%s = vec%u(0.0);\n", destination, reg_mask, size);
    }

    HeapFree(GetProcessHeap(), 0, set);
}

/* Context activation is done by the caller. */
static GLhandleARB generate_param_reorder_function(struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader *vs, const struct wined3d_shader *ps,
        const struct wined3d_gl_info *gl_info)
{
    GLhandleARB ret = 0;
    DWORD ps_major = ps ? ps->reg_maps.shader_version.major : 0;
    unsigned int i;
    const char *semantic_name;
    UINT semantic_idx;
    char reg_mask[6];
    const struct wined3d_shader_signature_element *output_signature = vs->output_signature;
    WORD map = vs->reg_maps.output_registers;

    shader_buffer_clear(buffer);

    shader_addline(buffer, "#version 120\n");

    if (ps_major < 3)
    {
        shader_addline(buffer, "void order_ps_input(in vec4 vs_out[%u])\n{\n", vs->limits.packed_output);

        for (i = 0; map; map >>= 1, ++i)
        {
            DWORD write_mask;

            if (!(map & 1)) continue;

            semantic_name = output_signature[i].semantic_name;
            semantic_idx = output_signature[i].semantic_idx;
            write_mask = output_signature[i].mask;
            shader_glsl_write_mask_to_str(write_mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
            {
                if (!semantic_idx)
                    shader_addline(buffer, "gl_FrontColor%s = vs_out[%u]%s;\n",
                            reg_mask, i, reg_mask);
                else if (semantic_idx == 1)
                    shader_addline(buffer, "gl_FrontSecondaryColor%s = vs_out[%u]%s;\n",
                            reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_POSITION))
            {
                shader_addline(buffer, "gl_Position%s = vs_out[%u]%s;\n",
                        reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
            {
                if (semantic_idx < 8)
                {
                    if (!(gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W) || ps_major > 0)
                        write_mask |= WINED3DSP_WRITEMASK_3;

                    shader_addline(buffer, "gl_TexCoord[%u]%s = vs_out[%u]%s;\n",
                            semantic_idx, reg_mask, i, reg_mask);
                    if (!(write_mask & WINED3DSP_WRITEMASK_3))
                        shader_addline(buffer, "gl_TexCoord[%u].w = 1.0;\n", semantic_idx);
                }
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE))
            {
                shader_addline(buffer, "gl_PointSize = vs_out[%u].%c;\n", i, reg_mask[1]);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_FOG))
            {
                shader_addline(buffer, "gl_FogFragCoord = clamp(vs_out[%u].%c, 0.0, 1.0);\n", i, reg_mask[1]);
            }
        }
        shader_addline(buffer, "}\n");
    }
    else
    {
        UINT in_count = min(vec4_varyings(ps_major, gl_info), ps->limits.packed_input);
        /* This one is tricky: a 3.0 pixel shader reads from a 3.0 vertex shader */
        shader_addline(buffer, "varying vec4 ps_in[%u];\n", in_count);
        shader_addline(buffer, "void order_ps_input(in vec4 vs_out[%u])\n{\n", vs->limits.packed_output);

        /* First, sort out position and point size. Those are not passed to the pixel shader */
        for (i = 0; map; map >>= 1, ++i)
        {
            if (!(map & 1)) continue;

            semantic_name = output_signature[i].semantic_name;
            shader_glsl_write_mask_to_str(output_signature[i].mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_POSITION))
            {
                shader_addline(buffer, "gl_Position%s = vs_out[%u]%s;\n",
                        reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_PSIZE))
            {
                shader_addline(buffer, "gl_PointSize = vs_out[%u].%c;\n", i, reg_mask[1]);
            }
        }

        /* Then, fix the pixel shader input */
        handle_ps3_input(buffer, gl_info, ps->u.ps.input_reg_map, ps->input_signature,
                &ps->reg_maps, output_signature, &vs->reg_maps);

        shader_addline(buffer, "}\n");
    }

    ret = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    checkGLcall("glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB)");
    shader_glsl_compile(gl_info, ret, buffer->buffer);

    return ret;
}

static void shader_glsl_generate_srgb_write_correction(struct wined3d_shader_buffer *buffer)
{
    shader_addline(buffer, "tmp0.xyz = pow(gl_FragData[0].xyz, vec3(srgb_const0.x));\n");
    shader_addline(buffer, "tmp0.xyz = tmp0.xyz * vec3(srgb_const0.y) - vec3(srgb_const0.z);\n");
    shader_addline(buffer, "tmp1.xyz = gl_FragData[0].xyz * vec3(srgb_const0.w);\n");
    shader_addline(buffer, "bvec3 srgb_compare = lessThan(gl_FragData[0].xyz, vec3(srgb_const1.x));\n");
    shader_addline(buffer, "gl_FragData[0].xyz = mix(tmp0.xyz, tmp1.xyz, vec3(srgb_compare));\n");
    shader_addline(buffer, "gl_FragData[0] = clamp(gl_FragData[0], 0.0, 1.0);\n");
}

static void shader_glsl_generate_fog_code(struct wined3d_shader_buffer *buffer, enum fogmode mode)
{
    switch (mode)
    {
        case FOG_OFF:
            return;

        case FOG_LINEAR:
            /* Fog = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start) */
            shader_addline(buffer, "float Fog = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);\n");
            break;

        case FOG_EXP:
            /* Fog = e^-(gl_Fog.density * gl_FogFragCoord) */
            shader_addline(buffer, "float Fog = exp(-gl_Fog.density * gl_FogFragCoord);\n");
            break;

        case FOG_EXP2:
            /* Fog = e^-((gl_Fog.density * gl_FogFragCoord)^2) */
            shader_addline(buffer, "float Fog = exp(-gl_Fog.density * gl_Fog.density * gl_FogFragCoord * gl_FogFragCoord);\n");
            break;

        default:
            ERR("Invalid fog mode %#x.\n", mode);
            return;
    }

    shader_addline(buffer, "gl_FragData[0].xyz = mix(gl_Fog.color.xyz, gl_FragData[0].xyz, clamp(Fog, 0.0, 1.0));\n");
}

/* Context activation is done by the caller. */
static void hardcode_local_constants(const struct wined3d_shader *shader,
        const struct wined3d_gl_info *gl_info, GLhandleARB programId, const char *prefix)
{
    const struct wined3d_shader_lconst *lconst;
    GLint tmp_loc;
    const float *value;
    char glsl_name[10];

    LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
    {
        value = (const float *)lconst->value;
        snprintf(glsl_name, sizeof(glsl_name), "%s_lc%u", prefix, lconst->idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
        GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, value));
    }
    checkGLcall("Hardcoding local constants");
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_pshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, const struct wined3d_shader *shader,
        const struct ps_compile_args *args, struct ps_np2fixup_info *np2fixup_info)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader object and assign it as the shader->prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.cur_np2fixup_info = np2fixup_info;

    shader_addline(buffer, "#version 120\n");

    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[ARB_SHADER_TEXTURE_LOD])
        shader_addline(buffer, "#extension GL_ARB_shader_texture_lod : enable\n");
    /* The spec says that it doesn't have to be explicitly enabled, but the
     * nvidia drivers write a warning if we don't do so. */
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    /* Base Declarations */
    shader_generate_glsl_declarations(context, buffer, shader, reg_maps, &priv_ctx);

    /* Pack 3.0 inputs */
    if (reg_maps->shader_version.major >= 3 && args->vp_mode != vertexshader)
        shader_glsl_input_pack(shader, buffer, shader->input_signature, reg_maps, args->vp_mode);

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

    TRACE("Compiling shader object %u\n", shader_obj);
    shader_glsl_compile(gl_info, shader_obj, buffer->buffer);

    /* Store the shader object */
    return shader_obj;
}

/* Context activation is done by the caller. */
static GLuint shader_glsl_generate_vshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, const struct wined3d_shader *shader,
        const struct vs_compile_args *args)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader program and assign it as the shader->prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));

    shader_addline(buffer, "#version 120\n");

    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;

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

    TRACE("Compiling shader object %u\n", shader_obj);
    shader_glsl_compile(gl_info, shader_obj, buffer->buffer);

    return shader_obj;
}

/* Context activation is done by the caller. */
static GLhandleARB shader_glsl_generate_geometry_shader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, const struct wined3d_shader *shader)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const DWORD *function = shader->function;
    struct shader_glsl_ctx_priv priv_ctx;
    GLhandleARB shader_id;

    shader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB));

    shader_addline(buffer, "#version 120\n");

    if (gl_info->supported[ARB_GEOMETRY_SHADER4])
        shader_addline(buffer, "#extension GL_ARB_geometry_shader4 : enable\n");
    if (gl_info->supported[ARB_SHADER_BIT_ENCODING])
        shader_addline(buffer, "#extension GL_ARB_shader_bit_encoding : enable\n");
    if (gl_info->supported[EXT_GPU_SHADER4])
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    shader_generate_glsl_declarations(context, buffer, shader, reg_maps, &priv_ctx);
    shader_generate_main(shader, buffer, reg_maps, function, &priv_ctx);
    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u.\n", shader_id);
    shader_glsl_compile(gl_info, shader_id, buffer->buffer);

    return shader_id;
}

static GLhandleARB find_glsl_pshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, struct wined3d_shader *shader,
        const struct ps_compile_args *args, const struct ps_np2fixup_info **np2fixup_info)
{
    struct wined3d_state *state = &shader->device->stateBlock->state;
    struct glsl_ps_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    struct ps_np2fixup_info *np2fixup;
    UINT i;
    DWORD new_size;
    GLhandleARB ret;

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
            return gl_shaders[i].prgId;
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

    pixelshader_update_samplers(&shader->reg_maps, state->textures);

    shader_buffer_clear(buffer);
    ret = shader_glsl_generate_pshader(context, buffer, shader, args, np2fixup);
    gl_shaders[shader_data->num_gl_shaders++].prgId = ret;

    return ret;
}

static inline BOOL vs_args_equal(const struct vs_compile_args *stored, const struct vs_compile_args *new,
                                 const DWORD use_map) {
    if((stored->swizzle_map & use_map) != new->swizzle_map) return FALSE;
    if((stored->clip_enabled) != new->clip_enabled) return FALSE;
    return stored->fog_src == new->fog_src;
}

static GLhandleARB find_glsl_vshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, struct wined3d_shader *shader,
        const struct vs_compile_args *args)
{
    UINT i;
    DWORD new_size;
    DWORD use_map = shader->device->strided_streams.use_map;
    struct glsl_vs_compiled_shader *gl_shaders, *new_array;
    struct glsl_shader_private *shader_data;
    GLhandleARB ret;

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
            return gl_shaders[i].prgId;
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

    shader_buffer_clear(buffer);
    ret = shader_glsl_generate_vshader(context, buffer, shader, args);
    gl_shaders[shader_data->num_gl_shaders++].prgId = ret;

    return ret;
}

static GLhandleARB find_glsl_geometry_shader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, struct wined3d_shader *shader)
{
    struct glsl_gs_compiled_shader *gl_shaders;
    struct glsl_shader_private *shader_data;
    GLhandleARB ret;

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

    shader_buffer_clear(buffer);
    ret = shader_glsl_generate_geometry_shader(context, buffer, shader);
    gl_shaders[shader_data->num_gl_shaders++].id = ret;

    return ret;
}

static const char *shader_glsl_get_ffp_fragment_op_arg(struct wined3d_shader_buffer *buffer,
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
            FIXME("Per-stage constants not implemented.\n");
            switch (stage)
            {
                case 0: ret = "const0"; break;
                case 1: ret = "const1"; break;
                case 2: ret = "const2"; break;
                case 3: ret = "const3"; break;
                case 4: ret = "const4"; break;
                case 5: ret = "const5"; break;
                case 6: ret = "const6"; break;
                case 7: ret = "const7"; break;
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

static void shader_glsl_ffp_fragment_op(struct wined3d_shader_buffer *buffer, unsigned int stage, BOOL color,
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
static GLuint shader_glsl_generate_ffp_fragment_shader(struct wined3d_shader_buffer *buffer,
        const struct ffp_frag_settings *settings, const struct wined3d_gl_info *gl_info)
{
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    BYTE lum_map = 0, bump_map = 0, tex_map = 0;
    const char *final_combiner_src = "ret";
    UINT lowest_disabled_stage;
    GLhandleARB shader_obj;
    DWORD arg0, arg1, arg2;
    unsigned int stage;

    shader_buffer_clear(buffer);

    /* Find out which textures are read */
    for (stage = 0; stage < MAX_TEXTURES; ++stage)
    {
        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
            break;

        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE)
            tex_map |= 1 << stage;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (settings->op[stage].dst == tempreg)
            tempreg_used = TRUE;

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
    }
    lowest_disabled_stage = stage;

    shader_addline(buffer, "#version 120\n");

    shader_addline(buffer, "vec4 tmp0, tmp1;\n");
    shader_addline(buffer, "vec4 ret;\n");
    if (tempreg_used || settings->sRGB_write)
        shader_addline(buffer, "vec4 temp_reg;\n");
    shader_addline(buffer, "vec4 arg0, arg1, arg2;\n");

    for (stage = 0; stage < MAX_TEXTURES; ++stage)
    {
        if (!(tex_map & (1 << stage)))
            continue;

        switch (settings->op[stage].tex_type)
        {
            case tex_1d:
                shader_addline(buffer, "uniform sampler1D ps_sampler%u;\n", stage);
                break;
            case tex_2d:
                shader_addline(buffer, "uniform sampler2D ps_sampler%u;\n", stage);
                break;
            case tex_3d:
                shader_addline(buffer, "uniform sampler3D ps_sampler%u;\n", stage);
                break;
            case tex_cube:
                shader_addline(buffer, "uniform samplerCube ps_sampler%u;\n", stage);
                break;
            case tex_rect:
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
    shader_addline(buffer, "uniform vec4 specular_enable;\n");

    if (settings->sRGB_write)
    {
        shader_addline(buffer, "const vec4 srgb_const0 = vec4(%.8e, %.8e, %.8e, %.8e);\n",
                srgb_pow, srgb_mul_high, srgb_sub_high, srgb_mul_low);
        shader_addline(buffer, "const vec4 srgb_const1 = vec4(%.8e, 0.0, 0.0, 0.0);\n",
                srgb_cmp);
    }

    shader_addline(buffer, "void main()\n{\n");

    if (lowest_disabled_stage < 7 && settings->emul_clipplanes)
        shader_addline(buffer, "if (any(lessThan(gl_texCoord[7], vec4(0.0)))) discard;\n");

    /* Generate texture sampling instructions) */
    for (stage = 0; stage < MAX_TEXTURES && settings->op[stage].cop != WINED3D_TOP_DISABLE; ++stage)
    {
        const char *texture_function, *coord_mask;
        char tex_reg_name[8];
        BOOL proj, clamp;

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

        if (settings->op[stage].cop == WINED3D_TOP_BUMPENVMAP
                || settings->op[stage].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
            clamp = FALSE;
        else
            clamp = TRUE;

        switch (settings->op[stage].tex_type)
        {
            case tex_1d:
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
            case tex_2d:
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
            case tex_3d:
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
            case tex_cube:
                texture_function = "textureCube";
                coord_mask = "xyz";
                break;
            case tex_rect:
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

            if (clamp)
                shader_addline(buffer, "tex%u = clamp(%s(ps_sampler%u, ret.%s), 0.0, 1.0);\n",
                        stage, texture_function, stage, coord_mask);
            else
                shader_addline(buffer, "tex%u = %s(ps_sampler%u, ret.%s);\n",
                        stage, texture_function, stage, coord_mask);

            if (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
                shader_addline(buffer, "tex%u *= clamp(tex%u.z * bumpenv_lum_scale%u + bumpenv_lum_offset%u, 0.0, 1.0);\n",
                        stage, stage - 1, stage - 1, stage - 1);
        }
        else if (settings->op[stage].projected == proj_count3)
        {
            if (clamp)
                shader_addline(buffer, "tex%u = clamp(%s(ps_sampler%u, gl_TexCoord[%u].xyz), 0.0, 1.0);\n",
                        stage, texture_function, stage, stage);
            else
                shader_addline(buffer, "tex%u = %s(ps_sampler%u, gl_TexCoord[%u].xyz);\n",
                        stage, texture_function, stage, stage);
        }
        else
        {
            if (clamp)
                shader_addline(buffer, "tex%u = clamp(%s(ps_sampler%u, gl_TexCoord[%u].%s), 0.0, 1.0);\n",
                        stage, texture_function, stage, stage, coord_mask);
            else
                shader_addline(buffer, "tex%u = %s(ps_sampler%u, gl_TexCoord[%u].%s);\n",
                        stage, texture_function, stage, stage, coord_mask);
        }

        sprintf(tex_reg_name, "tex%u", stage);
        shader_glsl_color_correction_ext(buffer, tex_reg_name, WINED3DSP_WRITEMASK_ALL,
                settings->op[stage].color_fixup);
    }

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

    shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    shader_glsl_compile(gl_info, shader_obj, buffer->buffer);
    return shader_obj;
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
    glsl_desc->id = shader_glsl_generate_ffp_fragment_shader(&priv->shader_buffer, args, gl_info);
    list_init(&glsl_desc->linked_programs);
    add_ffp_frag_shader(&priv->ffp_fragment_shaders, &glsl_desc->entry);

    return glsl_desc;
}


static void shader_glsl_init_vs_uniform_locations(const struct wined3d_gl_info *gl_info,
        GLhandleARB program_id, struct glsl_vs_program *vs)
{
    unsigned int i;
    char name[32];

    vs->uniform_f_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLhandleARB) * gl_info->limits.glsl_vs_float_constants);
    for (i = 0; i < gl_info->limits.glsl_vs_float_constants; ++i)
    {
        snprintf(name, sizeof(name), "vs_c[%u]", i);
        vs->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
    }

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        snprintf(name, sizeof(name), "vs_i[%u]", i);
        vs->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
    }

    vs->pos_fixup_location = GL_EXTCALL(glGetUniformLocationARB(program_id, "posFixup"));
}

static void shader_glsl_init_ps_uniform_locations(const struct wined3d_gl_info *gl_info,
        GLhandleARB program_id, struct glsl_ps_program *ps)
{
    unsigned int i;
    char name[32];

    ps->uniform_f_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLhandleARB) * gl_info->limits.glsl_ps_float_constants);
    for (i = 0; i < gl_info->limits.glsl_ps_float_constants; ++i)
    {
        snprintf(name, sizeof(name), "ps_c[%u]", i);
        ps->uniform_f_locations[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
    }

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        snprintf(name, sizeof(name), "ps_i[%u]", i);
        ps->uniform_i_locations[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
    }

    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        snprintf(name, sizeof(name), "bumpenv_mat%u", i);
        ps->bumpenv_mat_location[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
        snprintf(name, sizeof(name), "bumpenv_lum_scale%u", i);
        ps->bumpenv_lum_scale_location[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
        snprintf(name, sizeof(name), "bumpenv_lum_offset%u", i);
        ps->bumpenv_lum_offset_location[i] = GL_EXTCALL(glGetUniformLocationARB(program_id, name));
    }

    ps->tex_factor_location = GL_EXTCALL(glGetUniformLocationARB(program_id, "tex_factor"));
    ps->specular_enable_location = GL_EXTCALL(glGetUniformLocationARB(program_id, "specular_enable"));
    ps->np2_fixup_location = GL_EXTCALL(glGetUniformLocationARB(program_id, "ps_samplerNP2Fixup"));
    ps->ycorrection_location = GL_EXTCALL(glGetUniformLocationARB(program_id, "ycorrection"));
}

/* Context activation is done by the caller. */
static void set_glsl_shader_program(const struct wined3d_context *context, struct wined3d_device *device,
        enum wined3d_shader_mode vertex_mode, enum wined3d_shader_mode fragment_mode)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct ps_np2fixup_info *np2fixup_info = NULL;
    struct shader_glsl_priv *priv = device->shader_priv;
    struct glsl_shader_prog_link *entry = NULL;
    struct wined3d_shader *vshader = NULL;
    struct wined3d_shader *gshader = NULL;
    struct wined3d_shader *pshader = NULL;
    GLhandleARB programId                  = 0;
    GLhandleARB reorder_shader_id          = 0;
    unsigned int i;
    struct ps_compile_args ps_compile_args;
    struct vs_compile_args vs_compile_args;
    GLhandleARB vs_id, gs_id, ps_id;
    struct list *ps_list;

    if (vertex_mode == WINED3D_SHADER_MODE_SHADER)
    {
        vshader = state->vertex_shader;
        find_vs_compile_args(state, vshader, &vs_compile_args);
        vs_id = find_glsl_vshader(context, &priv->shader_buffer, vshader, &vs_compile_args);

        if ((gshader = state->geometry_shader))
            gs_id = find_glsl_geometry_shader(context, &priv->shader_buffer, gshader);
        else
            gs_id = 0;
    }
    else
    {
        vs_id = 0;
        gs_id = 0;
    }

    if (fragment_mode == WINED3D_SHADER_MODE_SHADER)
    {
        pshader = state->pixel_shader;
        find_ps_compile_args(state, pshader, &ps_compile_args);
        ps_id = find_glsl_pshader(context, &priv->shader_buffer,
                pshader, &ps_compile_args, &np2fixup_info);
        ps_list = &pshader->linked_programs;
    }
    else if (fragment_mode == WINED3D_SHADER_MODE_FFP && priv->fragment_pipe == &glsl_fragment_pipe)
    {
        struct glsl_ffp_fragment_shader *ffp_shader;
        struct ffp_frag_settings settings;

        gen_ffp_frag_op(device, state, &settings, FALSE);
        ffp_shader = shader_glsl_find_ffp_fragment_shader(priv, gl_info, &settings);
        ps_id = ffp_shader->id;
        ps_list = &ffp_shader->linked_programs;
    }
    else
    {
        ps_id = 0;
    }

    if ((!vs_id && !gs_id && !ps_id) || (entry = get_glsl_program_entry(priv, vs_id, gs_id, ps_id)))
    {
        priv->glsl_program = entry;
        return;
    }

    /* If we get to this point, then no matching program exists, so we create one */
    programId = GL_EXTCALL(glCreateProgramObjectARB());
    TRACE("Created new GLSL shader program %u\n", programId);

    /* Create the entry */
    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(struct glsl_shader_prog_link));
    entry->programId = programId;
    entry->vs.id = vs_id;
    entry->gs.id = gs_id;
    entry->ps.id = ps_id;
    entry->constant_version = 0;
    entry->ps.np2_fixup_info = np2fixup_info;
    /* Add the hash table entry */
    add_glsl_program_entry(priv, entry);

    /* Set the current program */
    priv->glsl_program = entry;

    /* Attach GLSL vshader */
    if (vshader)
    {
        WORD map = vshader->reg_maps.input_registers;
        char tmp_name[10];

        reorder_shader_id = generate_param_reorder_function(&priv->shader_buffer, vshader, pshader, gl_info);
        TRACE("Attaching GLSL shader object %u to program %u\n", reorder_shader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, reorder_shader_id));
        checkGLcall("glAttachObjectARB");
        /* Flag the reorder function for deletion, then it will be freed automatically when the program
         * is destroyed
         */
        GL_EXTCALL(glDeleteObjectARB(reorder_shader_id));

        TRACE("Attaching GLSL shader object %u to program %u.\n", vs_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, vs_id));
        checkGLcall("glAttachObjectARB");

        /* Bind vertex attributes to a corresponding index number to match
         * the same index numbers as ARB_vertex_programs (makes loading
         * vertex attributes simpler).  With this method, we can use the
         * exact same code to load the attributes later for both ARB and
         * GLSL shaders.
         *
         * We have to do this here because we need to know the Program ID
         * in order to make the bindings work, and it has to be done prior
         * to linking the GLSL program. */
        for (i = 0; map; map >>= 1, ++i)
        {
            if (!(map & 1)) continue;

            snprintf(tmp_name, sizeof(tmp_name), "vs_in%u", i);
            GL_EXTCALL(glBindAttribLocationARB(programId, i, tmp_name));
        }
        checkGLcall("glBindAttribLocationARB");

        list_add_head(&vshader->linked_programs, &entry->vs.shader_entry);
    }

    if (gshader)
    {
        TRACE("Attaching GLSL geometry shader object %u to program %u.\n", gs_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, gs_id));
        checkGLcall("glAttachObjectARB");

        TRACE("input type %s, output type %s, vertices out %u.\n",
                debug_d3dprimitivetype(gshader->u.gs.input_type),
                debug_d3dprimitivetype(gshader->u.gs.output_type),
                gshader->u.gs.vertices_out);
        GL_EXTCALL(glProgramParameteriARB(programId, GL_GEOMETRY_INPUT_TYPE_ARB,
                gl_primitive_type_from_d3d(gshader->u.gs.input_type)));
        GL_EXTCALL(glProgramParameteriARB(programId, GL_GEOMETRY_OUTPUT_TYPE_ARB,
                gl_primitive_type_from_d3d(gshader->u.gs.output_type)));
        GL_EXTCALL(glProgramParameteriARB(programId, GL_GEOMETRY_VERTICES_OUT_ARB,
                gshader->u.gs.vertices_out));
        checkGLcall("glProgramParameteriARB");

        list_add_head(&gshader->linked_programs, &entry->gs.shader_entry);
    }

    /* Attach GLSL pshader */
    if (ps_id)
    {
        TRACE("Attaching GLSL shader object %u to program %u.\n", ps_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, ps_id));
        checkGLcall("glAttachObjectARB");

        list_add_head(ps_list, &entry->ps.shader_entry);
    }

    /* Link the program */
    TRACE("Linking GLSL shader program %u\n", programId);
    GL_EXTCALL(glLinkProgramARB(programId));
    shader_glsl_validate_link(gl_info, programId);

    shader_glsl_init_vs_uniform_locations(gl_info, programId, &entry->vs);
    shader_glsl_init_ps_uniform_locations(gl_info, programId, &entry->ps);
    checkGLcall("Find glsl program uniform locations");

    if (pshader && pshader->reg_maps.shader_version.major >= 3
            && pshader->u.ps.declared_in_count > vec4_varyings(3, gl_info))
    {
        TRACE("Shader %d needs vertex color clamping disabled\n", programId);
        entry->vs.vertex_color_clamp = GL_FALSE;
    }
    else
    {
        entry->vs.vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    /* Set the shader to allow uniform loading on it */
    GL_EXTCALL(glUseProgramObjectARB(programId));
    checkGLcall("glUseProgramObjectARB(programId)");

    /* Load the vertex and pixel samplers now. The function that finds the mappings makes sure
     * that it stays the same for each vertexshader-pixelshader pair(=linked glsl program). If
     * a pshader with fixed function pipeline is used there are no vertex samplers, and if a
     * vertex shader with fixed function pixel processing is used we make sure that the card
     * supports enough samplers to allow the max number of vertex samplers with all possible
     * fixed function fragment processing setups. So once the program is linked these samplers
     * won't change.
     */
    shader_glsl_load_vsamplers(gl_info, device->texUnitMap, programId);
    shader_glsl_load_psamplers(gl_info, device->texUnitMap, programId);

    /* If the local constants do not have to be loaded with the environment constants,
     * load them now to have them hardcoded in the GLSL program. This saves some CPU cycles
     * later
     */
    if (pshader && !pshader->load_local_constsF)
        hardcode_local_constants(pshader, gl_info, programId, "ps");
    if (vshader && !vshader->load_local_constsF)
        hardcode_local_constants(vshader, gl_info, programId, "vs");
}

/* Context activation is done by the caller. */
static GLhandleARB create_glsl_blt_shader(const struct wined3d_gl_info *gl_info, enum tex_types tex_type, BOOL masked)
{
    GLhandleARB program_id;
    GLhandleARB vshader_id, pshader_id;
    const char *blt_pshader;

    static const char *blt_vshader =
        "#version 120\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "}\n";

    static const char * const blt_pshaders_full[tex_type_count] =
    {
        /* tex_1d */
        NULL,
        /* tex_2d */
        "#version 120\n"
        "uniform sampler2D sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* tex_3d */
        NULL,
        /* tex_cube */
        "#version 120\n"
        "uniform samplerCube sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = textureCube(sampler, gl_TexCoord[0].xyz).x;\n"
        "}\n",
        /* tex_rect */
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2DRect(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
    };

    static const char * const blt_pshaders_masked[tex_type_count] =
    {
        /* tex_1d */
        NULL,
        /* tex_2d */
        "#version 120\n"
        "uniform sampler2D sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* tex_3d */
        NULL,
        /* tex_cube */
        "#version 120\n"
        "uniform samplerCube sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = textureCube(sampler, gl_TexCoord[0].xyz).x;\n"
        "}\n",
        /* tex_rect */
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect sampler;\n"
        "uniform vec4 mask;\n"
        "void main(void)\n"
        "{\n"
        "    if (all(lessThan(gl_FragCoord.xy, mask.zw))) discard;\n"
        "    gl_FragDepth = texture2DRect(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
    };

    blt_pshader = masked ? blt_pshaders_masked[tex_type] : blt_pshaders_full[tex_type];
    if (!blt_pshader)
    {
        FIXME("tex_type %#x not supported\n", tex_type);
        return 0;
    }

    vshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    shader_glsl_compile(gl_info, vshader_id, blt_vshader);

    pshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    shader_glsl_compile(gl_info, pshader_id, blt_pshader);

    program_id = GL_EXTCALL(glCreateProgramObjectARB());
    GL_EXTCALL(glAttachObjectARB(program_id, vshader_id));
    GL_EXTCALL(glAttachObjectARB(program_id, pshader_id));
    GL_EXTCALL(glLinkProgramARB(program_id));

    shader_glsl_validate_link(gl_info, program_id);

    /* Once linked we can mark the shaders for deletion. They will be deleted once the program
     * is destroyed
     */
    GL_EXTCALL(glDeleteObjectARB(vshader_id));
    GL_EXTCALL(glDeleteObjectARB(pshader_id));
    return program_id;
}

/* Context activation is done by the caller. */
static void shader_glsl_select(const struct wined3d_context *context, enum wined3d_shader_mode vertex_mode,
        enum wined3d_shader_mode fragment_mode)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_device *device = context->swapchain->device;
    struct shader_glsl_priv *priv = device->shader_priv;
    GLhandleARB program_id = 0;
    GLenum old_vertex_color_clamp, current_vertex_color_clamp;

    old_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vs.vertex_color_clamp : GL_FIXED_ONLY_ARB;
    set_glsl_shader_program(context, device, vertex_mode, fragment_mode);
    current_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vs.vertex_color_clamp : GL_FIXED_ONLY_ARB;
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

    program_id = priv->glsl_program ? priv->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);
    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");

    /* In case that NP2 texcoord fixup data is found for the selected program, trigger a reload of the
     * constants. This has to be done because it can't be guaranteed that sampler() (from state.c) is
     * called between selecting the shader and using it, which results in wrong fixup for some frames. */
    if (priv->glsl_program && priv->glsl_program->ps.np2_fixup_info)
    {
        shader_glsl_load_np2fixup_constants(priv, gl_info, &device->stateBlock->state);
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_select_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info,
        enum tex_types tex_type, const SIZE *ds_mask_size)
{
    BOOL masked = ds_mask_size->cx && ds_mask_size->cy;
    struct shader_glsl_priv *priv = shader_priv;
    GLhandleARB *blt_program;
    GLint loc;

    blt_program = masked ? &priv->depth_blt_program_masked[tex_type] : &priv->depth_blt_program_full[tex_type];
    if (!*blt_program)
    {
        *blt_program = create_glsl_blt_shader(gl_info, tex_type, masked);
        loc = GL_EXTCALL(glGetUniformLocationARB(*blt_program, "sampler"));
        GL_EXTCALL(glUseProgramObjectARB(*blt_program));
        GL_EXTCALL(glUniform1iARB(loc, 0));
    }
    else
    {
        GL_EXTCALL(glUseProgramObjectARB(*blt_program));
    }

    if (masked)
    {
        loc = GL_EXTCALL(glGetUniformLocationARB(*blt_program, "mask"));
        GL_EXTCALL(glUniform4fARB(loc, 0.0f, 0.0f, (float)ds_mask_size->cx, (float)ds_mask_size->cy));
    }
}

/* Context activation is done by the caller. */
static void shader_glsl_deselect_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info)
{
    struct shader_glsl_priv *priv = shader_priv;
    GLhandleARB program_id;

    program_id = priv->glsl_program ? priv->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);

    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
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

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, ps.shader_entry)
                {
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting pixel shader %u.\n", gl_shaders[i].prgId);
                    if (priv->glsl_program && priv->glsl_program->ps.id == gl_shaders[i].prgId)
                        shader_glsl_select(context, WINED3D_SHADER_MODE_NONE, WINED3D_SHADER_MODE_NONE);
                    GL_EXTCALL(glDeleteObjectARB(gl_shaders[i].prgId));
                    checkGLcall("glDeleteObjectARB");
                }
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.ps);

                break;
            }

            case WINED3D_SHADER_TYPE_VERTEX:
            {
                struct glsl_vs_compiled_shader *gl_shaders = shader_data->gl_shaders.vs;

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, vs.shader_entry)
                {
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting vertex shader %u.\n", gl_shaders[i].prgId);
                    if (priv->glsl_program && priv->glsl_program->vs.id == gl_shaders[i].prgId)
                        shader_glsl_select(context, WINED3D_SHADER_MODE_NONE, WINED3D_SHADER_MODE_NONE);
                    GL_EXTCALL(glDeleteObjectARB(gl_shaders[i].prgId));
                    checkGLcall("glDeleteObjectARB");
                }
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.vs);

                break;
            }

            case WINED3D_SHADER_TYPE_GEOMETRY:
            {
                struct glsl_gs_compiled_shader *gl_shaders = shader_data->gl_shaders.gs;

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs,
                        struct glsl_shader_prog_link, gs.shader_entry)
                {
                    delete_glsl_program_entry(priv, gl_info, entry);
                }

                for (i = 0; i < shader_data->num_gl_shaders; ++i)
                {
                    TRACE("Deleting geometry shader %u.\n", gl_shaders[i].id);
                    if (priv->glsl_program && priv->glsl_program->gs.id == gl_shaders[i].id)
                        shader_glsl_select(context, WINED3D_SHADER_MODE_NONE, WINED3D_SHADER_MODE_NONE);
                    GL_EXTCALL(glDeleteObjectARB(gl_shaders[i].id));
                    checkGLcall("glDeleteObjectARB");
                }
                HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders.gs);

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
    SIZE_T size = (constant_count + 1) * sizeof(*heap->entries) + constant_count * sizeof(*heap->positions);
    void *mem = HeapAlloc(GetProcessHeap(), 0, size);

    if (!mem)
    {
        ERR("Failed to allocate memory\n");
        return FALSE;
    }

    heap->entries = mem;
    heap->entries[1].version = 0;
    heap->positions = (unsigned int *)(heap->entries + constant_count + 1);
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

static HRESULT shader_glsl_alloc(struct wined3d_device *device, const struct fragment_pipeline *fragment_pipe)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct shader_glsl_priv *priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_glsl_priv));
    SIZE_T stack_size = wined3d_log2i(max(gl_info->limits.glsl_vs_float_constants,
            gl_info->limits.glsl_ps_float_constants)) + 1;
    struct fragment_caps fragment_caps;
    void *fragment_priv;

    if (!(fragment_priv = fragment_pipe->alloc_private(&glsl_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        HeapFree(GetProcessHeap(), 0, priv);
        return E_FAIL;
    }

    if (!shader_buffer_init(&priv->shader_buffer))
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
    fragment_pipe->get_caps(gl_info, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;
    device->fragment_priv = fragment_priv;
    priv->fragment_pipe = fragment_pipe;

    device->shader_priv = priv;
    return WINED3D_OK;

fail:
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    shader_buffer_free(&priv->shader_buffer);
    fragment_pipe->free_private(device);
    HeapFree(GetProcessHeap(), 0, priv);
    return E_OUTOFMEMORY;
}

/* Context activation is done by the caller. */
static void shader_glsl_free(struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct shader_glsl_priv *priv = device->shader_priv;
    int i;

    for (i = 0; i < tex_type_count; ++i)
    {
        if (priv->depth_blt_program_full[i])
        {
            GL_EXTCALL(glDeleteObjectARB(priv->depth_blt_program_full[i]));
        }
        if (priv->depth_blt_program_masked[i])
        {
            GL_EXTCALL(glDeleteObjectARB(priv->depth_blt_program_masked[i]));
        }
    }

    wine_rb_destroy(&priv->program_lookup, NULL, NULL);
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    shader_buffer_free(&priv->shader_buffer);
    priv->fragment_pipe->free_private(device);

    HeapFree(GetProcessHeap(), 0, device->shader_priv);
    device->shader_priv = NULL;
}

static void shader_glsl_context_destroyed(void *shader_priv, const struct wined3d_context *context) {}

static void shader_glsl_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *caps)
{
    UINT shader_model;

    if (gl_info->supported[EXT_GPU_SHADER4] && gl_info->supported[ARB_SHADER_BIT_ENCODING]
            && gl_info->supported[ARB_GEOMETRY_SHADER4] && gl_info->glsl_version >= MAKEDWORD_VERSION(1, 50)
            && gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX] && gl_info->supported[ARB_DRAW_INSTANCED])
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
    caps->ps_1x_max_value = 8.0;

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
    /* WINED3DSIH_EXP                   */ shader_glsl_map2gl,
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
    /* WINED3DSIH_ITOF                  */ shader_glsl_to_float,
    /* WINED3DSIH_LABEL                 */ shader_glsl_label,
    /* WINED3DSIH_LD                    */ NULL,
    /* WINED3DSIH_LIT                   */ shader_glsl_lit,
    /* WINED3DSIH_LOG                   */ shader_glsl_log,
    /* WINED3DSIH_LOGP                  */ shader_glsl_log,
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
    /* WINED3DSIH_NOP                   */ shader_glsl_nop,
    /* WINED3DSIH_NRM                   */ shader_glsl_nrm,
    /* WINED3DSIH_PHASE                 */ shader_glsl_nop,
    /* WINED3DSIH_POW                   */ shader_glsl_pow,
    /* WINED3DSIH_RCP                   */ shader_glsl_rcp,
    /* WINED3DSIH_REP                   */ shader_glsl_rep,
    /* WINED3DSIH_RET                   */ shader_glsl_ret,
    /* WINED3DSIH_ROUND_NI              */ shader_glsl_map2gl,
    /* WINED3DSIH_RSQ                   */ shader_glsl_rsq,
    /* WINED3DSIH_SAMPLE                */ NULL,
    /* WINED3DSIH_SAMPLE_GRAD           */ NULL,
    /* WINED3DSIH_SAMPLE_LOD            */ NULL,
    /* WINED3DSIH_SETP                  */ NULL,
    /* WINED3DSIH_SGE                   */ shader_glsl_compare,
    /* WINED3DSIH_SGN                   */ shader_glsl_sgn,
    /* WINED3DSIH_SINCOS                */ shader_glsl_sincos,
    /* WINED3DSIH_SLT                   */ shader_glsl_compare,
    /* WINED3DSIH_SQRT                  */ NULL,
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
    shader_glsl_select_depth_blt,
    shader_glsl_deselect_depth_blt,
    shader_glsl_update_float_vertex_constants,
    shader_glsl_update_float_pixel_constants,
    shader_glsl_load_constants,
    shader_glsl_load_np2fixup_constants,
    shader_glsl_destroy,
    shader_glsl_alloc,
    shader_glsl_free,
    shader_glsl_context_destroyed,
    shader_glsl_get_caps,
    shader_glsl_color_fixup_supported,
    shader_glsl_has_ffp_proj_control,
};

static void glsl_fragment_pipe_enable(const struct wined3d_gl_info *gl_info, BOOL enable)
{
    /* Nothing to do. */
}

static void glsl_fragment_pipe_get_caps(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps)
{
    caps->wined3d_caps = WINED3D_FRAGMENT_CAP_PROJ_CONTROL
            | WINED3D_FRAGMENT_CAP_SRGB_WRITE;
    caps->PrimitiveMiscCaps = WINED3DPMISCCAPS_TSSARGTEMP;
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

struct glsl_ffp_destroy_ctx
{
    struct shader_glsl_priv *priv;
    const struct wined3d_gl_info *gl_info;
};

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
    ctx->gl_info->gl_ops.ext.p_glDeleteObjectARB(shader->id);
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

    context->select_shader = 1;
    context->load_constants = 1;
}

static void glsl_fragment_pipe_fog(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    BOOL use_vshader = use_vs(state);
    enum fogsource new_source;

    context->select_shader = 1;
    context->load_constants = 1;

    if (!state->render_states[WINED3D_RS_FOGENABLE])
        return;

    if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
    {
        if (use_vshader)
            new_source = FOGSOURCE_VS;
        else if (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE || context->last_was_rhw)
            new_source = FOGSOURCE_COORD;
        else
            new_source = FOGSOURCE_FFP;
    }
    else
    {
        new_source = FOGSOURCE_FFP;
    }

    if (new_source != context->fog_source)
    {
        context->fog_source = new_source;
        state_fogstartend(context, state, STATE_RENDER(WINED3D_RS_FOGSTART));
    }
}

static void glsl_fragment_pipe_tex_transform(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->select_shader = 1;
    context->load_constants = 1;
}

static void glsl_fragment_pipe_invalidate_constants(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    context->load_constants = 1;
}

static const struct StateEntryTemplate glsl_fragment_pipe_state_template[] =
{
    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),                    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),                     glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),               {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG1),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG2),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG0),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),             {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),          {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),           glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT01),          {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT10),          {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT11),          {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),           NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),         {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),          glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LOFFSET),        {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_PIXELSHADER,                                         {STATE_PIXELSHADER,                                          glsl_fragment_pipe_shader              }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGENABLE),                        {STATE_RENDER(WINED3D_RS_FOGENABLE),                         glsl_fragment_pipe_fog                 }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),                     {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),                    {STATE_RENDER(WINED3D_RS_FOGENABLE),                         NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGSTART),                         {STATE_RENDER(WINED3D_RS_FOGSTART),                          state_fogstartend                      }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGEND),                           {STATE_RENDER(WINED3D_RS_FOGSTART),                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                  {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                   state_srgbwrite                        }, ARB_FRAMEBUFFER_SRGB},
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),                  {STATE_PIXELSHADER,                                          NULL                                   }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGCOLOR),                         {STATE_RENDER(WINED3D_RS_FOGCOLOR),                          state_fogcolor                         }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_FOGDENSITY),                       {STATE_RENDER(WINED3D_RS_FOGDENSITY),                        state_fogdensity                       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(0,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(1,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(2,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(3,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(4,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(5,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(6,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_TEXTURESTAGE(7,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), glsl_fragment_pipe_tex_transform       }, WINED3D_GL_EXT_NONE },
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                   {STATE_RENDER(WINED3D_RS_SPECULARENABLE),                    glsl_fragment_pipe_invalidate_constants}, WINED3D_GL_EXT_NONE },
    {0 /* Terminate */,                                         {0,                                                          0                                      }, WINED3D_GL_EXT_NONE },
};

const struct fragment_pipeline glsl_fragment_pipe =
{
    glsl_fragment_pipe_enable,
    glsl_fragment_pipe_get_caps,
    glsl_fragment_pipe_alloc,
    glsl_fragment_pipe_free,
    shader_glsl_color_fixup_supported,
    glsl_fragment_pipe_state_template,
};
