/*
 * Direct3D wine OpenGL include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2004 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2007 Roderick Colenbrander
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

#ifndef __WINE_WINED3D_GL_H
#define __WINE_WINED3D_GL_H

#define WINE_GLAPI __stdcall

#include <stdbool.h>
#include <stdint.h>

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

struct wined3d_swapchain_gl;
struct wined3d_texture_gl;

#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI 0x8837  /* not in the gl spec */

/* OpenGL extensions. */
enum wined3d_gl_extension
{
    WINED3D_GL_EXT_NONE,

    /* APPLE */
    APPLE_FENCE,
    APPLE_FLOAT_PIXELS,
    APPLE_FLUSH_BUFFER_RANGE,
    APPLE_FLUSH_RENDER,
    APPLE_RGB_422,
    APPLE_YCBCR_422,
    /* ARB */
    ARB_BASE_INSTANCE,
    ARB_BLEND_FUNC_EXTENDED,
    ARB_BUFFER_STORAGE,
    ARB_CLEAR_BUFFER_OBJECT,
    ARB_CLEAR_TEXTURE,
    ARB_CLIP_CONTROL,
    ARB_COLOR_BUFFER_FLOAT,
    ARB_COMPUTE_SHADER,
    ARB_CONSERVATIVE_DEPTH,
    ARB_COPY_BUFFER,
    ARB_COPY_IMAGE,
    ARB_CULL_DISTANCE,
    ARB_DEBUG_OUTPUT,
    ARB_DEPTH_BUFFER_FLOAT,
    ARB_DEPTH_CLAMP,
    ARB_DEPTH_TEXTURE,
    ARB_DERIVATIVE_CONTROL,
    ARB_DRAW_BUFFERS,
    ARB_DRAW_BUFFERS_BLEND,
    ARB_DRAW_ELEMENTS_BASE_VERTEX,
    ARB_DRAW_INDIRECT,
    ARB_DRAW_INSTANCED,
    ARB_ES2_COMPATIBILITY,
    ARB_ES3_COMPATIBILITY,
    ARB_EXPLICIT_ATTRIB_LOCATION,
    ARB_FRAGMENT_COORD_CONVENTIONS,
    ARB_FRAGMENT_LAYER_VIEWPORT,
    ARB_FRAGMENT_PROGRAM,
    ARB_FRAGMENT_PROGRAM_SHADOW,
    ARB_FRAGMENT_SHADER,
    ARB_FRAMEBUFFER_NO_ATTACHMENTS,
    ARB_FRAMEBUFFER_OBJECT,
    ARB_FRAMEBUFFER_SRGB,
    ARB_GEOMETRY_SHADER4,
    ARB_GPU_SHADER5,
    ARB_HALF_FLOAT_PIXEL,
    ARB_HALF_FLOAT_VERTEX,
    ARB_INSTANCED_ARRAYS,
    ARB_INTERNALFORMAT_QUERY,
    ARB_INTERNALFORMAT_QUERY2,
    ARB_MAP_BUFFER_ALIGNMENT,
    ARB_MAP_BUFFER_RANGE,
    ARB_MULTISAMPLE,
    ARB_MULTITEXTURE,
    ARB_OCCLUSION_QUERY,
    ARB_PIPELINE_STATISTICS_QUERY,
    ARB_PIXEL_BUFFER_OBJECT,
    ARB_POINT_PARAMETERS,
    ARB_POINT_SPRITE,
    ARB_POLYGON_OFFSET_CLAMP,
    ARB_PROVOKING_VERTEX,
    ARB_QUERY_BUFFER_OBJECT,
    ARB_SAMPLE_SHADING,
    ARB_SAMPLER_OBJECTS,
    ARB_SEAMLESS_CUBE_MAP,
    ARB_SHADER_ATOMIC_COUNTERS,
    ARB_SHADER_VIEWPORT_LAYER_ARRAY,
    ARB_SHADER_BIT_ENCODING,
    ARB_SHADER_IMAGE_LOAD_STORE,
    ARB_SHADER_IMAGE_SIZE,
    ARB_SHADER_STENCIL_EXPORT,
    ARB_SHADER_STORAGE_BUFFER_OBJECT,
    ARB_SHADER_TEXTURE_IMAGE_SAMPLES,
    ARB_SHADER_TEXTURE_LOD,
    ARB_SHADING_LANGUAGE_100,
    ARB_SHADING_LANGUAGE_420PACK,
    ARB_SHADING_LANGUAGE_PACKING,
    ARB_SHADOW,
    ARB_STENCIL_TEXTURING,
    ARB_SYNC,
    ARB_TESSELLATION_SHADER,
    ARB_TEXTURE_BORDER_CLAMP,
    ARB_TEXTURE_BUFFER_OBJECT,
    ARB_TEXTURE_BUFFER_RANGE,
    ARB_TEXTURE_COMPRESSION,
    ARB_TEXTURE_COMPRESSION_BPTC,
    ARB_TEXTURE_COMPRESSION_RGTC,
    ARB_TEXTURE_CUBE_MAP,
    ARB_TEXTURE_CUBE_MAP_ARRAY,
    ARB_TEXTURE_ENV_COMBINE,
    ARB_TEXTURE_ENV_DOT3,
    ARB_TEXTURE_FILTER_ANISOTROPIC,
    ARB_TEXTURE_FLOAT,
    ARB_TEXTURE_GATHER,
    ARB_TEXTURE_MIRRORED_REPEAT,
    ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE,
    ARB_TEXTURE_MULTISAMPLE,
    ARB_TEXTURE_NON_POWER_OF_TWO,
    ARB_TEXTURE_QUERY_LEVELS,
    ARB_TEXTURE_RG,
    ARB_TEXTURE_RGB10_A2UI,
    ARB_TEXTURE_STORAGE,
    ARB_TEXTURE_STORAGE_MULTISAMPLE,
    ARB_TEXTURE_SWIZZLE,
    ARB_TEXTURE_VIEW,
    ARB_TIMER_QUERY,
    ARB_TRANSFORM_FEEDBACK2,
    ARB_TRANSFORM_FEEDBACK3,
    ARB_UNIFORM_BUFFER_OBJECT,
    ARB_VERTEX_ARRAY_BGRA,
    ARB_VERTEX_BUFFER_OBJECT,
    ARB_VERTEX_PROGRAM,
    ARB_VERTEX_SHADER,
    ARB_VERTEX_TYPE_10F_11F_11F_REV,
    ARB_VERTEX_TYPE_2_10_10_10_REV,
    ARB_VIEWPORT_ARRAY,
    ARB_TEXTURE_BARRIER,
    /* ATI */
    ATI_FRAGMENT_SHADER,
    ATI_SEPARATE_STENCIL,
    ATI_TEXTURE_COMPRESSION_3DC,
    ATI_TEXTURE_ENV_COMBINE3,
    ATI_TEXTURE_MIRROR_ONCE,
    /* EXT */
    EXT_BLEND_COLOR,
    EXT_BLEND_EQUATION_SEPARATE,
    EXT_BLEND_FUNC_SEPARATE,
    EXT_BLEND_MINMAX,
    EXT_BLEND_SUBTRACT,
    EXT_DEPTH_BOUNDS_TEST,
    EXT_DRAW_BUFFERS2,
    EXT_FOG_COORD,
    EXT_FRAMEBUFFER_BLIT,
    EXT_FRAMEBUFFER_MULTISAMPLE,
    EXT_FRAMEBUFFER_MULTISAMPLE_BLIT_SCALED,
    EXT_FRAMEBUFFER_OBJECT,
    EXT_GPU_PROGRAM_PARAMETERS,
    EXT_GPU_SHADER4,
    EXT_MEMORY_OBJECT,
    EXT_PACKED_DEPTH_STENCIL,
    EXT_PACKED_FLOAT,
    EXT_POINT_PARAMETERS,
    EXT_PROVOKING_VERTEX,
    EXT_SECONDARY_COLOR,
    EXT_SHADER_INTEGER_MIX,
    EXT_STENCIL_TWO_SIDE,
    EXT_STENCIL_WRAP,
    EXT_TEXTURE3D,
    EXT_TEXTURE_ARRAY,
    EXT_TEXTURE_COMPRESSION_RGTC,
    EXT_TEXTURE_COMPRESSION_S3TC,
    EXT_TEXTURE_ENV_COMBINE,
    EXT_TEXTURE_ENV_DOT3,
    EXT_TEXTURE_INTEGER,
    EXT_TEXTURE_LOD_BIAS,
    EXT_TEXTURE_MIRROR_CLAMP,
    EXT_TEXTURE_SHADOW_LOD,
    EXT_TEXTURE_SHARED_EXPONENT,
    EXT_TEXTURE_SNORM,
    EXT_TEXTURE_SRGB,
    EXT_TEXTURE_SRGB_DECODE,
    /* NVIDIA */
    NV_FENCE,
    NV_FOG_DISTANCE,
    NV_FRAGMENT_PROGRAM,
    NV_FRAGMENT_PROGRAM2,
    NV_FRAGMENT_PROGRAM_OPTION,
    NV_HALF_FLOAT,
    NV_LIGHT_MAX_EXPONENT,
    NV_POINT_SPRITE,
    NV_REGISTER_COMBINERS,
    NV_REGISTER_COMBINERS2,
    NV_TEXGEN_REFLECTION,
    NV_TEXTURE_ENV_COMBINE4,
    NV_TEXTURE_SHADER,
    NV_TEXTURE_SHADER2,
    NV_VERTEX_PROGRAM,
    NV_VERTEX_PROGRAM1_1,
    NV_VERTEX_PROGRAM2,
    NV_VERTEX_PROGRAM2_OPTION,
    NV_VERTEX_PROGRAM3,
    NV_TEXTURE_BARRIER,
    /* WGL extensions */
    WGL_ARB_PIXEL_FORMAT,
    WGL_EXT_SWAP_CONTROL,
    WGL_WINE_PIXEL_FORMAT_PASSTHROUGH,
    WGL_WINE_QUERY_RENDERER,
    /* Internally used */
    WINED3D_GL_BLEND_EQUATION,
    WINED3D_GL_LEGACY_CONTEXT,
    WINED3D_GL_NORMALIZED_TEXRECT,
    WINED3D_GL_PRIMITIVE_QUERY,
    WINED3D_GL_VERSION_2_0,
    WINED3D_GL_VERSION_3_2,
    WINED3D_GLSL_130,

