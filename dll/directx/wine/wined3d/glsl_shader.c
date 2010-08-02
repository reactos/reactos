/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green
 * Copyright 2006-2007 Henri Verbeet
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

/*
 * D3D shader asm has swizzles on source parameters, and write masks for
 * destination parameters. GLSL uses swizzles for both. The result of this is
 * that for example "mov dst.xw, src.zyxw" becomes "dst.xw = src.zw" in GLSL.
 * Ie, to generate a proper GLSL source swizzle, we need to take the D3D write
 * mask for the destination parameter into account.
 */

#include "config.h"
#include <limits.h>
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

#define WINED3D_GLSL_SAMPLE_PROJECTED   0x1
#define WINED3D_GLSL_SAMPLE_RECT        0x2
#define WINED3D_GLSL_SAMPLE_LOD         0x4
#define WINED3D_GLSL_SAMPLE_GRAD        0x8

typedef struct {
    char reg_name[150];
    char mask_str[6];
} glsl_dst_param_t;

typedef struct {
    char reg_name[150];
    char param_str[200];
} glsl_src_param_t;

typedef struct {
    const char *name;
    DWORD coord_mask;
} glsl_sample_function_t;

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
};

/* Struct to maintain data about a linked GLSL program */
struct glsl_shader_prog_link {
    struct wine_rb_entry        program_lookup_entry;
    struct list                 vshader_entry;
    struct list                 pshader_entry;
    GLhandleARB                 programId;
    GLint                       *vuniformF_locations;
    GLint                       *puniformF_locations;
    GLint                       vuniformI_locations[MAX_CONST_I];
    GLint                       puniformI_locations[MAX_CONST_I];
    GLint                       posFixup_location;
    GLint                       np2Fixup_location;
    GLint                       bumpenvmat_location[MAX_TEXTURES];
    GLint                       luminancescale_location[MAX_TEXTURES];
    GLint                       luminanceoffset_location[MAX_TEXTURES];
    GLint                       ycorrection_location;
    GLenum                      vertex_color_clamp;
    IWineD3DVertexShader        *vshader;
    IWineD3DPixelShader         *pshader;
    struct vs_compile_args      vs_args;
    struct ps_compile_args      ps_args;
    UINT                        constant_version;
    const struct ps_np2fixup_info *np2Fixup_info;
};

typedef struct {
    IWineD3DVertexShader        *vshader;
    IWineD3DPixelShader         *pshader;
    struct ps_compile_args      ps_args;
    struct vs_compile_args      vs_args;
} glsl_program_key_t;

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

struct glsl_pshader_private
{
    struct glsl_ps_compiled_shader  *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
};

struct glsl_vs_compiled_shader
{
    struct vs_compile_args          args;
    GLhandleARB                     prgId;
};

struct glsl_vshader_private
{
    struct glsl_vs_compiled_shader  *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
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

/** Prints the GLSL info log which will contain error messages if they exist */
/* GL locking is done by the caller */
static void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLhandleARB obj)
{
    int infologLength = 0;
    char *infoLog;
    unsigned int i;
    BOOL is_spam;

    static const char * const spam[] =
    {
        "Vertex shader was successfully compiled to run on hardware.\n",    /* fglrx          */
        "Fragment shader was successfully compiled to run on hardware.\n",  /* fglrx, with \n */
        "Fragment shader was successfully compiled to run on hardware.",    /* fglrx, no \n   */
        "Fragment shader(s) linked, vertex shader(s) linked. \n ",          /* fglrx, with \n */
        "Fragment shader(s) linked, vertex shader(s) linked.",              /* fglrx, no \n   */
        "Vertex shader(s) linked, no fragment shader(s) defined. \n ",      /* fglrx, with \n */
        "Vertex shader(s) linked, no fragment shader(s) defined.",          /* fglrx, no \n   */
        "Fragment shader(s) linked, no vertex shader(s) defined. \n ",      /* fglrx, with \n */
        "Fragment shader(s) linked, no vertex shader(s) defined.",          /* fglrx, no \n   */
    };

    if (!TRACE_ON(d3d_shader) && !FIXME_ON(d3d_shader)) return;

    GL_EXTCALL(glGetObjectParameterivARB(obj,
               GL_OBJECT_INFO_LOG_LENGTH_ARB,
               &infologLength));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (infologLength > 1)
    {
        char *ptr, *line;

        /* Fglrx doesn't terminate the string properly, but it tells us the proper length.
         * So use HEAP_ZERO_MEMORY to avoid uninitialized bytes
         */
        infoLog = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, infologLength);
        GL_EXTCALL(glGetInfoLogARB(obj, infologLength, NULL, infoLog));
        is_spam = FALSE;

        for(i = 0; i < sizeof(spam) / sizeof(spam[0]); i++) {
            if(strcmp(infoLog, spam[i]) == 0) {
                is_spam = TRUE;
                break;
            }
        }

        ptr = infoLog;
        if (is_spam)
        {
            TRACE("Spam received from GLSL shader #%u:\n", obj);
            while ((line = get_info_log_line(&ptr))) TRACE("    %s\n", line);
        }
        else
        {
            FIXME("Error received from GLSL shader #%u:\n", obj);
            while ((line = get_info_log_line(&ptr))) FIXME("    %s\n", line);
        }
        HeapFree(GetProcessHeap(), 0, infoLog);
    }
}

/* GL locking is done by the caller. */
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

/* GL locking is done by the caller. */
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

/**
 * Loads (pixel shader) samplers
 */
/* GL locking is done by the caller */
static void shader_glsl_load_psamplers(const struct wined3d_gl_info *gl_info,
        DWORD *tex_unit_map, GLhandleARB programId)
{
    GLint name_loc;
    int i;
    char sampler_name[20];

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
        snprintf(sampler_name, sizeof(sampler_name), "Psampler%d", i);
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

/* GL locking is done by the caller */
static void shader_glsl_load_vsamplers(const struct wined3d_gl_info *gl_info,
        DWORD *tex_unit_map, GLhandleARB programId)
{
    GLint name_loc;
    char sampler_name[20];
    int i;

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        snprintf(sampler_name, sizeof(sampler_name), "Vsampler%d", i);
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

/* GL locking is done by the caller */
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
            {
                heap_idx >>= 1;
                --stack_idx;
                break;
            }
        }
    }
    checkGLcall("walk_constant_heap()");
}

/* GL locking is done by the caller */
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

/* GL locking is done by the caller */
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
            {
                heap_idx >>= 1;
                --stack_idx;
                break;
            }
        }
    }
    checkGLcall("walk_constant_heap_clamped()");
}

/* Loads floating point constants (aka uniforms) into the currently set GLSL program. */
/* GL locking is done by the caller */
static void shader_glsl_load_constantsF(IWineD3DBaseShaderImpl *This, const struct wined3d_gl_info *gl_info,
        const float *constants, const GLint *constant_locations, const struct constant_heap *heap,
        unsigned char *stack, UINT version)
{
    const local_constant *lconst;

    /* 1.X pshaders have the constants clamped to [-1;1] implicitly. */
    if (This->baseShader.reg_maps.shader_version.major == 1
            && shader_is_pshader_version(This->baseShader.reg_maps.shader_version.type))
        walk_constant_heap_clamped(gl_info, constants, constant_locations, heap, stack, version);
    else
        walk_constant_heap(gl_info, constants, constant_locations, heap, stack, version);

    if (!This->baseShader.load_local_constsF)
    {
        TRACE("No need to load local float constants for this shader\n");
        return;
    }

    /* Immediate constants are clamped to [-1;1] at shader creation time if needed */
    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry)
    {
        GLint location = constant_locations[lconst->idx];
        /* We found this uniform name in the program - go ahead and send the data */
        if (location != -1) GL_EXTCALL(glUniform4fvARB(location, 1, (const GLfloat *)lconst->value));
    }
    checkGLcall("glUniform4fvARB()");
}

/* Loads integer constants (aka uniforms) into the currently set GLSL program. */
/* GL locking is done by the caller */
static void shader_glsl_load_constantsI(IWineD3DBaseShaderImpl *This, const struct wined3d_gl_info *gl_info,
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
    ptr = list_head(&This->baseShader.constantsI);
    while (ptr) {
        const struct local_constant *lconst = LIST_ENTRY(ptr, const struct local_constant, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i, %i, %i, %i\n", idx,
            values[0], values[1], values[2], values[3]);

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4ivARB(locations[idx], 1, values));
        checkGLcall("glUniform4ivARB");
        ptr = list_next(&This->baseShader.constantsI, ptr);
    }
}

/* Loads boolean constants (aka uniforms) into the currently set GLSL program. */
/* GL locking is done by the caller */
static void shader_glsl_load_constantsB(IWineD3DBaseShaderImpl *This, const struct wined3d_gl_info *gl_info,
        GLhandleARB programId, const BOOL *constants, WORD constants_set)
{
    GLint tmp_loc;
    unsigned int i;
    char tmp_name[8];
    const char *prefix;
    struct list* ptr;

    switch (This->baseShader.reg_maps.shader_version.type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            prefix = "VB";
            break;

        case WINED3D_SHADER_TYPE_GEOMETRY:
            prefix = "GB";
            break;

        case WINED3D_SHADER_TYPE_PIXEL:
            prefix = "PB";
            break;

        default:
            FIXME("Unknown shader type %#x.\n",
                    This->baseShader.reg_maps.shader_version.type);
            prefix = "UB";
            break;
    }

    /* TODO: Benchmark and see if it would be beneficial to store the
     * locations of the constants to avoid looking up each time */
    for (i = 0; constants_set; constants_set >>= 1, ++i)
    {
        if (!(constants_set & 1)) continue;

        TRACE_(d3d_constants)("Loading constants %i: %i;\n", i, constants[i]);

        /* TODO: Benchmark and see if it would be beneficial to store the
         * locations of the constants to avoid looking up each time */
        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1)
        {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, &constants[i]));
            checkGLcall("glUniform1ivARB");
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsB);
    while (ptr) {
        const struct local_constant *lconst = LIST_ENTRY(ptr, const struct local_constant, entry);
        unsigned int idx = lconst->idx;
        const GLint *values = (const GLint *)lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i\n", idx, values[0]);

        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform1ivARB");
        }
        ptr = list_next(&This->baseShader.constantsB, ptr);
    }
}

static void reset_program_constant_version(struct wine_rb_entry *entry, void *context)
{
    WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry)->constant_version = 0;
}

/**
 * Loads the texture dimensions for NP2 fixup into the currently set GLSL program.
 */
/* GL locking is done by the caller (state handler) */
static void shader_glsl_load_np2fixup_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {

    const IWineD3DDeviceImpl* deviceImpl = (const IWineD3DDeviceImpl*) device;
    const struct glsl_shader_prog_link* prog = ((struct shader_glsl_priv *)(deviceImpl->shader_priv))->glsl_program;

    if (!prog) {
        /* No GLSL program set - nothing to do. */
        return;
    }

    if (!usePixelShader) {
        /* NP2 texcoord fixup is (currently) only done for pixelshaders. */
        return;
    }

    if (prog->ps_args.np2_fixup && -1 != prog->np2Fixup_location) {
        const struct wined3d_gl_info *gl_info = &deviceImpl->adapter->gl_info;
        const IWineD3DStateBlockImpl* stateBlock = (const IWineD3DStateBlockImpl*) deviceImpl->stateBlock;
        UINT i;
        UINT fixup = prog->ps_args.np2_fixup;
        GLfloat np2fixup_constants[4 * MAX_FRAGMENT_SAMPLERS];

        for (i = 0; fixup; fixup >>= 1, ++i) {
            const unsigned char idx = prog->np2Fixup_info->idx[i];
            const IWineD3DBaseTextureImpl* const tex = (const IWineD3DBaseTextureImpl*) stateBlock->textures[i];
            GLfloat* tex_dim = &np2fixup_constants[(idx >> 1) * 4];

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

        GL_EXTCALL(glUniform4fvARB(prog->np2Fixup_location, prog->np2Fixup_info->num_consts, np2fixup_constants));
    }
}

/**
 * Loads the app-supplied constants into the currently set GLSL program.
 */
/* GL locking is done by the caller (state handler) */
static void shader_glsl_load_constants(const struct wined3d_context *context,
        char usePixelShader, char useVertexShader)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    IWineD3DDeviceImpl *device = context->swapchain->device;
    IWineD3DStateBlockImpl* stateBlock = device->stateBlock;
    struct shader_glsl_priv *priv = device->shader_priv;

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

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, stateBlock->vertexShaderConstantF,
                prog->vuniformF_locations, &priv->vconst_heap, priv->stack, constant_version);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, prog->vuniformI_locations, stateBlock->vertexShaderConstantI,
                stateBlock->changed.vertexShaderConstantsI & vshader->baseShader.reg_maps.integer_constants);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, stateBlock->vertexShaderConstantB,
                stateBlock->changed.vertexShaderConstantsB & vshader->baseShader.reg_maps.boolean_constants);

        /* Upload the position fixup params */
        GL_EXTCALL(glUniform4fvARB(prog->posFixup_location, 1, &device->posFixup[0]));
        checkGLcall("glUniform4fvARB");
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, stateBlock->pixelShaderConstantF,
                prog->puniformF_locations, &priv->pconst_heap, priv->stack, constant_version);

        /* Load DirectX 9 integer constants/uniforms for pixel shader */
        shader_glsl_load_constantsI(pshader, gl_info, prog->puniformI_locations, stateBlock->pixelShaderConstantI,
                stateBlock->changed.pixelShaderConstantsI & pshader->baseShader.reg_maps.integer_constants);

        /* Load DirectX 9 boolean constants/uniforms for pixel shader */
        shader_glsl_load_constantsB(pshader, gl_info, programId, stateBlock->pixelShaderConstantB,
                stateBlock->changed.pixelShaderConstantsB & pshader->baseShader.reg_maps.boolean_constants);

        /* Upload the environment bump map matrix if needed. The needsbumpmat member specifies the texture stage to load the matrix from.
         * It can't be 0 for a valid texbem instruction.
         */
        for(i = 0; i < MAX_TEXTURES; i++) {
            const float *data;

            if(prog->bumpenvmat_location[i] == -1) continue;

            data = (const float *)&stateBlock->textureState[i][WINED3DTSS_BUMPENVMAT00];
            GL_EXTCALL(glUniformMatrix2fvARB(prog->bumpenvmat_location[i], 1, 0, data));
            checkGLcall("glUniformMatrix2fvARB");

            /* texbeml needs the luminance scale and offset too. If texbeml is used, needsbumpmat
             * is set too, so we can check that in the needsbumpmat check
             */
            if(prog->luminancescale_location[i] != -1) {
                const GLfloat *scale = (const GLfloat *)&stateBlock->textureState[i][WINED3DTSS_BUMPENVLSCALE];
                const GLfloat *offset = (const GLfloat *)&stateBlock->textureState[i][WINED3DTSS_BUMPENVLOFFSET];

                GL_EXTCALL(glUniform1fvARB(prog->luminancescale_location[i], 1, scale));
                checkGLcall("glUniform1fvARB");
                GL_EXTCALL(glUniform1fvARB(prog->luminanceoffset_location[i], 1, offset));
                checkGLcall("glUniform1fvARB");
            }
        }

        if(((IWineD3DPixelShaderImpl *) pshader)->vpos_uniform) {
            float correction_params[4];

            if (context->render_offscreen)
            {
                correction_params[0] = 0.0f;
                correction_params[1] = 1.0f;
            } else {
                /* position is window relative, not viewport relative */
                correction_params[0] = context->current_rt->currentDesc.Height;
                correction_params[1] = -1.0f;
            }
            GL_EXTCALL(glUniform4fvARB(prog->ycorrection_location, 1, correction_params));
        }
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