    WINED3D_GL_EXT_COUNT,
};

struct wined3d_fbo_ops
{
    GLboolean (WINE_GLAPI *glIsRenderbuffer)(GLuint renderbuffer);
    void (WINE_GLAPI *glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (WINE_GLAPI *glDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
    void (WINE_GLAPI *glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
    void (WINE_GLAPI *glRenderbufferStorage)(GLenum target, GLenum internalformat,
            GLsizei width, GLsizei height);
    void (WINE_GLAPI *glRenderbufferStorageMultisample)(GLenum target, GLsizei samples,
            GLenum internalformat, GLsizei width, GLsizei height);
    void (WINE_GLAPI *glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
    GLboolean (WINE_GLAPI *glIsFramebuffer)(GLuint framebuffer);
    void (WINE_GLAPI *glBindFramebuffer)(GLenum target, GLuint framebuffer);
    void (WINE_GLAPI *glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
    void (WINE_GLAPI *glGenFramebuffers)(GLsizei n, GLuint *framebuffers);
    GLenum (WINE_GLAPI *glCheckFramebufferStatus)(GLenum target);
    void (WINE_GLAPI *glFramebufferTexture)(GLenum target, GLenum attachment,
            GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture1D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture2D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture3D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level, GLint layer);
    void (WINE_GLAPI *glFramebufferTextureLayer)(GLenum target, GLenum attachment,
            GLuint texture, GLint level, GLint layer);
    void (WINE_GLAPI *glFramebufferRenderbuffer)(GLenum target, GLenum attachment,
            GLenum renderbuffertarget, GLuint renderbuffer);
    void (WINE_GLAPI *glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment,
            GLenum pname, GLint *params);
    void (WINE_GLAPI *glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    void (WINE_GLAPI *glGenerateMipmap)(GLenum target);
};

struct wined3d_gl_limits
{
    unsigned int buffers;
    unsigned int lights;
    unsigned int ffp_textures;
    unsigned int texture_coords;
    unsigned int uniform_blocks[WINED3D_SHADER_TYPE_COUNT];
    unsigned int samplers[WINED3D_SHADER_TYPE_COUNT];
    unsigned int graphics_samplers;
    unsigned int combined_samplers;
    unsigned int general_combiners;
    unsigned int user_clip_distances;
    unsigned int texture_size;
    unsigned int texture3d_size;
    unsigned int anisotropy;
    float shininess;
    unsigned int samples;
    unsigned int vertex_attribs;

    unsigned int texture_buffer_offset_alignment;

    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned int viewport_subpixel_bits;

    unsigned int glsl_varyings;
    unsigned int glsl_vs_float_constants;
    unsigned int glsl_ps_float_constants;

    unsigned int arb_vs_float_constants;
    unsigned int arb_vs_native_constants;
    unsigned int arb_vs_instructions;
    unsigned int arb_vs_temps;
    unsigned int arb_ps_float_constants;
    unsigned int arb_ps_local_constants;
    unsigned int arb_ps_native_constants;
    unsigned int arb_ps_instructions;
    unsigned int arb_ps_temps;
};

void wined3d_gl_limits_get_texture_unit_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count);
void wined3d_gl_limits_get_uniform_block_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count);

#define WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT       0x00000001
#define WINED3D_QUIRK_GLSL_CLIP_VARYING         0x00000004
#define WINED3D_QUIRK_ALLOWS_SPECULAR_ALPHA     0x00000008
#define WINED3D_QUIRK_NV_CLIP_BROKEN            0x00000010
#define WINED3D_QUIRK_FBO_TEX_UPDATE            0x00000020
#define WINED3D_QUIRK_BROKEN_RGBA16             0x00000040
#define WINED3D_QUIRK_INFO_LOG_SPAM             0x00000080
#define WINED3D_QUIRK_LIMITED_TEX_FILTERING     0x00000100
#define WINED3D_QUIRK_BROKEN_ARB_FOG            0x00000200
#define WINED3D_QUIRK_NO_INDEPENDENT_BIT_DEPTHS 0x00000400

typedef void (WINE_GLAPI *wined3d_generic_attrib_func)(GLuint idx, const void *data);

struct wined3d_ffp_attrib_ops
{
    wined3d_generic_attrib_func generic[WINED3D_FFP_EMIT_COUNT];
};

struct wined3d_gl_info
{
    unsigned int selected_gl_version;
    unsigned int glsl_version;
    struct wined3d_gl_limits limits;
    unsigned int reserved_glsl_constants, reserved_arb_constants;
    uint32_t quirks;
    BOOL supported[WINED3D_GL_EXT_COUNT];
    GLint wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP + 1];
    float filling_convention_offset;

    HGLRC (WINAPI *p_wglCreateContextAttribsARB)(HDC dc, HGLRC share, const GLint *attribs);
    struct wined3d_ffp_attrib_ops ffp_attrib_ops;
    struct opengl_funcs gl_ops;
    struct wined3d_fbo_ops fbo_ops;

    void (WINE_GLAPI *p_glDisableWINE)(GLenum cap);
    void (WINE_GLAPI *p_glEnableWINE)(GLenum cap);
};

#define GL_EXTCALL(f) (gl_info->gl_ops.ext.p_##f)

void install_gl_compat_wrapper(struct wined3d_gl_info *gl_info, enum wined3d_gl_extension ext);
void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLuint id, BOOL program);
void shader_glsl_validate_link(const struct wined3d_gl_info *gl_info, GLuint program);
GLenum wined3d_buffer_gl_binding_from_bind_flags(const struct wined3d_gl_info *gl_info, uint32_t bind_flags);
void wined3d_check_gl_call(const struct wined3d_gl_info *gl_info,
        const char *file, unsigned int line, const char *name);

struct min_lookup
{
    GLenum mip[WINED3D_TEXF_LINEAR + 1];
};

extern const struct min_lookup minMipLookup[WINED3D_TEXF_LINEAR + 1];
extern const GLenum magLookup[WINED3D_TEXF_LINEAR + 1];

static inline GLenum wined3d_gl_mag_filter(enum wined3d_texture_filter_type mag_filter)
{
    return magLookup[mag_filter];
}

static inline GLenum wined3d_gl_min_mip_filter(enum wined3d_texture_filter_type min_filter,
        enum wined3d_texture_filter_type mip_filter)
{
    return minMipLookup[min_filter].mip[mip_filter];
}

GLenum wined3d_gl_compare_func(enum wined3d_cmp_func f);

const char *debug_fboattachment(GLenum attachment);
const char *debug_fbostatus(GLenum status);
const char *debug_glerror(GLenum error);

static inline bool wined3d_fence_supported(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_SYNC] || gl_info->supported[NV_FENCE] || gl_info->supported[APPLE_FENCE];
}

/* Checking of API calls */
/* --------------------- */
#ifndef WINE_NO_DEBUG_MSGS
#define checkGLcall(A)                                              \
    do                                                              \
    {                                                               \
        if (__WINE_IS_DEBUG_ON(_ERR, &__wine_dbch_d3d)              \
                && !gl_info->supported[ARB_DEBUG_OUTPUT])           \
            wined3d_check_gl_call(gl_info, __FILE__, __LINE__, A);  \
    } while(0)
#else
#define checkGLcall(A) do {} while(0)
#endif

struct wined3d_bo_gl
{
    struct wined3d_bo b;

    GLuint id;

    struct wined3d_allocator_block *memory;

    GLsizeiptr size;
    GLenum binding;
    GLenum usage;

    GLbitfield flags;
    uint64_t command_fence_id;
};

static inline struct wined3d_bo_gl *wined3d_bo_gl(struct wined3d_bo *bo)
{
    return CONTAINING_RECORD(bo, struct wined3d_bo_gl, b);
}

static inline GLuint wined3d_bo_gl_id(struct wined3d_bo *bo)
{
    return bo ? wined3d_bo_gl(bo)->id : 0;
}

union wined3d_gl_fence_object
{
    GLuint id;
    GLsync sync;
};

enum wined3d_fence_result
{
    WINED3D_FENCE_OK,
    WINED3D_FENCE_WAITING,
    WINED3D_FENCE_NOT_STARTED,
    WINED3D_FENCE_WRONG_THREAD,
    WINED3D_FENCE_ERROR,
};

struct wined3d_fence
{
    struct list entry;
    union wined3d_gl_fence_object object;
    struct wined3d_context_gl *context_gl;
};

HRESULT wined3d_fence_create(struct wined3d_device *device, struct wined3d_fence **fence);
void wined3d_fence_destroy(struct wined3d_fence *fence);
void wined3d_fence_issue(struct wined3d_fence *fence, struct wined3d_device *device);
enum wined3d_fence_result wined3d_fence_wait(const struct wined3d_fence *fence, struct wined3d_device *device);
enum wined3d_fence_result wined3d_fence_test(const struct wined3d_fence *fence,
        struct wined3d_device *device, uint32_t flags);

HRESULT wined3d_query_gl_create(struct wined3d_device *device, enum wined3d_query_type type, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query);
void wined3d_query_gl_destroy_buffer_object(struct wined3d_context_gl *context_gl, struct wined3d_query *query);

struct wined3d_event_query
{
    struct wined3d_query query;