static inline void update_heap_entry(struct constant_heap *heap, unsigned int idx,
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

static void shader_glsl_update_float_vertex_constants(IWineD3DDevice *iface, UINT start, UINT count)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_glsl_priv *priv = This->shader_priv;
    struct constant_heap *heap = &priv->vconst_heap;
    UINT i;

    for (i = start; i < count + start; ++i)
    {
        if (!This->stateBlock->changed.vertexShaderConstantsF[i])
            update_heap_entry(heap, i, heap->size++, priv->next_constant_version);
        else
            update_heap_entry(heap, i, heap->positions[i], priv->next_constant_version);
    }
}

static void shader_glsl_update_float_pixel_constants(IWineD3DDevice *iface, UINT start, UINT count)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_glsl_priv *priv = This->shader_priv;
    struct constant_heap *heap = &priv->pconst_heap;
    UINT i;

    for (i = start; i < count + start; ++i)
    {
        if (!This->stateBlock->changed.pixelShaderConstantsF[i])
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
        struct wined3d_shader_buffer *buffer, IWineD3DBaseShader *iface,
        const shader_reg_maps *reg_maps, struct shader_glsl_ctx_priv *ctx_priv)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
    const struct ps_compile_args *ps_args = ctx_priv->cur_ps_args;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i, extra_constants_needed = 0;
    const local_constant *lconst;
    DWORD map;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(reg_maps->shader_version.type);
    char prefix = pshader ? 'P' : 'V';

    /* Prototype the subroutines */
    for (i = 0, map = reg_maps->labels; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "void subroutine%u();\n", i);
    }

    /* Declare the constants (aka uniforms) */
    if (This->baseShader.limits.constant_float > 0) {
        unsigned max_constantsF;
        /* Unless the shader uses indirect addressing, always declare the maximum array size and ignore that we need some
         * uniforms privately. E.g. if GL supports 256 uniforms, and we need 2 for the pos fixup and immediate values, still
         * declare VC[256]. If the shader needs more uniforms than we have it won't work in any case. If it uses less, the
         * compiler will figure out which uniforms are really used and strip them out. This allows a shader to use c255 on
         * a dx9 card, as long as it doesn't also use all the other constants.
         *
         * If the shader uses indirect addressing the compiler must assume that all declared uniforms are used. In this case,
         * declare only the amount that we're assured to have.
         *
         * Thus we run into problems in these two cases:
         * 1) The shader really uses more uniforms than supported
         * 2) The shader uses indirect addressing, less constants than supported, but uses a constant index > #supported consts
         */
        if (pshader)
        {
            /* No indirect addressing here. */
            max_constantsF = gl_info->limits.glsl_ps_float_constants;
        }
        else
        {
            if(This->baseShader.reg_maps.usesrelconstF) {
                /* Subtract the other potential uniforms from the max available (bools, ints, and 1 row of projection matrix).
                 * Subtract another uniform for immediate values, which have to be loaded via uniform by the driver as well.
                 * The shader code only uses 0.5, 2.0, 1.0, 128 and -128 in vertex shader code, so one vec4 should be enough
                 * (Unfortunately the Nvidia driver doesn't store 128 and -128 in one float).
                 *
                 * Writing gl_ClipVertex requires one uniform for each clipplane as well.
                 */
                max_constantsF = gl_info->limits.glsl_vs_float_constants - 3;
                if(ctx_priv->cur_vs_args->clip_enabled)
                {
                    max_constantsF -= gl_info->limits.clipplanes;
                }
                max_constantsF -= count_bits(This->baseShader.reg_maps.integer_constants);
                /* Strictly speaking a bool only uses one scalar, but the nvidia(Linux) compiler doesn't pack them properly,
                 * so each scalar requires a full vec4. We could work around this by packing the booleans ourselves, but
                 * for now take this into account when calculating the number of available constants
                 */
                max_constantsF -= count_bits(This->baseShader.reg_maps.boolean_constants);
                /* Set by driver quirks in directx.c */
                max_constantsF -= gl_info->reserved_glsl_constants;
            }
            else
            {
                max_constantsF = gl_info->limits.glsl_vs_float_constants;
            }
        }
        max_constantsF = min(This->baseShader.limits.constant_float, max_constantsF);
        shader_addline(buffer, "uniform vec4 %cC[%u];\n", prefix, max_constantsF);
    }

    /* Always declare the full set of constants, the compiler can remove the unused ones because d3d doesn't(yet)
     * support indirect int and bool constant addressing. This avoids problems if the app uses e.g. i0 and i9.
     */
    if (This->baseShader.limits.constant_int > 0 && This->baseShader.reg_maps.integer_constants)
        shader_addline(buffer, "uniform ivec4 %cI[%u];\n", prefix, This->baseShader.limits.constant_int);

    if (This->baseShader.limits.constant_bool > 0 && This->baseShader.reg_maps.boolean_constants)
        shader_addline(buffer, "uniform bool %cB[%u];\n", prefix, This->baseShader.limits.constant_bool);

    if(!pshader) {
        shader_addline(buffer, "uniform vec4 posFixup;\n");
        /* Predeclaration; This function is added at link time based on the pixel shader.
         * VS 3.0 shaders have an array OUT[] the shader writes to, earlier versions don't have
         * that. We know the input to the reorder function at vertex shader compile time, so
         * we can deal with that. The reorder function for a 1.x and 2.x vertex shader can just
         * read gl_FrontColor. The output depends on the pixel shader. The reorder function for a
         * 1.x and 2.x pshader or for fixed function will write gl_FrontColor, and for a 3.0 shader
         * it will write to the varying array. Here we depend on the shader optimizer on sorting that
         * out. The nvidia driver only does that if the parameter is inout instead of out, hence the
         * inout.
         */
        if (reg_maps->shader_version.major >= 3)
        {
            shader_addline(buffer, "void order_ps_input(in vec4[%u]);\n", MAX_REG_OUTPUT);
        } else {
            shader_addline(buffer, "void order_ps_input();\n");
        }
    } else {
        for (i = 0, map = reg_maps->bumpmat; map; map >>= 1, ++i)
        {
            if (!(map & 1)) continue;

            shader_addline(buffer, "uniform mat2 bumpenvmat%d;\n", i);

            if (reg_maps->luminanceparams & (1 << i))
            {
                shader_addline(buffer, "uniform float luminancescale%d;\n", i);
                shader_addline(buffer, "uniform float luminanceoffset%d;\n", i);
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
            if (This->baseShader.limits.constant_float + extra_constants_needed
                    + 1 < gl_info->limits.glsl_ps_float_constants)
            {
                shader_addline(buffer, "uniform vec4 ycorrection;\n");
                ((IWineD3DPixelShaderImpl *) This)->vpos_uniform = 1;
                extra_constants_needed++;
            } else {
                /* This happens because we do not have proper tracking of the constant registers that are
                 * actually used, only the max limit of the shader version
                 */
                FIXME("Cannot find a free uniform for vpos correction params\n");
                shader_addline(buffer, "const vec4 ycorrection = vec4(%f, %f, 0.0, 0.0);\n",
                        context->render_offscreen ? 0.0f : device->render_targets[0]->currentDesc.Height,
                        context->render_offscreen ? 1.0f : -1.0f);
            }
            shader_addline(buffer, "vec4 vpos;\n");
        }
    }

    /* Declare texture samplers */
    for (i = 0; i < This->baseShader.limits.sampler; i++) {
        if (reg_maps->sampler_type[i])
        {
            switch (reg_maps->sampler_type[i])
            {
                case WINED3DSTT_1D:
                    if (pshader && ps_args->shadow & (1 << i))
                        shader_addline(buffer, "uniform sampler1DShadow %csampler%u;\n", prefix, i);
                    else
                        shader_addline(buffer, "uniform sampler1D %csampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_2D:
                    if (pshader && ps_args->shadow & (1 << i))
                    {
                        if (device->stateBlock->textures[i]
                                && IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[i])
                                == GL_TEXTURE_RECTANGLE_ARB)
                            shader_addline(buffer, "uniform sampler2DRectShadow %csampler%u;\n", prefix, i);
                        else
                            shader_addline(buffer, "uniform sampler2DShadow %csampler%u;\n", prefix, i);
                    }
                    else
                    {
                        if (device->stateBlock->textures[i]
                                && IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[i])
                                == GL_TEXTURE_RECTANGLE_ARB)
                            shader_addline(buffer, "uniform sampler2DRect %csampler%u;\n", prefix, i);
                        else
                            shader_addline(buffer, "uniform sampler2D %csampler%u;\n", prefix, i);
                    }
                    break;
                case WINED3DSTT_CUBE:
                    if (pshader && ps_args->shadow & (1 << i)) FIXME("Unsupported Cube shadow sampler.\n");
                    shader_addline(buffer, "uniform samplerCube %csampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_VOLUME:
                    if (pshader && ps_args->shadow & (1 << i)) FIXME("Unsupported 3D shadow sampler.\n");
                    shader_addline(buffer, "uniform sampler3D %csampler%u;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %csampler%u;\n", prefix, i);
                    FIXME("Unrecognized sampler type: %#x\n", reg_maps->sampler_type[i]);
                    break;
            }
        }
    }

    /* Declare uniforms for NP2 texcoord fixup:
     * This is NOT done inside the loop that declares the texture samplers since the NP2 fixup code
     * is currently only used for the GeforceFX series and when forcing the ARB_npot extension off.
     * Modern cards just skip the code anyway, so put it inside a separate loop. */
    if (pshader && ps_args->np2_fixup) {

        struct ps_np2fixup_info* const fixup = ctx_priv->cur_np2fixup_info;
        UINT cur = 0;

        /* NP2/RECT textures in OpenGL use texcoords in the range [0,width]x[0,height]
         * while D3D has them in the (normalized) [0,1]x[0,1] range.
         * samplerNP2Fixup stores texture dimensions and is updated through
         * shader_glsl_load_np2fixup_constants when the sampler changes. */

        for (i = 0; i < This->baseShader.limits.sampler; ++i) {
            if (reg_maps->sampler_type[i]) {
                if (!(ps_args->np2_fixup & (1 << i))) continue;

                if (WINED3DSTT_2D != reg_maps->sampler_type[i]) {
                    FIXME("Non-2D texture is flagged for NP2 texcoord fixup.\n");
                    continue;
                }

                fixup->idx[i] = cur++;
            }
        }

        fixup->num_consts = (cur + 1) >> 1;
        shader_addline(buffer, "uniform vec4 %csamplerNP2Fixup[%u];\n", prefix, fixup->num_consts);
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

    /* Declare input register varyings. Only pixel shader, vertex shaders have that declared in the
     * helper function shader that is linked in at link time
     */
    if (pshader && reg_maps->shader_version.major >= 3)
    {
        if (use_vs(device->stateBlock))
        {
            shader_addline(buffer, "varying vec4 IN[%u];\n", vec4_varyings(reg_maps->shader_version.major, gl_info));
        } else {
            /* TODO: Write a replacement shader for the fixed function vertex pipeline, so this isn't needed.
             * For fixed function vertex processing + 3.0 pixel shader we need a separate function in the
             * pixel shader that reads the fixed function color into the packed input registers.
             */
            shader_addline(buffer, "vec4 IN[%u];\n", vec4_varyings(reg_maps->shader_version.major, gl_info));
        }
    }

    /* Declare output register temporaries */
    if(This->baseShader.limits.packed_output) {
        shader_addline(buffer, "vec4 OUT[%u];\n", This->baseShader.limits.packed_output);
    }

    /* Declare temporary variables */
    for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "vec4 R%u;\n", i);
    }

    /* Declare attributes */
    if (reg_maps->shader_version.type == WINED3D_SHADER_TYPE_VERTEX)
    {
        for (i = 0, map = reg_maps->input_registers; map; map >>= 1, ++i)
        {
            if (map & 1) shader_addline(buffer, "attribute vec4 attrib%i;\n", i);
        }
    }

    /* Declare loop registers aLx */
    for (i = 0; i < reg_maps->loop_depth; i++) {
        shader_addline(buffer, "int aL%u;\n", i);
        shader_addline(buffer, "int tmpInt%u;\n", i);
    }

    /* Temporary variables for matrix operations */
    shader_addline(buffer, "vec4 tmp0;\n");
    shader_addline(buffer, "vec4 tmp1;\n");

    /* Local constants use a different name so they can be loaded once at shader link time
     * They can't be hardcoded into the shader text via LC = {x, y, z, w}; because the
     * float -> string conversion can cause precision loss.
     */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            shader_addline(buffer, "uniform vec4 %cLC%u;\n", prefix, lconst->idx);
        }
    }

    shader_addline(buffer, "const float FLT_MAX = 1e38;\n");

    /* Start the main program */
    shader_addline(buffer, "void main() {\n");
    if(pshader && reg_maps->vpos) {
        /* DirectX apps expect integer values, while OpenGL drivers add approximately 0.5. This causes
         * off-by-one problems as spotted by the vPos d3d9 visual test. Unfortunately the ATI cards do
         * not add exactly 0.5, but rather something like 0.49999999 or 0.50000001, which still causes
         * precision troubles when we just substract 0.5.
         *
         * To deal with that just floor() the position. This will eliminate the fraction on all cards.
         *
         * TODO: Test how that behaves with multisampling once we can enable multisampling in winex11.
         *
         * An advantage of floor is that it works even if the driver doesn't add 1/2. It is somewhat
         * questionable if 1.5, 2.5, ... are the proper values to return in gl_FragCoord, even though
         * coordinates specify the pixel centers instead of the pixel corners. This code will behave
         * correctly on drivers that returns integer values.
         */
        shader_addline(buffer, "vpos = floor(vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1));\n");
    }
}

/*****************************************************************************
 * Functions to generate GLSL strings from DirectX Shader bytecode begin here.
 *
 * For more information, see http://wiki.winehq.org/DirectX-Shaders
 ****************************************************************************/