    struct wined3d_fence fence;
    BOOL signalled;
};

struct wined3d_occlusion_query
{
    struct wined3d_query query;

    struct list entry;
    GLuint id;
    struct wined3d_context_gl *context_gl;
    uint64_t samples;
    BOOL started;
};

struct wined3d_timestamp_query
{
    struct wined3d_query query;

    struct list entry;
    GLuint id;
    struct wined3d_context_gl *context_gl;
    uint64_t timestamp;
};

union wined3d_gl_so_statistics_query
{
    GLuint id[2];
    struct
    {
        GLuint written;
        GLuint generated;
    } query;
};

struct wined3d_so_statistics_query
{
    struct wined3d_query query;

    struct list entry;
    union wined3d_gl_so_statistics_query u;
    struct wined3d_context_gl *context_gl;
    unsigned int stream_idx;
    struct wined3d_query_data_so_statistics statistics;
    BOOL started;
};

union wined3d_gl_pipeline_statistics_query
{
    GLuint id[11];
    struct
    {
        GLuint vertices;
        GLuint primitives;
        GLuint vertex_shader;
        GLuint tess_control_shader;
        GLuint tess_eval_shader;
        GLuint geometry_shader;
        GLuint geometry_primitives;
        GLuint fragment_shader;
        GLuint compute_shader;
        GLuint clipping_input;
        GLuint clipping_output;
    } query;
};

struct wined3d_pipeline_statistics_query
{
    struct wined3d_query query;

    struct list entry;
    union wined3d_gl_pipeline_statistics_query u;
    struct wined3d_context_gl *context_gl;
    struct wined3d_query_data_pipeline_statistics statistics;
    BOOL started;
};

struct wined3d_gl_view
{
    GLenum target;
    GLuint name;
};

struct wined3d_rendertarget_info
{
    struct wined3d_gl_view gl_view;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    unsigned int layer_count;
};

struct wined3d_command_fence_gl
{
    uint64_t id;
    struct wined3d_fence *fence;
};

#define MAX_GL_FRAGMENT_SAMPLERS 32

struct wined3d_fbo_resource
{
    GLuint object;
    GLenum target;
    GLuint level, layer;
};

#define WINED3D_FBO_ENTRY_FLAG_ATTACHED      0x1
#define WINED3D_FBO_ENTRY_FLAG_DEPTH         0x2
#define WINED3D_FBO_ENTRY_FLAG_STENCIL       0x4

struct fbo_entry
{
    struct list entry;
    uint32_t flags;
    uint32_t rt_mask;
    GLuint id;
    struct wined3d_fbo_entry_key
    {
        uint32_t rb_namespace;
        struct wined3d_fbo_resource objects[WINED3D_MAX_RENDER_TARGETS + 1];
    } key;
};

struct wined3d_context_gl
{
    struct wined3d_context c;

    const struct wined3d_gl_info *gl_info;

    DWORD tid; /* Thread ID which owns this context at the moment. */

    uint32_t dc_is_private : 1;
    uint32_t dc_has_format : 1; /* Only meaningful for private DCs. */
    uint32_t fog_enabled : 1;
    uint32_t rebind_fbo : 1;
    uint32_t needs_set : 1;
    uint32_t internal_format_set : 1;
    uint32_t valid : 1;
    uint32_t padding : 25;

    uint32_t default_attrib_value_set;

    GLenum tracking_parm; /* Which source is tracking current colour. */
    GLenum untracked_materials[2];
    SIZE blit_size;
    unsigned int active_texture;

    GLenum *texture_type;

    /* The WGL context. */
    unsigned int level;
    HGLRC restore_ctx;
    HDC restore_dc;
    int restore_pf;
    HWND restore_pf_win;
    HGLRC gl_ctx;
    HDC dc;
    int pixel_format;
    HWND window;
    GLint aux_buffers;

    /* FBOs. */
    unsigned int fbo_entry_count;
    struct list fbo_list;
    struct list fbo_destroy_list;
    struct fbo_entry *current_fbo;
    GLuint fbo_read_binding;
    GLuint fbo_draw_binding;
    struct wined3d_rendertarget_info blit_targets[WINED3D_MAX_RENDER_TARGETS];
    uint32_t draw_buffers_mask; /* Enabled draw buffers, 31 max. */

    /* Queries. */
    struct list occlusion_queries;
    struct list fences;
    struct list timestamp_queries;
    struct list so_statistics_queries;
    struct list pipeline_statistics_queries;

    GLuint *free_occlusion_queries;
    SIZE_T free_occlusion_query_size;
    unsigned int free_occlusion_query_count;

    union wined3d_gl_fence_object *free_fences;
    SIZE_T free_fence_size;
    unsigned int free_fence_count;

    GLuint *free_timestamp_queries;
    SIZE_T free_timestamp_query_size;
    unsigned int free_timestamp_query_count;

    union wined3d_gl_so_statistics_query *free_so_statistics_queries;
    SIZE_T free_so_statistics_query_size;
    unsigned int free_so_statistics_query_count;

    union wined3d_gl_pipeline_statistics_query *free_pipeline_statistics_queries;
    SIZE_T free_pipeline_statistics_query_size;
    unsigned int free_pipeline_statistics_query_count;

    GLuint blit_vbo;

    unsigned int tex_unit_map[WINED3D_MAX_COMBINED_SAMPLERS];
    unsigned int rev_tex_unit_map[MAX_GL_FRAGMENT_SAMPLERS + WINED3D_MAX_VERTEX_SAMPLERS];

    /* Extension emulation. */
    GLint gl_fog_source;
    GLfloat fog_coord_value;
    GLfloat colour[4], fog_start, fog_end, fog_colour[4];

    GLuint dummy_arbfp_prog;

    struct
    {
        struct wined3d_command_fence_gl *fences;
        SIZE_T fences_size;
        SIZE_T fence_count;
    } submitted;
};

static inline struct wined3d_context_gl *wined3d_context_gl(struct wined3d_context *context)
{
    return CONTAINING_RECORD(context, struct wined3d_context_gl, c);
}

static inline const struct wined3d_context_gl *wined3d_context_gl_const(const struct wined3d_context *context)
{
    return CONTAINING_RECORD(context, struct wined3d_context_gl, c);
}

HGLRC context_create_wgl_attribs(const struct wined3d_gl_info *gl_info, HDC hdc, HGLRC share_ctx);
DWORD context_get_tls_idx(void);
void context_gl_apply_texture_draw_state(struct wined3d_context_gl *context_gl,
        struct wined3d_texture *texture, unsigned int sub_resource_idx, unsigned int location);
void context_gl_resource_released(struct wined3d_device *device, GLuint name, BOOL rb_namespace);

struct wined3d_context *wined3d_context_gl_acquire(const struct wined3d_device *device,
        struct wined3d_texture *texture, unsigned int sub_resource_idx);
void wined3d_context_gl_active_texture(struct wined3d_context_gl *context_gl,
        const struct wined3d_gl_info *gl_info, unsigned int unit);
void wined3d_context_gl_alloc_fence(struct wined3d_context_gl *context_gl, struct wined3d_fence *fence);
void wined3d_context_gl_alloc_occlusion_query(struct wined3d_context_gl *context_gl,
        struct wined3d_occlusion_query *query);
void wined3d_context_gl_alloc_pipeline_statistics_query(struct wined3d_context_gl *context_gl,
        struct wined3d_pipeline_statistics_query *query);
void wined3d_context_gl_alloc_so_statistics_query(struct wined3d_context_gl *context_gl,
        struct wined3d_so_statistics_query *query);
void wined3d_context_gl_alloc_timestamp_query(struct wined3d_context_gl *context_gl,
        struct wined3d_timestamp_query *query);
GLuint wined3d_context_gl_allocate_vram_chunk_buffer(struct wined3d_context_gl *context_gl,
        unsigned int pool, size_t size);
void wined3d_context_gl_apply_blit_state(struct wined3d_context_gl *context_gl, const struct wined3d_device *device);
BOOL wined3d_context_gl_apply_clear_state(struct wined3d_context_gl *context_gl, const struct wined3d_state *state,
        unsigned int rt_count, const struct wined3d_fb_state *fb);
void wined3d_context_gl_apply_fbo_state_explicit(struct wined3d_context_gl *context_gl, GLenum target,
        struct wined3d_resource *rt, unsigned int rt_sub_resource_idx,
        struct wined3d_resource *ds, unsigned int ds_sub_resource_idx, uint32_t location);
void wined3d_context_gl_bind_bo(struct wined3d_context_gl *context_gl, GLenum binding, GLuint name);
void wined3d_context_gl_bind_dummy_textures(const struct wined3d_context_gl *context_gl);
void wined3d_context_gl_bind_texture(struct wined3d_context_gl *context_gl, GLenum target, GLuint name);
void wined3d_context_gl_check_fbo_status(const struct wined3d_context_gl *context_gl, GLenum target);
void wined3d_context_gl_copy_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_bo_address *dst, const struct wined3d_bo_address *src,
        unsigned int range_count, const struct wined3d_range *ranges, uint32_t map_flags);