/* Prototypes */
static void shader_glsl_add_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, glsl_src_param_t *glsl_src);

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
static void shader_glsl_gen_modifier(DWORD src_modifier, const char *in_reg, const char *in_regswizzle, char *out_str)
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
    static const char * const hwrastout_reg_names[] = { "gl_Position", "gl_FogFragCoord", "gl_PointSize" };

    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    char pshader = shader_is_pshader_version(This->baseShader.reg_maps.shader_version.type);

    *is_color = FALSE;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            sprintf(register_name, "R%u", reg->idx);
            break;

        case WINED3DSPR_INPUT:
            /* vertex shaders */
            if (!pshader)
            {
                struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
                if (priv->cur_vs_args->swizzle_map & (1 << reg->idx)) *is_color = TRUE;
                sprintf(register_name, "attrib%u", reg->idx);
                break;
            }

            /* pixel shaders >= 3.0 */
            if (This->baseShader.reg_maps.shader_version.major >= 3)
            {
                DWORD idx = ((IWineD3DPixelShaderImpl *)This)->input_reg_map[reg->idx];
                unsigned int in_count = vec4_varyings(This->baseShader.reg_maps.shader_version.major, gl_info);

                if (reg->rel_addr)
                {
                    glsl_src_param_t rel_param;

                    shader_glsl_add_src_param(ins, reg->rel_addr, WINED3DSP_WRITEMASK_0, &rel_param);

                    /* Removing a + 0 would be an obvious optimization, but macos doesn't see the NOP
                     * operation there */
                    if (idx)
                    {
                        if (((IWineD3DPixelShaderImpl *)This)->declared_in_count > in_count)
                        {
                            sprintf(register_name,
                                    "((%s + %u) > %d ? (%s + %u) > %d ? gl_SecondaryColor : gl_Color : IN[%s + %u])",
                                    rel_param.param_str, idx, in_count - 1, rel_param.param_str, idx, in_count,
                                    rel_param.param_str, idx);
                        }
                        else
                        {
                            sprintf(register_name, "IN[%s + %u]", rel_param.param_str, idx);
                        }
                    }
                    else
                    {
                        if (((IWineD3DPixelShaderImpl *)This)->declared_in_count > in_count)
                        {
                            sprintf(register_name, "((%s) > %d ? (%s) > %d ? gl_SecondaryColor : gl_Color : IN[%s])",
                                    rel_param.param_str, in_count - 1, rel_param.param_str, in_count,
                                    rel_param.param_str);
                        }
                        else
                        {
                            sprintf(register_name, "IN[%s]", rel_param.param_str);
                        }
                    }
                }
                else
                {
                    if (idx == in_count) sprintf(register_name, "gl_Color");
                    else if (idx == in_count + 1) sprintf(register_name, "gl_SecondaryColor");
                    else sprintf(register_name, "IN[%u]", idx);
                }
            }
            else
            {
                if (reg->idx == 0) strcpy(register_name, "gl_Color");
                else strcpy(register_name, "gl_SecondaryColor");
                break;
            }
            break;

        case WINED3DSPR_CONST:
            {
                const char prefix = pshader ? 'P' : 'V';

                /* Relative addressing */
                if (reg->rel_addr)
                {
                    glsl_src_param_t rel_param;
                    shader_glsl_add_src_param(ins, reg->rel_addr, WINED3DSP_WRITEMASK_0, &rel_param);
                    if (reg->idx) sprintf(register_name, "%cC[%s + %u]", prefix, rel_param.param_str, reg->idx);
                    else sprintf(register_name, "%cC[%s]", prefix, rel_param.param_str);
                }
                else
                {
                    if (shader_constant_is_local(This, reg->idx))
                        sprintf(register_name, "%cLC%u", prefix, reg->idx);
                    else
                        sprintf(register_name, "%cC[%u]", prefix, reg->idx);
                }
            }
            break;

        case WINED3DSPR_CONSTINT:
            if (pshader) sprintf(register_name, "PI[%u]", reg->idx);
            else sprintf(register_name, "VI[%u]", reg->idx);
            break;

        case WINED3DSPR_CONSTBOOL:
            if (pshader) sprintf(register_name, "PB[%u]", reg->idx);
            else sprintf(register_name, "VB[%u]", reg->idx);
            break;

        case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
            if (pshader) sprintf(register_name, "T%u", reg->idx);
            else sprintf(register_name, "A%u", reg->idx);
            break;

        case WINED3DSPR_LOOP:
            sprintf(register_name, "aL%u", This->baseShader.cur_loop_regno - 1);
            break;

        case WINED3DSPR_SAMPLER:
            if (pshader) sprintf(register_name, "Psampler%u", reg->idx);
            else sprintf(register_name, "Vsampler%u", reg->idx);
            break;

        case WINED3DSPR_COLOROUT:
            if (reg->idx >= gl_info->limits.buffers)
                WARN("Write to render target %u, only %d supported.\n", reg->idx, gl_info->limits.buffers);

            sprintf(register_name, "gl_FragData[%u]", reg->idx);
            break;

        case WINED3DSPR_RASTOUT:
            sprintf(register_name, "%s", hwrastout_reg_names[reg->idx]);
            break;

        case WINED3DSPR_DEPTHOUT:
            sprintf(register_name, "gl_FragDepth");
            break;

        case WINED3DSPR_ATTROUT:
            if (reg->idx == 0) sprintf(register_name, "gl_FrontColor");
            else sprintf(register_name, "gl_FrontSecondaryColor");
            break;

        case WINED3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0: WINED3DSPR_OUTPUT */
            if (This->baseShader.reg_maps.shader_version.major >= 3) sprintf(register_name, "OUT[%u]", reg->idx);
            else sprintf(register_name, "gl_TexCoord[%u]", reg->idx);
            break;

        case WINED3DSPR_MISCTYPE:
            if (reg->idx == 0)
            {
                /* vPos */
                sprintf(register_name, "vpos");
            }
            else if (reg->idx == 1)
            {
                /* Note that gl_FrontFacing is a bool, while vFace is
                 * a float for which the sign determines front/back */
                sprintf(register_name, "(gl_FrontFacing ? 1.0 : -1.0)");
            }
            else
            {
                FIXME("Unhandled misctype register %d\n", reg->idx);
                sprintf(register_name, "unrecognized_register");
            }
            break;

        case WINED3DSPR_IMMCONST:
            switch (reg->immconst_type)
            {
                case WINED3D_IMMCONST_FLOAT:
                    sprintf(register_name, "%.8e", *(const float *)reg->immconst_data);
                    break;

                case WINED3D_IMMCONST_FLOAT4:
                    sprintf(register_name, "vec4(%.8e, %.8e, %.8e, %.8e)",
                            *(const float *)&reg->immconst_data[0], *(const float *)&reg->immconst_data[1],
                            *(const float *)&reg->immconst_data[2], *(const float *)&reg->immconst_data[3]);
                    break;

                default:
                    FIXME("Unhandled immconst type %#x\n", reg->immconst_type);
                    sprintf(register_name, "<unhandled_immconst_type %#x>", reg->immconst_type);
            }
            break;

        default:
            FIXME("Unhandled register name Type(%d)\n", reg->type);
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
        const struct wined3d_shader_src_param *wined3d_src, DWORD mask, glsl_src_param_t *glsl_src)
{
    BOOL is_color = FALSE;
    char swizzle_str[6];

    glsl_src->reg_name[0] = '\0';
    glsl_src->param_str[0] = '\0';
    swizzle_str[0] = '\0';

    shader_glsl_get_register_name(&wined3d_src->reg, glsl_src->reg_name, &is_color, ins);
    shader_glsl_get_swizzle(wined3d_src, is_color, mask, swizzle_str);
    shader_glsl_gen_modifier(wined3d_src->modifiers, glsl_src->reg_name, swizzle_str, glsl_src->param_str);
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static DWORD shader_glsl_add_dst_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_dst_param *wined3d_dst, glsl_dst_param_t *glsl_dst)
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
    glsl_dst_param_t glsl_dst;
    DWORD mask;

    mask = shader_glsl_add_dst_param(ins, dst, &glsl_dst);
    if (mask) shader_addline(buffer, "%s%s = %s(", glsl_dst.reg_name, glsl_dst.mask_str, shift_glsl_tab[dst->shift]);

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
    glsl_dst_param_t dst_param;
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

static inline const char *shader_get_comp_op(DWORD op)
{
    switch (op) {
        case COMPARISON_GT: return ">";
        case COMPARISON_EQ: return "==";
        case COMPARISON_GE: return ">=";
        case COMPARISON_LT: return "<";
        case COMPARISON_NE: return "!=";
        case COMPARISON_LE: return "<=";
        default:
            FIXME("Unrecognized comparison value: %u\n", op);
            return "(\?\?)";
    }
}

static void shader_glsl_get_sample_function(const struct wined3d_shader_context *ctx,
        DWORD sampler_idx, DWORD flags, glsl_sample_function_t *sample_function)
{
    WINED3DSAMPLER_TEXTURE_TYPE sampler_type = ctx->reg_maps->sampler_type[sampler_idx];
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    BOOL shadow = shader_is_pshader_version(ctx->reg_maps->shader_version.type)
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
                FIXME("Unsupported Cube shadow function.\n ");
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
                FIXME("Unsupported 3D shadow function.\n ");
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

static void shader_glsl_color_correction(const struct wined3d_shader_instruction *ins, struct color_fixup_desc fixup)
{
    struct wined3d_shader_dst_param dst;
    unsigned int mask_size, remaining;
    glsl_dst_param_t dst_param;
    char arguments[256];
    DWORD mask;

    mask = 0;
    if (fixup.x_sign_fixup || fixup.x_source != CHANNEL_SOURCE_X) mask |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_sign_fixup || fixup.y_source != CHANNEL_SOURCE_Y) mask |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_sign_fixup || fixup.z_source != CHANNEL_SOURCE_Z) mask |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_sign_fixup || fixup.w_source != CHANNEL_SOURCE_W) mask |= WINED3DSP_WRITEMASK_3;
    mask &= ins->dst[0].write_mask;

    if (!mask) return; /* Nothing to do */

    if (is_complex_fixup(fixup))
    {
        enum complex_fixup complex_fixup = get_complex_fixup(fixup);
        FIXME("Complex fixup (%#x) not supported\n",complex_fixup);
        return;
    }

    mask_size = shader_glsl_get_write_mask_size(mask);

    dst = ins->dst[0];
    dst.write_mask = mask;
    shader_glsl_add_dst_param(ins, &dst, &dst_param);

    arguments[0] = '\0';
    remaining = mask_size;
    if (mask & WINED3DSP_WRITEMASK_0)
    {
        shader_glsl_append_fixup_arg(arguments, dst_param.reg_name, fixup.x_sign_fixup, fixup.x_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_1)
    {
        shader_glsl_append_fixup_arg(arguments, dst_param.reg_name, fixup.y_sign_fixup, fixup.y_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_2)
    {
        shader_glsl_append_fixup_arg(arguments, dst_param.reg_name, fixup.z_sign_fixup, fixup.z_source);
        if (--remaining) strcat(arguments, ", ");
    }
    if (mask & WINED3DSP_WRITEMASK_3)
    {
        shader_glsl_append_fixup_arg(arguments, dst_param.reg_name, fixup.w_sign_fixup, fixup.w_source);
        if (--remaining) strcat(arguments, ", ");
    }

    if (mask_size > 1)
    {
        shader_addline(ins->ctx->buffer, "%s%s = vec%u(%s);\n",
                dst_param.reg_name, dst_param.mask_str, mask_size, arguments);
    }
    else
    {
        shader_addline(ins->ctx->buffer, "%s%s = %s;\n", dst_param.reg_name, dst_param.mask_str, arguments);
    }
}

static void PRINTF_ATTR(8, 9) shader_glsl_gen_sample_code(const struct wined3d_shader_instruction *ins,
        DWORD sampler, const glsl_sample_function_t *sample_function, DWORD swizzle,
        const char *dx, const char *dy,
        const char *bias, const char *coord_reg_fmt, ...)
{
    const char *sampler_base;
    char dst_swizzle[6];
    struct color_fixup_desc fixup;
    BOOL np2_fixup = FALSE;
    va_list args;

    shader_glsl_swizzle_to_str(swizzle, FALSE, ins->dst[0].write_mask, dst_swizzle);

    if (shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type))
    {
        const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
        fixup = priv->cur_ps_args->color_fixup[sampler];
        sampler_base = "Psampler";

        if(priv->cur_ps_args->np2_fixup & (1 << sampler)) {
            if(bias) {
                FIXME("Biased sampling from NP2 textures is unsupported\n");
            } else {
                np2_fixup = TRUE;
            }
        }
    } else {
        sampler_base = "Vsampler";
        fixup = COLOR_FIXUP_IDENTITY; /* FIXME: Vshader color fixup */
    }

    shader_glsl_append_dst(ins->ctx->buffer, ins);

    shader_addline(ins->ctx->buffer, "%s(%s%u, ", sample_function->name, sampler_base, sampler);

    va_start(args, coord_reg_fmt);
    shader_vaddline(ins->ctx->buffer, coord_reg_fmt, args);
    va_end(args);

    if(bias) {
        shader_addline(ins->ctx->buffer, ", %s)%s);\n", bias, dst_swizzle);
    } else {
        if (np2_fixup) {
            const struct shader_glsl_ctx_priv *priv = ins->ctx->backend_data;
            const unsigned char idx = priv->cur_np2fixup_info->idx[sampler];

            shader_addline(ins->ctx->buffer, " * PsamplerNP2Fixup[%u].%s)%s);\n", idx >> 1,
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

/* Generate GLSL arithmetic functions (dst = src1 + src2) */
static void shader_glsl_arith(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD write_mask;
    char op;

    /* Determine the GLSL operator to use based on the opcode */
    switch (ins->handler_idx)
    {
        case WINED3DSIH_MUL: op = '*'; break;
        case WINED3DSIH_ADD: op = '+'; break;
        case WINED3DSIH_SUB: op = '-'; break;
        default:
            op = ' ';
            FIXME("Opcode %#x not yet handled in GLSL\n", ins->handler_idx);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
    shader_addline(buffer, "%s %c %s);\n", src0_param.param_str, op, src1_param.param_str);
}

/* Process the WINED3DSIO_MOV opcode using GLSL (dst = src) */
static void shader_glsl_mov(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src0_param);

    /* In vs_1_1 WINED3DSIO_MOV can write to the address register. In later
     * shader versions WINED3DSIO_MOVA is used for this. */
    if (ins->ctx->reg_maps->shader_version.major == 1
            && !shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type)
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
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
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
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    char dst_mask[6];

    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_mask, &src1_param);
    shader_addline(ins->ctx->buffer, "cross(%s, %s)%s);\n", src0_param.param_str, src1_param.param_str, dst_mask);
}

/* Process the WINED3DSIO_POW instruction in GLSL (dst = |src0|^src1)
 * Src0 and src1 are scalars. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_pow(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(pow(abs(%s), %s)));\n", dst_size, src0_param.param_str, src1_param.param_str);
    } else {
        shader_addline(buffer, "pow(abs(%s), %s));\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Process the WINED3DSIO_LOG instruction in GLSL (dst = log2(|src0|))
 * Src0 is a scalar. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_log(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, ins);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);

    if (dst_size > 1)
    {
        shader_addline(buffer, "vec%d(%s == 0.0 ? -FLT_MAX : log2(abs(%s))));\n",
                dst_size, src0_param.param_str, src0_param.param_str);
    }
    else
    {
        shader_addline(buffer, "%s == 0.0 ? -FLT_MAX : log2(abs(%s)));\n",
                src0_param.param_str, src0_param.param_str);
    }
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
static void shader_glsl_map2gl(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src_param;
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

static void shader_glsl_nrm(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src_param;
    unsigned int mask_size;
    DWORD write_mask;
    char dst_mask[6];

    write_mask = shader_glsl_get_write_mask(ins->dst, dst_mask);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], write_mask, &src_param);

    shader_addline(buffer, "tmp0.x = length(%s);\n", src_param.param_str);
    shader_glsl_append_dst(buffer, ins);
    if (mask_size > 1)
    {
        shader_addline(buffer, "tmp0.x == 0.0 ? vec%u(0.0) : (%s / tmp0.x));\n",
                mask_size, src_param.param_str);
    }
    else
    {
        shader_addline(buffer, "tmp0.x == 0.0 ? 0.0 : (%s / tmp0.x));\n",
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
    glsl_src_param_t src_param;

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

/** Process the RCP (reciprocal or inverse) opcode in GLSL (dst = 1 / src) */
static void shader_glsl_rcp(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1)
    {
        shader_addline(ins->ctx->buffer, "vec%d(%s == 0.0 ? FLT_MAX : 1.0 / %s));\n",
                mask_size, src_param.param_str, src_param.param_str);
    }
    else
    {
        shader_addline(ins->ctx->buffer, "%s == 0.0 ? FLT_MAX : 1.0 / %s);\n",
                src_param.param_str, src_param.param_str);
    }
}

static void shader_glsl_rsq(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(buffer, ins);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1)
    {
        shader_addline(buffer, "vec%d(%s == 0.0 ? FLT_MAX : inversesqrt(abs(%s))));\n",
                mask_size, src_param.param_str, src_param.param_str);
    }
    else
    {
        shader_addline(buffer, "%s == 0.0 ? FLT_MAX : inversesqrt(abs(%s)));\n",
                src_param.param_str, src_param.param_str);
    }
}

/** Process signed comparison opcodes in GLSL. */
static void shader_glsl_compare(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
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

/** Process CMP instruction in GLSL (dst = src0 >= 0.0 ? src1 : src2), per channel */
static void shader_glsl_cmp(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask, cmp_channel = 0;
    unsigned int i, j;
    char mask_char[6];
    BOOL temp_destination = FALSE;

    if (shader_is_scalar(&ins->src[0].reg))
    {
        write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);

        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_ALL, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

        shader_addline(ins->ctx->buffer, "%s >= 0.0 ? %s : %s);\n",
                       src0_param.param_str, src1_param.param_str, src2_param.param_str);
    } else {
        DWORD dst_mask = ins->dst[0].write_mask;
        struct wined3d_shader_dst_param dst = ins->dst[0];

        /* Cycle through all source0 channels */
        for (i=0; i<4; i++) {
            write_mask = 0;
            /* Find the destination channels which use the current source0 channel */
            for (j=0; j<4; j++) {
                if (((ins->src[0].swizzle >> (2 * j)) & 0x3) == i)
                {
                    write_mask |= WINED3DSP_WRITEMASK_0 << j;
                    cmp_channel = WINED3DSP_WRITEMASK_0 << j;
                }
            }
            dst.write_mask = dst_mask & write_mask;

            /* Splitting the cmp instruction up in multiple lines imposes a problem:
            * The first lines may overwrite source parameters of the following lines.
            * Deal with that by using a temporary destination register if needed
            */
            if ((ins->src[0].reg.idx == ins->dst[0].reg.idx
                    && ins->src[0].reg.type == ins->dst[0].reg.type)
                    || (ins->src[1].reg.idx == ins->dst[0].reg.idx
                    && ins->src[1].reg.type == ins->dst[0].reg.type)
                    || (ins->src[2].reg.idx == ins->dst[0].reg.idx
                    && ins->src[2].reg.type == ins->dst[0].reg.type))
            {
                write_mask = shader_glsl_get_write_mask(&dst, mask_char);
                if (!write_mask) continue;
                shader_addline(ins->ctx->buffer, "tmp0%s = (", mask_char);
                temp_destination = TRUE;
            } else {
                write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &dst);
                if (!write_mask) continue;
            }

            shader_glsl_add_src_param(ins, &ins->src[0], cmp_channel, &src0_param);
            shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
            shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

            shader_addline(ins->ctx->buffer, "%s >= 0.0 ? %s : %s);\n",
                        src0_param.param_str, src1_param.param_str, src2_param.param_str);
        }

        if(temp_destination) {
            shader_glsl_get_write_mask(&ins->dst[0], mask_char);
            shader_glsl_append_dst(ins->ctx->buffer, ins);
            shader_addline(ins->ctx->buffer, "tmp0%s);\n", mask_char);
        }
    }

}