void wined3d_context_gl_destroy(struct wined3d_context_gl *context_gl);
void wined3d_context_gl_destroy_bo(struct wined3d_context_gl *context_gl, struct wined3d_bo_gl *bo);
void wined3d_context_gl_draw_shaded_quad(struct wined3d_context_gl *context_gl, struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, const RECT *src_rect, const RECT *dst_rect,
        enum wined3d_texture_filter_type filter);
void wined3d_context_gl_enable_clip_distances(struct wined3d_context_gl *context_gl, uint32_t mask);
void wined3d_context_gl_end_transform_feedback(struct wined3d_context_gl *context_gl);
void wined3d_context_gl_flush_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_const_bo_address *data, size_t size);
void wined3d_context_gl_free_fence(struct wined3d_fence *fence);
void wined3d_context_gl_free_occlusion_query(struct wined3d_occlusion_query *query);
void wined3d_context_gl_free_pipeline_statistics_query(struct wined3d_pipeline_statistics_query *query);
void wined3d_context_gl_free_so_statistics_query(struct wined3d_so_statistics_query *query);
void wined3d_context_gl_free_timestamp_query(struct wined3d_timestamp_query *query);
struct wined3d_context_gl *wined3d_context_gl_get_current(void);
GLenum wined3d_context_gl_get_offscreen_gl_buffer(const struct wined3d_context_gl *context_gl);
const unsigned int *wined3d_context_gl_get_tex_unit_mapping(const struct wined3d_context_gl *context_gl,
        const struct wined3d_shader_version *shader_version, unsigned int *base, unsigned int *count);
HRESULT wined3d_context_gl_init(struct wined3d_context_gl *context_gl, struct wined3d_swapchain_gl *swapchain_gl);
void *wined3d_context_gl_map_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_bo_address *data, size_t size, uint32_t flags);
struct wined3d_context_gl *wined3d_context_gl_reacquire(struct wined3d_context_gl *context_gl);
void wined3d_context_gl_release(struct wined3d_context_gl *context_gl);
BOOL wined3d_context_gl_set_current(struct wined3d_context_gl *context_gl);
void wined3d_context_gl_submit_command_fence(struct wined3d_context_gl *context_gl);
void wined3d_context_gl_texture_update(struct wined3d_context_gl *context_gl,
        const struct wined3d_texture_gl *texture_gl);
void wined3d_context_gl_unmap_bo_address(struct wined3d_context_gl *context_gl, const struct wined3d_bo_address *data,
        unsigned int range_count, const struct wined3d_range *ranges);
void wined3d_context_gl_update_stream_sources(struct wined3d_context_gl *context_gl, const struct wined3d_state *state);
void wined3d_context_gl_wait_command_fence(struct wined3d_context_gl *context_gl, uint64_t id);

void wined3d_fbo_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info);
void wined3d_ffp_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info);
struct wined3d_blitter *wined3d_glsl_blitter_create(struct wined3d_blitter **next, const struct wined3d_device *device);
void wined3d_raw_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info);

void ffp_vertex_update_clip_plane_constants(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state);

struct wined3d_caps_gl_ctx
{
    HDC dc;
    HWND wnd;
    HGLRC gl_ctx;
    HDC restore_dc;
    HGLRC restore_gl_ctx;

    const struct wined3d_gl_info *gl_info;
    GLuint test_vbo;
    GLuint test_program_id;

    const struct wined3d_gpu_description *gpu_description;
    UINT64 vram_bytes;
};

BOOL wined3d_caps_gl_ctx_test_viewport_subpixel_bits(struct wined3d_caps_gl_ctx *ctx);
bool wined3d_caps_gl_ctx_test_filling_convention(struct wined3d_caps_gl_ctx *ctx, float offset);

struct wined3d_adapter_gl
{
    struct wined3d_adapter a;

    struct wined3d_gl_info gl_info;

    struct wined3d_pixel_format *pixel_formats;
    unsigned int pixel_format_count;
};

static inline struct wined3d_adapter_gl *wined3d_adapter_gl(struct wined3d_adapter *adapter)
{
    return CONTAINING_RECORD(adapter, struct wined3d_adapter_gl, a);
}

static inline const struct wined3d_adapter_gl *wined3d_adapter_gl_const(const struct wined3d_adapter *adapter)
{
    return CONTAINING_RECORD(adapter, struct wined3d_adapter_gl, a);
}

BOOL wined3d_adapter_gl_init_format_info(struct wined3d_adapter *adapter, struct wined3d_caps_gl_ctx *ctx);

struct wined3d_allocator_chunk_gl
{
    struct wined3d_allocator_chunk c;
    unsigned int memory_type;
    GLuint gl_buffer;
};

static inline struct wined3d_allocator_chunk_gl *wined3d_allocator_chunk_gl(struct wined3d_allocator_chunk *chunk)
{
    return CONTAINING_RECORD(chunk, struct wined3d_allocator_chunk_gl, c);
}

struct wined3d_dummy_textures
{
    GLuint tex_1d;
    GLuint tex_2d;
    GLuint tex_rect;
    GLuint tex_3d;
    GLuint tex_cube;
    GLuint tex_cube_array;
    GLuint tex_1d_array;
    GLuint tex_2d_array;
    GLuint tex_buffer;
    GLuint tex_2d_ms;
    GLuint tex_2d_ms_array;
};

struct wined3d_device_gl
{
    struct wined3d_device d;

    /* Textures for when no other textures are bound. */
    struct wined3d_dummy_textures dummy_textures;

    CRITICAL_SECTION allocator_cs;
    struct wined3d_allocator allocator;
    uint64_t completed_fence_id;
    uint64_t current_fence_id;
    uint64_t retired_bo_size;

    struct wined3d_retired_block_gl
    {
        struct wined3d_allocator_block *block;
        uint64_t fence_id;
    } *retired_blocks;
    SIZE_T retired_blocks_size;
    SIZE_T retired_block_count;

    HWND backup_wnd;
    HDC backup_dc;
};

static inline struct wined3d_device_gl *wined3d_device_gl(struct wined3d_device *device)
{
    return CONTAINING_RECORD(device, struct wined3d_device_gl, d);
}

static inline struct wined3d_device_gl *wined3d_device_gl_from_allocator(struct wined3d_allocator *allocator)
{
    return CONTAINING_RECORD(allocator, struct wined3d_device_gl, allocator);
}

static inline void wined3d_device_gl_allocator_lock(struct wined3d_device_gl *device_gl)
{
    EnterCriticalSection(&device_gl->allocator_cs);
}

static inline void wined3d_device_gl_allocator_unlock(struct wined3d_device_gl *device_gl)
{
    LeaveCriticalSection(&device_gl->allocator_cs);
}

static inline void wined3d_allocator_chunk_gl_lock(struct wined3d_allocator_chunk_gl *chunk_gl)
{
    wined3d_device_gl_allocator_lock(wined3d_device_gl_from_allocator(chunk_gl->c.allocator));
}

static inline void wined3d_allocator_chunk_gl_unlock(struct wined3d_allocator_chunk_gl *chunk_gl)
{
    wined3d_device_gl_allocator_unlock(wined3d_device_gl_from_allocator(chunk_gl->c.allocator));
}

bool wined3d_device_gl_create_bo(struct wined3d_device_gl *device_gl,
        struct wined3d_context_gl *context_gl, GLsizeiptr size, GLenum binding,
        GLenum usage, bool coherent, GLbitfield flags, struct wined3d_bo_gl *bo);
void wined3d_device_gl_create_primary_opengl_context_cs(void *object);
void wined3d_device_gl_delete_opengl_contexts_cs(void *object);
HDC wined3d_device_gl_get_backup_dc(struct wined3d_device_gl *device_gl);
GLbitfield wined3d_device_gl_get_memory_type_flags(unsigned int memory_type_idx);

GLbitfield wined3d_resource_gl_map_flags(const struct wined3d_bo_gl *bo, DWORD d3d_flags);
GLenum wined3d_resource_gl_legacy_map_flags(DWORD d3d_flags);
GLbitfield wined3d_resource_gl_storage_flags(const struct wined3d_resource *resource);

static inline void wined3d_context_gl_reference_bo(struct wined3d_context_gl *context_gl, struct wined3d_bo_gl *bo_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);

    bo_gl->command_fence_id = device_gl->current_fence_id;
}

static inline void wined3d_context_gl_reference_buffer(struct wined3d_context_gl *context_gl,
        struct wined3d_buffer *buffer)
{
    if (buffer->buffer_object)
        wined3d_context_gl_reference_bo(context_gl, wined3d_bo_gl(buffer->buffer_object));
}

struct gl_texture
{
    struct wined3d_sampler_desc sampler_desc;
    GLuint name;
};

struct wined3d_renderbuffer_entry
{
    struct list entry;
    GLuint id;
    unsigned int width, height;
};

struct wined3d_texture_gl
{
    struct wined3d_texture t;

    struct gl_texture texture_rgb, texture_srgb;

    GLenum target;

    GLuint rb_multisample;
    GLuint rb_resolved;

    struct list renderbuffers;
    const struct wined3d_renderbuffer_entry *current_renderbuffer;
};

static inline struct wined3d_texture_gl *wined3d_texture_gl(struct wined3d_texture *texture)
{
    return CONTAINING_RECORD(texture, struct wined3d_texture_gl, t);
}

static inline struct gl_texture *wined3d_texture_gl_get_gl_texture(struct wined3d_texture_gl *texture_gl, bool srgb)
{
    return srgb ? &texture_gl->texture_srgb : &texture_gl->texture_rgb;
}

static inline GLenum wined3d_texture_gl_get_sub_resource_target(const struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx)
{
    static const GLenum cube_targets[] =
    {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
    };

    if (texture_gl->t.resource.usage & WINED3DUSAGE_LEGACY_CUBEMAP)
        return cube_targets[sub_resource_idx / texture_gl->t.level_count];
    return texture_gl->target;
}

static inline GLuint wined3d_texture_gl_get_texture_name(const struct wined3d_texture_gl *texture_gl,
        const struct wined3d_context *context, bool srgb)
{
    if (srgb && needs_separate_srgb_gl_texture(context, &texture_gl->t))
        return texture_gl->texture_srgb.name;
    return texture_gl->texture_rgb.name;
}

static inline bool wined3d_texture_gl_is_multisample_location(const struct wined3d_texture_gl *texture_gl,
        uint32_t location)
{
    if (location == WINED3D_LOCATION_RB_MULTISAMPLE)
        return true;
    if (location != WINED3D_LOCATION_TEXTURE_RGB && location != WINED3D_LOCATION_TEXTURE_SRGB)
        return false;
    return texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE || texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
}

struct wined3d_blt_info
{
    GLenum bind_target;
    struct wined3d_vec3 texcoords[4];
};

void texture2d_get_blt_info(const struct wined3d_texture_gl *texture_gl, unsigned int sub_resource_idx,
        const RECT *rect, struct wined3d_blt_info *info);
void texture2d_read_from_framebuffer(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD src_location, DWORD dst_location);

void wined3d_gl_texture_swizzle_from_color_fixup(GLint swizzle[4], struct color_fixup_desc fixup);
GLenum wined3d_texture_get_gl_buffer(const struct wined3d_texture *texture);