/** Process the CND opcode in GLSL (dst = (src0 > 0.5) ? src1 : src2) */
/* For ps 1.1-1.3, only a single component of src0 is used. For ps 1.4
 * the compare is done per component of src0. */
static void shader_glsl_cnd(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_dst_param dst;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask, cmp_channel = 0;
    unsigned int i, j;
    DWORD dst_mask;
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
    /* Cycle through all source0 channels */
    dst_mask = ins->dst[0].write_mask;
    dst = ins->dst[0];
    for (i=0; i<4; i++) {
        write_mask = 0;
        /* Find the destination channels which use the current source0 channel */
        for (j=0; j<4; j++) {
            if (((ins->src[0].swizzle >> (2 * j)) & 0x3) == i)
            {
                write_mask |= WINED3DSP_WRITEMASK_0 << j;
                cmp_channel = WINED3DSP_WRITEMASK_0 << j;
            }
        }

        dst.write_mask = dst_mask & write_mask;
        write_mask = shader_glsl_append_dst_ext(ins->ctx->buffer, ins, &dst);
        if (!write_mask) continue;

        shader_glsl_add_src_param(ins, &ins->src[0], cmp_channel, &src0_param);
        shader_glsl_add_src_param(ins, &ins->src[1], write_mask, &src1_param);
        shader_glsl_add_src_param(ins, &ins->src[2], write_mask, &src2_param);

        shader_addline(ins->ctx->buffer, "%s > 0.5 ? %s : %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
    }
}

/** GLSL code generation for WINED3DSIO_MAD: Multiply the first 2 opcodes, then add the last */
static void shader_glsl_mad(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
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
        ++tmp_src[1].reg.idx;
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
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
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
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src3_param;
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
     * if both x and y are > 0, we get pow(y * 1.0, power), as it is supposed to
     */
    shader_addline(ins->ctx->buffer,
            "vec4(1.0, max(%s, 0.0), pow(max(0.0, %s) * step(0.0, %s), clamp(%s, -128.0, 128.0)), 1.0)%s);\n",
            src0_param.param_str, src1_param.param_str, src0_param.param_str, src3_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
static void shader_glsl_dst(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0y_param;
    glsl_src_param_t src0z_param;
    glsl_src_param_t src1y_param;
    glsl_src_param_t src1w_param;
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
    glsl_src_param_t src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);

    switch (write_mask) {
        case WINED3DSP_WRITEMASK_0:
            shader_addline(ins->ctx->buffer, "cos(%s));\n", src0_param.param_str);
            break;

        case WINED3DSP_WRITEMASK_1:
            shader_addline(ins->ctx->buffer, "sin(%s));\n", src0_param.param_str);
            break;

        case (WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1):
            shader_addline(ins->ctx->buffer, "vec2(cos(%s), sin(%s)));\n", src0_param.param_str, src0_param.param_str);
            break;

        default:
            ERR("Write mask should be .x, .y or .xy\n");
            break;
    }
}

/* sgn in vs_2_0 has 2 extra parameters(registers for temporary storage) which we don't use
 * here. But those extra parameters require a dedicated function for sgn, since map2gl would
 * generate invalid code
 */
static void shader_glsl_sgn(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
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
    glsl_src_param_t src1_param;
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    const DWORD *control_values = NULL;
    const local_constant *constant;

    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_ALL, &src1_param);

    /* Try to hardcode the loop control parameters if possible. Direct3D 9 class hardware doesn't support real
     * varying indexing, but Microsoft designed this feature for Shader model 2.x+. If the loop control is
     * known at compile time, the GLSL compiler can unroll the loop, and replace indirect addressing with direct
     * addressing.
     */
    if (ins->src[1].reg.type == WINED3DSPR_CONSTINT)
    {
        LIST_FOR_EACH_ENTRY(constant, &shader->baseShader.constantsI, local_constant, entry) {
            if (constant->idx == ins->src[1].reg.idx)
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
            shader_addline(ins->ctx->buffer, "for (aL%u = %u; aL%u < (%u * %d + %u); aL%u += %d) {\n",
                    shader->baseShader.cur_loop_depth, loop_control.start,
                    shader->baseShader.cur_loop_depth, loop_control.count, loop_control.step, loop_control.start,
                    shader->baseShader.cur_loop_depth, loop_control.step);
        }
        else if (loop_control.step < 0)
        {
            shader_addline(ins->ctx->buffer, "for (aL%u = %u; aL%u > (%u * %d + %u); aL%u += %d) {\n",
                    shader->baseShader.cur_loop_depth, loop_control.start,
                    shader->baseShader.cur_loop_depth, loop_control.count, loop_control.step, loop_control.start,
                    shader->baseShader.cur_loop_depth, loop_control.step);
        }
        else
        {
            shader_addline(ins->ctx->buffer, "for (aL%u = %u, tmpInt%u = 0; tmpInt%u < %u; tmpInt%u++) {\n",
                    shader->baseShader.cur_loop_depth, loop_control.start, shader->baseShader.cur_loop_depth,
                    shader->baseShader.cur_loop_depth, loop_control.count,
                    shader->baseShader.cur_loop_depth);
        }
    } else {
        shader_addline(ins->ctx->buffer,
                "for (tmpInt%u = 0, aL%u = %s.y; tmpInt%u < %s.x; tmpInt%u++, aL%u += %s.z) {\n",
                shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_regno,
                src1_param.reg_name, shader->baseShader.cur_loop_depth, src1_param.reg_name,
                shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_regno, src1_param.reg_name);
    }

    shader->baseShader.cur_loop_depth++;
    shader->baseShader.cur_loop_regno++;
}

static void shader_glsl_end(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;

    shader_addline(ins->ctx->buffer, "}\n");

    if (ins->handler_idx == WINED3DSIH_ENDLOOP)
    {
        shader->baseShader.cur_loop_depth--;
        shader->baseShader.cur_loop_regno--;
    }

    if (ins->handler_idx == WINED3DSIH_ENDREP)
    {
        shader->baseShader.cur_loop_depth--;
    }
}

static void shader_glsl_rep(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    glsl_src_param_t src0_param;
    const DWORD *control_values = NULL;
    const local_constant *constant;

    /* Try to hardcode local values to help the GLSL compiler to unroll and optimize the loop */
    if (ins->src[0].reg.type == WINED3DSPR_CONSTINT)
    {
        LIST_FOR_EACH_ENTRY(constant, &shader->baseShader.constantsI, local_constant, entry)
        {
            if (constant->idx == ins->src[0].reg.idx)
            {
                control_values = constant->value;
                break;
            }
        }
    }

    if(control_values) {
        shader_addline(ins->ctx->buffer, "for (tmpInt%d = 0; tmpInt%d < %d; tmpInt%d++) {\n",
                       shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_depth,
                       control_values[0], shader->baseShader.cur_loop_depth);
    } else {
        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
        shader_addline(ins->ctx->buffer, "for (tmpInt%d = 0; tmpInt%d < %s; tmpInt%d++) {\n",
                shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_depth,
                src0_param.param_str, shader->baseShader.cur_loop_depth);
    }
    shader->baseShader.cur_loop_depth++;
}

static void shader_glsl_if(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(ins->ctx->buffer, "if (%s) {\n", src0_param.param_str);
}

static void shader_glsl_ifc(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(ins->ctx->buffer, "if (%s %s %s) {\n",
            src0_param.param_str, shader_get_comp_op(ins->flags), src1_param.param_str);
}

static void shader_glsl_else(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "} else {\n");
}

static void shader_glsl_break(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "break;\n");
}

/* FIXME: According to MSDN the compare is done per component. */
static void shader_glsl_breakc(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(ins->ctx->buffer, "if (%s %s %s) break;\n",
            src0_param.param_str, shader_get_comp_op(ins->flags), src1_param.param_str);
}

static void shader_glsl_label(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "}\n");
    shader_addline(ins->ctx->buffer, "void subroutine%u () {\n",  ins->src[0].reg.idx);
}

static void shader_glsl_call(const struct wined3d_shader_instruction *ins)
{
    shader_addline(ins->ctx->buffer, "subroutine%u();\n", ins->src[0].reg.idx);
}

static void shader_glsl_callnz(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0, &src1_param);
    shader_addline(ins->ctx->buffer, "if (%s) subroutine%u();\n", src1_param.param_str, ins->src[0].reg.idx);
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
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    glsl_sample_function_t sample_function;
    DWORD sample_flags = 0;
    DWORD sampler_idx;
    DWORD mask = 0, swizzle;

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0)) sampler_idx = ins->dst[0].reg.idx;
    else sampler_idx = ins->src[1].reg.idx;

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD flags = deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS];
        WINED3DSAMPLER_TEXTURE_TYPE sampler_type = ins->ctx->reg_maps->sampler_type[sampler_idx];

        /* Projected cube textures don't make a lot of sense, the resulting coordinates stay the same. */
        if (flags & WINED3DTTFF_PROJECTED && sampler_type != WINED3DSTT_CUBE) {
            sample_flags |= WINED3D_GLSL_SAMPLE_PROJECTED;
            switch (flags & ~WINED3DTTFF_PROJECTED) {
                case WINED3DTTFF_COUNT1: FIXME("WINED3DTTFF_PROJECTED with WINED3DTTFF_COUNT1?\n"); break;
                case WINED3DTTFF_COUNT2: mask = WINED3DSP_WRITEMASK_1; break;
                case WINED3DTTFF_COUNT3: mask = WINED3DSP_WRITEMASK_2; break;
                case WINED3DTTFF_COUNT4:
                case WINED3DTTFF_DISABLE: mask = WINED3DSP_WRITEMASK_3; break;
            }
        }
    }
    else if (shader_version < WINED3D_SHADER_VERSION(2,0))
    {
        DWORD src_mod = ins->src[0].modifiers;

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

    if(deviceImpl->stateBlock->textures[sampler_idx] &&
       IWineD3DBaseTexture_GetTextureDimensions(deviceImpl->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;
    }

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
    } else {
        glsl_src_param_t coord_param;
        shader_glsl_add_src_param(ins, &ins->src[0], mask, &coord_param);
        if (ins->flags & WINED3DSI_TEXLD_BIAS)
        {
            glsl_src_param_t bias;
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
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param, dx_param, dy_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_GRAD;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4])
    {
        FIXME("texldd used, but not supported by hardware. Falling back to regular tex\n");
        return shader_glsl_tex(ins);
    }

    sampler_idx = ins->src[1].reg.idx;
    if(deviceImpl->stateBlock->textures[sampler_idx] &&
       IWineD3DBaseTexture_GetTextureDimensions(deviceImpl->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;
    }

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);
    shader_glsl_add_src_param(ins, &ins->src[2], sample_function.coord_mask, &dx_param);
    shader_glsl_add_src_param(ins, &ins->src[3], sample_function.coord_mask, &dy_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, swizzle, dx_param.param_str, dy_param.param_str, NULL,
                                "%s", coord_param.param_str);
}