void wined3d_texture_gl_apply_sampler_desc(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_context_gl *context_gl);
void wined3d_texture_gl_bind(struct wined3d_texture_gl *texture_gl, struct wined3d_context_gl *context_gl, BOOL srgb);
void wined3d_texture_gl_bind_and_dirtify(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb);
HRESULT wined3d_texture_gl_init(struct wined3d_texture_gl *texture_gl, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_texture_gl_prepare_texture(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb);
void wined3d_texture_gl_set_compatible_renderbuffer(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, unsigned int level, const struct wined3d_rendertarget_info *rt);

struct wined3d_sampler_gl
{
    struct wined3d_sampler s;

    GLuint name;
};

static inline struct wined3d_sampler_gl *wined3d_sampler_gl(struct wined3d_sampler *sampler)
{
    return CONTAINING_RECORD(sampler, struct wined3d_sampler_gl, s);
}

void wined3d_sampler_gl_bind(struct wined3d_sampler_gl *sampler_gl, unsigned int unit,
        struct wined3d_texture_gl *texture_gl, const struct wined3d_context_gl *context_gl);
void wined3d_sampler_gl_init(struct wined3d_sampler_gl *sampler_gl, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_buffer_gl
{
    struct wined3d_buffer b;
};

static inline struct wined3d_buffer_gl *wined3d_buffer_gl(struct wined3d_buffer *buffer)
{
    return CONTAINING_RECORD(buffer, struct wined3d_buffer_gl, b);
}

static inline const struct wined3d_buffer_gl *wined3d_buffer_gl_const(const struct wined3d_buffer *buffer)
{
    return CONTAINING_RECORD(buffer, struct wined3d_buffer_gl, b);
}

HRESULT wined3d_buffer_gl_init(struct wined3d_buffer_gl *buffer_gl, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_rendertarget_view_gl
{
    struct wined3d_rendertarget_view v;
    struct wined3d_gl_view gl_view;
};

static inline struct wined3d_rendertarget_view_gl *wined3d_rendertarget_view_gl(
        struct wined3d_rendertarget_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_rendertarget_view_gl, v);
}

HRESULT wined3d_rendertarget_view_gl_init(struct wined3d_rendertarget_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_shader_resource_view_gl
{
    struct wined3d_shader_resource_view v;
    struct wined3d_bo_user bo_user;
    struct wined3d_gl_view gl_view;
};

static inline struct wined3d_shader_resource_view_gl *wined3d_shader_resource_view_gl(
        struct wined3d_shader_resource_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_shader_resource_view_gl, v);
}

void wined3d_shader_resource_view_gl_bind(struct wined3d_shader_resource_view_gl *view_gl, unsigned int unit,
        struct wined3d_sampler_gl *sampler_gl, struct wined3d_context_gl *context_gl);
void wined3d_shader_resource_view_gl_generate_mipmap(struct wined3d_shader_resource_view_gl *srv_gl,
        struct wined3d_context_gl *context_gl);
HRESULT wined3d_shader_resource_view_gl_init(struct wined3d_shader_resource_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_shader_resource_view_gl_update(struct wined3d_shader_resource_view_gl *srv_gl,
        struct wined3d_context_gl *context_gl);

struct wined3d_unordered_access_view_gl
{
    struct wined3d_unordered_access_view v;
    struct wined3d_bo_user bo_user;
    struct wined3d_gl_view gl_view;
    struct wined3d_bo_gl counter_bo;
};

static inline struct wined3d_unordered_access_view_gl *wined3d_unordered_access_view_gl(
        struct wined3d_unordered_access_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_unordered_access_view_gl, v);
}

void wined3d_unordered_access_view_gl_clear(struct wined3d_unordered_access_view_gl *view_gl,
        const struct wined3d_uvec4 *clear_value, struct wined3d_context_gl *context_gl, bool fp);
HRESULT wined3d_unordered_access_view_gl_init(struct wined3d_unordered_access_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_unordered_access_view_gl_update(struct wined3d_unordered_access_view_gl *uav_gl,
        struct wined3d_context_gl *context_gl);

struct wined3d_swapchain_gl
{
    struct wined3d_swapchain s;
};

static inline struct wined3d_swapchain_gl *wined3d_swapchain_gl(struct wined3d_swapchain *swapchain)
{
    return CONTAINING_RECORD(swapchain, struct wined3d_swapchain_gl, s);
}

void wined3d_swapchain_gl_cleanup(struct wined3d_swapchain_gl *swapchain_gl);
struct wined3d_context_gl *wined3d_swapchain_gl_get_context(struct wined3d_swapchain_gl *swapchain_gl);
HRESULT wined3d_swapchain_gl_init(struct wined3d_swapchain_gl *swapchain_gl, struct wined3d_device *device,
        const struct wined3d_swapchain_desc *desc, struct wined3d_swapchain_state_parent *state_parent,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_format_gl
{
    struct wined3d_format f;

    GLenum vtx_type;
    GLint vtx_format;

    GLint internal;
    GLint srgb_internal;
    GLint rt_internal;
    GLint format;
    GLint type;

    GLenum view_class;
};

static inline const struct wined3d_format_gl *wined3d_format_gl(const struct wined3d_format *format)
{
    return CONTAINING_RECORD(format, struct wined3d_format_gl, f);
}

static inline GLuint wined3d_gl_get_internal_format(struct wined3d_resource *resource,
    const struct wined3d_format_gl *format_gl, bool srgb)
{
    if (srgb)
        return format_gl->srgb_internal;
    else if ((resource->bind_flags & WINED3D_BIND_RENDER_TARGET) && wined3d_resource_is_offscreen(resource))
        return format_gl->rt_internal;
    else
        return format_gl->internal;
}

#endif /* __WINE_WINED3D_GL */