static void shader_glsl_texldl(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    const struct wined3d_gl_info *gl_info = ins->ctx->gl_info;
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param, lod_param;
    DWORD sample_flags = WINED3D_GLSL_SAMPLE_LOD;
    DWORD sampler_idx;
    DWORD swizzle = ins->src[1].swizzle;

    sampler_idx = ins->src[1].reg.idx;
    if(deviceImpl->stateBlock->textures[sampler_idx] &&
       IWineD3DBaseTexture_GetTextureDimensions(deviceImpl->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
        sample_flags |= WINED3D_GLSL_SAMPLE_RECT;
    }
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, sample_flags, &sample_function);
    shader_glsl_add_src_param(ins, &ins->src[0], sample_function.coord_mask, &coord_param);

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_3, &lod_param);

    if (!gl_info->supported[ARB_SHADER_TEXTURE_LOD] && !gl_info->supported[EXT_GPU_SHADER4]
            && shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type))
    {
        /* The GLSL spec claims the Lod sampling functions are only supported in vertex shaders.
         * However, they seem to work just fine in fragment shaders as well. */
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
                ins->dst[0].reg.idx, dst_mask);
    } else {
        DWORD reg = ins->src[0].reg.idx;
        DWORD src_mod = ins->src[0].modifiers;
        char dst_swizzle[6];

        shader_glsl_get_swizzle(&ins->src[0], FALSE, write_mask, dst_swizzle);

        if (src_mod == WINED3DSPSM_DZ) {
            glsl_src_param_t div_param;
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
            shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_2, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        } else if (src_mod == WINED3DSPSM_DW) {
            glsl_src_param_t div_param;
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
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
    glsl_src_param_t src0_param;
    glsl_sample_function_t sample_function;
    DWORD sampler_idx = ins->dst[0].reg.idx;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
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
    glsl_src_param_t src0_param;
    DWORD dstreg = ins->dst[0].reg.idx;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
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
    glsl_dst_param_t dst_param;

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
    DWORD dstreg = ins->dst[0].reg.idx;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    shader_addline(ins->ctx->buffer, "tmp0.y = dot(T%u.xyz, %s);\n", dstreg, src0_param.param_str);
    shader_addline(ins->ctx->buffer, "gl_FragDepth = (tmp0.y == 0.0) ? 1.0 : clamp(tmp0.x / tmp0.y, 0.0, 1.0);\n");
}

/** Process the WINED3DSIO_TEXM3X2PAD instruction in GLSL
 * Calculate the 1st of a 2-row matrix multiplication. */
static void shader_glsl_texm3x2pad(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
static void shader_glsl_texm3x3pad(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.%c = dot(T%u.xyz, %s);\n", 'x' + current_state->current_row, reg, src0_param.param_str);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

static void shader_glsl_texm3x2tex(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    glsl_src_param_t src0_param;
    glsl_sample_function_t sample_function;

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
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;
    DWORD reg = ins->dst[0].reg.idx;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_addline(ins->ctx->buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xyz");

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3 instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply */
static void shader_glsl_texm3x3(const struct wined3d_shader_instruction *ins)
{
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD reg = ins->dst[0].reg.idx;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_glsl_get_write_mask(&ins->dst[0], dst_mask);
    shader_addline(ins->ctx->buffer, "vec4(tmp0.xy, dot(T%u.xyz, %s), 1.0)%s);\n", reg, src0_param.param_str, dst_mask);

    current_state->current_row = 0;
}

/* Process the WINED3DSIO_TEXM3X3SPEC instruction in GLSL
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void shader_glsl_texm3x3spec(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    DWORD reg = ins->dst[0].reg.idx;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], src_mask, &src1_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
    /* Reflection calculation */
    shader_addline(buffer, "tmp0.xyz = -reflect((%s), normalize(tmp0.xyz));\n", src1_param.param_str);

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xyz");

    current_state->current_row = 0;
}

/* Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void shader_glsl_texm3x3vspec(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(ins, &ins->src[0], src_mask, &src0_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_param.param_str);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "tmp1.xyz = normalize(vec3(gl_TexCoord[%u].w, gl_TexCoord[%u].w, gl_TexCoord[%u].w));\n",
            current_state->texcoord_w[0], current_state->texcoord_w[1], reg);
    shader_addline(buffer, "tmp0.xyz = -reflect(tmp1.xyz, normalize(tmp0.xyz));\n");

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, reg, 0, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_glsl_gen_sample_code(ins, reg, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL, "tmp0.xyz");

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * texbem is pshader <= 1.3 only, this saves a few version checks
 */
static void shader_glsl_texbem(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param;
    DWORD sampler_idx;
    DWORD mask;
    DWORD flags;
    char coord_mask[6];

    sampler_idx = ins->dst[0].reg.idx;
    flags = deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS];

    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    mask = sample_function.coord_mask;

    shader_glsl_write_mask_to_str(mask, coord_mask);

    /* with projective textures, texbem only divides the static texture coord, not the displacement,
         * so we can't let the GL handle this.
         */
    if (flags & WINED3DTTFF_PROJECTED) {
        DWORD div_mask=0;
        char coord_div_mask[3];
        switch (flags & ~WINED3DTTFF_PROJECTED) {
            case WINED3DTTFF_COUNT1: FIXME("WINED3DTTFF_PROJECTED with WINED3DTTFF_COUNT1?\n"); break;
            case WINED3DTTFF_COUNT2: div_mask = WINED3DSP_WRITEMASK_1; break;
            case WINED3DTTFF_COUNT3: div_mask = WINED3DSP_WRITEMASK_2; break;
            case WINED3DTTFF_COUNT4:
            case WINED3DTTFF_DISABLE: div_mask = WINED3DSP_WRITEMASK_3; break;
        }
        shader_glsl_write_mask_to_str(div_mask, coord_div_mask);
        shader_addline(ins->ctx->buffer, "T%u%s /= T%u%s;\n", sampler_idx, coord_mask, sampler_idx, coord_div_mask);
    }

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &coord_param);

    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "T%u%s + vec4(bumpenvmat%d * %s, 0.0, 0.0)%s", sampler_idx, coord_mask, sampler_idx,
            coord_param.param_str, coord_mask);

    if (ins->handler_idx == WINED3DSIH_TEXBEML)
    {
        glsl_src_param_t luminance_param;
        glsl_dst_param_t dst_param;

        shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_2, &luminance_param);
        shader_glsl_add_dst_param(ins, &ins->dst[0], &dst_param);

        shader_addline(ins->ctx->buffer, "%s%s *= (%s * luminancescale%d + luminanceoffset%d);\n",
                dst_param.reg_name, dst_param.mask_str,
                luminance_param.param_str, sampler_idx, sampler_idx);
    }
}

static void shader_glsl_bem(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param, src1_param;
    DWORD sampler_idx = ins->dst[0].reg.idx;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(ins, &ins->src[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src1_param);

    shader_glsl_append_dst(ins->ctx->buffer, ins);
    shader_addline(ins->ctx->buffer, "%s + bumpenvmat%d * %s);\n",
            src0_param.param_str, sampler_idx, src1_param.param_str);
}

/** Process the WINED3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
static void shader_glsl_texreg2ar(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    DWORD sampler_idx = ins->dst[0].reg.idx;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "%s.wx", src0_param.reg_name);
}

/** Process the WINED3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
static void shader_glsl_texreg2gb(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    DWORD sampler_idx = ins->dst[0].reg.idx;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(ins, &ins->src[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_glsl_get_sample_function(ins->ctx, sampler_idx, 0, &sample_function);
    shader_glsl_gen_sample_code(ins, sampler_idx, &sample_function, WINED3DSP_NOSWIZZLE, NULL, NULL, NULL,
            "%s.yz", src0_param.reg_name);
}

/** Process the WINED3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
static void shader_glsl_texreg2rgb(const struct wined3d_shader_instruction *ins)
{
    glsl_src_param_t src0_param;
    DWORD sampler_idx = ins->dst[0].reg.idx;
    glsl_sample_function_t sample_function;

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
    glsl_dst_param_t dst_param;

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
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
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

static void shader_glsl_input_pack(IWineD3DPixelShader *iface, struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader_signature_element *input_signature, const struct shader_reg_maps *reg_maps,
        enum vertexprocessing_mode vertexprocessing)
{
    unsigned int i;
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    WORD map = reg_maps->input_registers;

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

        if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_TEXCOORD))
        {
            if (semantic_idx < 8 && vertexprocessing == pretransformed)
                shader_addline(buffer, "IN[%u]%s = gl_TexCoord[%u]%s;\n",
                        This->input_reg_map[i], reg_mask, semantic_idx, reg_mask);
            else
                shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        This->input_reg_map[i], reg_mask, reg_mask);
        }
        else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_COLOR))
        {
            if (semantic_idx == 0)
                shader_addline(buffer, "IN[%u]%s = vec4(gl_Color)%s;\n",
                        This->input_reg_map[i], reg_mask, reg_mask);
            else if (semantic_idx == 1)
                shader_addline(buffer, "IN[%u]%s = vec4(gl_SecondaryColor)%s;\n",
                        This->input_reg_map[i], reg_mask, reg_mask);
            else
                shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        This->input_reg_map[i], reg_mask, reg_mask);
        }
        else
        {
            shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                    This->input_reg_map[i], reg_mask, reg_mask);
        }
    }
}

/*********************************************
 * Vertex Shader Specific Code begins here
 ********************************************/

static void add_glsl_program_entry(struct shader_glsl_priv *priv, struct glsl_shader_prog_link *entry) {
    glsl_program_key_t key;

    key.vshader = entry->vshader;
    key.pshader = entry->pshader;
    key.vs_args = entry->vs_args;
    key.ps_args = entry->ps_args;

    if (wine_rb_put(&priv->program_lookup, &key, &entry->program_lookup_entry) == -1)
    {
        ERR("Failed to insert program entry.\n");
    }
}

static struct glsl_shader_prog_link *get_glsl_program_entry(struct shader_glsl_priv *priv,
        IWineD3DVertexShader *vshader, IWineD3DPixelShader *pshader, struct vs_compile_args *vs_args,
        struct ps_compile_args *ps_args) {
    struct wine_rb_entry *entry;
    glsl_program_key_t key;

    key.vshader = vshader;
    key.pshader = pshader;
    key.vs_args = *vs_args;
    key.ps_args = *ps_args;

    entry = wine_rb_get(&priv->program_lookup, &key);
    return entry ? WINE_RB_ENTRY_VALUE(entry, struct glsl_shader_prog_link, program_lookup_entry) : NULL;
}

/* GL locking is done by the caller */
static void delete_glsl_program_entry(struct shader_glsl_priv *priv, const struct wined3d_gl_info *gl_info,
        struct glsl_shader_prog_link *entry)
{
    glsl_program_key_t key;

    key.vshader = entry->vshader;
    key.pshader = entry->pshader;
    key.vs_args = entry->vs_args;
    key.ps_args = entry->ps_args;
    wine_rb_remove(&priv->program_lookup, &key);

    GL_EXTCALL(glDeleteObjectARB(entry->programId));
    if (entry->vshader) list_remove(&entry->vshader_entry);
    if (entry->pshader) list_remove(&entry->pshader_entry);
    HeapFree(GetProcessHeap(), 0, entry->vuniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry->puniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry);
}

static void handle_ps3_input(struct wined3d_shader_buffer *buffer, const struct wined3d_gl_info *gl_info, const DWORD *map,
        const struct wined3d_shader_signature_element *input_signature, const struct shader_reg_maps *reg_maps_in,
        const struct wined3d_shader_signature_element *output_signature, const struct shader_reg_maps *reg_maps_out)
{
    unsigned int i, j;
    const char *semantic_name_in, *semantic_name_out;
    UINT semantic_idx_in, semantic_idx_out;
    DWORD *set;
    DWORD in_idx;
    unsigned int in_count = vec4_varyings(3, gl_info);
    char reg_mask[6], reg_mask_out[6];
    char destination[50];
    WORD input_map, output_map;

    set = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*set) * (in_count + 2));

    if (!output_signature)
    {
        /* Save gl_FrontColor & gl_FrontSecondaryColor before overwriting them. */
        shader_addline(buffer, "vec4 front_color = gl_FrontColor;\n");
        shader_addline(buffer, "vec4 front_secondary_color = gl_FrontSecondaryColor;\n");
    }

    input_map = reg_maps_in->input_registers;
    for (i = 0; input_map; input_map >>= 1, ++i)
    {
        if (!(input_map & 1)) continue;

        in_idx = map[i];
        if (in_idx >= (in_count + 2)) {
            FIXME("More input varyings declared than supported, expect issues\n");
            continue;
        }
        else if (map[i] == ~0U)
        {
            /* Declared, but not read register */
            continue;
        }

        if (in_idx == in_count) {
            sprintf(destination, "gl_FrontColor");
        } else if (in_idx == in_count + 1) {
            sprintf(destination, "gl_FrontSecondaryColor");
        } else {
            sprintf(destination, "IN[%u]", in_idx);
        }

        semantic_name_in = input_signature[i].semantic_name;
        semantic_idx_in = input_signature[i].semantic_idx;
        set[map[i]] = input_signature[i].mask;
        shader_glsl_write_mask_to_str(input_signature[i].mask, reg_mask);

        if (!output_signature)
        {
            if (shader_match_semantic(semantic_name_in, WINED3DDECLUSAGE_COLOR))
            {
                if (semantic_idx_in == 0)
                    shader_addline(buffer, "%s%s = front_color%s;\n",
                            destination, reg_mask, reg_mask);
                else if (semantic_idx_in == 1)
                    shader_addline(buffer, "%s%s = front_secondary_color%s;\n",
                            destination, reg_mask, reg_mask);
                else
                    shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                            destination, reg_mask, reg_mask);
            }
            else if (shader_match_semantic(semantic_name_in, WINED3DDECLUSAGE_TEXCOORD))
            {
                if (semantic_idx_in < 8)
                {
                    shader_addline(buffer, "%s%s = gl_TexCoord[%u]%s;\n",
                            destination, reg_mask, semantic_idx_in, reg_mask);
                }
                else
                {
                    shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                            destination, reg_mask, reg_mask);
                }
            }
            else if (shader_match_semantic(semantic_name_in, WINED3DDECLUSAGE_FOG))
            {
                shader_addline(buffer, "%s%s = vec4(gl_FogFragCoord, 0.0, 0.0, 0.0)%s;\n",
                        destination, reg_mask, reg_mask);
            }
            else
            {
                shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                        destination, reg_mask, reg_mask);
            }
        } else {
            BOOL found = FALSE;

            output_map = reg_maps_out->output_registers;
            for (j = 0; output_map; output_map >>= 1, ++j)
            {
                if (!(output_map & 1)) continue;

                semantic_name_out = output_signature[j].semantic_name;
                semantic_idx_out = output_signature[j].semantic_idx;
                shader_glsl_write_mask_to_str(output_signature[j].mask, reg_mask_out);

                if (semantic_idx_in == semantic_idx_out
                        && !strcmp(semantic_name_in, semantic_name_out))
                {
                    shader_addline(buffer, "%s%s = OUT[%u]%s;\n",
                            destination, reg_mask, j, reg_mask);
                    found = TRUE;
                }
            }
            if(!found) {
                shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                               destination, reg_mask, reg_mask);
            }
        }
    }

    /* This is solely to make the compiler / linker happy and avoid warning about undefined
     * varyings. It shouldn't result in any real code executed on the GPU, since all read
     * input varyings are assigned above, if the optimizer works properly.
     */
    for(i = 0; i < in_count + 2; i++) {
        if (set[i] && set[i] != WINED3DSP_WRITEMASK_ALL)
        {
            unsigned int size = 0;
            memset(reg_mask, 0, sizeof(reg_mask));
            if(!(set[i] & WINED3DSP_WRITEMASK_0)) {
                reg_mask[size] = 'x';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_1)) {
                reg_mask[size] = 'y';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_2)) {
                reg_mask[size] = 'z';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_3)) {
                reg_mask[size] = 'w';
                size++;
            }

            if (i == in_count) {
                sprintf(destination, "gl_FrontColor");
            } else if (i == in_count + 1) {
                sprintf(destination, "gl_FrontSecondaryColor");
            } else {
                sprintf(destination, "IN[%u]", i);
            }

            if (size == 1) {
                shader_addline(buffer, "%s.%s = 0.0;\n", destination, reg_mask);
            } else {
                shader_addline(buffer, "%s.%s = vec%u(0.0);\n", destination, reg_mask, size);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, set);
}

/* GL locking is done by the caller */
static GLhandleARB generate_param_reorder_function(struct wined3d_shader_buffer *buffer,
        IWineD3DVertexShader *vertexshader, IWineD3DPixelShader *pixelshader, const struct wined3d_gl_info *gl_info)
{
    GLhandleARB ret = 0;
    IWineD3DVertexShaderImpl *vs = (IWineD3DVertexShaderImpl *) vertexshader;
    IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) pixelshader;
    IWineD3DDeviceImpl *device;
    DWORD vs_major = vs->baseShader.reg_maps.shader_version.major;
    DWORD ps_major = ps ? ps->baseShader.reg_maps.shader_version.major : 0;
    unsigned int i;
    const char *semantic_name;
    UINT semantic_idx;
    char reg_mask[6];
    const struct wined3d_shader_signature_element *output_signature;

    shader_buffer_clear(buffer);

    shader_addline(buffer, "#version 120\n");

    if(vs_major < 3 && ps_major < 3) {
        /* That one is easy: The vertex shader writes to the builtin varyings, the pixel shader reads from them.
         * Take care about the texcoord .w fixup though if we're using the fixed function fragment pipeline
         */
        device = (IWineD3DDeviceImpl *) vs->baseShader.device;
        if ((gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W)
                && ps_major == 0 && vs_major > 0 && !device->frag_pipe->ffp_proj_control)
        {
            shader_addline(buffer, "void order_ps_input() {\n");
            for(i = 0; i < min(8, MAX_REG_TEXCRD); i++) {
                if(vs->baseShader.reg_maps.texcoord_mask[i] != 0 &&
                   vs->baseShader.reg_maps.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL) {
                    shader_addline(buffer, "gl_TexCoord[%u].w = 1.0;\n", i);
                }
            }
            shader_addline(buffer, "}\n");
        } else {
            shader_addline(buffer, "void order_ps_input() { /* do nothing */ }\n");
        }
    } else if(ps_major < 3 && vs_major >= 3) {
        WORD map = vs->baseShader.reg_maps.output_registers;

        /* The vertex shader writes to its own varyings, the pixel shader needs them in the builtin ones */
        output_signature = vs->baseShader.output_signature;

        shader_addline(buffer, "void order_ps_input(in vec4 OUT[%u]) {\n", MAX_REG_OUTPUT);
        for (i = 0; map; map >>= 1, ++i)
        {
            DWORD write_mask;

            if (!(map & 1)) continue;

            semantic_name = output_signature[i].semantic_name;
            semantic_idx = output_signature[i].semantic_idx;
            write_mask = output_signature[i].mask;
            shader_glsl_write_mask_to_str(write_mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_COLOR))
            {
                if (semantic_idx == 0)
                    shader_addline(buffer, "gl_FrontColor%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
                else if (semantic_idx == 1)
                    shader_addline(buffer, "gl_FrontSecondaryColor%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_POSITION))
            {
                shader_addline(buffer, "gl_Position%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_TEXCOORD))
            {
                if (semantic_idx < 8)
                {
                    if (!(gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W) || ps_major > 0)
                        write_mask |= WINED3DSP_WRITEMASK_3;

                    shader_addline(buffer, "gl_TexCoord[%u]%s = OUT[%u]%s;\n",
                            semantic_idx, reg_mask, i, reg_mask);
                    if (!(write_mask & WINED3DSP_WRITEMASK_3))
                        shader_addline(buffer, "gl_TexCoord[%u].w = 1.0;\n", semantic_idx);
                }
            }
            else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_PSIZE))
            {
                shader_addline(buffer, "gl_PointSize = OUT[%u].x;\n", i);
            }
            else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_FOG))
            {
                shader_addline(buffer, "gl_FogFragCoord = OUT[%u].%c;\n", i, reg_mask[1]);
            }
        }
        shader_addline(buffer, "}\n");

    } else if(ps_major >= 3 && vs_major >= 3) {
        WORD map = vs->baseShader.reg_maps.output_registers;

        output_signature = vs->baseShader.output_signature;

        /* This one is tricky: a 3.0 pixel shader reads from a 3.0 vertex shader */
        shader_addline(buffer, "varying vec4 IN[%u];\n", vec4_varyings(3, gl_info));
        shader_addline(buffer, "void order_ps_input(in vec4 OUT[%u]) {\n", MAX_REG_OUTPUT);

        /* First, sort out position and point size. Those are not passed to the pixel shader */
        for (i = 0; map; map >>= 1, ++i)
        {
            if (!(map & 1)) continue;

            semantic_name = output_signature[i].semantic_name;
            shader_glsl_write_mask_to_str(output_signature[i].mask, reg_mask);

            if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_POSITION))
            {
                shader_addline(buffer, "gl_Position%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
            }
            else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_PSIZE))
            {
                shader_addline(buffer, "gl_PointSize = OUT[%u].x;\n", i);
            }
        }

        /* Then, fix the pixel shader input */
        handle_ps3_input(buffer, gl_info, ps->input_reg_map, ps->baseShader.input_signature,
                &ps->baseShader.reg_maps, output_signature, &vs->baseShader.reg_maps);

        shader_addline(buffer, "}\n");
    } else if(ps_major >= 3 && vs_major < 3) {
        shader_addline(buffer, "varying vec4 IN[%u];\n", vec4_varyings(3, gl_info));
        shader_addline(buffer, "void order_ps_input() {\n");
        /* The vertex shader wrote to the builtin varyings. There is no need to figure out position and
         * point size, but we depend on the optimizers kindness to find out that the pixel shader doesn't
         * read gl_TexCoord and gl_ColorX, otherwise we'll run out of varyings
         */
        handle_ps3_input(buffer, gl_info, ps->input_reg_map, ps->baseShader.input_signature,
                &ps->baseShader.reg_maps, NULL, NULL);
        shader_addline(buffer, "}\n");
    } else {
        ERR("Unexpected vertex and pixel shader version condition: vs: %d, ps: %d\n", vs_major, ps_major);
    }

    ret = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    checkGLcall("glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB)");
    GL_EXTCALL(glShaderSourceARB(ret, 1, (const char**)&buffer->buffer, NULL));
    checkGLcall("glShaderSourceARB(ret, 1, &buffer->buffer, NULL)");
    GL_EXTCALL(glCompileShaderARB(ret));
    checkGLcall("glCompileShaderARB(ret)");

    return ret;
}

/* GL locking is done by the caller */
static void hardcode_local_constants(IWineD3DBaseShaderImpl *shader, const struct wined3d_gl_info *gl_info,
        GLhandleARB programId, char prefix)
{
    const local_constant *lconst;
    GLint tmp_loc;
    const float *value;
    char glsl_name[8];

    LIST_FOR_EACH_ENTRY(lconst, &shader->baseShader.constantsF, local_constant, entry) {
        value = (const float *)lconst->value;
        snprintf(glsl_name, sizeof(glsl_name), "%cLC%u", prefix, lconst->idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
        GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, value));
    }
    checkGLcall("Hardcoding local constants");
}

/* GL locking is done by the caller */
static GLuint shader_glsl_generate_pshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, IWineD3DPixelShaderImpl *This,
        const struct ps_compile_args *args, struct ps_np2fixup_info *np2fixup_info)
{
    const struct shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    CONST DWORD *function = This->baseShader.function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader object and assign it as the shader->prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.cur_np2fixup_info = np2fixup_info;

    shader_addline(buffer, "#version 120\n");

    if (gl_info->supported[ARB_SHADER_TEXTURE_LOD] && reg_maps->usestexldd)
    {
        shader_addline(buffer, "#extension GL_ARB_shader_texture_lod : enable\n");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        /* The spec says that it doesn't have to be explicitly enabled, but the nvidia
         * drivers write a warning if we don't do so
         */
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");
    }
    if (gl_info->supported[EXT_GPU_SHADER4])
    {
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");
    }

    /* Base Declarations */
    shader_generate_glsl_declarations(context, buffer, (IWineD3DBaseShader *)This, reg_maps, &priv_ctx);

    /* Pack 3.0 inputs */
    if (reg_maps->shader_version.major >= 3 && args->vp_mode != vertexshader)
    {
        shader_glsl_input_pack((IWineD3DPixelShader *) This, buffer,
                This->baseShader.input_signature, reg_maps, args->vp_mode);
    }

    /* Base Shader Body */
    shader_generate_main((IWineD3DBaseShader *)This, buffer, reg_maps, function, &priv_ctx);

    /* Pixel shaders < 2.0 place the resulting color in R0 implicitly */
    if (reg_maps->shader_version.major < 2)
    {
        /* Some older cards like GeforceFX ones don't support multiple buffers, so also not gl_FragData */
        shader_addline(buffer, "gl_FragData[0] = R0;\n");
    }

    if (args->srgb_correction)
    {
        shader_addline(buffer, "tmp0.xyz = pow(gl_FragData[0].xyz, vec3(srgb_const0.x));\n");
        shader_addline(buffer, "tmp0.xyz = tmp0.xyz * vec3(srgb_const0.y) - vec3(srgb_const0.z);\n");
        shader_addline(buffer, "tmp1.xyz = gl_FragData[0].xyz * vec3(srgb_const0.w);\n");
        shader_addline(buffer, "bvec3 srgb_compare = lessThan(gl_FragData[0].xyz, vec3(srgb_const1.x));\n");
        shader_addline(buffer, "gl_FragData[0].xyz = mix(tmp0.xyz, tmp1.xyz, vec3(srgb_compare));\n");
        shader_addline(buffer, "gl_FragData[0] = clamp(gl_FragData[0], 0.0, 1.0);\n");
    }
    /* Pixel shader < 3.0 do not replace the fog stage.
     * This implements linear fog computation and blending.
     * TODO: non linear fog
     * NOTE: gl_Fog.start and gl_Fog.end don't hold fog start s and end e but
     * -1/(e-s) and e/(e-s) respectively.
     */
    if (reg_maps->shader_version.major < 3)
    {
        switch(args->fog) {
            case FOG_OFF: break;
            case FOG_LINEAR:
                shader_addline(buffer, "float fogstart = -1.0 / (gl_Fog.end - gl_Fog.start);\n");
                shader_addline(buffer, "float fogend = gl_Fog.end * -fogstart;\n");
                shader_addline(buffer, "float Fog = clamp(gl_FogFragCoord * fogstart + fogend, 0.0, 1.0);\n");
                shader_addline(buffer, "gl_FragData[0].xyz = mix(gl_Fog.color.xyz, gl_FragData[0].xyz, Fog);\n");
                break;
            case FOG_EXP:
                /* Fog = e^(-gl_Fog.density * gl_FogFragCoord) */
                shader_addline(buffer, "float Fog = exp(-gl_Fog.density * gl_FogFragCoord);\n");
                shader_addline(buffer, "Fog = clamp(Fog, 0.0, 1.0);\n");
                shader_addline(buffer, "gl_FragData[0].xyz = mix(gl_Fog.color.xyz, gl_FragData[0].xyz, Fog);\n");
                break;
            case FOG_EXP2:
                /* Fog = e^(-(gl_Fog.density * gl_FogFragCoord)^2) */
                shader_addline(buffer, "float Fog = exp(-gl_Fog.density * gl_Fog.density * gl_FogFragCoord * gl_FogFragCoord);\n");
                shader_addline(buffer, "Fog = clamp(Fog, 0.0, 1.0);\n");
                shader_addline(buffer, "gl_FragData[0].xyz = mix(gl_Fog.color.xyz, gl_FragData[0].xyz, Fog);\n");
                break;
        }
    }

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u\n", shader_obj);
    GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer->buffer, NULL));
    GL_EXTCALL(glCompileShaderARB(shader_obj));
    print_glsl_info_log(gl_info, shader_obj);

    /* Store the shader object */
    return shader_obj;
}

/* GL locking is done by the caller */
static GLuint shader_glsl_generate_vshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, IWineD3DVertexShaderImpl *This,
        const struct vs_compile_args *args)
{
    const struct shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    CONST DWORD *function = This->baseShader.function;
    struct shader_glsl_ctx_priv priv_ctx;

    /* Create the hw GLSL shader program and assign it as the shader->prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));

    shader_addline(buffer, "#version 120\n");

    if (gl_info->supported[EXT_GPU_SHADER4])
    {
        shader_addline(buffer, "#extension GL_EXT_gpu_shader4 : enable\n");
    }

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;

    /* Base Declarations */
    shader_generate_glsl_declarations(context, buffer, (IWineD3DBaseShader *)This, reg_maps, &priv_ctx);

    /* Base Shader Body */
    shader_generate_main((IWineD3DBaseShader*)This, buffer, reg_maps, function, &priv_ctx);

    /* Unpack 3.0 outputs */
    if (reg_maps->shader_version.major >= 3) shader_addline(buffer, "order_ps_input(OUT);\n");
    else shader_addline(buffer, "order_ps_input();\n");

    /* The D3DRS_FOGTABLEMODE render state defines if the shader-generated fog coord is used
     * or if the fragment depth is used. If the fragment depth is used(FOGTABLEMODE != NONE),
     * the fog frag coord is thrown away. If the fog frag coord is used, but not written by
     * the shader, it is set to 0.0(fully fogged, since start = 1.0, end = 0.0)
     */
    if(args->fog_src == VS_FOG_Z) {
        shader_addline(buffer, "gl_FogFragCoord = gl_Position.z;\n");
    } else if (!reg_maps->fog) {
        shader_addline(buffer, "gl_FogFragCoord = 0.0;\n");
    }

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad.
     */
    shader_addline(buffer, "gl_Position.y = gl_Position.y * posFixup.y;\n");
    shader_addline(buffer, "gl_Position.xy += posFixup.zw * gl_Position.ww;\n");
    if(args->clip_enabled) {
        shader_addline(buffer, "gl_ClipVertex = gl_Position;\n");
    }

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     *
     * Basically we want (in homogeneous coordinates) z = z * 2 - 1. However, shaders are run
     * before the homogeneous divide, so we have to take the w into account: z = ((z / w) * 2 - 1) * w,
     * which is the same as z = z * 2 - w.
     */
    shader_addline(buffer, "gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u\n", shader_obj);
    GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer->buffer, NULL));
    GL_EXTCALL(glCompileShaderARB(shader_obj));
    print_glsl_info_log(gl_info, shader_obj);

    return shader_obj;
}

static GLhandleARB find_glsl_pshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, IWineD3DPixelShaderImpl *shader,
        const struct ps_compile_args *args, const struct ps_np2fixup_info **np2fixup_info)
{
    UINT i;
    DWORD new_size;
    struct glsl_ps_compiled_shader *new_array;
    struct glsl_pshader_private    *shader_data;
    struct ps_np2fixup_info        *np2fixup = NULL;
    GLhandleARB ret;

    if (!shader->baseShader.backend_data)
    {
        shader->baseShader.backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
        if (!shader->baseShader.backend_data)
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->baseShader.backend_data;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader_data->num_gl_shaders; i++) {
        if(memcmp(&shader_data->gl_shaders[i].args, args, sizeof(*args)) == 0) {
            if(args->np2_fixup) *np2fixup_info = &shader_data->gl_shaders[i].np2fixup;
            return shader_data->gl_shaders[i].prgId;
        }
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);
    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader_data->gl_shaders,
                                    new_size * sizeof(*shader_data->gl_shaders));
        } else {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*shader_data->gl_shaders));
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

    memset(&shader_data->gl_shaders[shader_data->num_gl_shaders].np2fixup, 0, sizeof(struct ps_np2fixup_info));
    if (args->np2_fixup) np2fixup = &shader_data->gl_shaders[shader_data->num_gl_shaders].np2fixup;

    pixelshader_update_samplers(&shader->baseShader.reg_maps,
            ((IWineD3DDeviceImpl *)shader->baseShader.device)->stateBlock->textures);

    shader_buffer_clear(buffer);
    ret = shader_glsl_generate_pshader(context, buffer, shader, args, np2fixup);
    shader_data->gl_shaders[shader_data->num_gl_shaders++].prgId = ret;
    *np2fixup_info = np2fixup;

    return ret;
}

static inline BOOL vs_args_equal(const struct vs_compile_args *stored, const struct vs_compile_args *new,
                                 const DWORD use_map) {
    if((stored->swizzle_map & use_map) != new->swizzle_map) return FALSE;
    if((stored->clip_enabled) != new->clip_enabled) return FALSE;
    return stored->fog_src == new->fog_src;
}

static GLhandleARB find_glsl_vshader(const struct wined3d_context *context,
        struct wined3d_shader_buffer *buffer, IWineD3DVertexShaderImpl *shader,
        const struct vs_compile_args *args)
{
    UINT i;
    DWORD new_size;
    struct glsl_vs_compiled_shader *new_array;
    DWORD use_map = ((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.use_map;
    struct glsl_vshader_private *shader_data;
    GLhandleARB ret;

    if (!shader->baseShader.backend_data)
    {
        shader->baseShader.backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
        if (!shader->baseShader.backend_data)
        {
            ERR("Failed to allocate backend data.\n");
            return 0;
        }
    }
    shader_data = shader->baseShader.backend_data;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader_data->num_gl_shaders; i++) {
        if(vs_args_equal(&shader_data->gl_shaders[i].args, args, use_map)) {
            return shader_data->gl_shaders[i].prgId;
        }
    }

    TRACE("No matching GL shader found for shader %p, compiling a new shader.\n", shader);

    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader_data->gl_shaders,
                                    new_size * sizeof(*shader_data->gl_shaders));
        } else {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*shader_data->gl_shaders));
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

    shader_buffer_clear(buffer);
    ret = shader_glsl_generate_vshader(context, buffer, shader, args);
    shader_data->gl_shaders[shader_data->num_gl_shaders++].prgId = ret;

    return ret;
}

/** Sets the GLSL program ID for the given pixel and vertex shader combination.
 * It sets the programId on the current StateBlock (because it should be called
 * inside of the DrawPrimitive() part of the render loop).
 *
 * If a program for the given combination does not exist, create one, and store
 * the program in the hash table.  If it creates a program, it will link the
 * given objects, too.
 */

/* GL locking is done by the caller */
static void set_glsl_shader_program(const struct wined3d_context *context,
        IWineD3DDeviceImpl *device, BOOL use_ps, BOOL use_vs)
{
    IWineD3DVertexShader *vshader = use_vs ? device->stateBlock->vertexShader : NULL;
    IWineD3DPixelShader *pshader = use_ps ? device->stateBlock->pixelShader : NULL;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct shader_glsl_priv *priv = device->shader_priv;
    struct glsl_shader_prog_link *entry    = NULL;
    GLhandleARB programId                  = 0;
    GLhandleARB reorder_shader_id          = 0;
    unsigned int i;
    char glsl_name[8];
    struct ps_compile_args ps_compile_args;
    struct vs_compile_args vs_compile_args;

    if (vshader) find_vs_compile_args((IWineD3DVertexShaderImpl *)vshader, device->stateBlock, &vs_compile_args);
    if (pshader) find_ps_compile_args((IWineD3DPixelShaderImpl *)pshader, device->stateBlock, &ps_compile_args);

    entry = get_glsl_program_entry(priv, vshader, pshader, &vs_compile_args, &ps_compile_args);
    if (entry) {
        priv->glsl_program = entry;
        return;
    }

    /* If we get to this point, then no matching program exists, so we create one */
    programId = GL_EXTCALL(glCreateProgramObjectARB());
    TRACE("Created new GLSL shader program %u\n", programId);

    /* Create the entry */
    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(struct glsl_shader_prog_link));
    entry->programId = programId;
    entry->vshader = vshader;
    entry->pshader = pshader;
    entry->vs_args = vs_compile_args;
    entry->ps_args = ps_compile_args;
    entry->constant_version = 0;
    entry->np2Fixup_info = NULL;
    /* Add the hash table entry */
    add_glsl_program_entry(priv, entry);

    /* Set the current program */
    priv->glsl_program = entry;

    /* Attach GLSL vshader */
    if (vshader)
    {
        GLhandleARB vshader_id = find_glsl_vshader(context, &priv->shader_buffer,
                (IWineD3DVertexShaderImpl *)vshader, &vs_compile_args);
        WORD map = ((IWineD3DBaseShaderImpl *)vshader)->baseShader.reg_maps.input_registers;
        char tmp_name[10];

        reorder_shader_id = generate_param_reorder_function(&priv->shader_buffer, vshader, pshader, gl_info);
        TRACE("Attaching GLSL shader object %u to program %u\n", reorder_shader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, reorder_shader_id));
        checkGLcall("glAttachObjectARB");
        /* Flag the reorder function for deletion, then it will be freed automatically when the program
         * is destroyed
         */
        GL_EXTCALL(glDeleteObjectARB(reorder_shader_id));

        TRACE("Attaching GLSL shader object %u to program %u\n", vshader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, vshader_id));
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

            snprintf(tmp_name, sizeof(tmp_name), "attrib%u", i);
            GL_EXTCALL(glBindAttribLocationARB(programId, i, tmp_name));
        }
        checkGLcall("glBindAttribLocationARB");

        list_add_head(&((IWineD3DBaseShaderImpl *)vshader)->baseShader.linked_programs, &entry->vshader_entry);
    }

    /* Attach GLSL pshader */
    if (pshader)
    {
        GLhandleARB pshader_id = find_glsl_pshader(context, &priv->shader_buffer,
                (IWineD3DPixelShaderImpl *)pshader, &ps_compile_args, &entry->np2Fixup_info);
        TRACE("Attaching GLSL shader object %u to program %u\n", pshader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, pshader_id));
        checkGLcall("glAttachObjectARB");

        list_add_head(&((IWineD3DBaseShaderImpl *)pshader)->baseShader.linked_programs, &entry->pshader_entry);
    }

    /* Link the program */
    TRACE("Linking GLSL shader program %u\n", programId);
    GL_EXTCALL(glLinkProgramARB(programId));
    shader_glsl_validate_link(gl_info, programId);

    entry->vuniformF_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLhandleARB) * gl_info->limits.glsl_vs_float_constants);
    for (i = 0; i < gl_info->limits.glsl_vs_float_constants; ++i)
    {
        snprintf(glsl_name, sizeof(glsl_name), "VC[%i]", i);
        entry->vuniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    for (i = 0; i < MAX_CONST_I; ++i)
    {
        snprintf(glsl_name, sizeof(glsl_name), "VI[%i]", i);
        entry->vuniformI_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    entry->puniformF_locations = HeapAlloc(GetProcessHeap(), 0,
            sizeof(GLhandleARB) * gl_info->limits.glsl_ps_float_constants);
    for (i = 0; i < gl_info->limits.glsl_ps_float_constants; ++i)
    {
        snprintf(glsl_name, sizeof(glsl_name), "PC[%i]", i);
        entry->puniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    for (i = 0; i < MAX_CONST_I; ++i)
    {
        snprintf(glsl_name, sizeof(glsl_name), "PI[%i]", i);
        entry->puniformI_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }

    if(pshader) {
        char name[32];

        for(i = 0; i < MAX_TEXTURES; i++) {
            sprintf(name, "bumpenvmat%u", i);
            entry->bumpenvmat_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
            sprintf(name, "luminancescale%u", i);
            entry->luminancescale_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
            sprintf(name, "luminanceoffset%u", i);
            entry->luminanceoffset_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
        }

        if (ps_compile_args.np2_fixup) {
            if (entry->np2Fixup_info) {
                entry->np2Fixup_location = GL_EXTCALL(glGetUniformLocationARB(programId, "PsamplerNP2Fixup"));
            } else {
                FIXME("NP2 texcoord fixup needed for this pixelshader, but no fixup uniform found.\n");
            }
        }
    }

    entry->posFixup_location = GL_EXTCALL(glGetUniformLocationARB(programId, "posFixup"));
    entry->ycorrection_location = GL_EXTCALL(glGetUniformLocationARB(programId, "ycorrection"));
    checkGLcall("Find glsl program uniform locations");

    if (pshader
            && ((IWineD3DPixelShaderImpl *)pshader)->baseShader.reg_maps.shader_version.major >= 3
            && ((IWineD3DPixelShaderImpl *)pshader)->declared_in_count > vec4_varyings(3, gl_info))
    {
        TRACE("Shader %d needs vertex color clamping disabled\n", programId);
        entry->vertex_color_clamp = GL_FALSE;
    } else {
        entry->vertex_color_clamp = GL_FIXED_ONLY_ARB;
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
    if (vshader) shader_glsl_load_vsamplers(gl_info, device->texUnitMap, programId);
    if (pshader) shader_glsl_load_psamplers(gl_info, device->texUnitMap, programId);

    /* If the local constants do not have to be loaded with the environment constants,
     * load them now to have them hardcoded in the GLSL program. This saves some CPU cycles
     * later
     */
    if (pshader && !((IWineD3DBaseShaderImpl *)pshader)->baseShader.load_local_constsF)
    {
        hardcode_local_constants((IWineD3DBaseShaderImpl *) pshader, gl_info, programId, 'P');
    }
    if (vshader && !((IWineD3DBaseShaderImpl *)vshader)->baseShader.load_local_constsF)
    {
        hardcode_local_constants((IWineD3DBaseShaderImpl *) vshader, gl_info, programId, 'V');
    }
}

/* GL locking is done by the caller */
static GLhandleARB create_glsl_blt_shader(const struct wined3d_gl_info *gl_info, enum tex_types tex_type, BOOL masked)
{
    GLhandleARB program_id;
    GLhandleARB vshader_id, pshader_id;
    const char *blt_pshader;

    static const char *blt_vshader[] =
    {
        "#version 120\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "}\n"
    };

    static const char *blt_pshaders_full[tex_type_count] =
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

    static const char *blt_pshaders_masked[tex_type_count] =
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
        tex_type = tex_2d;
    }

    vshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(vshader_id, 1, blt_vshader, NULL));
    GL_EXTCALL(glCompileShaderARB(vshader_id));

    pshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(pshader_id, 1, &blt_pshader, NULL));
    GL_EXTCALL(glCompileShaderARB(pshader_id));

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

/* GL locking is done by the caller */
static void shader_glsl_select(const struct wined3d_context *context, BOOL usePS, BOOL useVS)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    IWineD3DDeviceImpl *device = context->swapchain->device;
    struct shader_glsl_priv *priv = device->shader_priv;
    GLhandleARB program_id = 0;
    GLenum old_vertex_color_clamp, current_vertex_color_clamp;

    old_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vertex_color_clamp : GL_FIXED_ONLY_ARB;

    if (useVS || usePS) set_glsl_shader_program(context, device, usePS, useVS);
    else priv->glsl_program = NULL;

    current_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vertex_color_clamp : GL_FIXED_ONLY_ARB;

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
    if (priv->glsl_program && priv->glsl_program->np2Fixup_info)
    {
        shader_glsl_load_np2fixup_constants((IWineD3DDevice *)device, usePS, useVS);
    }
}

/* GL locking is done by the caller */
static void shader_glsl_select_depth_blt(IWineD3DDevice *iface,
        enum tex_types tex_type, const SIZE *ds_mask_size)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    BOOL masked = ds_mask_size->cx && ds_mask_size->cy;
    struct shader_glsl_priv *priv = This->shader_priv;
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

/* GL locking is done by the caller */
static void shader_glsl_deselect_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = This->shader_priv;
    GLhandleARB program_id;

    program_id = priv->glsl_program ? priv->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);

    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
}

static void shader_glsl_destroy(IWineD3DBaseShader *iface) {
    const struct list *linked_programs;
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)This->baseShader.device;
    struct shader_glsl_priv *priv = device->shader_priv;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    /* Note: Do not use QueryInterface here to find out which shader type this is because this code
     * can be called from IWineD3DBaseShader::Release
     */
    char pshader = shader_is_pshader_version(This->baseShader.reg_maps.shader_version.type);

    if(pshader) {
        struct glsl_pshader_private *shader_data;
        shader_data = This->baseShader.backend_data;
        if(!shader_data || shader_data->num_gl_shaders == 0)
        {
            HeapFree(GetProcessHeap(), 0, shader_data);
            This->baseShader.backend_data = NULL;
            return;
        }

        context = context_acquire(device, NULL);
        gl_info = context->gl_info;

        if (priv->glsl_program && (IWineD3DBaseShader *)priv->glsl_program->pshader == iface)
        {
            ENTER_GL();
            shader_glsl_select(context, FALSE, FALSE);
            LEAVE_GL();
        }
    } else {
        struct glsl_vshader_private *shader_data;
        shader_data = This->baseShader.backend_data;
        if(!shader_data || shader_data->num_gl_shaders == 0)
        {
            HeapFree(GetProcessHeap(), 0, shader_data);
            This->baseShader.backend_data = NULL;
            return;
        }

        context = context_acquire(device, NULL);
        gl_info = context->gl_info;

        if (priv->glsl_program && (IWineD3DBaseShader *)priv->glsl_program->vshader == iface)
        {
            ENTER_GL();
            shader_glsl_select(context, FALSE, FALSE);
            LEAVE_GL();
        }
    }

    linked_programs = &This->baseShader.linked_programs;

    TRACE("Deleting linked programs\n");
    if (linked_programs->next) {
        struct glsl_shader_prog_link *entry, *entry2;

        ENTER_GL();
        if(pshader) {
            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, pshader_entry) {
                delete_glsl_program_entry(priv, gl_info, entry);
            }
        } else {
            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, vshader_entry) {
                delete_glsl_program_entry(priv, gl_info, entry);
            }
        }
        LEAVE_GL();
    }

    if(pshader) {
        UINT i;
        struct glsl_pshader_private *shader_data = This->baseShader.backend_data;

        ENTER_GL();
        for(i = 0; i < shader_data->num_gl_shaders; i++) {
            TRACE("deleting pshader %u\n", shader_data->gl_shaders[i].prgId);
            GL_EXTCALL(glDeleteObjectARB(shader_data->gl_shaders[i].prgId));
            checkGLcall("glDeleteObjectARB");
        }
        LEAVE_GL();
        HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders);
    }
    else
    {
        UINT i;
        struct glsl_vshader_private *shader_data = This->baseShader.backend_data;

        ENTER_GL();
        for(i = 0; i < shader_data->num_gl_shaders; i++) {
            TRACE("deleting vshader %u\n", shader_data->gl_shaders[i].prgId);
            GL_EXTCALL(glDeleteObjectARB(shader_data->gl_shaders[i].prgId));
            checkGLcall("glDeleteObjectARB");
        }
        LEAVE_GL();
        HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders);
    }

    HeapFree(GetProcessHeap(), 0, This->baseShader.backend_data);
    This->baseShader.backend_data = NULL;

    context_release(context);
}

static int glsl_program_key_compare(const void *key, const struct wine_rb_entry *entry)
{
    const glsl_program_key_t *k = key;
    const struct glsl_shader_prog_link *prog = WINE_RB_ENTRY_VALUE(entry,
            const struct glsl_shader_prog_link, program_lookup_entry);
    int cmp;

    if (k->vshader > prog->vshader) return 1;
    else if (k->vshader < prog->vshader) return -1;

    if (k->pshader > prog->pshader) return 1;
    else if (k->pshader < prog->pshader) return -1;

    if (k->vshader && (cmp = memcmp(&k->vs_args, &prog->vs_args, sizeof(prog->vs_args)))) return cmp;
    if (k->pshader && (cmp = memcmp(&k->ps_args, &prog->ps_args, sizeof(prog->ps_args)))) return cmp;

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

static HRESULT shader_glsl_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_glsl_priv));
    SIZE_T stack_size = wined3d_log2i(max(gl_info->limits.glsl_vs_float_constants,
            gl_info->limits.glsl_ps_float_constants)) + 1;

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

    This->shader_priv = priv;
    return WINED3D_OK;

fail:
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    shader_buffer_free(&priv->shader_buffer);
    HeapFree(GetProcessHeap(), 0, priv);
    return E_OUTOFMEMORY;
}

/* Context activation is done by the caller. */
static void shader_glsl_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = This->shader_priv;
    int i;

    ENTER_GL();
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
    LEAVE_GL();

    wine_rb_destroy(&priv->program_lookup, NULL, NULL);
    constant_heap_free(&priv->pconst_heap);
    constant_heap_free(&priv->vconst_heap);
    HeapFree(GetProcessHeap(), 0, priv->stack);
    shader_buffer_free(&priv->shader_buffer);

    HeapFree(GetProcessHeap(), 0, This->shader_priv);
    This->shader_priv = NULL;
}

static BOOL shader_glsl_dirty_const(IWineD3DDevice *iface) {
    /* TODO: GL_EXT_bindable_uniform can be used to share constants across shaders */
    return FALSE;
}

static void shader_glsl_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *pCaps)
{
    /* Nvidia Geforce6/7 or Ati R4xx/R5xx cards with GLSL support, support VS 3.0 but older Nvidia/Ati
     * models with GLSL support only support 2.0. In case of nvidia we can detect VS 2.0 support based
     * on the version of NV_vertex_program.
     * For Ati cards there's no way using glsl (it abstracts the lowlevel info away) and also not
     * using ARB_vertex_program. It is safe to assume that when a card supports pixel shader 2.0 it
     * supports vertex shader 2.0 too and the way around. We can detect ps2.0 using the maximum number
     * of native instructions, so use that here. For more info see the pixel shader versioning code below.
     */
    if ((gl_info->supported[NV_VERTEX_PROGRAM2] && !gl_info->supported[NV_VERTEX_PROGRAM3])
            || gl_info->limits.arb_ps_instructions <= 512)
        pCaps->VertexShaderVersion = WINED3DVS_VERSION(2,0);
    else
        pCaps->VertexShaderVersion = WINED3DVS_VERSION(3,0);
    TRACE_(d3d_caps)("Hardware vertex shader version %d.%d enabled (GLSL)\n", (pCaps->VertexShaderVersion >> 8) & 0xff, pCaps->VertexShaderVersion & 0xff);
    pCaps->MaxVertexShaderConst = gl_info->limits.glsl_vs_float_constants;

    /* Older DX9-class videocards (GeforceFX / Radeon >9500/X*00) only support pixel shader 2.0/2.0a/2.0b.
     * In OpenGL the extensions related to GLSL abstract lowlevel GL info away which is needed
     * to distinguish between 2.0 and 3.0 (and 2.0a/2.0b). In case of Nvidia we use their fragment
     * program extensions. On other hardware including ATI GL_ARB_fragment_program offers the info
     * in max native instructions. Intel and others also offer the info in this extension but they
     * don't support GLSL (at least on Windows).
     *
     * PS2.0 requires at least 96 instructions, 2.0a/2.0b go up to 512. Assume that if the number
     * of instructions is 512 or less we have to do with ps2.0 hardware.
     * NOTE: ps3.0 hardware requires 512 or more instructions but ati and nvidia offer 'enough' (1024 vs 4096) on their most basic ps3.0 hardware.
     */
    if ((gl_info->supported[NV_FRAGMENT_PROGRAM] && !gl_info->supported[NV_FRAGMENT_PROGRAM2])
            || gl_info->limits.arb_ps_instructions <= 512)
        pCaps->PixelShaderVersion = WINED3DPS_VERSION(2,0);
    else
        pCaps->PixelShaderVersion = WINED3DPS_VERSION(3,0);

    pCaps->MaxPixelShaderConst = gl_info->limits.glsl_ps_float_constants;

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
    pCaps->PixelShader1xMaxValue = 8.0;
    TRACE_(d3d_caps)("Hardware pixel shader version %d.%d enabled (GLSL)\n", (pCaps->PixelShaderVersion >> 8) & 0xff, pCaps->PixelShaderVersion & 0xff);

    pCaps->VSClipping = TRUE;
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
    /* WINED3DSIH_ABS           */ shader_glsl_map2gl,
    /* WINED3DSIH_ADD           */ shader_glsl_arith,
    /* WINED3DSIH_BEM           */ shader_glsl_bem,
    /* WINED3DSIH_BREAK         */ shader_glsl_break,
    /* WINED3DSIH_BREAKC        */ shader_glsl_breakc,
    /* WINED3DSIH_BREAKP        */ NULL,
    /* WINED3DSIH_CALL          */ shader_glsl_call,
    /* WINED3DSIH_CALLNZ        */ shader_glsl_callnz,
    /* WINED3DSIH_CMP           */ shader_glsl_cmp,
    /* WINED3DSIH_CND           */ shader_glsl_cnd,
    /* WINED3DSIH_CRS           */ shader_glsl_cross,
    /* WINED3DSIH_CUT           */ NULL,
    /* WINED3DSIH_DCL           */ NULL,
    /* WINED3DSIH_DEF           */ NULL,
    /* WINED3DSIH_DEFB          */ NULL,
    /* WINED3DSIH_DEFI          */ NULL,
    /* WINED3DSIH_DP2ADD        */ shader_glsl_dp2add,
    /* WINED3DSIH_DP3           */ shader_glsl_dot,
    /* WINED3DSIH_DP4           */ shader_glsl_dot,
    /* WINED3DSIH_DST           */ shader_glsl_dst,
    /* WINED3DSIH_DSX           */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY           */ shader_glsl_map2gl,
    /* WINED3DSIH_ELSE          */ shader_glsl_else,
    /* WINED3DSIH_EMIT          */ NULL,
    /* WINED3DSIH_ENDIF         */ shader_glsl_end,
    /* WINED3DSIH_ENDLOOP       */ shader_glsl_end,
    /* WINED3DSIH_ENDREP        */ shader_glsl_end,
    /* WINED3DSIH_EXP           */ shader_glsl_map2gl,
    /* WINED3DSIH_EXPP          */ shader_glsl_expp,
    /* WINED3DSIH_FRC           */ shader_glsl_map2gl,
    /* WINED3DSIH_IADD          */ NULL,
    /* WINED3DSIH_IF            */ shader_glsl_if,
    /* WINED3DSIH_IFC           */ shader_glsl_ifc,
    /* WINED3DSIH_IGE           */ NULL,
    /* WINED3DSIH_LABEL         */ shader_glsl_label,
    /* WINED3DSIH_LIT           */ shader_glsl_lit,
    /* WINED3DSIH_LOG           */ shader_glsl_log,
    /* WINED3DSIH_LOGP          */ shader_glsl_log,
    /* WINED3DSIH_LOOP          */ shader_glsl_loop,
    /* WINED3DSIH_LRP           */ shader_glsl_lrp,
    /* WINED3DSIH_LT            */ NULL,
    /* WINED3DSIH_M3x2          */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x3          */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x4          */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x3          */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x4          */ shader_glsl_mnxn,
    /* WINED3DSIH_MAD           */ shader_glsl_mad,
    /* WINED3DSIH_MAX           */ shader_glsl_map2gl,
    /* WINED3DSIH_MIN           */ shader_glsl_map2gl,
    /* WINED3DSIH_MOV           */ shader_glsl_mov,
    /* WINED3DSIH_MOVA          */ shader_glsl_mov,
    /* WINED3DSIH_MUL           */ shader_glsl_arith,
    /* WINED3DSIH_NOP           */ NULL,
    /* WINED3DSIH_NRM           */ shader_glsl_nrm,
    /* WINED3DSIH_PHASE         */ NULL,
    /* WINED3DSIH_POW           */ shader_glsl_pow,
    /* WINED3DSIH_RCP           */ shader_glsl_rcp,
    /* WINED3DSIH_REP           */ shader_glsl_rep,
    /* WINED3DSIH_RET           */ shader_glsl_ret,
    /* WINED3DSIH_RSQ           */ shader_glsl_rsq,
    /* WINED3DSIH_SETP          */ NULL,
    /* WINED3DSIH_SGE           */ shader_glsl_compare,
    /* WINED3DSIH_SGN           */ shader_glsl_sgn,
    /* WINED3DSIH_SINCOS        */ shader_glsl_sincos,
    /* WINED3DSIH_SLT           */ shader_glsl_compare,
    /* WINED3DSIH_SUB           */ shader_glsl_arith,
    /* WINED3DSIH_TEX           */ shader_glsl_tex,
    /* WINED3DSIH_TEXBEM        */ shader_glsl_texbem,
    /* WINED3DSIH_TEXBEML       */ shader_glsl_texbem,
    /* WINED3DSIH_TEXCOORD      */ shader_glsl_texcoord,
    /* WINED3DSIH_TEXDEPTH      */ shader_glsl_texdepth,
    /* WINED3DSIH_TEXDP3        */ shader_glsl_texdp3,
    /* WINED3DSIH_TEXDP3TEX     */ shader_glsl_texdp3tex,
    /* WINED3DSIH_TEXKILL       */ shader_glsl_texkill,
    /* WINED3DSIH_TEXLDD        */ shader_glsl_texldd,
    /* WINED3DSIH_TEXLDL        */ shader_glsl_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH  */ shader_glsl_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD    */ shader_glsl_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX    */ shader_glsl_texm3x2tex,
    /* WINED3DSIH_TEXM3x3       */ shader_glsl_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF   */ NULL,
    /* WINED3DSIH_TEXM3x3PAD    */ shader_glsl_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC   */ shader_glsl_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX    */ shader_glsl_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC  */ shader_glsl_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR     */ shader_glsl_texreg2ar,
    /* WINED3DSIH_TEXREG2GB     */ shader_glsl_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB    */ shader_glsl_texreg2rgb,
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

const shader_backend_t glsl_shader_backend = {
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
    shader_glsl_dirty_const,
    shader_glsl_get_caps,
    shader_glsl_color_fixup_supported,
};
