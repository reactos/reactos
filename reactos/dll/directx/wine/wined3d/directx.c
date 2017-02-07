/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define WINE_DEFAULT_VIDMEM (64 * 1024 * 1024)

/* The driver names reflect the lowest GPU supported
 * by a certain driver, so DRIVER_AMD_R300 supports
 * R3xx, R4xx and R5xx GPUs. */
enum wined3d_display_driver
{
    DRIVER_AMD_RAGE_128PRO,
    DRIVER_AMD_R100,
    DRIVER_AMD_R300,
    DRIVER_AMD_R600,
    DRIVER_INTEL_GMA800,
    DRIVER_INTEL_GMA900,
    DRIVER_INTEL_GMA950,
    DRIVER_INTEL_GMA3000,
    DRIVER_NVIDIA_TNT,
    DRIVER_NVIDIA_GEFORCE2MX,
    DRIVER_NVIDIA_GEFORCEFX,
    DRIVER_NVIDIA_GEFORCE6,
    DRIVER_UNKNOWN
};

enum wined3d_driver_model
{
    DRIVER_MODEL_WIN9X,
    DRIVER_MODEL_NT40,
    DRIVER_MODEL_NT5X,
    DRIVER_MODEL_NT6X
};

enum wined3d_gl_vendor
{
    GL_VENDOR_UNKNOWN,
    GL_VENDOR_APPLE,
    GL_VENDOR_FGLRX,
    GL_VENDOR_INTEL,
    GL_VENDOR_MESA,
    GL_VENDOR_NVIDIA,
};

/* The d3d device ID */
static const GUID IID_D3DDEVICE_D3DUID = { 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };

/* Extension detection */
static const struct
{
    const char *extension_string;
    enum wined3d_gl_extension extension;
    DWORD version;
}
EXTENSION_MAP[] =
{
    /* APPLE */
    {"GL_APPLE_client_storage",             APPLE_CLIENT_STORAGE,           0                           },
    {"GL_APPLE_fence",                      APPLE_FENCE,                    0                           },
    {"GL_APPLE_float_pixels",               APPLE_FLOAT_PIXELS,             0                           },
    {"GL_APPLE_flush_buffer_range",         APPLE_FLUSH_BUFFER_RANGE,       0                           },
    {"GL_APPLE_flush_render",               APPLE_FLUSH_RENDER,             0                           },
    {"GL_APPLE_ycbcr_422",                  APPLE_YCBCR_422,                0                           },

    /* ARB */
    {"GL_ARB_color_buffer_float",           ARB_COLOR_BUFFER_FLOAT,         0                           },
    {"GL_ARB_depth_buffer_float",           ARB_DEPTH_BUFFER_FLOAT,         0                           },
    {"GL_ARB_depth_clamp",                  ARB_DEPTH_CLAMP,                0                           },
    {"GL_ARB_depth_texture",                ARB_DEPTH_TEXTURE,              0                           },
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS,               0                           },
    {"GL_ARB_draw_elements_base_vertex",    ARB_DRAW_ELEMENTS_BASE_VERTEX,  0                           },
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM,           0                           },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER,            0                           },
    {"GL_ARB_framebuffer_object",           ARB_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_ARB_geometry_shader4",             ARB_GEOMETRY_SHADER4,           0                           },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL,           0                           },
    {"GL_ARB_half_float_vertex",            ARB_HALF_FLOAT_VERTEX,          0                           },
    {"GL_ARB_map_buffer_alignment",         ARB_MAP_BUFFER_ALIGNMENT,       0                           },
    {"GL_ARB_map_buffer_range",             ARB_MAP_BUFFER_RANGE,           0                           },
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE,                0                           }, /* needs GLX_ARB_MULTISAMPLE as well */
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE,               0                           },
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY,            0                           },
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT,        0                           },
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS,           0                           },
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE,               0                           },
    {"GL_ARB_provoking_vertex",             ARB_PROVOKING_VERTEX,           0                           },
    {"GL_ARB_shader_objects",               ARB_SHADER_OBJECTS,             0                           },
    {"GL_ARB_shader_texture_lod",           ARB_SHADER_TEXTURE_LOD,         0                           },
    {"GL_ARB_shading_language_100",         ARB_SHADING_LANGUAGE_100,       0                           },
    {"GL_ARB_shadow",                       ARB_SHADOW,                     0                           },
    {"GL_ARB_sync",                         ARB_SYNC,                       0                           },
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP,       0                           },
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION,        0                           },
    {"GL_ARB_texture_compression_rgtc",     ARB_TEXTURE_COMPRESSION_RGTC,   0                           },
    {"GL_ARB_texture_cube_map",             ARB_TEXTURE_CUBE_MAP,           0                           },
    {"GL_ARB_texture_env_add",              ARB_TEXTURE_ENV_ADD,            0                           },
    {"GL_ARB_texture_env_combine",          ARB_TEXTURE_ENV_COMBINE,        0                           },
    {"GL_ARB_texture_env_dot3",             ARB_TEXTURE_ENV_DOT3,           0                           },
    {"GL_ARB_texture_float",                ARB_TEXTURE_FLOAT,              0                           },
    {"GL_ARB_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT,    0                           },
    {"GL_ARB_texture_non_power_of_two",     ARB_TEXTURE_NON_POWER_OF_TWO,   MAKEDWORD_VERSION(2, 0)     },
    {"GL_ARB_texture_rectangle",            ARB_TEXTURE_RECTANGLE,          0                           },
    {"GL_ARB_texture_rg",                   ARB_TEXTURE_RG,                 0                           },
    {"GL_ARB_vertex_array_bgra",            ARB_VERTEX_ARRAY_BGRA,          0                           },
    {"GL_ARB_vertex_blend",                 ARB_VERTEX_BLEND,               0                           },
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT,       0                           },
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM,             0                           },
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER,              0                           },

    /* ATI */
    {"GL_ATI_fragment_shader",              ATI_FRAGMENT_SHADER,            0                           },
    {"GL_ATI_separate_stencil",             ATI_SEPARATE_STENCIL,           0                           },
    {"GL_ATI_texture_compression_3dc",      ATI_TEXTURE_COMPRESSION_3DC,    0                           },
    {"GL_ATI_texture_env_combine3",         ATI_TEXTURE_ENV_COMBINE3,       0                           },
    {"GL_ATI_texture_mirror_once",          ATI_TEXTURE_MIRROR_ONCE,        0                           },

    /* EXT */
    {"GL_EXT_blend_color",                  EXT_BLEND_COLOR,                0                           },
    {"GL_EXT_blend_equation_separate",      EXT_BLEND_EQUATION_SEPARATE,    0                           },
    {"GL_EXT_blend_func_separate",          EXT_BLEND_FUNC_SEPARATE,        0                           },
    {"GL_EXT_blend_minmax",                 EXT_BLEND_MINMAX,               0                           },
    {"GL_EXT_blend_subtract",               EXT_BLEND_SUBTRACT,             0                           },
    {"GL_EXT_depth_bounds_test",            EXT_DEPTH_BOUNDS_TEST,          0                           },
    {"GL_EXT_draw_buffers2",                EXT_DRAW_BUFFERS2,              0                           },
    {"GL_EXT_fog_coord",                    EXT_FOG_COORD,                  0                           },
    {"GL_EXT_framebuffer_blit",             EXT_FRAMEBUFFER_BLIT,           0                           },
    {"GL_EXT_framebuffer_multisample",      EXT_FRAMEBUFFER_MULTISAMPLE,    0                           },
    {"GL_EXT_framebuffer_object",           EXT_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_EXT_gpu_program_parameters",       EXT_GPU_PROGRAM_PARAMETERS,     0                           },
    {"GL_EXT_gpu_shader4",                  EXT_GPU_SHADER4,                0                           },
    {"GL_EXT_packed_depth_stencil",         EXT_PACKED_DEPTH_STENCIL,       0                           },
    {"GL_EXT_paletted_texture",             EXT_PALETTED_TEXTURE,           0                           },
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS,           0                           },
    {"GL_EXT_provoking_vertex",             EXT_PROVOKING_VERTEX,           0                           },
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR,            0                           },
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE,           0                           },
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP,               0                           },
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D,                  MAKEDWORD_VERSION(1, 2)     },
    {"GL_EXT_texture_compression_rgtc",     EXT_TEXTURE_COMPRESSION_RGTC,   0                           },
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC,   0                           },
    {"GL_EXT_texture_env_add",              EXT_TEXTURE_ENV_ADD,            0                           },
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE,        0                           },
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3,           0                           },
    {"GL_EXT_texture_filter_anisotropic",   EXT_TEXTURE_FILTER_ANISOTROPIC, 0                           },
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS,           0                           },
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB,               0                           },
    {"GL_EXT_texture_sRGB_decode",          EXT_TEXTURE_SRGB_DECODE,        0                           },
    {"GL_EXT_vertex_array_bgra",            EXT_VERTEX_ARRAY_BGRA,          0                           },

    /* NV */
    {"GL_NV_depth_clamp",                   NV_DEPTH_CLAMP,                 0                           },
    {"GL_NV_fence",                         NV_FENCE,                       0                           },
    {"GL_NV_fog_distance",                  NV_FOG_DISTANCE,                0                           },
    {"GL_NV_fragment_program",              NV_FRAGMENT_PROGRAM,            0                           },
    {"GL_NV_fragment_program2",             NV_FRAGMENT_PROGRAM2,           0                           },
    {"GL_NV_fragment_program_option",       NV_FRAGMENT_PROGRAM_OPTION,     0                           },
    {"GL_NV_half_float",                    NV_HALF_FLOAT,                  0                           },
    {"GL_NV_light_max_exponent",            NV_LIGHT_MAX_EXPONENT,          0                           },
    {"GL_NV_point_sprite",                  NV_POINT_SPRITE,                0                           },
    {"GL_NV_register_combiners",            NV_REGISTER_COMBINERS,          0                           },
    {"GL_NV_register_combiners2",           NV_REGISTER_COMBINERS2,         0                           },
    {"GL_NV_texgen_reflection",             NV_TEXGEN_REFLECTION,           0                           },
    {"GL_NV_texture_env_combine4",          NV_TEXTURE_ENV_COMBINE4,        0                           },
    {"GL_NV_texture_shader",                NV_TEXTURE_SHADER,              0                           },
    {"GL_NV_texture_shader2",               NV_TEXTURE_SHADER2,             0                           },
    {"GL_NV_vertex_program",                NV_VERTEX_PROGRAM,              0                           },
    {"GL_NV_vertex_program1_1",             NV_VERTEX_PROGRAM1_1,           0                           },
    {"GL_NV_vertex_program2",               NV_VERTEX_PROGRAM2,             0                           },
    {"GL_NV_vertex_program2_option",        NV_VERTEX_PROGRAM2_OPTION,      0                           },
    {"GL_NV_vertex_program3",               NV_VERTEX_PROGRAM3,             0                           },

    /* SGI */
    {"GL_SGIS_generate_mipmap",             SGIS_GENERATE_MIPMAP,           0                           },
};

/**********************************************************
 * Utility functions follow
 **********************************************************/

const struct min_lookup minMipLookup[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST_MIPMAP_NEAREST,  GL_NEAREST_MIPMAP_LINEAR}}, /* POINT*/
    {{GL_LINEAR,    GL_LINEAR_MIPMAP_NEAREST,   GL_LINEAR_MIPMAP_LINEAR}},  /* LINEAR */
};

const struct min_lookup minMipLookup_noFilter[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* POINT */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* LINEAR */
};

const struct min_lookup minMipLookup_noMip[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* POINT */
    {{GL_LINEAR,    GL_LINEAR,                  GL_LINEAR }},               /* LINEAR */
};

const GLenum magLookup[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_LINEAR,
};

const GLenum magLookup_noFilter[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_NEAREST,
};

/* drawStridedSlow attributes */
glAttribFunc position_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc diffuse_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc specular_func_3ubv;
glAttribFunc specular_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc normal_funcs[WINED3D_FFP_EMIT_COUNT];
glMultiTexCoordFunc multi_texcoord_funcs[WINED3D_FFP_EMIT_COUNT];

/**
 * Note: GL seems to trap if GetDeviceCaps is called before any HWND's created,
 * i.e., there is no GL Context - Get a default rendering context to enable the
 * function query some info from GL.
 */

struct wined3d_fake_gl_ctx
{
    HDC dc;
    HWND wnd;
    HGLRC gl_ctx;
    HDC restore_dc;
    HGLRC restore_gl_ctx;
};

static void WineD3D_ReleaseFakeGLContext(const struct wined3d_fake_gl_ctx *ctx)
{
    TRACE_(d3d_caps)("Destroying fake GL context.\n");

    if (!pwglMakeCurrent(NULL, NULL))
    {
        ERR_(d3d_caps)("Failed to disable fake GL context.\n");
    }
#if 0
    if (!pwglDeleteContext(ctx->gl_ctx))
    {
        DWORD err = GetLastError();
        ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx->gl_ctx, err);
    }
#endif
    ReleaseDC(ctx->wnd, ctx->dc);
    DestroyWindow(ctx->wnd);

    if (ctx->restore_gl_ctx && !pwglMakeCurrent(ctx->restore_dc, ctx->restore_gl_ctx))
    {
        ERR_(d3d_caps)("Failed to restore previous GL context.\n");
    }
}

/* Do not call while under the GL lock. */
static BOOL WineD3D_CreateFakeGLContext(struct wined3d_fake_gl_ctx *ctx)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormat;

    TRACE("getting context...\n");

    ctx->restore_dc = pwglGetCurrentDC();
    ctx->restore_gl_ctx = pwglGetCurrentContext();

    /* We need a fake window as a hdc retrieved using GetDC(0) can't be used for much GL purposes. */
    ctx->wnd = CreateWindowA(WINED3D_OPENGL_WINDOW_CLASS_NAME, "WineD3D fake window",
            WS_OVERLAPPEDWINDOW, 10, 10, 10, 10, NULL, NULL, NULL, NULL);
    if (!ctx->wnd)
    {
        ERR_(d3d_caps)("Failed to create a window.\n");
        goto fail;
    }

    ctx->dc = GetDC(ctx->wnd);
    if (!ctx->dc)
    {
        ERR_(d3d_caps)("Failed to get a DC.\n");
        goto fail;
    }

    /* PixelFormat selection */
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW; /* PFD_GENERIC_ACCELERATED */
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iPixelFormat = ChoosePixelFormat(ctx->dc, &pfd);
    if (!iPixelFormat)
    {
        /* If this happens something is very wrong as ChoosePixelFormat barely fails. */
        ERR_(d3d_caps)("Can't find a suitable iPixelFormat.\n");
        goto fail;
    }
    DescribePixelFormat(ctx->dc, iPixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(ctx->dc, iPixelFormat, &pfd);

    /* Create a GL context. */
    ctx->gl_ctx = pwglCreateContext(ctx->dc);
    if (!ctx->gl_ctx)
    {
        WARN_(d3d_caps)("Error creating default context for capabilities initialization.\n");
        goto fail;
    }

    /* Make it the current GL context. */
    if (!pwglMakeCurrent(ctx->dc, ctx->gl_ctx))
    {
        ERR_(d3d_caps)("Failed to make fake GL context current.\n");
        goto fail;
    }

    return TRUE;

fail:
    if (ctx->gl_ctx) pwglDeleteContext(ctx->gl_ctx);
    ctx->gl_ctx = NULL;
    if (ctx->dc) ReleaseDC(ctx->wnd, ctx->dc);
    ctx->dc = NULL;
    if (ctx->wnd) DestroyWindow(ctx->wnd);
    ctx->wnd = NULL;
    if (ctx->restore_gl_ctx && !pwglMakeCurrent(ctx->restore_dc, ctx->restore_gl_ctx))
    {
        ERR_(d3d_caps)("Failed to restore previous GL context.\n");
    }

    return FALSE;
}

/* Adjust the amount of used texture memory */
unsigned int adapter_adjust_memory(struct wined3d_adapter *adapter, int amount)
{
    adapter->UsedTextureRam += amount;
    TRACE("Adjusted adapter memory by %d to %d.\n", amount, adapter->UsedTextureRam);
    return adapter->UsedTextureRam;
}

static void wined3d_adapter_cleanup(struct wined3d_adapter *adapter)
{
    HeapFree(GetProcessHeap(), 0, adapter->gl_info.formats);
    HeapFree(GetProcessHeap(), 0, adapter->cfgs);
}

ULONG CDECL wined3d_incref(struct wined3d *wined3d)
{
    ULONG refcount = InterlockedIncrement(&wined3d->ref);

    TRACE("%p increasing refcount to %u.\n", wined3d, refcount);

    return refcount;
}

ULONG CDECL wined3d_decref(struct wined3d *wined3d)
{
    ULONG refcount = InterlockedDecrement(&wined3d->ref);

    TRACE("%p decreasing refcount to %u.\n", wined3d, refcount);

    if (!refcount)
    {
        unsigned int i;

        for (i = 0; i < wined3d->adapter_count; ++i)
        {
            wined3d_adapter_cleanup(&wined3d->adapters[i]);
        }
        HeapFree(GetProcessHeap(), 0, wined3d);
    }

    return refcount;
}

/* GL locking is done by the caller */
static inline BOOL test_arb_vs_offset_limit(const struct wined3d_gl_info *gl_info)
{
    GLuint prog;
    BOOL ret = FALSE;
    const char *testcode =
        "!!ARBvp1.0\n"
        "PARAM C[66] = { program.env[0..65] };\n"
        "ADDRESS A0;"
        "PARAM zero = {0.0, 0.0, 0.0, 0.0};\n"
        "ARL A0.x, zero.x;\n"
        "MOV result.position, C[A0.x + 65];\n"
        "END\n";

    while(glGetError());
    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog) {
        ERR("Failed to create an ARB offset limit test program\n");
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  strlen(testcode), testcode));
    if (glGetError())
    {
        TRACE("OpenGL implementation does not allow indirect addressing offsets > 63\n");
        TRACE("error: %s\n", debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
    } else TRACE("OpenGL implementation allows offsets > 63\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("ARB vp offset limit test cleanup");

    return ret;
}

static DWORD ver_for_ext(enum wined3d_gl_extension ext)
{
    unsigned int i;
    for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i) {
        if(EXTENSION_MAP[i].extension == ext) {
            return EXTENSION_MAP[i].version;
        }
    }
    return 0;
}

static BOOL match_amd_r300_to_500(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_AMD) return FALSE;
    if (device == CARD_AMD_RADEON_9500) return TRUE;
    if (device == CARD_AMD_RADEON_X700) return TRUE;
    if (device == CARD_AMD_RADEON_X1600) return TRUE;
    return FALSE;
}

static BOOL match_geforce5(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor == HW_VENDOR_NVIDIA)
    {
        if (device == CARD_NVIDIA_GEFORCEFX_5200 ||
            device == CARD_NVIDIA_GEFORCEFX_5600 ||
            device == CARD_NVIDIA_GEFORCEFX_5800)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL match_apple(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* MacOS has various specialities in the extensions it advertises. Some have to be loaded from
     * the opengl 1.2+ core, while other extensions are advertised, but software emulated. So try to
     * detect the Apple OpenGL implementation to apply some extension fixups afterwards.
     *
     * Detecting this isn't really easy. The vendor string doesn't mention Apple. Compile-time checks
     * aren't sufficient either because a Linux binary may display on a macos X server via remote X11.
     * So try to detect the GL implementation by looking at certain Apple extensions. Some extensions
     * like client storage might be supported on other implementations too, but GL_APPLE_flush_render
     * is specific to the Mac OS X window management, and GL_APPLE_ycbcr_422 is QuickTime specific. So
     * the chance that other implementations support them is rather small since Win32 QuickTime uses
     * DirectDraw, not OpenGL.
     *
     * This test has been moved into wined3d_guess_gl_vendor()
     */
    if (gl_vendor == GL_VENDOR_APPLE)
    {
        return TRUE;
    }
    return FALSE;
}

/* Context activation is done by the caller. */
static void test_pbo_functionality(struct wined3d_gl_info *gl_info)
{
    /* Some OpenGL implementations, namely Apple's Geforce 8 driver, advertises PBOs,
     * but glTexSubImage from a PBO fails miserably, with the first line repeated over
     * all the texture. This function detects this bug by its symptom and disables PBOs
     * if the test fails.
     *
     * The test uploads a 4x4 texture via the PBO in the "native" format GL_BGRA,
     * GL_UNSIGNED_INT_8_8_8_8_REV. This format triggers the bug, and it is what we use
     * for D3DFMT_A8R8G8B8. Then the texture is read back without any PBO and the data
     * read back is compared to the original. If they are equal PBOs are assumed to work,
     * otherwise the PBO extension is disabled. */
    GLuint texture, pbo;
    static const unsigned int pattern[] =
    {
        0x00000000, 0x000000ff, 0x0000ff00, 0x40ff0000,
        0x80ffffff, 0x40ffff00, 0x00ff00ff, 0x0000ffff,
        0x00ffff00, 0x00ff00ff, 0x0000ffff, 0x000000ff,
        0x80ff00ff, 0x0000ffff, 0x00ff00ff, 0x40ff00ff
    };
    unsigned int check[sizeof(pattern) / sizeof(pattern[0])];

    /* No PBO -> No point in testing them. */
    if (!gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]) return;

    ENTER_GL();

    while (glGetError());
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    checkGLcall("Specifying the PBO test texture");

    GL_EXTCALL(glGenBuffersARB(1, &pbo));
    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo));
    GL_EXTCALL(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, sizeof(pattern), pattern, GL_STREAM_DRAW_ARB));
    checkGLcall("Specifying the PBO test pbo");

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("Loading the PBO test texture");

    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
    LEAVE_GL();

    wglFinish(); /* just to be sure */

    memset(check, 0, sizeof(check));
    ENTER_GL();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, check);
    checkGLcall("Reading back the PBO test texture");

    glDeleteTextures(1, &texture);
    GL_EXTCALL(glDeleteBuffersARB(1, &pbo));
    checkGLcall("PBO test cleanup");

    LEAVE_GL();

    if (memcmp(check, pattern, sizeof(check)))
    {
        WARN_(d3d_caps)("PBO test failed, read back data doesn't match original.\n");
        WARN_(d3d_caps)("Disabling PBOs. This may result in slower performance.\n");
        gl_info->supported[ARB_PIXEL_BUFFER_OBJECT] = FALSE;
    }
    else
    {
        TRACE_(d3d_caps)("PBO test successful.\n");
    }
}

static BOOL match_apple_intel(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return (card_vendor == HW_VENDOR_INTEL) && (gl_vendor == GL_VENDOR_APPLE);
}

static BOOL match_apple_nonr500ati(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (gl_vendor != GL_VENDOR_APPLE) return FALSE;
    if (card_vendor != HW_VENDOR_AMD) return FALSE;
    if (device == CARD_AMD_RADEON_X1600) return FALSE;
    return TRUE;
}

static BOOL match_dx10_capable(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* DX9 cards support 40 single float varyings in hardware, most drivers report 32. ATI misreports
     * 44 varyings. So assume that if we have more than 44 varyings we have a dx10 card.
     * This detection is for the gl_ClipPos varying quirk. If a d3d9 card really supports more than 44
     * varyings and we subtract one in dx9 shaders its not going to hurt us because the dx9 limit is
     * hardcoded
     *
     * dx10 cards usually have 64 varyings */
    return gl_info->limits.glsl_varyings > 44;
}

static BOOL match_not_dx10_capable(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return !match_dx10_capable(gl_info, gl_renderer, gl_vendor, card_vendor, device);
}

/* A GL context is provided by the caller */
static BOOL match_allows_spec_alpha(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLenum error;
    DWORD data[16];

    if (!gl_info->supported[EXT_SECONDARY_COLOR]) return FALSE;

    ENTER_GL();
    while(glGetError());
    GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE, 4, data);
    error = glGetError();
    LEAVE_GL();

    if(error == GL_NO_ERROR)
    {
        TRACE("GL Implementation accepts 4 component specular color pointers\n");
        return TRUE;
    }
    else
    {
        TRACE("GL implementation does not accept 4 component specular colors, error %s\n",
              debug_glerror(error));
        return FALSE;
    }
}

/* A GL context is provided by the caller */
static BOOL match_broken_nv_clip(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLuint prog;
    BOOL ret = FALSE;
    GLint pos;
    const char *testcode =
        "!!ARBvp1.0\n"
        "OPTION NV_vertex_program2;\n"
        "MOV result.clip[0], 0.0;\n"
        "MOV result.position, 0.0;\n"
        "END\n";

    if (!gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]) return FALSE;

    ENTER_GL();
    while(glGetError());

    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog)
    {
        ERR("Failed to create the NVvp clip test program\n");
        LEAVE_GL();
        return FALSE;
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  strlen(testcode), testcode));
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if(pos != -1)
    {
        WARN("GL_NV_vertex_program2_option result.clip[] test failed\n");
        TRACE("error: %s\n", debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
        while(glGetError());
    }
    else TRACE("GL_NV_vertex_program2_option result.clip[] test passed\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("GL_NV_vertex_program2_option result.clip[] test cleanup");

    LEAVE_GL();
    return ret;
}

/* Context activation is done by the caller. */
static BOOL match_fbo_tex_update(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    char data[4 * 4 * 4];
    GLuint tex, fbo;
    GLenum status;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return FALSE;

    memset(data, 0xcc, sizeof(data));

    ENTER_GL();

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("glTexImage2D");

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    checkGLcall("glFramebufferTexture2D");

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) ERR("FBO status %#x\n", status);
    checkGLcall("glCheckFramebufferStatus");

    memset(data, 0x11, sizeof(data));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glTexSubImage2D");

    glClearColor(0.996f, 0.729f, 0.745f, 0.792f);
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");

    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glGetTexImage");

    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLcall("glBindTexture");

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    checkGLcall("glDeleteTextures");

    LEAVE_GL();

    return *(DWORD *)data == 0x11111111;
}

/* Context activation is done by the caller. */
static BOOL match_broken_rgba16(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* GL_RGBA16 uses GL_RGBA8 internally on Geforce 7 and older cards.
     * This leads to graphical bugs in Half Life 2 and Unreal engine games. */
    GLuint tex;
    GLint size;

    ENTER_GL();

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
    checkGLcall("glTexImage2D");

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &size);
    checkGLcall("glGetTexLevelParameteriv");
    TRACE("Real color depth is %d\n", size);

    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLcall("glBindTexture");
    glDeleteTextures(1, &tex);
    checkGLcall("glDeleteTextures");

    LEAVE_GL();

    return size < 16;
}

static BOOL match_fglrx(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return gl_vendor == GL_VENDOR_FGLRX;
}

static void quirk_arb_constants(struct wined3d_gl_info *gl_info)
{
    TRACE_(d3d_caps)("Using ARB vs constant limit(=%u) for GLSL.\n", gl_info->limits.arb_vs_native_constants);
    gl_info->limits.glsl_vs_float_constants = gl_info->limits.arb_vs_native_constants;
    TRACE_(d3d_caps)("Using ARB ps constant limit(=%u) for GLSL.\n", gl_info->limits.arb_ps_native_constants);
    gl_info->limits.glsl_ps_float_constants = gl_info->limits.arb_ps_native_constants;
}

static void quirk_apple_glsl_constants(struct wined3d_gl_info *gl_info)
{
    quirk_arb_constants(gl_info);
    /* MacOS needs uniforms for relative addressing offsets. This can accumulate to quite a few uniforms.
     * Beyond that the general uniform isn't optimal, so reserve a number of uniforms. 12 vec4's should
     * allow 48 different offsets or other helper immediate values. */
    TRACE_(d3d_caps)("Reserving 12 GLSL constants for compiler private use.\n");
    gl_info->reserved_glsl_constants = max(gl_info->reserved_glsl_constants, 12);
}

static void quirk_amd_dx9(struct wined3d_gl_info *gl_info)
{
    /* MacOS advertises GL_ARB_texture_non_power_of_two on ATI r500 and earlier cards, although
     * these cards only support GL_ARB_texture_rectangle(D3DPTEXTURECAPS_NONPOW2CONDITIONAL).
     * If real NP2 textures are used, the driver falls back to software. We could just remove the
     * extension and use GL_ARB_texture_rectangle instead, but texture_rectangle is inconvenient
     * due to the non-normalized texture coordinates. Thus set an internal extension flag,
     * GL_WINE_normalized_texrect, which signals the code that it can use non power of two textures
     * as per GL_ARB_texture_non_power_of_two, but has to stick to the texture_rectangle limits.
     *
     * fglrx doesn't advertise GL_ARB_texture_non_power_of_two, but it advertises opengl 2.0 which
     * has this extension promoted to core. The extension loading code sets this extension supported
     * due to that, so this code works on fglrx as well. */
    if(gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        TRACE("GL_ARB_texture_non_power_of_two advertised on R500 or earlier card, removing.\n");
        gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
        gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT] = TRUE;
    }
}

static void quirk_no_np2(struct wined3d_gl_info *gl_info)
{
    /*  The nVidia GeForceFX series reports OpenGL 2.0 capabilities with the latest drivers versions, but
     *  doesn't explicitly advertise the ARB_tex_npot extension in the GL extension string.
     *  This usually means that ARB_tex_npot is supported in hardware as long as the application is staying
     *  within the limits enforced by the ARB_texture_rectangle extension. This however is not true for the
     *  FX series, which instantly falls back to a slower software path as soon as ARB_tex_npot is used.
     *  We therefore completely remove ARB_tex_npot from the list of supported extensions.
     *
     *  Note that wine_normalized_texrect can't be used in this case because internally it uses ARB_tex_npot,
     *  triggering the software fallback. There is not much we can do here apart from disabling the
     *  software-emulated extension and reenable ARB_tex_rect (which was previously disabled
     *  in wined3d_adapter_init_gl_caps).
     *  This fixup removes performance problems on both the FX 5900 and FX 5700 (e.g. for framebuffer
     *  post-processing effects in the game "Max Payne 2").
     *  The behaviour can be verified through a simple test app attached in bugreport #14724. */
    TRACE("GL_ARB_texture_non_power_of_two advertised through OpenGL 2.0 on NV FX card, removing.\n");
    gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
    gl_info->supported[ARB_TEXTURE_RECTANGLE] = TRUE;
}

static void quirk_texcoord_w(struct wined3d_gl_info *gl_info)
{
    /* The Intel GPUs on MacOS set the .w register of texcoords to 0.0 by default, which causes problems
     * with fixed function fragment processing. Ideally this flag should be detected with a test shader
     * and OpenGL feedback mode, but some GL implementations (MacOS ATI at least, probably all MacOS ones)
     * do not like vertex shaders in feedback mode and return an error, even though it should be valid
     * according to the spec.
     *
     * We don't want to enable this on all cards, as it adds an extra instruction per texcoord used. This
     * makes the shader slower and eats instruction slots which should be available to the d3d app.
     *
     * ATI Radeon HD 2xxx cards on MacOS have the issue. Instead of checking for the buggy cards, blacklist
     * all radeon cards on Macs and whitelist the good ones. That way we're prepared for the future. If
     * this workaround is activated on cards that do not need it, it won't break things, just affect
     * performance negatively. */
    TRACE("Enabling vertex texture coord fixes in vertex shaders.\n");
    gl_info->quirks |= WINED3D_QUIRK_SET_TEXCOORD_W;
}

static void quirk_clip_varying(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_GLSL_CLIP_VARYING;
}

static void quirk_allows_specular_alpha(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_ALLOWS_SPECULAR_ALPHA;
}

static void quirk_disable_nvvp_clip(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_NV_CLIP_BROKEN;
}

static void quirk_fbo_tex_update(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_FBO_TEX_UPDATE;
}

static void quirk_broken_rgba16(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_BROKEN_RGBA16;
}

static void quirk_infolog_spam(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_INFO_LOG_SPAM;
}

static void quirk_limited_tex_filtering(struct wined3d_gl_info *gl_info)
{
    /* Nvidia GeForce 6xxx and 7xxx support accelerated VTF only on a few
       selected texture formats. They are apparently the only DX9 class GPUs
       supporting VTF.
       Also, DX9-era GPUs are somewhat limited with float textures
       filtering and blending. */
    gl_info->quirks |= WINED3D_QUIRK_LIMITED_TEX_FILTERING;
}

struct driver_quirk
{
    BOOL (*match)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device);
    void (*apply)(struct wined3d_gl_info *gl_info);
    const char *description;
};

static const struct driver_quirk quirk_table[] =
{
    {
        match_amd_r300_to_500,
        quirk_amd_dx9,
        "AMD normalized texrect quirk"
    },
    /* MacOS advertises more GLSL vertex shader uniforms than supported by the hardware, and if more are
     * used it falls back to software. While the compiler can detect if the shader uses all declared
     * uniforms, the optimization fails if the shader uses relative addressing. So any GLSL shader
     * using relative addressing falls back to software.
     *
     * ARB vp gives the correct amount of uniforms, so use it instead of GLSL. */
    {
        match_apple,
        quirk_apple_glsl_constants,
        "Apple GLSL uniform override"
    },
    {
        match_geforce5,
        quirk_no_np2,
        "Geforce 5 NP2 disable"
    },
    {
        match_apple_intel,
        quirk_texcoord_w,
        "Init texcoord .w for Apple Intel GPU driver"
    },
    {
        match_apple_nonr500ati,
        quirk_texcoord_w,
        "Init texcoord .w for Apple ATI >= r600 GPU driver"
    },
    {
        match_dx10_capable,
        quirk_clip_varying,
        "Reserved varying for gl_ClipPos"
    },
    {
        /* GL_EXT_secondary_color does not allow 4 component secondary colors, but most
         * GL implementations accept it. The Mac GL is the only implementation known to
         * reject it.
         *
         * If we can pass 4 component specular colors, do it, because (a) we don't have
         * to screw around with the data, and (b) the D3D fixed function vertex pipeline
         * passes specular alpha to the pixel shader if any is used. Otherwise the
         * specular alpha is used to pass the fog coordinate, which we pass to opengl
         * via GL_EXT_fog_coord.
         */
        match_allows_spec_alpha,
        quirk_allows_specular_alpha,
        "Allow specular alpha quirk"
    },
    {
        match_broken_nv_clip,
        quirk_disable_nvvp_clip,
        "Apple NV_vertex_program clip bug quirk"
    },
    {
        match_fbo_tex_update,
        quirk_fbo_tex_update,
        "FBO rebind for attachment updates"
    },
    {
        match_broken_rgba16,
        quirk_broken_rgba16,
        "True RGBA16 is not available"
    },
    {
        match_fglrx,
        quirk_infolog_spam,
        "Not printing GLSL infolog"
    },
    {
        match_not_dx10_capable,
        quirk_limited_tex_filtering,
        "Texture filtering, blending and VTF support is limited"
    },
};

/* Certain applications (Steam) complain if we report an outdated driver version. In general,
 * reporting a driver version is moot because we are not the Windows driver, and we have different
 * bugs, features, etc.
 *
 * The driver version has the form "x.y.z.w".
 *
 * "x" is the Windows version the driver is meant for:
 * 4 -> 95/98/NT4
 * 5 -> 2000
 * 6 -> 2000/XP
 * 7 -> Vista
 * 8 -> Win 7
 *
 * "y" is the maximum Direct3D version the driver supports.
 * y  -> d3d version mapping:
 * 11 -> d3d6
 * 12 -> d3d7
 * 13 -> d3d8
 * 14 -> d3d9
 * 15 -> d3d10
 * 16 -> d3d10.1
 * 17 -> d3d11
 *
 * "z" is the subversion number.
 *
 * "w" is the vendor specific driver build number.
 */

struct driver_version_information
{
    enum wined3d_display_driver driver;
    enum wined3d_driver_model driver_model;
    const char *driver_name;            /* name of Windows driver */
    WORD version;                       /* version word ('y'), contained in low word of DriverVersion.HighPart */
    WORD subversion;                    /* subversion word ('z'), contained in high word of DriverVersion.LowPart */
    WORD build;                         /* build number ('w'), contained in low word of DriverVersion.LowPart */
};

/* The driver version table contains driver information for different devices on several OS versions. */
static const struct driver_version_information driver_version_table[] =
{
    /* AMD
     * - Radeon HD2x00 (R600) and up supported by current drivers.
     * - Radeon 9500 (R300) - X1*00 (R5xx) supported up to Catalyst 9.3 (Linux) and 10.2 (XP/Vista/Win7)
     * - Radeon 7xxx (R100) - 9250 (RV250) supported up to Catalyst 6.11 (XP)
     * - Rage 128 supported up to XP, latest official build 6.13.3279 dated October 2001 */
    {DRIVER_AMD_RAGE_128PRO,    DRIVER_MODEL_NT5X,  "ati2dvaa.dll", 13, 3279,  0},
    {DRIVER_AMD_R100,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 14, 10, 6614},
    {DRIVER_AMD_R300,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 14, 10, 6764},
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 14, 10, 8681},
    {DRIVER_AMD_R300,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 14, 10, 741 },
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 14, 10, 741 },

    /* Intel
     * The drivers are unified but not all versions support all GPUs. At some point the 2k/xp
     * drivers used ialmrnt5.dll for GMA800/GMA900 but at some point the file was renamed to
     * igxprd32.dll but the GMA800 driver was never updated. */
    {DRIVER_INTEL_GMA800,       DRIVER_MODEL_NT5X,  "ialmrnt5.dll", 14, 10, 3889},
    {DRIVER_INTEL_GMA900,       DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 4764},
    {DRIVER_INTEL_GMA950,       DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 4926},
    {DRIVER_INTEL_GMA3000,      DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 5218},
    {DRIVER_INTEL_GMA950,       DRIVER_MODEL_NT6X,  "igdumd32.dll", 14, 10, 1504},
    {DRIVER_INTEL_GMA3000,      DRIVER_MODEL_NT6X,  "igdumd32.dll", 15, 10, 1666},

    /* Nvidia
     * - Geforce6 and newer cards are supported by the current driver (197.x) on XP-Win7
     * - GeforceFX support is up to 173.x on <= XP
     * - Geforce2MX/3/4 up to 96.x on <= XP
     * - TNT/Geforce1/2 up to 71.x on <= XP
     * All version numbers used below are from the Linux nvidia drivers. */
    {DRIVER_NVIDIA_TNT,         DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 7186},
    {DRIVER_NVIDIA_GEFORCE2MX,  DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 9371},
    {DRIVER_NVIDIA_GEFORCEFX,   DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 11, 7516},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT5X,  "nv4_disp.dll", 15, 12, 6658},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT6X,  "nvd3dum.dll",  15, 12, 6658},
};

struct gpu_description
{
    WORD vendor;                    /* reported PCI card vendor ID  */
    WORD card;                      /* reported PCI card device ID  */
    const char *description;        /* Description of the card e.g. NVIDIA RIVA TNT */
    enum wined3d_display_driver driver;
    unsigned int vidmem;
};

/* The amount of video memory stored in the gpu description table is the minimum amount of video memory
 * found on a board containing a specific GPU. */
static const struct gpu_description gpu_description_table[] =
{
    /* Nvidia cards */
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT,           "NVIDIA RIVA TNT",                  DRIVER_NVIDIA_TNT,       16  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT2,          "NVIDIA RIVA TNT2/TNT2 Pro",        DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE,            "NVIDIA GeForce 256",               DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2,           "NVIDIA GeForce2 GTS/GeForce2 Pro", DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2_MX,        "NVIDIA GeForce2 MX/MX 400",        DRIVER_NVIDIA_GEFORCE2MX,32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE3,           "NVIDIA GeForce3",                  DRIVER_NVIDIA_GEFORCE2MX,64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_MX,        "NVIDIA GeForce4 MX 460",           DRIVER_NVIDIA_GEFORCE2MX,64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_TI4200,    "NVIDIA GeForce4 Ti 4200",          DRIVER_NVIDIA_GEFORCE2MX,64, },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5200,     "NVIDIA GeForce FX 5200",           DRIVER_NVIDIA_GEFORCEFX, 64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5600,     "NVIDIA GeForce FX 5600",           DRIVER_NVIDIA_GEFORCEFX, 128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5800,     "NVIDIA GeForce FX 5800",           DRIVER_NVIDIA_GEFORCEFX, 256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6200,       "NVIDIA GeForce 6200",              DRIVER_NVIDIA_GEFORCE6,  64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6600GT,     "NVIDIA GeForce 6600 GT",           DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6800,       "NVIDIA GeForce 6800",              DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7300,       "NVIDIA GeForce Go 7300",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7400,       "NVIDIA GeForce Go 7400",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7600,       "NVIDIA GeForce 7600 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7800GT,     "NVIDIA GeForce 7800 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8400GS,     "NVIDIA GeForce 8400 GS",           DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          DRIVER_NVIDIA_GEFORCE6,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          DRIVER_NVIDIA_GEFORCE6,  320 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTX,    "NVIDIA GeForce 8800 GTX",          DRIVER_NVIDIA_GEFORCE6,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           DRIVER_NVIDIA_GEFORCE6,  384 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           DRIVER_NVIDIA_GEFORCE6,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_210,        "NVIDIA GeForce 210",               DRIVER_NVIDIA_GEFORCE6,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT220,      "NVIDIA GeForce GT 220",            DRIVER_NVIDIA_GEFORCE6,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            DRIVER_NVIDIA_GEFORCE6,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           DRIVER_NVIDIA_GEFORCE6,  896 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT320M,     "NVIDIA GeForce GT 320M",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT325M,     "NVIDIA GeForce GT 325M",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT330,      "NVIDIA GeForce GT 330",            DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS350M,    "NVIDIA GeForce GTS 350M",          DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT420,      "NVIDIA GeForce GT 420",            DRIVER_NVIDIA_GEFORCE6,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT430,      "NVIDIA GeForce GT 430",            DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT440,      "NVIDIA GeForce GT 440",            DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS450,     "NVIDIA GeForce GTS 450",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460,     "NVIDIA GeForce GTX 460",           DRIVER_NVIDIA_GEFORCE6,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460M,    "NVIDIA GeForce GTX 460M",          DRIVER_NVIDIA_GEFORCE6,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX465,     "NVIDIA GeForce GTX 465",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX470,     "NVIDIA GeForce GTX 470",           DRIVER_NVIDIA_GEFORCE6,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX480,     "NVIDIA GeForce GTX 480",           DRIVER_NVIDIA_GEFORCE6,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT540M,     "NVIDIA GeForce GT 540M",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX550,     "NVIDIA GeForce GTX 550 Ti",        DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT555M,     "NVIDIA GeForce GT 555M",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560TI,   "NVIDIA GeForce GTX 560 Ti",        DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560,     "NVIDIA GeForce GTX 560",           DRIVER_NVIDIA_GEFORCE6,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX570,     "NVIDIA GeForce GTX 570",           DRIVER_NVIDIA_GEFORCE6,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX580,     "NVIDIA GeForce GTX 580",           DRIVER_NVIDIA_GEFORCE6,  1536},

    /* AMD cards */
    {HW_VENDOR_AMD,        CARD_AMD_RAGE_128PRO,           "ATI Rage Fury",                    DRIVER_AMD_RAGE_128PRO,  16  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_7200,           "ATI RADEON 7200 SERIES",           DRIVER_AMD_R100,         32  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_8500,           "ATI RADEON 8500 SERIES",           DRIVER_AMD_R100,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_9500,           "ATI Radeon 9500",                  DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_XPRESS_200M,    "ATI RADEON XPRESS 200M Series",    DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X700,           "ATI Radeon X700 SE",               DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X1600,          "ATI Radeon X1600 Series",          DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2350,         "ATI Mobility Radeon HD 2350",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2600,         "ATI Mobility Radeon HD 2600",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2900,         "ATI Radeon HD 2900 XT",            DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD3200,         "ATI Radeon HD 3200 Graphics",      DRIVER_AMD_R600,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4350,         "ATI Radeon HD 4350",               DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4600,         "ATI Radeon HD 4600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4700,         "ATI Radeon HD 4700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4800,         "ATI Radeon HD 4800 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5400,         "ATI Radeon HD 5400 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5600,         "ATI Radeon HD 5600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5700,         "ATI Radeon HD 5700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5800,         "ATI Radeon HD 5800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5900,         "ATI Radeon HD 5900 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6300,         "AMD Radeon HD 6300 series Graphics", DRIVER_AMD_R600,       1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6400,         "AMD Radeon HD 6400 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6410D,        "AMD Radeon HD 6410D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6550D,        "AMD Radeon HD 6550D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600,         "AMD Radeon HD 6600 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600M,        "AMD Radeon HD 6600M Series",       DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6800,         "AMD Radeon HD 6800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6900,         "AMD Radeon HD 6900 Series",        DRIVER_AMD_R600,         2048},
    /* Intel cards */
    {HW_VENDOR_INTEL,      CARD_INTEL_830M,                "Intel(R) 82830M Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_855GM,               "Intel(R) 82852/82855 GM/GME Graphics Controller",           DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_845G,                "Intel(R) 845G",                                             DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_865G,                "Intel(R) 82865G Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915G,                "Intel(R) 82915G/GV/910GL Express Chipset Family",           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_E7221G,              "Intel(R) E7221G",                                           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915GM,               "Mobile Intel(R) 915GM/GMS,910GML Express Chipset Family",   DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945G,                "Intel(R) 945G",                                             DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GM,               "Mobile Intel(R) 945GM Express Chipset Family",              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GME,              "Intel(R) 945GME",                                           DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q35,                 "Intel(R) Q35",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_G33,                 "Intel(R) G33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q33,                 "Intel(R) Q33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVG,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVM,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_965Q,                "Intel(R) 965Q",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965G,                "Intel(R) 965G",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_946GZ,               "Intel(R) 946GZ",                                            DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GM,               "Mobile Intel(R) 965 Express Chipset Family",                DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GME,              "Intel(R) 965GME",                                           DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_GM45,                "Mobile Intel(R) GM45 Express Chipset Family",               DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_IGD,                 "Intel(R) Integrated Graphics Device",                       DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G45,                 "Intel(R) G45/G43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_Q45,                 "Intel(R) Q45/Q43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G41,                 "Intel(R) G41",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_B43,                 "Intel(R) B43",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKD,                "Intel(R) Ironlake Desktop",                                 DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKM,                "Intel(R) Ironlake Mobile",                                  DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBD,                "Intel(R) Sandybridge Desktop",                              DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBM,                "Intel(R) Sandybridge Mobile",                               DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBS,                "Intel(R) Sandybridge Server",                               DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBD,                "Intel(R) Ivybridge Desktop",                                DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBM,                "Intel(R) Ivybridge Mobile",                                 DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBS,                "Intel(R) Ivybridge Server",                                 DRIVER_INTEL_GMA3000, 1024},
};

static const struct driver_version_information *get_driver_version_info(enum wined3d_display_driver driver,
        enum wined3d_driver_model driver_model)
{
    unsigned int i;

    TRACE("Looking up version info for driver=%d driver_model=%d\n", driver, driver_model);
    for (i = 0; i < (sizeof(driver_version_table) / sizeof(driver_version_table[0])); i++)
    {
        const struct driver_version_information *entry = &driver_version_table[i];

        if (entry->driver == driver && entry->driver_model == driver_model)
        {
            TRACE_(d3d_caps)("Found driver '%s' version=%d subversion=%d build=%d\n",
                entry->driver_name, entry->version, entry->subversion, entry->build);

            return entry;
        }
    }
    return NULL;
}

static void init_driver_info(struct wined3d_driver_info *driver_info,
        enum wined3d_pci_vendor vendor, enum wined3d_pci_device device)
{
    OSVERSIONINFOW os_version;
    WORD driver_os_version;
    unsigned int i;
    enum wined3d_display_driver driver = DRIVER_UNKNOWN;
    enum wined3d_driver_model driver_model;
    const struct driver_version_information *version_info;

    if (wined3d_settings.pci_vendor_id != PCI_VENDOR_NONE)
    {
        TRACE_(d3d_caps)("Overriding PCI vendor ID with: %04x\n", wined3d_settings.pci_vendor_id);
        vendor = wined3d_settings.pci_vendor_id;
    }
    driver_info->vendor = vendor;

    if (wined3d_settings.pci_device_id != PCI_DEVICE_NONE)
    {
        TRACE_(d3d_caps)("Overriding PCI device ID with: %04x\n", wined3d_settings.pci_device_id);
        device = wined3d_settings.pci_device_id;
    }
    driver_info->device = device;

    /* Set a default amount of video memory (64MB). In general this code isn't used unless the user
     * overrides the pci ids to a card which is not in our database. */
    driver_info->vidmem = WINE_DEFAULT_VIDMEM;

    memset(&os_version, 0, sizeof(os_version));
    os_version.dwOSVersionInfoSize = sizeof(os_version);
    if (!GetVersionExW(&os_version))
    {
        ERR("Failed to get OS version, reporting 2000/XP.\n");
        driver_os_version = 6;
        driver_model = DRIVER_MODEL_NT5X;
    }
    else
    {
        TRACE("OS version %u.%u.\n", os_version.dwMajorVersion, os_version.dwMinorVersion);
        switch (os_version.dwMajorVersion)
        {
            case 4:
                /* If needed we could distinguish between 9x and NT4, but this code won't make
                 * sense for NT4 since it had no way to obtain this info through DirectDraw 3.0.
                 */
                driver_os_version = 4;
                driver_model = DRIVER_MODEL_WIN9X;
                break;

            case 5:
                driver_os_version = 6;
                driver_model = DRIVER_MODEL_NT5X;
                break;

            case 6:
                if (os_version.dwMinorVersion == 0)
                {
                    driver_os_version = 7;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                else
                {
                    if (os_version.dwMinorVersion > 1)
                    {
                        FIXME("Unhandled OS version %u.%u, reporting Win 7.\n",
                                os_version.dwMajorVersion, os_version.dwMinorVersion);
                    }
                    driver_os_version = 8;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                break;

            default:
                FIXME("Unhandled OS version %u.%u, reporting 2000/XP.\n",
                        os_version.dwMajorVersion, os_version.dwMinorVersion);
                driver_os_version = 6;
                driver_model = DRIVER_MODEL_NT5X;
                break;
        }
    }

    /* When we reach this stage we always have a vendor or device id (it can be a default one).
     * This means that unless the ids are overriden, we will always find a GPU description. */
    for (i = 0; i < (sizeof(gpu_description_table) / sizeof(gpu_description_table[0])); i++)
    {
        if (vendor == gpu_description_table[i].vendor && device == gpu_description_table[i].card)
        {
            TRACE_(d3d_caps)("Found card %04x:%04x in driver DB.\n", vendor, device);

            driver_info->description = gpu_description_table[i].description;
            driver_info->vidmem = gpu_description_table[i].vidmem * 1024*1024;
            driver = gpu_description_table[i].driver;
            break;
        }
    }

    if (wined3d_settings.emulated_textureram)
    {
        TRACE_(d3d_caps)("Overriding amount of video memory with: %d byte\n", wined3d_settings.emulated_textureram);
        driver_info->vidmem = wined3d_settings.emulated_textureram;
    }

    /* Try to obtain driver version information for the current Windows version. This fails in
     * some cases:
     * - the gpu is not available on the currently selected OS version:
     *   - Geforce GTX480 on Win98. When running applications in compatibility mode on Windows,
     *     version information for the current Windows version is returned instead of faked info.
     *     We do the same and assume the default Windows version to emulate is WinXP.
     *
     *   - Videocard is a Riva TNT but winver is set to win7 (there are no drivers for this beast)
     *     For now return the XP driver info. Perhaps later on we should return VESA.
     *
     * - the gpu is not in our database (can happen when the user overrides the vendor_id / device_id)
     *   This could be an indication that our database is not up to date, so this should be fixed.
     */
    version_info = get_driver_version_info(driver, driver_model);
    if (version_info)
    {
        driver_info->name = version_info->driver_name;
        driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, version_info->version);
        driver_info->version_low = MAKEDWORD_VERSION(version_info->subversion, version_info->build);
    }
    else
    {
        version_info = get_driver_version_info(driver, DRIVER_MODEL_NT5X);
        if (version_info)
        {
            driver_info->name = version_info->driver_name;
            driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, version_info->version);
            driver_info->version_low = MAKEDWORD_VERSION(version_info->subversion, version_info->build);
        }
        else
        {
            driver_info->description = "Direct3D HAL";
            driver_info->name = "Display";
            driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, 15);
            driver_info->version_low = MAKEDWORD_VERSION(8, 6); /* Nvidia RIVA TNT, arbitrary */

            FIXME("Unable to find a driver/device info for vendor_id=%#x device_id=%#x for driver_model=%d\n",
                    vendor, device, driver_model);
        }
    }

    TRACE_(d3d_caps)("Reporting (fake) driver version 0x%08x-0x%08x.\n",
            driver_info->version_high, driver_info->version_low);
}

/* Context activation is done by the caller. */
static void fixup_extensions(struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    unsigned int i;

    for (i = 0; i < (sizeof(quirk_table) / sizeof(*quirk_table)); ++i)
    {
        if (!quirk_table[i].match(gl_info, gl_renderer, gl_vendor, card_vendor, device)) continue;
        TRACE_(d3d_caps)("Applying driver quirk \"%s\".\n", quirk_table[i].description);
        quirk_table[i].apply(gl_info);
    }

    /* Find out if PBOs work as they are supposed to. */
    test_pbo_functionality(gl_info);
}

static DWORD wined3d_parse_gl_version(const char *gl_version)
{
    const char *ptr = gl_version;
    int major, minor;

    major = atoi(ptr);
    if (major <= 0) ERR_(d3d_caps)("Invalid opengl major version: %d.\n", major);

    while (isdigit(*ptr)) ++ptr;
    if (*ptr++ != '.') ERR_(d3d_caps)("Invalid opengl version string: %s.\n", debugstr_a(gl_version));

    minor = atoi(ptr);

    TRACE_(d3d_caps)("Found OpenGL version: %d.%d.\n", major, minor);

    return MAKEDWORD_VERSION(major, minor);
}

static enum wined3d_gl_vendor wined3d_guess_gl_vendor(const struct wined3d_gl_info *gl_info,
        const char *gl_vendor_string, const char *gl_renderer)
{

    /* MacOS has various specialities in the extensions it advertises. Some have to be loaded from
     * the opengl 1.2+ core, while other extensions are advertised, but software emulated. So try to
     * detect the Apple OpenGL implementation to apply some extension fixups afterwards.
     *
     * Detecting this isn't really easy. The vendor string doesn't mention Apple. Compile-time checks
     * aren't sufficient either because a Linux binary may display on a macos X server via remote X11.
     * So try to detect the GL implementation by looking at certain Apple extensions. Some extensions
     * like client storage might be supported on other implementations too, but GL_APPLE_flush_render
     * is specific to the Mac OS X window management, and GL_APPLE_ycbcr_422 is QuickTime specific. So
     * the chance that other implementations support them is rather small since Win32 QuickTime uses
     * DirectDraw, not OpenGL. */
    if (gl_info->supported[APPLE_FENCE]
            && gl_info->supported[APPLE_CLIENT_STORAGE]
            && gl_info->supported[APPLE_FLUSH_RENDER]
            && gl_info->supported[APPLE_YCBCR_422])
        return GL_VENDOR_APPLE;

    if (strstr(gl_vendor_string, "NVIDIA"))
        return GL_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI"))
        return GL_VENDOR_FGLRX;

    if (strstr(gl_vendor_string, "Intel(R)")
            /* Intel switched from Intel(R) to Intel® recently, so just match Intel. */
            || strstr(gl_renderer, "Intel")
            || strstr(gl_vendor_string, "Intel Inc."))
        return GL_VENDOR_INTEL;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "DRI R300 Project")
            || strstr(gl_vendor_string, "X.Org R300 Project")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc.")
            || strstr(gl_renderer, "Mesa")
            || strstr(gl_renderer, "Gallium"))
        return GL_VENDOR_MESA;

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning GL_VENDOR_UNKNOWN.\n",
            debugstr_a(gl_vendor_string));

    return GL_VENDOR_UNKNOWN;
}

static enum wined3d_pci_vendor wined3d_guess_card_vendor(const char *gl_vendor_string, const char *gl_renderer)
{
    if (strstr(gl_vendor_string, "NVIDIA")
            || strstr(gl_vendor_string, "nouveau"))
        return HW_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "X.Org R300 Project")
            || strstr(gl_renderer, "AMD")
            || strstr(gl_renderer, "R100")
            || strstr(gl_renderer, "R200")
            || strstr(gl_renderer, "R300")
            || strstr(gl_renderer, "R600")
            || strstr(gl_renderer, "R700"))
        return HW_VENDOR_AMD;

    if (strstr(gl_vendor_string, "Intel(R)")
            /* Intel switched from Intel(R) to Intel® recently, so just match Intel. */
            || strstr(gl_renderer, "Intel")
            || strstr(gl_renderer, "i915")
            || strstr(gl_vendor_string, "Intel Inc."))
        return HW_VENDOR_INTEL;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Brian Paul")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc."))
        return HW_VENDOR_SOFTWARE;

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning HW_VENDOR_NVIDIA.\n", debugstr_a(gl_vendor_string));

    return HW_VENDOR_NVIDIA;
}

static UINT d3d_level_from_gl_info(const struct wined3d_gl_info *gl_info)
{
    UINT level = 0;

    if (gl_info->supported[ARB_MULTITEXTURE])
        level = 6;
    if (gl_info->supported[ARB_TEXTURE_COMPRESSION]
            && gl_info->supported[ARB_TEXTURE_CUBE_MAP]
            && gl_info->supported[ARB_TEXTURE_ENV_DOT3])
        level = 7;
    if (level == 7 && gl_info->supported[ARB_MULTISAMPLE]
            && gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
        level = 8;
    if (level == 8 && gl_info->supported[ARB_FRAGMENT_PROGRAM]
            && gl_info->supported[ARB_VERTEX_SHADER])
        level = 9;
    if (level == 9 && gl_info->supported[EXT_GPU_SHADER4])
        level = 10;

    return level;
}

static enum wined3d_pci_device select_card_nvidia_binary(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer)
{
    UINT d3d_level = d3d_level_from_gl_info(gl_info);
    unsigned int i;

    if (d3d_level >= 10)
    {
        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            {"GTX 580",     CARD_NVIDIA_GEFORCE_GTX580},    /* Geforce 500 - highend */
            {"GTX 570",     CARD_NVIDIA_GEFORCE_GTX570},    /* Geforce 500 - midend high */
            {"GTX 560 Ti",  CARD_NVIDIA_GEFORCE_GTX560TI},  /* Geforce 500 - midend */
            {"GTX 560",     CARD_NVIDIA_GEFORCE_GTX560},    /* Geforce 500 - midend */
            {"GT 555M",     CARD_NVIDIA_GEFORCE_GT555M},    /* Geforce 500 - midend mobile */
            {"GTX 550 Ti",  CARD_NVIDIA_GEFORCE_GTX550},    /* Geforce 500 - midend */
            {"GT 540M",     CARD_NVIDIA_GEFORCE_GT540M},    /* Geforce 500 - midend mobile */
            {"GTX 480",     CARD_NVIDIA_GEFORCE_GTX480},    /* Geforce 400 - highend */
            {"GTX 470",     CARD_NVIDIA_GEFORCE_GTX470},    /* Geforce 400 - midend high */
            {"GTX 465",     CARD_NVIDIA_GEFORCE_GTX465},    /* Geforce 400 - midend */
            {"GTX 460M",    CARD_NVIDIA_GEFORCE_GTX460M},   /* Geforce 400 - highend mobile */
            {"GTX 460",     CARD_NVIDIA_GEFORCE_GTX460},    /* Geforce 400 - midend */
            {"GTS 450",     CARD_NVIDIA_GEFORCE_GTS450},    /* Geforce 400 - midend low */
            {"GT 440",      CARD_NVIDIA_GEFORCE_GT440},     /* Geforce 400 - lowend */
            {"GT 430",      CARD_NVIDIA_GEFORCE_GT430},     /* Geforce 400 - lowend */
            {"GT 420",      CARD_NVIDIA_GEFORCE_GT420},     /* Geforce 400 - lowend */
            {"GT 330",      CARD_NVIDIA_GEFORCE_GT330},     /* Geforce 300 - highend */
            {"GTS 360M",    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
            {"GTS 350M",    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
            {"GT 330M",     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
            {"GT 325M",     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
            {"GT 320M",     CARD_NVIDIA_GEFORCE_GT320M},    /* Geforce 300 - midend mobile */
            {"GTX 295",     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
            {"GTX 285",     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
            {"GTX 280",     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
            {"GTX 275",     CARD_NVIDIA_GEFORCE_GTX275},    /* Geforce 200 - midend high */
            {"GTX 260",     CARD_NVIDIA_GEFORCE_GTX260},    /* Geforce 200 - midend */
            {"GT 240",      CARD_NVIDIA_GEFORCE_GT240},     /* Geforce 200 - midend */
            {"GT 220",      CARD_NVIDIA_GEFORCE_GT220},     /* Geforce 200 - lowend */
            {"Geforce 310", CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
            {"Geforce 305", CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
            {"Geforce 210", CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
            {"G 210",       CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
            {"GTS 250",     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
            {"GTS 150",     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
            {"9800",        CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
            {"GT 140",      CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
            {"9600",        CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
            {"GT 130",      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
            {"GT 120",      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
            {"9500",        CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
            {"9400",        CARD_NVIDIA_GEFORCE_9400GT},    /* Geforce 9 - lowend */
            {"9300",        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
            {"9200",        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
            {"9100",        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
            {"G 100",       CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
            {"8800 GTX",    CARD_NVIDIA_GEFORCE_8800GTX},   /* Geforce 8 - highend high */
            {"8800",        CARD_NVIDIA_GEFORCE_8800GTS},   /* Geforce 8 - highend */
            {"8600 M",      CARD_NVIDIA_GEFORCE_8600MGT},   /* Geforce 8 - midend mobile */
            {"8700",        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
            {"8600",        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
            {"8500",        CARD_NVIDIA_GEFORCE_8400GS},    /* Geforce 8 - mid-lowend */
            {"8400",        CARD_NVIDIA_GEFORCE_8400GS},    /* Geforce 8 - mid-lowend */
            {"8300",        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
            {"8200",        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
            {"8100",        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }

        /* Geforce8-compatible fall back if the GPU is not in the list yet */
        return CARD_NVIDIA_GEFORCE_8300GS;
    }

    /* Both the GeforceFX, 6xxx and 7xxx series support D3D9. The last two types have more
     * shader capabilities, so we use the shader capabilities to distinguish between FX and 6xxx/7xxx.
     */
    if (d3d_level >= 9 && gl_info->supported[NV_VERTEX_PROGRAM3])
    {
        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            {"Quadro FX 5", CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
            {"Quadro FX 4", CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
            {"7950",        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
            {"7900",        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
            {"7800",        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
            {"7700",        CARD_NVIDIA_GEFORCE_7600},      /* Geforce 7 - midend */
            {"7600",        CARD_NVIDIA_GEFORCE_7600},      /* Geforce 7 - midend */
            {"7400",        CARD_NVIDIA_GEFORCE_7400},      /* Geforce 7 - lower medium */
            {"7300",        CARD_NVIDIA_GEFORCE_7300},      /* Geforce 7 - lowend */
            {"6800",        CARD_NVIDIA_GEFORCE_6800},      /* Geforce 6 - highend */
            {"6700",        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
            {"6610",        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
            {"6600",        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }

        /* Geforce 6/7 - lowend */
        return CARD_NVIDIA_GEFORCE_6200; /* Geforce 6100/6150/6200/7300/7400/7500 */
    }

    if (d3d_level >= 9)
    {
        /* GeforceFX - highend */
        if (strstr(gl_renderer, "5800")
                || strstr(gl_renderer, "5900")
                || strstr(gl_renderer, "5950")
                || strstr(gl_renderer, "Quadro FX"))
        {
            return CARD_NVIDIA_GEFORCEFX_5800;
        }

        /* GeforceFX - midend */
        if (strstr(gl_renderer, "5600")
                || strstr(gl_renderer, "5650")
                || strstr(gl_renderer, "5700")
                || strstr(gl_renderer, "5750"))
        {
            return CARD_NVIDIA_GEFORCEFX_5600;
        }

        /* GeforceFX - lowend */
        return CARD_NVIDIA_GEFORCEFX_5200; /* GeforceFX 5100/5200/5250/5300/5500 */
    }

    if (d3d_level >= 8)
    {
        if (strstr(gl_renderer, "GeForce4 Ti") || strstr(gl_renderer, "Quadro4"))
        {
            return CARD_NVIDIA_GEFORCE4_TI4200; /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800, Quadro4 */
        }

        return CARD_NVIDIA_GEFORCE3; /* Geforce3 standard/Ti200/Ti500, Quadro DCC */
    }

    if (d3d_level >= 7)
    {
        if (strstr(gl_renderer, "GeForce4 MX"))
        {
            return CARD_NVIDIA_GEFORCE4_MX; /* MX420/MX440/MX460/MX4000 */
        }

        if (strstr(gl_renderer, "GeForce2 MX") || strstr(gl_renderer, "Quadro2 MXR"))
        {
            return CARD_NVIDIA_GEFORCE2_MX; /* Geforce2 standard/MX100/MX200/MX400, Quadro2 MXR */
        }

        if (strstr(gl_renderer, "GeForce2") || strstr(gl_renderer, "Quadro2"))
        {
            return CARD_NVIDIA_GEFORCE2; /* Geforce2 GTS/Pro/Ti/Ultra, Quadro2 */
        }

        return CARD_NVIDIA_GEFORCE; /* Geforce 256/DDR, Quadro */
    }

    if (strstr(gl_renderer, "TNT2"))
    {
        return CARD_NVIDIA_RIVA_TNT2; /* Riva TNT2 standard/M64/Pro/Ultra */
    }

    return CARD_NVIDIA_RIVA_TNT; /* Riva TNT, Vanta */
}

static enum wined3d_pci_device select_card_amd_binary(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer)
{
    UINT d3d_level = d3d_level_from_gl_info(gl_info);

    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
    if (d3d_level >= 10)
    {
        unsigned int i;

        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            /* Northern Islands */
            {"HD 6900", CARD_AMD_RADEON_HD6900},
            {"HD 6800", CARD_AMD_RADEON_HD6800},
            {"HD 6630M",CARD_AMD_RADEON_HD6600M},
            {"HD 6600M",CARD_AMD_RADEON_HD6600M},
            {"HD 6600", CARD_AMD_RADEON_HD6600},
            {"HD 6500M",CARD_AMD_RADEON_HD6600M},
            {"HD 6500", CARD_AMD_RADEON_HD6600},
            {"HD 6400", CARD_AMD_RADEON_HD6400},
            {"HD 6300", CARD_AMD_RADEON_HD6300},
            {"HD 6200", CARD_AMD_RADEON_HD6300},
            /* Evergreen */
            {"HD 5870", CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS PRO */
            {"HD 5850", CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS XT */
            {"HD 5800", CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS HD58xx generic renderer string */
            {"HD 5770", CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER XT */
            {"HD 5750", CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER LE */
            {"HD 5700", CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER HD57xx generic renderer string */
            {"HD 5670", CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD XT */
            {"HD 5570", CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD PRO mapped to HD5600 series */
            {"HD 5550", CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD LE mapped to HD5600 series */
            {"HD 5450", CARD_AMD_RADEON_HD5400},    /* Radeon EG CEDAR PRO */
            /* R700 */
            {"HD 4890", CARD_AMD_RADEON_HD4800},    /* Radeon RV790 */
            {"HD 4870", CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
            {"HD 4850", CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
            {"HD 4830", CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
            {"HD 4800", CARD_AMD_RADEON_HD4800},    /* Radeon RV7xx HD48xx generic renderer string */
            {"HD 4770", CARD_AMD_RADEON_HD4700},    /* Radeon RV740 */
            {"HD 4700", CARD_AMD_RADEON_HD4700},    /* Radeon RV7xx HD47xx generic renderer string */
            {"HD 4670", CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
            {"HD 4650", CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
            {"HD 4600", CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
            {"HD 4550", CARD_AMD_RADEON_HD4350},    /* Radeon RV710 */
            {"HD 4350", CARD_AMD_RADEON_HD4350},    /* Radeon RV710 */
            /* R600/R700 integrated */
            {"HD 3300", CARD_AMD_RADEON_HD3200},
            {"HD 3200", CARD_AMD_RADEON_HD3200},
            {"HD 3100", CARD_AMD_RADEON_HD3200},
            /* R600 */
            {"HD 3870", CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
            {"HD 3850", CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
            {"HD 2900", CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
            {"HD 3830", CARD_AMD_RADEON_HD2600},    /* China-only midend */
            {"HD 3690", CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
            {"HD 3650", CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
            {"HD 2600", CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
            {"HD 3470", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"HD 3450", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"HD 3430", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"HD 3400", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"HD 2400", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"HD 2350", CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }

        /* Default for when no GPU has been found */
        return CARD_AMD_RADEON_HD3200;
    }

    if (d3d_level >= 9)
    {
        /* Radeon R5xx */
        if (strstr(gl_renderer, "X1600")
                || strstr(gl_renderer, "X1650")
                || strstr(gl_renderer, "X1800")
                || strstr(gl_renderer, "X1900")
                || strstr(gl_renderer, "X1950"))
        {
            return CARD_AMD_RADEON_X1600;
        }

        /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300/X2500/HD2300 (lowend R5xx)
         * Note X2300/X2500/HD2300 are R5xx GPUs with a 2xxx naming but they are still DX9-only */
        if (strstr(gl_renderer, "X700")
                || strstr(gl_renderer, "X800")
                || strstr(gl_renderer, "X850")
                || strstr(gl_renderer, "X1300")
                || strstr(gl_renderer, "X1400")
                || strstr(gl_renderer, "X1450")
                || strstr(gl_renderer, "X1550")
                || strstr(gl_renderer, "X2300")
                || strstr(gl_renderer, "X2500")
                || strstr(gl_renderer, "HD 2300")
                )
        {
            return CARD_AMD_RADEON_X700;
        }

        /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400MHz */
        if (strstr(gl_renderer, "Radeon Xpress"))
        {
            return CARD_AMD_RADEON_XPRESS_200M;
        }

        /* Radeon R3xx */
        return CARD_AMD_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
    }

    if (d3d_level >= 8)
        return CARD_AMD_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */

    if (d3d_level >= 7)
        return CARD_AMD_RADEON_7200; /* Radeon 7000/7100/7200/7500 */

    return CARD_AMD_RAGE_128PRO;
}

static enum wined3d_pci_device select_card_intel(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer)
{
    unsigned int i;

    static const struct
    {
        const char *renderer;
        enum wined3d_pci_device id;
    }
    cards[] =
    {
        /* Ivybridge */
        {"Ivybridge Server",            CARD_INTEL_IVBS},
        {"Ivybridge Mobile",            CARD_INTEL_IVBM},
        {"Ivybridge Desktop",           CARD_INTEL_IVBD},
        /* Sandybridge */
        {"Sandybridge Server",          CARD_INTEL_SNBS},
        {"Sandybridge Mobile",          CARD_INTEL_SNBM},
        {"Sandybridge Desktop",         CARD_INTEL_SNBD},
        /* Ironlake */
        {"Ironlake Mobile",             CARD_INTEL_ILKM},
        {"Ironlake Desktop",            CARD_INTEL_ILKD},
        /* G4x */
        {"B43",                         CARD_INTEL_B43},
        {"G41",                         CARD_INTEL_G41},
        {"G45",                         CARD_INTEL_G45},
        {"Q45",                         CARD_INTEL_Q45},
        {"Integrated Graphics Device",  CARD_INTEL_IGD},
        {"GM45",                        CARD_INTEL_GM45},
        /* i965 */
        {"965GME",                      CARD_INTEL_965GME},
        {"965GM",                       CARD_INTEL_965GM},
        {"X3100",                       CARD_INTEL_965GM},  /* MacOS */
        {"946GZ",                       CARD_INTEL_946GZ},
        {"965G",                        CARD_INTEL_965G},
        {"965Q",                        CARD_INTEL_965Q},
        /* i945 */
        {"Pineview M",                  CARD_INTEL_PNVM},
        {"Pineview G",                  CARD_INTEL_PNVG},
        {"IGD",                         CARD_INTEL_PNVG},
        {"Q33",                         CARD_INTEL_Q33},
        {"G33",                         CARD_INTEL_G33},
        {"Q35",                         CARD_INTEL_Q35},
        {"945GME",                      CARD_INTEL_945GME},
        {"945GM",                       CARD_INTEL_945GM},
        {"GMA 950",                     CARD_INTEL_945GM},  /* MacOS */
        {"945G",                        CARD_INTEL_945G},
        /* i915 */
        {"915GM",                       CARD_INTEL_915GM},
        {"E7221G",                      CARD_INTEL_E7221G},
        {"915G",                        CARD_INTEL_915G},
        /* i8xx */
        {"865G",                        CARD_INTEL_865G},
        {"845G",                        CARD_INTEL_845G},
        {"855GM",                       CARD_INTEL_855GM},
        {"830M",                        CARD_INTEL_830M},
    };

    for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
    {
        if (strstr(gl_renderer, cards[i].renderer))
            return cards[i].id;
    }

    return CARD_INTEL_915G;
}

static enum wined3d_pci_device select_card_amd_mesa(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer)
{
    UINT d3d_level;
    unsigned int i;

    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
    if (strstr(gl_renderer, "Gallium"))
    {
        /* 20101109 - These are never returned by current Gallium radeon
         * drivers: R700, RV790, R680, RV535, RV516, R410, RS485, RV360, RV351.
         *
         * These are returned but not handled: RC410, RV380. */
        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            /* Northern Islands */
            {"CAYMAN",  CARD_AMD_RADEON_HD6900},
            {"BARTS",   CARD_AMD_RADEON_HD6800},
            {"TURKS",   CARD_AMD_RADEON_HD6600},
            {"SUMO2",   CARD_AMD_RADEON_HD6410D},   /* SUMO2 first, because we do a strstr(). */
            {"SUMO",    CARD_AMD_RADEON_HD6550D},
            {"CAICOS",  CARD_AMD_RADEON_HD6400},
            {"PALM",    CARD_AMD_RADEON_HD6300},
            /* Evergreen */
            {"HEMLOCK", CARD_AMD_RADEON_HD5900},
            {"CYPRESS", CARD_AMD_RADEON_HD5800},
            {"JUNIPER", CARD_AMD_RADEON_HD5700},
            {"REDWOOD", CARD_AMD_RADEON_HD5600},
            {"CEDAR",   CARD_AMD_RADEON_HD5400},
            /* R700 */
            {"R700",    CARD_AMD_RADEON_HD4800},    /* HD4800 - highend */
            {"RV790",   CARD_AMD_RADEON_HD4800},
            {"RV770",   CARD_AMD_RADEON_HD4800},
            {"RV740",   CARD_AMD_RADEON_HD4700},    /* HD4700 - midend */
            {"RV730",   CARD_AMD_RADEON_HD4600},    /* HD4600 - midend */
            {"RV710",   CARD_AMD_RADEON_HD4350},    /* HD4500/HD4350 - lowend */
            /* R600/R700 integrated */
            {"RS880",   CARD_AMD_RADEON_HD3200},
            {"RS780",   CARD_AMD_RADEON_HD3200},
            /* R600 */
            {"R680",    CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
            {"R600",    CARD_AMD_RADEON_HD2900},
            {"RV670",   CARD_AMD_RADEON_HD2900},
            {"RV635",   CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend; HD3830 is China-only midend */
            {"RV630",   CARD_AMD_RADEON_HD2600},
            {"RV620",   CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
            {"RV610",   CARD_AMD_RADEON_HD2350},
            /* R500 */
            {"R580",    CARD_AMD_RADEON_X1600},
            {"R520",    CARD_AMD_RADEON_X1600},
            {"RV570",   CARD_AMD_RADEON_X1600},
            {"RV560",   CARD_AMD_RADEON_X1600},
            {"RV535",   CARD_AMD_RADEON_X1600},
            {"RV530",   CARD_AMD_RADEON_X1600},
            {"RV516",   CARD_AMD_RADEON_X700},      /* X700 is actually R400. */
            {"RV515",   CARD_AMD_RADEON_X700},
            /* R400 */
            {"R481",    CARD_AMD_RADEON_X700},
            {"R480",    CARD_AMD_RADEON_X700},
            {"R430",    CARD_AMD_RADEON_X700},
            {"R423",    CARD_AMD_RADEON_X700},
            {"R420",    CARD_AMD_RADEON_X700},
            {"R410",    CARD_AMD_RADEON_X700},
            {"RV410",   CARD_AMD_RADEON_X700},
            /* Radeon Xpress - onboard, DX9b, Shader 2.0, 300-400MHz */
            {"RS740",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS690",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS600",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS485",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS482",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS480",   CARD_AMD_RADEON_XPRESS_200M},
            {"RS400",   CARD_AMD_RADEON_XPRESS_200M},
            /* R300 */
            {"R360",    CARD_AMD_RADEON_9500},
            {"R350",    CARD_AMD_RADEON_9500},
            {"R300",    CARD_AMD_RADEON_9500},
            {"RV370",   CARD_AMD_RADEON_9500},
            {"RV360",   CARD_AMD_RADEON_9500},
            {"RV351",   CARD_AMD_RADEON_9500},
            {"RV350",   CARD_AMD_RADEON_9500},
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }
    }

    d3d_level = d3d_level_from_gl_info(gl_info);
    if (d3d_level >= 10)
        return CARD_AMD_RADEON_HD2600;

    if (d3d_level >= 9)
    {
        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            /* R700 */
            {"(R700",   CARD_AMD_RADEON_HD4800},    /* HD4800 - highend */
            {"(RV790",  CARD_AMD_RADEON_HD4800},
            {"(RV770",  CARD_AMD_RADEON_HD4800},
            {"(RV740",  CARD_AMD_RADEON_HD4700},    /* HD4700 - midend */
            {"(RV730",  CARD_AMD_RADEON_HD4600},    /* HD4600 - midend */
            {"(RV710",  CARD_AMD_RADEON_HD4350},    /* HD4500/HD4350 - lowend */
            /* R600/R700 integrated */
            {"RS880",   CARD_AMD_RADEON_HD3200},
            {"RS780",   CARD_AMD_RADEON_HD3200},
            /* R600 */
            {"(R680",   CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
            {"(R600",   CARD_AMD_RADEON_HD2900},
            {"(RV670",  CARD_AMD_RADEON_HD2900},
            {"(RV635",  CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend; HD3830 is China-only midend */
            {"(RV630",  CARD_AMD_RADEON_HD2600},
            {"(RV620",  CARD_AMD_RADEON_HD2350},    /* HD2300/HD2400/HD3400 - lowend */
            {"(RV610",  CARD_AMD_RADEON_HD2350},
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }

        return CARD_AMD_RADEON_9500;
    }

    if (d3d_level >= 8)
        return CARD_AMD_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */

    if (d3d_level >= 7)
        return CARD_AMD_RADEON_7200; /* Radeon 7000/7100/7200/7500 */

    return CARD_AMD_RAGE_128PRO;
}

static enum wined3d_pci_device select_card_nvidia_mesa(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer)
{
    UINT d3d_level;

    if (strstr(gl_renderer, "Gallium"))
    {
        unsigned int i;

        static const struct
        {
            const char *renderer;
            enum wined3d_pci_device id;
        }
        cards[] =
        {
            {"NVC8",    CARD_NVIDIA_GEFORCE_GTX570},
            {"NVC4",    CARD_NVIDIA_GEFORCE_GTX460},
            {"NVC3",    CARD_NVIDIA_GEFORCE_GT440},
            {"NVC0",    CARD_NVIDIA_GEFORCE_GTX480},
            {"NVAF",    CARD_NVIDIA_GEFORCE_GT320M},
            {"NVAC",    CARD_NVIDIA_GEFORCE_8200},
            {"NVAA",    CARD_NVIDIA_GEFORCE_8200},
            {"NVA8",    CARD_NVIDIA_GEFORCE_210},
            {"NVA5",    CARD_NVIDIA_GEFORCE_GT220},
            {"NVA3",    CARD_NVIDIA_GEFORCE_GT240},
            {"NVA0",    CARD_NVIDIA_GEFORCE_GTX280},
            {"NV98",    CARD_NVIDIA_GEFORCE_9200},
            {"NV96",    CARD_NVIDIA_GEFORCE_9400GT},
            {"NV94",    CARD_NVIDIA_GEFORCE_9600GT},
            {"NV92",    CARD_NVIDIA_GEFORCE_9800GT},
            {"NV86",    CARD_NVIDIA_GEFORCE_8500GT},
            {"NV84",    CARD_NVIDIA_GEFORCE_8600GT},
            {"NV68",    CARD_NVIDIA_GEFORCE_6200},      /* 7050 */
            {"NV67",    CARD_NVIDIA_GEFORCE_6200},      /* 7000M */
            {"NV63",    CARD_NVIDIA_GEFORCE_6200},      /* 7100 */
            {"NV50",    CARD_NVIDIA_GEFORCE_8800GTX},
            {"NV4E",    CARD_NVIDIA_GEFORCE_6200},      /* 6100 Go / 6150 Go */
            {"NV4C",    CARD_NVIDIA_GEFORCE_6200},      /* 6150SE */
            {"NV4B",    CARD_NVIDIA_GEFORCE_7600},
            {"NV4A",    CARD_NVIDIA_GEFORCE_6200},
            {"NV49",    CARD_NVIDIA_GEFORCE_7800GT},    /* 7900 */
            {"NV47",    CARD_NVIDIA_GEFORCE_7800GT},
            {"NV46",    CARD_NVIDIA_GEFORCE_7400},
            {"NV45",    CARD_NVIDIA_GEFORCE_6800},
            {"NV44",    CARD_NVIDIA_GEFORCE_6200},
            {"NV43",    CARD_NVIDIA_GEFORCE_6600GT},
            {"NV42",    CARD_NVIDIA_GEFORCE_6800},
            {"NV41",    CARD_NVIDIA_GEFORCE_6800},
            {"NV40",    CARD_NVIDIA_GEFORCE_6800},
            {"NV38",    CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5950 Ultra */
            {"NV36",    CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5700/5750 */
            {"NV35",    CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5900 */
            {"NV34",    CARD_NVIDIA_GEFORCEFX_5200},
            {"NV31",    CARD_NVIDIA_GEFORCEFX_5600},
            {"NV30",    CARD_NVIDIA_GEFORCEFX_5800},
            {"NV28",    CARD_NVIDIA_GEFORCE4_TI4200},
            {"NV25",    CARD_NVIDIA_GEFORCE4_TI4200},
            {"NV20",    CARD_NVIDIA_GEFORCE3},
            {"NV1F",    CARD_NVIDIA_GEFORCE4_MX},       /* GF4 MX IGP */
            {"NV1A",    CARD_NVIDIA_GEFORCE2},          /* GF2 IGP */
            {"NV18",    CARD_NVIDIA_GEFORCE4_MX},
            {"NV17",    CARD_NVIDIA_GEFORCE4_MX},
            {"NV16",    CARD_NVIDIA_GEFORCE2},
            {"NV15",    CARD_NVIDIA_GEFORCE2},
            {"NV11",    CARD_NVIDIA_GEFORCE2_MX},
            {"NV10",    CARD_NVIDIA_GEFORCE},
            {"NV05",    CARD_NVIDIA_RIVA_TNT2},
            {"NV04",    CARD_NVIDIA_RIVA_TNT},
            {"NV03",    CARD_NVIDIA_RIVA_128},
        };

        for (i = 0; i < sizeof(cards) / sizeof(*cards); ++i)
        {
            if (strstr(gl_renderer, cards[i].renderer))
                return cards[i].id;
        }
    }

    FIXME_(d3d_caps)("Unknown renderer %s.\n", debugstr_a(gl_renderer));

    d3d_level = d3d_level_from_gl_info(gl_info);
    if (d3d_level >= 9)
        return CARD_NVIDIA_GEFORCEFX_5600;
    if (d3d_level >= 8)
        return CARD_NVIDIA_GEFORCE3;
    if (d3d_level >= 7)
        return CARD_NVIDIA_GEFORCE;
    if (d3d_level >= 6)
        return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}


struct vendor_card_selection
{
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_vendor card_vendor;
    const char *description;        /* Description of the card selector i.e. Apple OS/X Intel */
    enum wined3d_pci_device (*select_card)(const struct wined3d_gl_info *gl_info, const char *gl_renderer);
};

static const struct vendor_card_selection vendor_card_select_table[] =
{
    {GL_VENDOR_NVIDIA, HW_VENDOR_NVIDIA,  "Nvidia binary driver",     select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_NVIDIA,  "Apple OSX NVidia binary driver",   select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_AMD,     "Apple OSX AMD/ATI binary driver",  select_card_amd_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_INTEL,   "Apple OSX Intel binary driver",    select_card_intel},
    {GL_VENDOR_FGLRX,  HW_VENDOR_AMD,     "AMD/ATI binary driver",    select_card_amd_binary},
    {GL_VENDOR_MESA,   HW_VENDOR_AMD,     "Mesa AMD/ATI driver",      select_card_amd_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_NVIDIA,  "Mesa Nouveau driver",      select_card_nvidia_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_INTEL,   "Mesa Intel driver",        select_card_intel},
    {GL_VENDOR_INTEL,  HW_VENDOR_INTEL,   "Mesa Intel driver",        select_card_intel}
};


static enum wined3d_pci_device wined3d_guess_card(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor *gl_vendor, enum wined3d_pci_vendor *card_vendor)
{
    UINT d3d_level;

    /* Above is a list of Nvidia and ATI GPUs. Both vendors have dozens of
     * different GPUs with roughly the same features. In most cases GPUs from a
     * certain family differ in clockspeeds, the amount of video memory and the
     * number of shader pipelines.
     *
     * A Direct3D device object contains the PCI id (vendor + device) of the
     * videocard which is used for rendering. Various applications use this
     * information to get a rough estimation of the features of the card and
     * some might use it for enabling 3d effects only on certain types of
     * videocards. In some cases games might even use it to work around bugs
     * which happen on certain videocards/driver combinations. The problem is
     * that OpenGL only exposes a rendering string containing the name of the
     * videocard and not the PCI id.
     *
     * Various games depend on the PCI id, so somehow we need to provide one.
     * A simple option is to parse the renderer string and translate this to
     * the right PCI id. This is a lot of work because there are more than 200
     * GPUs just for Nvidia. Various cards share the same renderer string, so
     * the amount of code might be 'small' but there are quite a number of
     * exceptions which would make this a pain to maintain. Another way would
     * be to query the PCI id from the operating system (assuming this is the
     * videocard which is used for rendering which is not always the case).
     * This would work but it is not very portable. Second it would not work
     * well in, let's say, a remote X situation in which the amount of 3d
     * features which can be used is limited.
     *
     * As said most games only use the PCI id to get an indication of the
     * capabilities of the card. It doesn't really matter if the given id is
     * the correct one if we return the id of a card with similar 3d features.
     *
     * The code below checks the OpenGL capabilities of a videocard and matches
     * that to a certain level of Direct3D functionality. Once a card passes
     * the Direct3D9 check, we know that the card (in case of Nvidia) is at
     * least a GeforceFX. To give a better estimate we do a basic check on the
     * renderer string but if that won't pass we return a default card. This
     * way is better than maintaining a full card database as even without a
     * full database we can return a card with similar features. Second the
     * size of the database can be made quite small because when you know what
     * type of 3d functionality a card has, you know to which GPU family the
     * GPU must belong. Because of this you only have to check a small part of
     * the renderer string to distinguishes between different models from that
     * family.
     *
     * The code also selects a default amount of video memory which we will
     * use for an estimation of the amount of free texture memory. In case of
     * real D3D the amount of texture memory includes video memory and system
     * memory (to be specific AGP memory or in case of PCIE TurboCache /
     * HyperMemory). We don't know how much system memory can be addressed by
     * the system but we can make a reasonable estimation about the amount of
     * video memory. If the value is slightly wrong it doesn't matter as we
     * didn't include AGP-like memory which makes the amount of addressable
     * memory higher and second OpenGL isn't that critical it moves to system
     * memory behind our backs if really needed. Note that the amount of video
     * memory can be overruled using a registry setting. */

    int i;

    for (i = 0; i < (sizeof(vendor_card_select_table) / sizeof(*vendor_card_select_table)); ++i)
    {
        if ((vendor_card_select_table[i].gl_vendor != *gl_vendor)
            || (vendor_card_select_table[i].card_vendor != *card_vendor))
                continue;
        TRACE_(d3d_caps)("Applying card_selector \"%s\".\n", vendor_card_select_table[i].description);
        return vendor_card_select_table[i].select_card(gl_info, gl_renderer);
    }

    FIXME_(d3d_caps)("No card selector available for GL vendor %#x and card vendor %04x (using GL_RENDERER %s).\n",
            *gl_vendor, *card_vendor, debugstr_a(gl_renderer));

    /* Default to generic Nvidia hardware based on the supported OpenGL extensions. The choice
     * for Nvidia was because the hardware and drivers they make are of good quality. This makes
     * them a good generic choice. */
    *card_vendor = HW_VENDOR_NVIDIA;
    d3d_level = d3d_level_from_gl_info(gl_info);
    if (d3d_level >= 9)
        return CARD_NVIDIA_GEFORCEFX_5600;
    if (d3d_level >= 8)
        return CARD_NVIDIA_GEFORCE3;
    if (d3d_level >= 7)
        return CARD_NVIDIA_GEFORCE;
    if (d3d_level >= 6)
        return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}

static const struct fragment_pipeline *select_fragment_implementation(const struct wined3d_gl_info *gl_info)
{
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if ((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM]) return &arbfp_fragment_pipeline;
    else if (ps_selected_mode == SHADER_ATI) return &atifs_fragment_pipeline;
    else if (gl_info->supported[NV_REGISTER_COMBINERS]
            && gl_info->supported[NV_TEXTURE_SHADER2]) return &nvts_fragment_pipeline;
    else if (gl_info->supported[NV_REGISTER_COMBINERS]) return &nvrc_fragment_pipeline;
    else return &ffp_fragment_pipeline;
}

static const struct wined3d_shader_backend_ops *select_shader_backend(const struct wined3d_gl_info *gl_info)
{
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if (vs_selected_mode == SHADER_GLSL || ps_selected_mode == SHADER_GLSL) return &glsl_shader_backend;
    if (vs_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_ARB) return &arb_program_shader_backend;
    return &none_shader_backend;
}

static const struct blit_shader *select_blit_implementation(const struct wined3d_gl_info *gl_info)
{
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if ((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM]) return &arbfp_blit;
    else return &ffp_blit;
}

static void load_gl_funcs(struct wined3d_gl_info *gl_info, DWORD gl_version)
{
    DWORD ver;

#define USE_GL_FUNC(type, pfn, ext, replace) \
    if (gl_info->supported[ext]) gl_info->pfn = (type)pwglGetProcAddress(#pfn); \
    else if ((ver = ver_for_ext(ext)) && ver <= gl_version) gl_info->pfn = (type)pwglGetProcAddress(#replace); \
    else gl_info->pfn = NULL;

    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

#define USE_GL_FUNC(type, pfn, ext, replace) gl_info->pfn = (type)pwglGetProcAddress(#pfn);
    WGL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC
}

/* Context activation is done by the caller. */
static BOOL wined3d_adapter_init_gl_caps(struct wined3d_adapter *adapter)
{
    struct wined3d_driver_info *driver_info = &adapter->driver_info;
    struct wined3d_gl_info *gl_info = &adapter->gl_info;
    const char *GL_Extensions    = NULL;
    const char *WGL_Extensions   = NULL;
    const char *gl_vendor_str, *gl_renderer_str, *gl_version_str;
    struct fragment_caps fragment_caps;
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_vendor card_vendor;
    enum wined3d_pci_device device;
    GLint       gl_max;
    GLfloat     gl_floatv[2];
    unsigned    i;
    HDC         hdc;
    DWORD gl_version;
    size_t len;

    TRACE_(d3d_caps)("(%p)\n", gl_info);

    ENTER_GL();

    gl_renderer_str = (const char *)glGetString(GL_RENDERER);
    TRACE_(d3d_caps)("GL_RENDERER: %s.\n", debugstr_a(gl_renderer_str));
    if (!gl_renderer_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_RENDERER.\n");
        return FALSE;
    }

    gl_vendor_str = (const char *)glGetString(GL_VENDOR);
    TRACE_(d3d_caps)("GL_VENDOR: %s.\n", debugstr_a(gl_vendor_str));
    if (!gl_vendor_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_VENDOR.\n");
        return FALSE;
    }

    /* Parse the GL_VERSION field into major and minor information */
    gl_version_str = (const char *)glGetString(GL_VERSION);
    TRACE_(d3d_caps)("GL_VERSION: %s.\n", debugstr_a(gl_version_str));
    if (!gl_version_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_VERSION.\n");
        return FALSE;
    }
    gl_version = wined3d_parse_gl_version(gl_version_str);

    /*
     * Initialize openGL extension related variables
     *  with Default values
     */
    memset(gl_info->supported, 0, sizeof(gl_info->supported));
    gl_info->limits.blends = 1;
    gl_info->limits.buffers = 1;
    gl_info->limits.textures = 1;
    gl_info->limits.texture_coords = 1;
    gl_info->limits.fragment_samplers = 1;
    gl_info->limits.vertex_samplers = 0;
    gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers + gl_info->limits.vertex_samplers;
    gl_info->limits.sampler_stages = 1;
    gl_info->limits.vertex_attribs = 16;
    gl_info->limits.glsl_vs_float_constants = 0;
    gl_info->limits.glsl_ps_float_constants = 0;
    gl_info->limits.arb_vs_float_constants = 0;
    gl_info->limits.arb_vs_native_constants = 0;
    gl_info->limits.arb_vs_instructions = 0;
    gl_info->limits.arb_vs_temps = 0;
    gl_info->limits.arb_ps_float_constants = 0;
    gl_info->limits.arb_ps_local_constants = 0;
    gl_info->limits.arb_ps_instructions = 0;
    gl_info->limits.arb_ps_temps = 0;

    /* Retrieve opengl defaults */
    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    gl_info->limits.clipplanes = min(WINED3DMAXUSERCLIPPLANES, gl_max);
    TRACE_(d3d_caps)("ClipPlanes support - num Planes=%d\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    gl_info->limits.lights = gl_max;
    TRACE_(d3d_caps)("Lights support - max lights=%d\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    gl_info->limits.texture_size = gl_max;
    TRACE_(d3d_caps)("Maximum texture size support - max texture size=%d\n", gl_max);

    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv);
    gl_info->limits.pointsize_min = gl_floatv[0];
    gl_info->limits.pointsize_max = gl_floatv[1];
    TRACE_(d3d_caps)("Maximum point size support - max point size=%f\n", gl_floatv[1]);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately. */
    GL_Extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!GL_Extensions)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_EXTENSIONS.\n");
        return FALSE;
    }

    LEAVE_GL();

    TRACE_(d3d_caps)("GL_Extensions reported:\n");

    gl_info->supported[WINED3D_GL_EXT_NONE] = TRUE;

    while (*GL_Extensions)
    {
        const char *start;

        while (isspace(*GL_Extensions)) ++GL_Extensions;
        start = GL_Extensions;
        while (!isspace(*GL_Extensions) && *GL_Extensions) ++GL_Extensions;

        len = GL_Extensions - start;
        if (!len) continue;

        TRACE_(d3d_caps)("- %s\n", debugstr_an(start, len));

        for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i)
        {
            if (len == strlen(EXTENSION_MAP[i].extension_string)
                    && !memcmp(start, EXTENSION_MAP[i].extension_string, len))
            {
                TRACE_(d3d_caps)(" FOUND: %s support.\n", EXTENSION_MAP[i].extension_string);
                gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
                break;
            }
        }
    }

    /* Now work out what GL support this card really has */
    load_gl_funcs( gl_info, gl_version );

    ENTER_GL();

    /* Now mark all the extensions supported which are included in the opengl core version. Do this *after*
     * loading the functions, otherwise the code above will load the extension entry points instead of the
     * core functions, which may not work. */
    for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i)
    {
        if (!gl_info->supported[EXTENSION_MAP[i].extension]
                && EXTENSION_MAP[i].version <= gl_version && EXTENSION_MAP[i].version)
        {
            TRACE_(d3d_caps)(" GL CORE: %s support.\n", EXTENSION_MAP[i].extension_string);
            gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
        }
    }

    if (gl_version >= MAKEDWORD_VERSION(2, 0)) gl_info->supported[WINED3D_GL_VERSION_2_0] = TRUE;

    if (gl_info->supported[APPLE_FENCE])
    {
        /* GL_NV_fence and GL_APPLE_fence provide the same functionality basically.
         * The apple extension interacts with some other apple exts. Disable the NV
         * extension if the apple one is support to prevent confusion in other parts
         * of the code. */
        gl_info->supported[NV_FENCE] = FALSE;
    }
    if (gl_info->supported[APPLE_FLOAT_PIXELS])
    {
        /* GL_APPLE_float_pixels == GL_ARB_texture_float + GL_ARB_half_float_pixel
         *
         * The enums are the same:
         * GL_RGBA16F_ARB     = GL_RGBA_FLOAT16_APPLE = 0x881A
         * GL_RGB16F_ARB      = GL_RGB_FLOAT16_APPLE  = 0x881B
         * GL_RGBA32F_ARB     = GL_RGBA_FLOAT32_APPLE = 0x8814
         * GL_RGB32F_ARB      = GL_RGB_FLOAT32_APPLE  = 0x8815
         * GL_HALF_FLOAT_ARB  = GL_HALF_APPLE         = 0x140B
         */
        if (!gl_info->supported[ARB_TEXTURE_FLOAT])
        {
            TRACE_(d3d_caps)(" IMPLIED: GL_ARB_texture_float support(from GL_APPLE_float_pixels.\n");
            gl_info->supported[ARB_TEXTURE_FLOAT] = TRUE;
        }
        if (!gl_info->supported[ARB_HALF_FLOAT_PIXEL])
        {
            TRACE_(d3d_caps)(" IMPLIED: GL_ARB_half_float_pixel support(from GL_APPLE_float_pixels.\n");
            gl_info->supported[ARB_HALF_FLOAT_PIXEL] = TRUE;
        }
    }
    if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
    {
        /* GL_ARB_map_buffer_range and GL_APPLE_flush_buffer_range provide the same
         * functionality. Prefer the ARB extension */
        gl_info->supported[APPLE_FLUSH_BUFFER_RANGE] = FALSE;
    }
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        TRACE_(d3d_caps)(" IMPLIED: NVIDIA (NV) Texture Gen Reflection support.\n");
        gl_info->supported[NV_TEXGEN_REFLECTION] = TRUE;
    }
    if (!gl_info->supported[ARB_DEPTH_CLAMP] && gl_info->supported[NV_DEPTH_CLAMP])
    {
        TRACE_(d3d_caps)(" IMPLIED: ARB_depth_clamp support (by NV_depth_clamp).\n");
        gl_info->supported[ARB_DEPTH_CLAMP] = TRUE;
    }
    if (!gl_info->supported[ARB_VERTEX_ARRAY_BGRA] && gl_info->supported[EXT_VERTEX_ARRAY_BGRA])
    {
        TRACE_(d3d_caps)(" IMPLIED: ARB_vertex_array_bgra support (by EXT_vertex_array_bgra).\n");
        gl_info->supported[ARB_VERTEX_ARRAY_BGRA] = TRUE;
    }
    if (!gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] && gl_info->supported[EXT_TEXTURE_COMPRESSION_RGTC])
    {
        TRACE_(d3d_caps)(" IMPLIED: ARB_texture_compression_rgtc support (by EXT_texture_compression_rgtc).\n");
        gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] = TRUE;
    }
    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        if (gl_info->supported[NV_REGISTER_COMBINERS])
        {
            /* Also disable ATI_FRAGMENT_SHADER if register combiners and texture_shader2
             * are supported. The nv extensions provide the same functionality as the
             * ATI one, and a bit more(signed pixelformats). */
            gl_info->supported[ATI_FRAGMENT_SHADER] = FALSE;
        }
    }

    if (gl_info->supported[ARB_MAP_BUFFER_ALIGNMENT])
    {
        glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &gl_max);
        TRACE_(d3d_caps)("Minimum buffer map alignment: %d.\n", gl_max);
    }
    else
    {
        WARN_(d3d_caps)("Driver doesn't guarantee a minimum buffer map alignment.\n");
    }
    if (gl_info->supported[NV_REGISTER_COMBINERS])
    {
        glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &gl_max);
        gl_info->limits.general_combiners = gl_max;
        TRACE_(d3d_caps)("Max general combiners: %d.\n", gl_max);
    }
    if (gl_info->supported[ARB_DRAW_BUFFERS])
    {
        glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &gl_max);
        gl_info->limits.buffers = gl_max;
        TRACE_(d3d_caps)("Max draw buffers: %u.\n", gl_max);
    }
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
        gl_info->limits.textures = min(MAX_TEXTURES, gl_max);
        TRACE_(d3d_caps)("Max textures: %d.\n", gl_info->limits.textures);
        glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &gl_max);
        gl_info->limits.texture_coords = min(MAX_TEXTURES, gl_max);
        TRACE_(d3d_caps)("Max texture coords: %d.\n", gl_info->limits.texture_coords);

        if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
        {
            GLint tmp;
            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.fragment_samplers = min(MAX_FRAGMENT_SAMPLERS, tmp);
        }
        else
        {
            gl_info->limits.fragment_samplers = max(gl_info->limits.fragment_samplers, gl_max);
        }
        TRACE_(d3d_caps)("Max fragment samplers: %d.\n", gl_info->limits.fragment_samplers);

        if (gl_info->supported[ARB_VERTEX_SHADER])
        {
            GLint tmp;
            glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.vertex_samplers = tmp;
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.combined_samplers = tmp;
            glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &tmp);
            gl_info->limits.vertex_attribs = tmp;

            /* Loading GLSL sampler uniforms is much simpler if we can assume that the sampler setup
             * is known at shader link time. In a vertex shader + pixel shader combination this isn't
             * an issue because then the sampler setup only depends on the two shaders. If a pixel
             * shader is used with fixed function vertex processing we're fine too because fixed function
             * vertex processing doesn't use any samplers. If fixed function fragment processing is
             * used we have to make sure that all vertex sampler setups are valid together with all
             * possible fixed function fragment processing setups. This is true if vsamplers + MAX_TEXTURES
             * <= max_samplers. This is true on all d3d9 cards that support vtf(gf 6 and gf7 cards).
             * dx9 radeon cards do not support vertex texture fetch. DX10 cards have 128 samplers, and
             * dx9 is limited to 8 fixed function texture stages and 4 vertex samplers. DX10 does not have
             * a fixed function pipeline anymore.
             *
             * So this is just a check to check that our assumption holds true. If not, write a warning
             * and reduce the number of vertex samplers or probably disable vertex texture fetch. */
            if (gl_info->limits.vertex_samplers && gl_info->limits.combined_samplers < 12
                    && MAX_TEXTURES + gl_info->limits.vertex_samplers > gl_info->limits.combined_samplers)
            {
                FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers.\n",
                        gl_info->limits.vertex_samplers, gl_info->limits.combined_samplers);
                FIXME("Expected vertex samplers + MAX_TEXTURES(=8) > combined_samplers.\n");
                if (gl_info->limits.combined_samplers > MAX_TEXTURES)
                    gl_info->limits.vertex_samplers = gl_info->limits.combined_samplers - MAX_TEXTURES;
                else
                    gl_info->limits.vertex_samplers = 0;
            }
        }
        else
        {
            gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers;
        }
        TRACE_(d3d_caps)("Max vertex samplers: %u.\n", gl_info->limits.vertex_samplers);
        TRACE_(d3d_caps)("Max combined samplers: %u.\n", gl_info->limits.combined_samplers);
    }
    if (gl_info->supported[ARB_VERTEX_BLEND])
    {
        glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
        gl_info->limits.blends = gl_max;
        TRACE_(d3d_caps)("Max blends: %u.\n", gl_info->limits.blends);
    }
    if (gl_info->supported[EXT_TEXTURE3D])
    {
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &gl_max);
        gl_info->limits.texture3d_size = gl_max;
        TRACE_(d3d_caps)("Max texture3D size: %d.\n", gl_info->limits.texture3d_size);
    }
    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
    {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
        gl_info->limits.anisotropy = gl_max;
        TRACE_(d3d_caps)("Max anisotropy: %d.\n", gl_info->limits.anisotropy);
    }
    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_float_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM float constants: %d.\n", gl_info->limits.arb_ps_float_constants);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_native_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_ps_native_constants);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
        gl_info->limits.arb_ps_temps = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_ps_temps);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
        gl_info->limits.arb_ps_instructions = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native instructions: %d.\n", gl_info->limits.arb_ps_instructions);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_local_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM local parameters: %d.\n", gl_info->limits.arb_ps_instructions);
    }
    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_vs_float_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM float constants: %d.\n", gl_info->limits.arb_vs_float_constants);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_vs_native_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_vs_native_constants);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
        gl_info->limits.arb_vs_temps = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_vs_temps);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
        gl_info->limits.arb_vs_instructions = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native instructions: %d.\n", gl_info->limits.arb_vs_instructions);

        if (test_arb_vs_offset_limit(gl_info)) gl_info->quirks |= WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT;
    }
    if (gl_info->supported[ARB_VERTEX_SHADER])
    {
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &gl_max);
        gl_info->limits.glsl_vs_float_constants = gl_max / 4;
        TRACE_(d3d_caps)("Max ARB_VERTEX_SHADER float constants: %u.\n", gl_info->limits.glsl_vs_float_constants);
    }
    if (gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &gl_max);
        gl_info->limits.glsl_ps_float_constants = gl_max / 4;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_SHADER float constants: %u.\n", gl_info->limits.glsl_ps_float_constants);
        glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max);
        gl_info->limits.glsl_varyings = gl_max;
        TRACE_(d3d_caps)("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_max, gl_max / 4);
    }
    if (gl_info->supported[ARB_SHADING_LANGUAGE_100])
    {
        const char *str = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION_ARB);
        unsigned int major, minor;

        TRACE_(d3d_caps)("GLSL version string: %s.\n", debugstr_a(str));

        /* The format of the GLSL version string is "major.minor[.release] [vendor info]". */
        sscanf(str, "%u.%u", &major, &minor);
        gl_info->glsl_version = MAKEDWORD_VERSION(major, minor);
    }
    if (gl_info->supported[NV_LIGHT_MAX_EXPONENT])
    {
        glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess);
    }
    else
    {
        gl_info->limits.shininess = 128.0f;
    }
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        /* If we have full NP2 texture support, disable
         * GL_ARB_texture_rectangle because we will never use it.
         * This saves a few redundant glDisable calls. */
        gl_info->supported[ARB_TEXTURE_RECTANGLE] = FALSE;
    }
    if (gl_info->supported[ATI_FRAGMENT_SHADER])
    {
        /* Disable NV_register_combiners and fragment shader if this is supported.
         * generally the NV extensions are preferred over the ATI ones, and this
         * extension is disabled if register_combiners and texture_shader2 are both
         * supported. So we reach this place only if we have incomplete NV dxlevel 8
         * fragment processing support. */
        gl_info->supported[NV_REGISTER_COMBINERS] = FALSE;
        gl_info->supported[NV_REGISTER_COMBINERS2] = FALSE;
        gl_info->supported[NV_TEXTURE_SHADER] = FALSE;
        gl_info->supported[NV_TEXTURE_SHADER2] = FALSE;
    }
    if (gl_info->supported[NV_HALF_FLOAT])
    {
        /* GL_ARB_half_float_vertex is a subset of GL_NV_half_float. */
        gl_info->supported[ARB_HALF_FLOAT_VERTEX] = TRUE;
    }
    checkGLcall("extension detection");

    LEAVE_GL();

    adapter->fragment_pipe = select_fragment_implementation(gl_info);
    adapter->shader_backend = select_shader_backend(gl_info);
    adapter->blitter = select_blit_implementation(gl_info);

    adapter->fragment_pipe->get_caps(gl_info, &fragment_caps);
    gl_info->limits.texture_stages = fragment_caps.MaxTextureBlendStages;
    TRACE_(d3d_caps)("Max texture stages: %u.\n", gl_info->limits.texture_stages);

    /* In some cases the number of texture stages can be larger than the number
     * of samplers. The GF4 for example can use only 2 samplers (no fragment
     * shaders), but 8 texture stages (register combiners). */
    gl_info->limits.sampler_stages = max(gl_info->limits.fragment_samplers, gl_info->limits.texture_stages);

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
    {
        gl_info->fbo_ops.glIsRenderbuffer = gl_info->glIsRenderbuffer;
        gl_info->fbo_ops.glBindRenderbuffer = gl_info->glBindRenderbuffer;
        gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->glDeleteRenderbuffers;
        gl_info->fbo_ops.glGenRenderbuffers = gl_info->glGenRenderbuffers;
        gl_info->fbo_ops.glRenderbufferStorage = gl_info->glRenderbufferStorage;
        gl_info->fbo_ops.glRenderbufferStorageMultisample = gl_info->glRenderbufferStorageMultisample;
        gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->glGetRenderbufferParameteriv;
        gl_info->fbo_ops.glIsFramebuffer = gl_info->glIsFramebuffer;
        gl_info->fbo_ops.glBindFramebuffer = gl_info->glBindFramebuffer;
        gl_info->fbo_ops.glDeleteFramebuffers = gl_info->glDeleteFramebuffers;
        gl_info->fbo_ops.glGenFramebuffers = gl_info->glGenFramebuffers;
        gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->glCheckFramebufferStatus;
        gl_info->fbo_ops.glFramebufferTexture1D = gl_info->glFramebufferTexture1D;
        gl_info->fbo_ops.glFramebufferTexture2D = gl_info->glFramebufferTexture2D;
        gl_info->fbo_ops.glFramebufferTexture3D = gl_info->glFramebufferTexture3D;
        gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->glFramebufferRenderbuffer;
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv = gl_info->glGetFramebufferAttachmentParameteriv;
        gl_info->fbo_ops.glBlitFramebuffer = gl_info->glBlitFramebuffer;
        gl_info->fbo_ops.glGenerateMipmap = gl_info->glGenerateMipmap;
        if (wined3d_settings.allow_multisampling)
        {
            glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
            gl_info->limits.samples = gl_max;
        }
    }
    else
    {
        if (gl_info->supported[EXT_FRAMEBUFFER_OBJECT])
        {
            gl_info->fbo_ops.glIsRenderbuffer = gl_info->glIsRenderbufferEXT;
            gl_info->fbo_ops.glBindRenderbuffer = gl_info->glBindRenderbufferEXT;
            gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->glDeleteRenderbuffersEXT;
            gl_info->fbo_ops.glGenRenderbuffers = gl_info->glGenRenderbuffersEXT;
            gl_info->fbo_ops.glRenderbufferStorage = gl_info->glRenderbufferStorageEXT;
            gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->glGetRenderbufferParameterivEXT;
            gl_info->fbo_ops.glIsFramebuffer = gl_info->glIsFramebufferEXT;
            gl_info->fbo_ops.glBindFramebuffer = gl_info->glBindFramebufferEXT;
            gl_info->fbo_ops.glDeleteFramebuffers = gl_info->glDeleteFramebuffersEXT;
            gl_info->fbo_ops.glGenFramebuffers = gl_info->glGenFramebuffersEXT;
            gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->glCheckFramebufferStatusEXT;
            gl_info->fbo_ops.glFramebufferTexture1D = gl_info->glFramebufferTexture1DEXT;
            gl_info->fbo_ops.glFramebufferTexture2D = gl_info->glFramebufferTexture2DEXT;
            gl_info->fbo_ops.glFramebufferTexture3D = gl_info->glFramebufferTexture3DEXT;
            gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->glFramebufferRenderbufferEXT;
            gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv = gl_info->glGetFramebufferAttachmentParameterivEXT;
            gl_info->fbo_ops.glGenerateMipmap = gl_info->glGenerateMipmapEXT;
        }
        else if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            WARN_(d3d_caps)("Framebuffer objects not supported, falling back to backbuffer offscreen rendering mode.\n");
            wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
        }
        if (gl_info->supported[EXT_FRAMEBUFFER_BLIT])
        {
            gl_info->fbo_ops.glBlitFramebuffer = gl_info->glBlitFramebufferEXT;
        }
        if (gl_info->supported[EXT_FRAMEBUFFER_MULTISAMPLE])
        {
            gl_info->fbo_ops.glRenderbufferStorageMultisample = gl_info->glRenderbufferStorageMultisampleEXT;
            if (wined3d_settings.allow_multisampling)
            {
                glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
                gl_info->limits.samples = gl_max;
            }
        }
    }

    /* MRTs are currently only supported when FBOs are used. */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
    {
        gl_info->limits.buffers = 1;
    }

    gl_vendor = wined3d_guess_gl_vendor(gl_info, gl_vendor_str, gl_renderer_str);
    card_vendor = wined3d_guess_card_vendor(gl_vendor_str, gl_renderer_str);
    TRACE_(d3d_caps)("found GL_VENDOR (%s)->(0x%04x/0x%04x)\n", debugstr_a(gl_vendor_str), gl_vendor, card_vendor);

    device = wined3d_guess_card(gl_info, gl_renderer_str, &gl_vendor, &card_vendor);
    TRACE_(d3d_caps)("FOUND (fake) card: 0x%x (vendor id), 0x%x (device id)\n", card_vendor, device);

    gl_info->wrap_lookup[WINED3D_TADDRESS_WRAP - WINED3D_TADDRESS_WRAP] = GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_CLAMP - WINED3D_TADDRESS_WRAP] = GL_CLAMP_TO_EDGE;
    gl_info->wrap_lookup[WINED3D_TADDRESS_BORDER - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] ? GL_MIRROR_CLAMP_TO_EDGE_ATI : GL_REPEAT;

    /* Make sure there's an active HDC else the WGL extensions will fail */
    hdc = pwglGetCurrentDC();
    if (hdc) {
        /* Not all GL drivers might offer WGL extensions e.g. VirtualBox */
        if(GL_EXTCALL(wglGetExtensionsStringARB))
            WGL_Extensions = GL_EXTCALL(wglGetExtensionsStringARB(hdc));

        if (!WGL_Extensions)
        {
            ERR("   WGL_Extensions returns NULL\n");
        }
        else
        {
            TRACE_(d3d_caps)("WGL_Extensions reported:\n");
            while (*WGL_Extensions)
            {
                const char *Start;
                char ThisExtn[256];

                while (isspace(*WGL_Extensions)) WGL_Extensions++;
                Start = WGL_Extensions;
                while (!isspace(*WGL_Extensions) && *WGL_Extensions) ++WGL_Extensions;

                len = WGL_Extensions - Start;
                if (!len || len >= sizeof(ThisExtn))
                    continue;

                memcpy(ThisExtn, Start, len);
                ThisExtn[len] = '\0';
                TRACE_(d3d_caps)("- %s\n", debugstr_a(ThisExtn));

                if (!strcmp(ThisExtn, "WGL_ARB_pixel_format")) {
                    gl_info->supported[WGL_ARB_PIXEL_FORMAT] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_ARB_pixel_format support\n");
                }
                if (!strcmp(ThisExtn, "WGL_EXT_swap_control")) {
                    gl_info->supported[WGL_EXT_SWAP_CONTROL] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_EXT_swap_control support\n");
                }
                if (!strcmp(ThisExtn, "WGL_WINE_pixel_format_passthrough")) {
                    gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_WINE_pixel_format_passthrough support\n");
                }
            }
        }
    }

    fixup_extensions(gl_info, gl_renderer_str, gl_vendor, card_vendor, device);
    init_driver_info(driver_info, card_vendor, device);
    add_gl_compat_wrappers(gl_info);

    return TRUE;
}

UINT CDECL wined3d_get_adapter_count(const struct wined3d *wined3d)
{
    TRACE_(d3d_caps)("wined3d %p, reporting %u adapters.\n",
            wined3d, wined3d->adapter_count);

    return wined3d->adapter_count;
}

HRESULT CDECL wined3d_register_software_device(struct wined3d *wined3d, void *init_function)
{
    FIXME("wined3d %p, init_function %p stub!\n", wined3d, init_function);

    return WINED3D_OK;
}

HMONITOR CDECL wined3d_get_adapter_monitor(const struct wined3d *wined3d, UINT adapter_idx)
{
    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u.\n", wined3d, adapter_idx);

    if (adapter_idx >= wined3d->adapter_count)
        return NULL;

    return MonitorFromPoint(wined3d->adapters[adapter_idx].monitorPoint, MONITOR_DEFAULTTOPRIMARY);
}

/* FIXME: GetAdapterModeCount and EnumAdapterModes currently only returns modes
     of the same bpp but different resolutions                                  */

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
UINT CDECL wined3d_get_adapter_mode_count(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id)
{
    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, format %s.\n", wined3d, adapter_idx, debug_d3dformat(format_id));

    if (adapter_idx >= wined3d->adapter_count)
        return 0;

    /* TODO: Store modes per adapter and read it from the adapter structure */
    if (!adapter_idx)
    {
        const struct wined3d_format *format = wined3d_get_format(&wined3d->adapters[adapter_idx].gl_info, format_id);
        UINT format_bits = format->byte_count * CHAR_BIT;
        unsigned int i = 0;
        unsigned int j = 0;
        DEVMODEW mode;

        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);

        while (EnumDisplaySettingsExW(NULL, j, &mode, 0))
        {
            ++j;

            if (format_id == WINED3DFMT_UNKNOWN)
            {
                /* This is for D3D8, do not enumerate P8 here */
                if (mode.dmBitsPerPel == 32 || mode.dmBitsPerPel == 16) ++i;
            }
            else if (mode.dmBitsPerPel == format_bits)
            {
                ++i;
            }
        }

        TRACE_(d3d_caps)("Returning %u matching modes (out of %u total) for adapter %u.\n", i, j, adapter_idx);

        return i;
    }
    else
    {
        FIXME_(d3d_caps)("Adapter not primary display.\n");
    }

    return 0;
}

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
HRESULT CDECL wined3d_enum_adapter_modes(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, UINT mode_idx, struct wined3d_display_mode *mode)
{
    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, format %s, mode_idx %u, mode %p.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), mode_idx, mode);

    /* Validate the parameters as much as possible */
    if (!mode || adapter_idx >= wined3d->adapter_count
            || mode_idx >= wined3d_get_adapter_mode_count(wined3d, adapter_idx, format_id))
    {
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: Store modes per adapter and read it from the adapter structure */
    if (!adapter_idx)
    {
        const struct wined3d_format *format = wined3d_get_format(&wined3d->adapters[adapter_idx].gl_info, format_id);
        UINT format_bits = format->byte_count * CHAR_BIT;
        DEVMODEW DevModeW;
        int ModeIdx = 0;
        UINT i = 0;
        int j = 0;

        ZeroMemory(&DevModeW, sizeof(DevModeW));
        DevModeW.dmSize = sizeof(DevModeW);

        /* If we are filtering to a specific format (D3D9), then need to skip
           all unrelated modes, but if mode is irrelevant (D3D8), then we can
           just count through the ones with valid bit depths */
        while (i <= mode_idx && EnumDisplaySettingsExW(NULL, j++, &DevModeW, 0))
        {
            if (format_id == WINED3DFMT_UNKNOWN)
            {
                /* This is for D3D8, do not enumerate P8 here */
                if (DevModeW.dmBitsPerPel == 32 || DevModeW.dmBitsPerPel == 16) ++i;
            }
            else if (DevModeW.dmBitsPerPel == format_bits)
            {
                ++i;
            }
        }

        if (!i)
        {
            TRACE_(d3d_caps)("No modes found for format (%x - %s)\n", format_id, debug_d3dformat(format_id));
            return WINED3DERR_INVALIDCALL;
        }
        ModeIdx = j - 1;

        /* Now get the display mode via the calculated index */
        if (EnumDisplaySettingsExW(NULL, ModeIdx, &DevModeW, 0))
        {
            mode->width = DevModeW.dmPelsWidth;
            mode->height = DevModeW.dmPelsHeight;
            mode->refresh_rate = DEFAULT_REFRESH_RATE;
            if (DevModeW.dmFields & DM_DISPLAYFREQUENCY)
                mode->refresh_rate = DevModeW.dmDisplayFrequency;

            if (format_id == WINED3DFMT_UNKNOWN)
                mode->format_id = pixelformat_for_depth(DevModeW.dmBitsPerPel);
            else
                mode->format_id = format_id;
        }
        else
        {
            TRACE_(d3d_caps)("Requested mode %u out of range.\n", mode_idx);
            return WINED3DERR_INVALIDCALL;
        }

        TRACE_(d3d_caps)("W %d H %d rr %d fmt (%x - %s) bpp %u\n",
                mode->width, mode->height, mode->refresh_rate, mode->format_id,
                debug_d3dformat(mode->format_id), DevModeW.dmBitsPerPel);
    }
    else
    {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_adapter_display_mode(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_display_mode *mode)
{
    TRACE("wined3d %p, adapter_idx %u, display_mode %p.\n", wined3d, adapter_idx, mode);

    if (!mode || adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    if (!adapter_idx)
    {
        DEVMODEW DevModeW;
        unsigned int bpp;

        ZeroMemory(&DevModeW, sizeof(DevModeW));
        DevModeW.dmSize = sizeof(DevModeW);

        EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &DevModeW, 0);
        mode->width = DevModeW.dmPelsWidth;
        mode->height = DevModeW.dmPelsHeight;
        bpp = DevModeW.dmBitsPerPel;
        mode->refresh_rate = DEFAULT_REFRESH_RATE;
        if (DevModeW.dmFields & DM_DISPLAYFREQUENCY)
            mode->refresh_rate = DevModeW.dmDisplayFrequency;
        mode->format_id = pixelformat_for_depth(bpp);
    }
    else
    {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    TRACE_(d3d_caps)("returning w:%d, h:%d, ref:%d, fmt:%s\n", mode->width,
          mode->height, mode->refresh_rate, debug_d3dformat(mode->format_id));
    return WINED3D_OK;
}

/* NOTE: due to structure differences between dx8 and dx9 D3DADAPTER_IDENTIFIER,
   and fields being inserted in the middle, a new structure is used in place    */
HRESULT CDECL wined3d_get_adapter_identifier(const struct wined3d *wined3d,
        UINT adapter_idx, DWORD flags, struct wined3d_adapter_identifier *identifier)
{
    const struct wined3d_adapter *adapter;
    size_t len;

    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, flags %#x, indentifier %p.\n",
            wined3d, adapter_idx, flags, identifier);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];

    /* Return the information requested */
    TRACE_(d3d_caps)("device/Vendor Name and Version detection using FillGLCaps\n");

    if (identifier->driver_size)
    {
        const char *name = adapter->driver_info.name;
        len = min(strlen(name), identifier->driver_size - 1);
        memcpy(identifier->driver, name, len);
        identifier->driver[len] = '\0';
    }

    if (identifier->description_size)
    {
        const char *description = adapter->driver_info.description;
        len = min(strlen(description), identifier->description_size - 1);
        memcpy(identifier->description, description, len);
        identifier->description[len] = '\0';
    }

    /* Note that d3d8 doesn't supply a device name. */
    if (identifier->device_name_size)
    {
        static const char *device_name = "\\\\.\\DISPLAY1"; /* FIXME: May depend on desktop? */

        len = strlen(device_name);
        if (len >= identifier->device_name_size)
        {
            ERR("Device name size too small.\n");
            return WINED3DERR_INVALIDCALL;
        }

        memcpy(identifier->device_name, device_name, len);
        identifier->device_name[len] = '\0';
    }

    identifier->driver_version.u.HighPart = adapter->driver_info.version_high;
    identifier->driver_version.u.LowPart = adapter->driver_info.version_low;
    identifier->vendor_id = adapter->driver_info.vendor;
    identifier->device_id = adapter->driver_info.device;
    identifier->subsystem_id = 0;
    identifier->revision = 0;
    memcpy(&identifier->device_identifier, &IID_D3DDEVICE_D3DUID, sizeof(identifier->device_identifier));
    identifier->whql_level = (flags & WINED3DENUM_NO_WHQL_LEVEL) ? 0 : 1;
    memcpy(&identifier->adapter_luid, &adapter->luid, sizeof(identifier->adapter_luid));
    identifier->video_memory = adapter->TextureRam;

    return WINED3D_OK;
}

static BOOL wined3d_check_pixel_format_color(const struct wined3d_gl_info *gl_info,
        const struct wined3d_pixel_format *cfg, const struct wined3d_format *format)
{
    BYTE redSize, greenSize, blueSize, alphaSize, colorBits;

    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags & WINED3DFMT_FLAG_FLOAT) return FALSE;

    if(cfg->iPixelType == WGL_TYPE_RGBA_ARB) { /* Integer RGBA formats */
        if (!getColorBits(format, &redSize, &greenSize, &blueSize, &alphaSize, &colorBits))
        {
            ERR("Unable to check compatibility for format %s.\n", debug_d3dformat(format->id));
            return FALSE;
        }

        if(cfg->redSize < redSize)
            return FALSE;

        if(cfg->greenSize < greenSize)
            return FALSE;

        if(cfg->blueSize < blueSize)
            return FALSE;

        if(cfg->alphaSize < alphaSize)
            return FALSE;

        return TRUE;
    }

    /* Probably a RGBA_float or color index mode */
    return FALSE;
}

static BOOL wined3d_check_pixel_format_depth(const struct wined3d_gl_info *gl_info,
        const struct wined3d_pixel_format *cfg, const struct wined3d_format *format)
{
    BYTE depthSize, stencilSize;
    BOOL lockable = FALSE;

    if (!getDepthStencilBits(format, &depthSize, &stencilSize))
    {
        ERR("Unable to check compatibility for format %s.\n", debug_d3dformat(format->id));
        return FALSE;
    }

    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags & WINED3DFMT_FLAG_FLOAT) return FALSE;

    if ((format->id == WINED3DFMT_D16_LOCKABLE) || (format->id == WINED3DFMT_D32_FLOAT))
        lockable = TRUE;

    /* On some modern cards like the Geforce8/9 GLX doesn't offer some dephthstencil formats which D3D9 reports.
     * We can safely report 'compatible' formats (e.g. D24 can be used for D16) as long as we aren't dealing with
     * a lockable format. This also helps D3D <= 7 as they expect D16 which isn't offered without this on Geforce8 cards. */
    if(!(cfg->depthSize == depthSize || (!lockable && cfg->depthSize > depthSize)))
        return FALSE;

    /* Some cards like Intel i915 ones only offer D24S8 but lots of games also need a format without stencil, so
     * allow more stencil bits than requested. */
    if(cfg->stencilSize < stencilSize)
        return FALSE;

    return TRUE;
}

HRESULT CDECL wined3d_check_depth_stencil_match(const struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id,
        enum wined3d_format_id render_target_format_id, enum wined3d_format_id depth_stencil_format_id)
{
    const struct wined3d_format *rt_format;
    const struct wined3d_format *ds_format;
    const struct wined3d_adapter *adapter;

    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, device_type %s,\n"
            "adapter_format %s, render_target_format %s, depth_stencil_format %s.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(adapter_format_id),
            debug_d3dformat(render_target_format_id), debug_d3dformat(depth_stencil_format_id));

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    rt_format = wined3d_get_format(&adapter->gl_info, render_target_format_id);
    ds_format = wined3d_get_format(&adapter->gl_info, depth_stencil_format_id);
    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if ((rt_format->flags & WINED3DFMT_FLAG_RENDERTARGET)
                && (ds_format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        {
            TRACE_(d3d_caps)("Formats match.\n");
            return WINED3D_OK;
        }
    }
    else
    {
        const struct wined3d_pixel_format *cfgs;
        unsigned int cfg_count;
        unsigned int i;

        cfgs = adapter->cfgs;
        cfg_count = adapter->cfg_count;
        for (i = 0; i < cfg_count; ++i)
        {
            if (wined3d_check_pixel_format_color(&adapter->gl_info, &cfgs[i], rt_format)
                    && wined3d_check_pixel_format_depth(&adapter->gl_info, &cfgs[i], ds_format))
            {
                TRACE_(d3d_caps)("Formats match.\n");
                return WINED3D_OK;
            }
        }
    }

    TRACE_(d3d_caps)("Unsupported format pair: %s and %s.\n",
            debug_d3dformat(render_target_format_id),
            debug_d3dformat(depth_stencil_format_id));

    return WINED3DERR_NOTAVAILABLE;
}

HRESULT CDECL wined3d_check_device_multisample_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id surface_format_id, BOOL windowed,
        enum wined3d_multisample_type multisample_type, DWORD *quality_levels)
{
    const struct wined3d_gl_info *gl_info;

    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, device_type %s, surface_format %s,\n"
            "windowed %#x, multisample_type %#x, quality_levels %p.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(surface_format_id),
            windowed, multisample_type, quality_levels);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    gl_info = &wined3d->adapters[adapter_idx].gl_info;

    if (multisample_type > gl_info->limits.samples)
    {
        TRACE("Returning not supported.\n");
        if (quality_levels)
            *quality_levels = 0;

        return WINED3DERR_NOTAVAILABLE;
    }

    if (quality_levels)
    {
        if (multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE)
            /* FIXME: This is probably wrong. */
            *quality_levels = gl_info->limits.samples;
        else
            *quality_levels = 1;
    }

    return WINED3D_OK;
}

/* Check if we support bumpmapping for a format */
static BOOL CheckBumpMapCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    /* Ask the fixed function pipeline implementation if it can deal
     * with the conversion. If we've got a GL extension giving native
     * support this will be an identity conversion. */
    return (format->flags & WINED3DFMT_FLAG_BUMPMAP)
            && adapter->fragment_pipe->color_fixup_supported(format->color_fixup);
}

/* Check if the given DisplayFormat + DepthStencilFormat combination is valid for the Adapter */
static BOOL CheckDepthStencilCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *display_format, const struct wined3d_format *ds_format)
{
    /* Only allow depth/stencil formats */
    if (!(ds_format->depth_size || ds_format->stencil_size)) return FALSE;

    /* Blacklist formats not supported on Windows */
    switch (ds_format->id)
    {
        case WINED3DFMT_S1_UINT_D15_UNORM: /* Breaks the shadowvol2 dx7 sdk sample */
        case WINED3DFMT_S4X4_UINT_D24_UNORM:
            TRACE_(d3d_caps)("[FAILED] - not supported on windows\n");
            return FALSE;

        default:
            break;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        /* With FBOs WGL limitations do not apply, but the format needs to be FBO attachable */
        if (ds_format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)) return TRUE;
    }
    else
    {
        unsigned int i;

        /* Walk through all WGL pixel formats to find a match */
        for (i = 0; i < adapter->cfg_count; ++i)
        {
            const struct wined3d_pixel_format *cfg = &adapter->cfgs[i];
            if (wined3d_check_pixel_format_color(&adapter->gl_info, cfg, display_format)
                    && wined3d_check_pixel_format_depth(&adapter->gl_info, cfg, ds_format))
                return TRUE;
        }
    }

    return FALSE;
}

static BOOL CheckFilterCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    /* The flags entry of a format contains the filtering capability */
    if ((format->flags & WINED3DFMT_FLAG_FILTERING)
            || !(adapter->gl_info.quirks & WINED3D_QUIRK_LIMITED_TEX_FILTERING))
        return TRUE;

    return FALSE;
}

/* Check the render target capabilities of a format */
static BOOL CheckRenderTargetCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format, const struct wined3d_format *check_format)
{
    /* Filter out non-RT formats */
    if (!(check_format->flags & WINED3DFMT_FLAG_RENDERTARGET)) return FALSE;
    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        BYTE AdapterRed, AdapterGreen, AdapterBlue, AdapterAlpha, AdapterTotalSize;
        BYTE CheckRed, CheckGreen, CheckBlue, CheckAlpha, CheckTotalSize;
        const struct wined3d_pixel_format *cfgs = adapter->cfgs;
        unsigned int i;

        getColorBits(adapter_format, &AdapterRed, &AdapterGreen, &AdapterBlue, &AdapterAlpha, &AdapterTotalSize);
        getColorBits(check_format, &CheckRed, &CheckGreen, &CheckBlue, &CheckAlpha, &CheckTotalSize);

        /* In backbuffer mode the front and backbuffer share the same WGL pixelformat.
         * The format must match in RGB, alpha is allowed to be different. (Only the backbuffer can have alpha) */
        if(!((AdapterRed == CheckRed) && (AdapterGreen == CheckGreen) && (AdapterBlue == CheckBlue))) {
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
        }

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        for (i = 0; i < adapter->cfg_count; ++i)
        {
            if (cfgs[i].windowDrawable
                    && wined3d_check_pixel_format_color(&adapter->gl_info, &cfgs[i], check_format))
            {
                TRACE_(d3d_caps)("Pixel format %d is compatible with format %s.\n",
                        cfgs[i].iPixelFormat, debug_d3dformat(check_format->id));
                return TRUE;
            }
        }
    }
    else if(wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        /* For now return TRUE for FBOs until we have some proper checks.
         * Note that this function will only be called when the format is around for texturing. */
        return TRUE;
    }
    return FALSE;
}

static BOOL CheckSrgbReadCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    return format->flags & WINED3DFMT_FLAG_SRGB_READ;
}

static BOOL CheckSrgbWriteCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    /* Only offer SRGB writing on X8R8G8B8/A8R8G8B8 when we use ARB or GLSL shaders as we are
     * doing the color fixup in shaders.
     * Note Windows drivers (at least on the Geforce 8800) also offer this on R5G6B5. */
    if (format->flags & WINED3DFMT_FLAG_SRGB_WRITE)
    {
        int vs_selected_mode;
        int ps_selected_mode;
        select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);

        if((ps_selected_mode == SHADER_ARB) || (ps_selected_mode == SHADER_GLSL)) {
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;
        }
    }

    TRACE_(d3d_caps)("[FAILED] - sRGB writes not supported by format %s.\n", debug_d3dformat(format->id));
    return FALSE;
}

/* Check if a format support blending in combination with pixel shaders */
static BOOL CheckPostPixelShaderBlendingCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *format)
{
    /* The flags entry of a format contains the post pixel shader blending capability */
    if (format->flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING) return TRUE;

    return FALSE;
}

static BOOL CheckWrapAndMipCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    /* OpenGL supports mipmapping on all formats basically. Wrapping is unsupported,
     * but we have to report mipmapping so we cannot reject this flag. Tests show that
     * windows reports WRAPANDMIP on unfilterable surfaces as well, apparently to show
     * that wrapping is supported. The lack of filtering will sort out the mipmapping
     * capability anyway.
     *
     * For now lets report this on all formats, but in the future we may want to
     * restrict it to some should games need that
     */
    return TRUE;
}

/* Check if a texture format is supported on the given adapter */
static BOOL CheckTextureCapability(const struct wined3d_adapter *adapter, const struct wined3d_format *format)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    switch (format->id)
    {
        /*****
         *  supported: RGB(A) formats
         */
        case WINED3DFMT_B8G8R8_UNORM:
            TRACE_(d3d_caps)("[FAILED] - Not enumerated on Windows\n");
            return FALSE;
        case WINED3DFMT_B8G8R8A8_UNORM:
        case WINED3DFMT_B8G8R8X8_UNORM:
        case WINED3DFMT_B5G6R5_UNORM:
        case WINED3DFMT_B5G5R5X1_UNORM:
        case WINED3DFMT_B5G5R5A1_UNORM:
        case WINED3DFMT_B4G4R4A4_UNORM:
        case WINED3DFMT_A8_UNORM:
        case WINED3DFMT_B4G4R4X4_UNORM:
        case WINED3DFMT_R8G8B8A8_UNORM:
        case WINED3DFMT_R8G8B8X8_UNORM:
        case WINED3DFMT_B10G10R10A2_UNORM:
        case WINED3DFMT_R10G10B10A2_UNORM:
        case WINED3DFMT_R16G16_UNORM:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        case WINED3DFMT_B2G3R3_UNORM:
            TRACE_(d3d_caps)("[FAILED] - Not supported on Windows\n");
            return FALSE;

        /*****
         *  Not supported: Palettized
         *  Only some Geforce/Voodoo3/G400 cards offer 8-bit textures in case of <=Direct3D7.
         *  Since it is not widely available, don't offer it. Further no Windows driver offers
         *  WINED3DFMT_P8_UINT_A8_NORM, so don't offer it either.
         */
        case WINED3DFMT_P8_UINT:
        case WINED3DFMT_P8_UINT_A8_UNORM:
            return FALSE;

        /*****
         *  Supported: (Alpha)-Luminance
         */
        case WINED3DFMT_L8_UNORM:
        case WINED3DFMT_L8A8_UNORM:
        case WINED3DFMT_L16_UNORM:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        /* Not supported on Windows, thus disabled */
        case WINED3DFMT_L4A4_UNORM:
            TRACE_(d3d_caps)("[FAILED] - not supported on windows\n");
            return FALSE;

        /*****
         *  Supported: Depth/Stencil formats
         */
        case WINED3DFMT_D16_LOCKABLE:
        case WINED3DFMT_D16_UNORM:
        case WINED3DFMT_X8D24_UNORM:
        case WINED3DFMT_D24_UNORM_S8_UINT:
        case WINED3DFMT_S8_UINT_D24_FLOAT:
        case WINED3DFMT_D32_UNORM:
        case WINED3DFMT_D32_FLOAT:
            return TRUE;

        case WINED3DFMT_INTZ:
            if (gl_info->supported[EXT_PACKED_DEPTH_STENCIL]
                    || gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
                return TRUE;
            return FALSE;

        /* Not supported on Windows */
        case WINED3DFMT_S1_UINT_D15_UNORM:
        case WINED3DFMT_S4X4_UINT_D24_UNORM:
            TRACE_(d3d_caps)("[FAILED] - not supported on windows\n");
            return FALSE;

        /*****
         *  Not supported everywhere(depends on GL_ATI_envmap_bumpmap or
         *  GL_NV_texture_shader). Emulated by shaders
         */
        case WINED3DFMT_R8G8_SNORM:
        case WINED3DFMT_R8G8_SNORM_L8X8_UNORM:
        case WINED3DFMT_R5G5_SNORM_L6_UNORM:
        case WINED3DFMT_R8G8B8A8_SNORM:
        case WINED3DFMT_R16G16_SNORM:
            /* Ask the shader backend if it can deal with the conversion. If
             * we've got a GL extension giving native support this will be an
             * identity conversion. */
            if (adapter->shader_backend->shader_color_fixup_supported(format->color_fixup))
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
            if (gl_info->supported[EXT_TEXTURE_COMPRESSION_S3TC])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;


        /*****
         *  Odd formats - not supported
         */
        case WINED3DFMT_VERTEXDATA:
        case WINED3DFMT_R16_UINT:
        case WINED3DFMT_R32_UINT:
        case WINED3DFMT_R16G16B16A16_SNORM:
        case WINED3DFMT_R10G10B10_SNORM_A2_UNORM:
        case WINED3DFMT_R10G11B11_SNORM:
        case WINED3DFMT_R16:
        case WINED3DFMT_AL16:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

        /*****
         *  WINED3DFMT_R8G8_SNORM_Cx: Not supported right now
         */
        case WINED3DFMT_R8G8_SNORM_Cx:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

        /* YUV formats */
        case WINED3DFMT_UYVY:
        case WINED3DFMT_YUY2:
            if (gl_info->supported[APPLE_YCBCR_422])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
        case WINED3DFMT_YV12:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_R16G16B16A16_UNORM:
            if (gl_info->quirks & WINED3D_QUIRK_BROKEN_RGBA16)
            {
                TRACE_(d3d_caps)("[FAILED]\n");
                return FALSE;
            }
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

            /* Not supported */
        case WINED3DFMT_B2G3R3A8_UNORM:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

            /* Floating point formats */
        case WINED3DFMT_R16_FLOAT:
        case WINED3DFMT_R16G16_FLOAT:
        case WINED3DFMT_R16G16B16A16_FLOAT:
            if (gl_info->supported[ARB_TEXTURE_FLOAT] && gl_info->supported[ARB_HALF_FLOAT_PIXEL])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_R32_FLOAT:
        case WINED3DFMT_R32G32_FLOAT:
        case WINED3DFMT_R32G32B32A32_FLOAT:
            if (gl_info->supported[ARB_TEXTURE_FLOAT])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        /* ATI instancing hack: Although ATI cards do not support Shader Model 3.0, they support
         * instancing. To query if the card supports instancing CheckDeviceFormat with the special format
         * MAKEFOURCC('I','N','S','T') is used. Should a (broken) app check for this provide a proper return value.
         * We can do instancing with all shader versions, but we need vertex shaders.
         *
         * Additionally applications have to set the D3DRS_POINTSIZE render state to MAKEFOURCC('I','N','S','T') once
         * to enable instancing. WineD3D doesn't need that and just ignores it.
         *
         * With Shader Model 3.0 capable cards Instancing 'just works' in Windows.
         */
        case WINED3DFMT_INST:
            TRACE("ATI Instancing check hack\n");
            if (gl_info->supported[ARB_VERTEX_PROGRAM] || gl_info->supported[ARB_VERTEX_SHADER])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        /* Some weird FOURCC formats */
        case WINED3DFMT_R8G8_B8G8:
        case WINED3DFMT_G8R8_G8B8:
        case WINED3DFMT_MULTI2_ARGB8:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        /* Vendor specific formats */
        case WINED3DFMT_ATI2N:
            if (gl_info->supported[ATI_TEXTURE_COMPRESSION_3DC]
                    || gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC])
            {
                if (adapter->shader_backend->shader_color_fixup_supported(format->color_fixup)
                        && adapter->fragment_pipe->color_fixup_supported(format->color_fixup))
                {
                    TRACE_(d3d_caps)("[OK]\n");
                    return TRUE;
                }

                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        /* Depth bound test. To query if the card supports it CheckDeviceFormat with the special
         * format MAKEFOURCC('N','V','D','B') is used.
         * It is enabled by setting D3DRS_ADAPTIVETESS_X render state to MAKEFOURCC('N','V','D','B') and
         * then controlled by setting D3DRS_ADAPTIVETESS_Z (zMin) and D3DRS_ADAPTIVETESS_W (zMax)
         * to test value.
         */
        case WINED3DFMT_NVDB:
            if (gl_info->supported[EXT_DEPTH_BOUNDS_TEST])
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_NVHU:
        case WINED3DFMT_NVHS:
            /* These formats seem to be similar to the HILO formats in GL_NV_texture_shader. NVHU
             * is said to be GL_UNSIGNED_HILO16, NVHS GL_SIGNED_HILO16. Rumours say that d3d computes
             * a 3rd channel similarly to D3DFMT_CxV8U8(So NVHS could be called D3DFMT_CxV16U16).
             * ATI refused to support formats which can easily be emulated with pixel shaders, so
             * Applications have to deal with not having NVHS and NVHU.
             */
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_NULL:
            if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
                return TRUE;
            return FALSE;

        case WINED3DFMT_UNKNOWN:
            return FALSE;

        default:
            ERR("Unhandled format %s.\n", debug_d3dformat(format->id));
            break;
    }
    return FALSE;
}

static BOOL CheckSurfaceCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format,
        const struct wined3d_format *check_format,
        WINED3DSURFTYPE SurfaceType)
{
    if (SurfaceType == SURFACE_GDI)
    {
        switch (check_format->id)
        {
            case WINED3DFMT_B8G8R8_UNORM:
                TRACE_(d3d_caps)("[FAILED] - Not enumerated on Windows\n");
                return FALSE;
            case WINED3DFMT_B8G8R8A8_UNORM:
            case WINED3DFMT_B8G8R8X8_UNORM:
            case WINED3DFMT_B5G6R5_UNORM:
            case WINED3DFMT_B5G5R5X1_UNORM:
            case WINED3DFMT_B5G5R5A1_UNORM:
            case WINED3DFMT_B4G4R4A4_UNORM:
            case WINED3DFMT_B2G3R3_UNORM:
            case WINED3DFMT_A8_UNORM:
            case WINED3DFMT_B2G3R3A8_UNORM:
            case WINED3DFMT_B4G4R4X4_UNORM:
            case WINED3DFMT_R10G10B10A2_UNORM:
            case WINED3DFMT_R8G8B8A8_UNORM:
            case WINED3DFMT_R8G8B8X8_UNORM:
            case WINED3DFMT_R16G16_UNORM:
            case WINED3DFMT_B10G10R10A2_UNORM:
            case WINED3DFMT_R16G16B16A16_UNORM:
            case WINED3DFMT_P8_UINT:
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            default:
                TRACE_(d3d_caps)("[FAILED] - not available on GDI surfaces\n");
                return FALSE;
        }
    }

    /* All format that are supported for textures are supported for surfaces as well */
    if (CheckTextureCapability(adapter, check_format)) return TRUE;
    /* All depth stencil formats are supported on surfaces */
    if (CheckDepthStencilCapability(adapter, adapter_format, check_format)) return TRUE;

    /* If opengl can't process the format natively, the blitter may be able to convert it */
    if (adapter->blitter->blit_supported(&adapter->gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
            NULL, WINED3D_POOL_DEFAULT, 0, check_format,
            NULL, WINED3D_POOL_DEFAULT, 0, adapter_format))
    {
        TRACE_(d3d_caps)("[OK]\n");
        return TRUE;
    }

    /* Reject other formats */
    TRACE_(d3d_caps)("[FAILED]\n");
    return FALSE;
}

static BOOL CheckVertexTextureCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *format)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    if (!gl_info->limits.vertex_samplers || !(format->flags & WINED3DFMT_FLAG_VTF))
        return FALSE;

    switch (format->id)
    {
        case WINED3DFMT_R32G32B32A32_FLOAT:
        case WINED3DFMT_R32_FLOAT:
            return TRUE;
        default:
            return !(gl_info->quirks & WINED3D_QUIRK_LIMITED_TEX_FILTERING);
    }
}

HRESULT CDECL wined3d_check_device_format(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id, DWORD usage,
        enum wined3d_resource_type resource_type, enum wined3d_format_id check_format_id, WINED3DSURFTYPE surface_type)
{
    const struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    const struct wined3d_format *adapter_format = wined3d_get_format(gl_info, adapter_format_id);
    const struct wined3d_format *format = wined3d_get_format(gl_info, check_format_id);
    DWORD usage_caps = 0;

    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, device_type %s, adapter_format %s, usage %s, %s,\n"
            "resource_type %s, check_format %s, surface_type %#x.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(adapter_format_id),
            debug_d3dusage(usage), debug_d3dusagequery(usage), debug_d3dresourcetype(resource_type),
            debug_d3dformat(check_format_id), surface_type);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    switch (resource_type)
    {
        case WINED3D_RTYPE_CUBE_TEXTURE:
            /* Cubetexture allows:
             *      - WINED3DUSAGE_AUTOGENMIPMAP
             *      - WINED3DUSAGE_DEPTHSTENCIL
             *      - WINED3DUSAGE_DYNAMIC
             *      - WINED3DUSAGE_NONSECURE (d3d9ex)
             *      - WINED3DUSAGE_RENDERTARGET
             *      - WINED3DUSAGE_SOFTWAREPROCESSING
             *      - WINED3DUSAGE_QUERY_WRAPANDMIP
             */
            if (surface_type != SURFACE_OPENGL)
            {
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (!gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            {
                TRACE_(d3d_caps)("[FAILED] - No cube texture support\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (!CheckTextureCapability(adapter, format))
            {
                TRACE_(d3d_caps)("[FAILED] - Cube texture format not supported\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (usage & WINED3DUSAGE_AUTOGENMIPMAP)
            {
                if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
                    /* When autogenmipmap isn't around continue and return
                     * WINED3DOK_NOAUTOGEN instead of D3D_OK. */
                    TRACE_(d3d_caps)("[FAILED] - No autogenmipmap support, but continuing\n");
                else
                    usage_caps |= WINED3DUSAGE_AUTOGENMIPMAP;
            }

            /* Always report dynamic locking. */
            if (usage & WINED3DUSAGE_DYNAMIC)
                usage_caps |= WINED3DUSAGE_DYNAMIC;

            if (usage & WINED3DUSAGE_RENDERTARGET)
            {
                if (!CheckRenderTargetCapability(adapter, adapter_format, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_RENDERTARGET;
            }

            /* Always report software processing. */
            if (usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                usage_caps |= WINED3DUSAGE_SOFTWAREPROCESSING;

            if (usage & WINED3DUSAGE_QUERY_FILTER)
            {
                if (!CheckFilterCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_FILTER;
            }

            if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
            {
                if (!CheckPostPixelShaderBlendingCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBREAD)
            {
                if (!CheckSrgbReadCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBREAD;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBWRITE)
            {
                if (!CheckSrgbWriteCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBWRITE;
            }

            if (usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE)
            {
                if (!CheckVertexTextureCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
            }

            if (usage & WINED3DUSAGE_QUERY_WRAPANDMIP)
            {
                if (!CheckWrapAndMipCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
            }
            break;

        case WINED3D_RTYPE_SURFACE:
            /* Surface allows:
             *      - WINED3DUSAGE_DEPTHSTENCIL
             *      - WINED3DUSAGE_NONSECURE (d3d9ex)
             *      - WINED3DUSAGE_RENDERTARGET
             */
            if (!CheckSurfaceCapability(adapter, adapter_format, format, surface_type))
            {
                TRACE_(d3d_caps)("[FAILED] - Not supported for plain surfaces\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (usage & WINED3DUSAGE_DEPTHSTENCIL)
            {
                if (!CheckDepthStencilCapability(adapter, adapter_format, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No depthstencil support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_DEPTHSTENCIL;
            }

            if (usage & WINED3DUSAGE_RENDERTARGET)
            {
                if (!CheckRenderTargetCapability(adapter, adapter_format, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_RENDERTARGET;
            }

            if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
            {
                if (!CheckPostPixelShaderBlendingCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            }
            break;

        case WINED3D_RTYPE_TEXTURE:
            /* Texture allows:
             *      - WINED3DUSAGE_AUTOGENMIPMAP
             *      - WINED3DUSAGE_DEPTHSTENCIL
             *      - WINED3DUSAGE_DMAP
             *      - WINED3DUSAGE_DYNAMIC
             *      - WINED3DUSAGE_NONSECURE (d3d9ex)
             *      - WINED3DUSAGE_RENDERTARGET
             *      - WINED3DUSAGE_SOFTWAREPROCESSING
             *      - WINED3DUSAGE_TEXTAPI (d3d9ex)
             *      - WINED3DUSAGE_QUERY_WRAPANDMIP
             */
            if (surface_type != SURFACE_OPENGL)
            {
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (!CheckTextureCapability(adapter, format))
            {
                TRACE_(d3d_caps)("[FAILED] - Texture format not supported\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (usage & WINED3DUSAGE_AUTOGENMIPMAP)
            {
                if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
                    /* When autogenmipmap isn't around continue and return
                     * WINED3DOK_NOAUTOGEN instead of D3D_OK. */
                    TRACE_(d3d_caps)("[FAILED] - No autogenmipmap support, but continuing\n");
                else
                    usage_caps |= WINED3DUSAGE_AUTOGENMIPMAP;
            }

            /* Always report dynamic locking. */
            if (usage & WINED3DUSAGE_DYNAMIC)
                usage_caps |= WINED3DUSAGE_DYNAMIC;

            if (usage & WINED3DUSAGE_RENDERTARGET)
            {
                if (!CheckRenderTargetCapability(adapter, adapter_format, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_RENDERTARGET;
            }

            /* Always report software processing. */
            if (usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                usage_caps |= WINED3DUSAGE_SOFTWAREPROCESSING;

            if (usage & WINED3DUSAGE_QUERY_FILTER)
            {
                if (!CheckFilterCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_FILTER;
            }

            if (usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP)
            {
                if (!CheckBumpMapCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No legacy bumpmap support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_LEGACYBUMPMAP;
            }

            if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
            {
                if (!CheckPostPixelShaderBlendingCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBREAD)
            {
                if (!CheckSrgbReadCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBREAD;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBWRITE)
            {
                if (!CheckSrgbWriteCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBWRITE;
            }

            if (usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE)
            {
                if (!CheckVertexTextureCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
            }

            if (usage & WINED3DUSAGE_QUERY_WRAPANDMIP)
            {
                if (!CheckWrapAndMipCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
            }

            if (usage & WINED3DUSAGE_DEPTHSTENCIL)
            {
                if (!CheckDepthStencilCapability(adapter, adapter_format, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No depth stencil support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                if ((format->flags & WINED3DFMT_FLAG_SHADOW) && !gl_info->supported[ARB_SHADOW])
                {
                    TRACE_(d3d_caps)("[FAILED] - No shadow sampler support.\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_DEPTHSTENCIL;
            }
            break;

        case WINED3D_RTYPE_VOLUME_TEXTURE:
        case WINED3D_RTYPE_VOLUME:
            /* Volume is to VolumeTexture what Surface is to Texture, but its
             * usage caps are not documented. Most driver seem to offer
             * (nearly) the same on Volume and VolumeTexture, so do that too.
             *
             * Volumetexture allows:
             *      - D3DUSAGE_DYNAMIC
             *      - D3DUSAGE_NONSECURE (d3d9ex)
             *      - D3DUSAGE_SOFTWAREPROCESSING
             *      - D3DUSAGE_QUERY_WRAPANDMIP
             */
            if (surface_type != SURFACE_OPENGL)
            {
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (!gl_info->supported[EXT_TEXTURE3D])
            {
                TRACE_(d3d_caps)("[FAILED] - No volume texture support\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            if (!CheckTextureCapability(adapter, format))
            {
                TRACE_(d3d_caps)("[FAILED] - Format not supported\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            /* Filter formats that need conversion; For one part, this
             * conversion is unimplemented, and volume textures are huge, so
             * it would be a big performance hit. Unless we hit an application
             * needing one of those formats, don't advertize them to avoid
             * leading applications into temptation. The windows drivers don't
             * support most of those formats on volumes anyway, except for
             * WINED3DFMT_R32_FLOAT. */
            switch (check_format_id)
            {
                case WINED3DFMT_P8_UINT:
                case WINED3DFMT_L4A4_UNORM:
                case WINED3DFMT_R32_FLOAT:
                case WINED3DFMT_R16_FLOAT:
                case WINED3DFMT_R8G8_SNORM_L8X8_UNORM:
                case WINED3DFMT_R5G5_SNORM_L6_UNORM:
                case WINED3DFMT_R16G16_UNORM:
                    TRACE_(d3d_caps)("[FAILED] - No converted formats on volumes\n");
                    return WINED3DERR_NOTAVAILABLE;

                case WINED3DFMT_R8G8B8A8_SNORM:
                case WINED3DFMT_R16G16_SNORM:
                    if (!gl_info->supported[NV_TEXTURE_SHADER])
                    {
                        TRACE_(d3d_caps)("[FAILED] - No converted formats on volumes\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                    break;

                case WINED3DFMT_R8G8_SNORM:
                    if (!gl_info->supported[NV_TEXTURE_SHADER])
                    {
                        TRACE_(d3d_caps)("[FAILED] - No converted formats on volumes\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                    break;

                case WINED3DFMT_DXT1:
                case WINED3DFMT_DXT2:
                case WINED3DFMT_DXT3:
                case WINED3DFMT_DXT4:
                case WINED3DFMT_DXT5:
                    /* The GL_EXT_texture_compression_s3tc spec requires that
                     * loading an s3tc compressed texture results in an error.
                     * While the D3D refrast does support s3tc volumes, at
                     * least the nvidia windows driver does not, so we're free
                     * not to support this format. */
                    TRACE_(d3d_caps)("[FAILED] - DXTn does not support 3D textures\n");
                    return WINED3DERR_NOTAVAILABLE;

                default:
                    /* Do nothing, continue with checking the format below */
                    break;
            }

            /* Always report dynamic locking. */
            if (usage & WINED3DUSAGE_DYNAMIC)
                usage_caps |= WINED3DUSAGE_DYNAMIC;

            /* Always report software processing. */
            if (usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                usage_caps |= WINED3DUSAGE_SOFTWAREPROCESSING;

            if (usage & WINED3DUSAGE_QUERY_FILTER)
            {
                if (!CheckFilterCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_FILTER;
            }

            if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
            {
                if (!CheckPostPixelShaderBlendingCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBREAD)
            {
                if (!CheckSrgbReadCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBREAD;
            }

            if (usage & WINED3DUSAGE_QUERY_SRGBWRITE)
            {
                if (!CheckSrgbWriteCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_SRGBWRITE;
            }

            if (usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE)
            {
                if (!CheckVertexTextureCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
            }

            if (usage & WINED3DUSAGE_QUERY_WRAPANDMIP)
            {
                if (!CheckWrapAndMipCapability(adapter, format))
                {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                usage_caps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
            }
            break;

        default:
            FIXME_(d3d_caps)("Unhandled resource type %s.\n", debug_d3dresourcetype(resource_type));
            return WINED3DERR_NOTAVAILABLE;
    }

    /* When the usage_caps exactly matches usage return WINED3D_OK except for
     * the situation in which WINED3DUSAGE_AUTOGENMIPMAP isn't around, then
     * WINED3DOK_NOAUTOGEN is returned if all the other usage flags match. */
    if (usage_caps == usage)
        return WINED3D_OK;
    if (usage_caps == (usage & ~WINED3DUSAGE_AUTOGENMIPMAP))
        return WINED3DOK_NOAUTOGEN;

    TRACE_(d3d_caps)("[FAILED] - Usage %#x requested for format %s and resource_type %s but only %#x is available.\n",
            usage, debug_d3dformat(check_format_id), debug_d3dresourcetype(resource_type), usage_caps);

    return WINED3DERR_NOTAVAILABLE;
}

HRESULT CDECL wined3d_check_device_format_conversion(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id src_format, enum wined3d_format_id dst_format)
{
    FIXME("wined3d %p, adapter_idx %u, device_type %s, src_format %s, dst_format %s stub!\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(src_format),
            debug_d3dformat(dst_format));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_check_device_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id display_format,
        enum wined3d_format_id backbuffer_format, BOOL windowed)
{
    UINT mode_count;
    HRESULT hr;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, display_format %s, backbuffer_format %s, windowed %#x.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(display_format),
            debug_d3dformat(backbuffer_format), windowed);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    /* The task of this function is to check whether a certain display / backbuffer format
     * combination is available on the given adapter. In fullscreen mode microsoft specified
     * that the display format shouldn't provide alpha and that ignoring alpha the backbuffer
     * and display format should match exactly.
     * In windowed mode format conversion can occur and this depends on the driver. When format
     * conversion is done, this function should nevertheless fail and applications need to use
     * CheckDeviceFormatConversion.
     * At the moment we assume that fullscreen and windowed have the same capabilities. */

    /* There are only 4 display formats. */
    if (!(display_format == WINED3DFMT_B5G6R5_UNORM
            || display_format == WINED3DFMT_B5G5R5X1_UNORM
            || display_format == WINED3DFMT_B8G8R8X8_UNORM
            || display_format == WINED3DFMT_B10G10R10A2_UNORM))
    {
        TRACE_(d3d_caps)("Format %s is not supported as display format.\n", debug_d3dformat(display_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* If the requested display format is not available, don't continue. */
    mode_count = wined3d_get_adapter_mode_count(wined3d, adapter_idx, display_format);
    if (!mode_count)
    {
        TRACE_(d3d_caps)("No available modes for display format %s.\n", debug_d3dformat(display_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Windowed mode allows you to specify WINED3DFMT_UNKNOWN for the backbuffer format,
     * it means 'reuse' the display format for the backbuffer. */
    if (!windowed && backbuffer_format == WINED3DFMT_UNKNOWN)
    {
        TRACE_(d3d_caps)("backbuffer_format WINED3FMT_UNKNOWN only available in windowed mode.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode WINED3DFMT_B5G6R5_UNORM can only be mixed with
     * backbuffer format WINED3DFMT_B5G6R5_UNORM. */
    if (display_format == WINED3DFMT_B5G6R5_UNORM && backbuffer_format != WINED3DFMT_B5G6R5_UNORM)
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s / %s.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode WINED3DFMT_B5G5R5X1_UNORM can only be mixed with
     * backbuffer formats WINED3DFMT_B5G5R5X1_UNORM and
     * WINED3DFMT_B5G5R5A1_UNORM. */
    if (display_format == WINED3DFMT_B5G5R5X1_UNORM
            && !(backbuffer_format == WINED3DFMT_B5G5R5X1_UNORM || backbuffer_format == WINED3DFMT_B5G5R5A1_UNORM))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s / %s.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode WINED3DFMT_B8G8R8X8_UNORM can only be mixed with
     * backbuffer formats WINED3DFMT_B8G8R8X8_UNORM and
     * WINED3DFMT_B8G8R8A8_UNORM. */
    if (display_format == WINED3DFMT_B8G8R8X8_UNORM
            && !(backbuffer_format == WINED3DFMT_B8G8R8X8_UNORM || backbuffer_format == WINED3DFMT_B8G8R8A8_UNORM))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s / %s.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* WINED3DFMT_B10G10R10A2_UNORM is only allowed in fullscreen mode and it
     * can only be mixed with backbuffer format WINED3DFMT_B10G10R10A2_UNORM. */
    if (display_format == WINED3DFMT_B10G10R10A2_UNORM
            && (backbuffer_format != WINED3DFMT_B10G10R10A2_UNORM || windowed))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s / %s.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Use CheckDeviceFormat to see if the backbuffer_format is usable with the given display_format */
    hr = wined3d_check_device_format(wined3d, adapter_idx, device_type, display_format,
            WINED3DUSAGE_RENDERTARGET, WINED3D_RTYPE_SURFACE, backbuffer_format, SURFACE_OPENGL);
    if (FAILED(hr))
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s / %s.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));

    return hr;
}

HRESULT CDECL wined3d_get_device_caps(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, WINED3DCAPS *caps)
{
    const struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int vs_selected_mode;
    int ps_selected_mode;
    struct shader_caps shader_caps;
    struct fragment_caps fragment_caps;
    DWORD ckey_caps, blit_caps, fx_caps, pal_caps;

    TRACE_(d3d_caps)("wined3d %p, adapter_idx %u, device_type %s, caps %p.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), caps);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);

    /* ------------------------------------------------
       The following fields apply to both d3d8 and d3d9
       ------------------------------------------------ */
    /* Not quite true, but use h/w supported by opengl I suppose */
    caps->DeviceType = (device_type == WINED3D_DEVICE_TYPE_HAL) ? WINED3D_DEVICE_TYPE_HAL : WINED3D_DEVICE_TYPE_REF;
    caps->AdapterOrdinal           = adapter_idx;

    caps->Caps                     = 0;
    caps->Caps2                    = WINED3DCAPS2_CANRENDERWINDOWED |
                                     WINED3DCAPS2_FULLSCREENGAMMA |
                                     WINED3DCAPS2_DYNAMICTEXTURES;
    if (gl_info->supported[SGIS_GENERATE_MIPMAP])
        caps->Caps2 |= WINED3DCAPS2_CANAUTOGENMIPMAP;

    caps->Caps3                    = WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |
                                     WINED3DCAPS3_COPY_TO_VIDMEM                   |
                                     WINED3DCAPS3_COPY_TO_SYSTEMMEM;

    caps->PresentationIntervals    = WINED3DPRESENT_INTERVAL_IMMEDIATE  |
                                     WINED3DPRESENT_INTERVAL_ONE;

    caps->CursorCaps               = WINED3DCURSORCAPS_COLOR            |
                                     WINED3DCURSORCAPS_LOWRES;

    caps->DevCaps                  = WINED3DDEVCAPS_FLOATTLVERTEX       |
                                     WINED3DDEVCAPS_EXECUTESYSTEMMEMORY |
                                     WINED3DDEVCAPS_TLVERTEXSYSTEMMEMORY|
                                     WINED3DDEVCAPS_TLVERTEXVIDEOMEMORY |
                                     WINED3DDEVCAPS_DRAWPRIMTLVERTEX    |
                                     WINED3DDEVCAPS_HWTRANSFORMANDLIGHT |
                                     WINED3DDEVCAPS_EXECUTEVIDEOMEMORY  |
                                     WINED3DDEVCAPS_PUREDEVICE          |
                                     WINED3DDEVCAPS_HWRASTERIZATION     |
                                     WINED3DDEVCAPS_TEXTUREVIDEOMEMORY  |
                                     WINED3DDEVCAPS_TEXTURESYSTEMMEMORY |
                                     WINED3DDEVCAPS_CANRENDERAFTERFLIP  |
                                     WINED3DDEVCAPS_DRAWPRIMITIVES2     |
                                     WINED3DDEVCAPS_DRAWPRIMITIVES2EX   |
                                     WINED3DDEVCAPS_RTPATCHES;

    caps->PrimitiveMiscCaps        = WINED3DPMISCCAPS_CULLNONE              |
                                     WINED3DPMISCCAPS_CULLCCW               |
                                     WINED3DPMISCCAPS_CULLCW                |
                                     WINED3DPMISCCAPS_COLORWRITEENABLE      |
                                     WINED3DPMISCCAPS_CLIPTLVERTS           |
                                     WINED3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                                     WINED3DPMISCCAPS_MASKZ                 |
                                     WINED3DPMISCCAPS_BLENDOP               |
                                     WINED3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING;
                                    /* TODO:
                                        WINED3DPMISCCAPS_NULLREFERENCE
                                        WINED3DPMISCCAPS_FOGANDSPECULARALPHA
                                        WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                                        WINED3DPMISCCAPS_FOGVERTEXCLAMPED */

    if (gl_info->supported[EXT_BLEND_EQUATION_SEPARATE] && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_SEPARATEALPHABLEND;
    if (gl_info->supported[EXT_DRAW_BUFFERS2])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS;

    caps->RasterCaps               = WINED3DPRASTERCAPS_DITHER    |
                                     WINED3DPRASTERCAPS_PAT       |
                                     WINED3DPRASTERCAPS_WFOG      |
                                     WINED3DPRASTERCAPS_ZFOG      |
                                     WINED3DPRASTERCAPS_FOGVERTEX |
                                     WINED3DPRASTERCAPS_FOGTABLE  |
                                     WINED3DPRASTERCAPS_STIPPLE   |
                                     WINED3DPRASTERCAPS_SUBPIXEL  |
                                     WINED3DPRASTERCAPS_ZTEST     |
                                     WINED3DPRASTERCAPS_SCISSORTEST   |
                                     WINED3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |
                                     WINED3DPRASTERCAPS_DEPTHBIAS;

    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
    {
        caps->RasterCaps  |= WINED3DPRASTERCAPS_ANISOTROPY    |
                             WINED3DPRASTERCAPS_ZBIAS         |
                             WINED3DPRASTERCAPS_MIPMAPLODBIAS;
    }
    if (gl_info->supported[NV_FOG_DISTANCE])
    {
        caps->RasterCaps          |= WINED3DPRASTERCAPS_FOGRANGE;
    }
                        /* FIXME Add:
                           WINED3DPRASTERCAPS_COLORPERSPECTIVE
                           WINED3DPRASTERCAPS_STRETCHBLTMULTISAMPLE
                           WINED3DPRASTERCAPS_ANTIALIASEDGES
                           WINED3DPRASTERCAPS_ZBUFFERLESSHSR
                           WINED3DPRASTERCAPS_WBUFFER */

    caps->ZCmpCaps =  WINED3DPCMPCAPS_ALWAYS       |
                      WINED3DPCMPCAPS_EQUAL        |
                      WINED3DPCMPCAPS_GREATER      |
                      WINED3DPCMPCAPS_GREATEREQUAL |
                      WINED3DPCMPCAPS_LESS         |
                      WINED3DPCMPCAPS_LESSEQUAL    |
                      WINED3DPCMPCAPS_NEVER        |
                      WINED3DPCMPCAPS_NOTEQUAL;

    caps->SrcBlendCaps  =  WINED3DPBLENDCAPS_BOTHINVSRCALPHA |
                           WINED3DPBLENDCAPS_BOTHSRCALPHA    |
                           WINED3DPBLENDCAPS_DESTALPHA       |
                           WINED3DPBLENDCAPS_DESTCOLOR       |
                           WINED3DPBLENDCAPS_INVDESTALPHA    |
                           WINED3DPBLENDCAPS_INVDESTCOLOR    |
                           WINED3DPBLENDCAPS_INVSRCALPHA     |
                           WINED3DPBLENDCAPS_INVSRCCOLOR     |
                           WINED3DPBLENDCAPS_ONE             |
                           WINED3DPBLENDCAPS_SRCALPHA        |
                           WINED3DPBLENDCAPS_SRCALPHASAT     |
                           WINED3DPBLENDCAPS_SRCCOLOR        |
                           WINED3DPBLENDCAPS_ZERO;

    caps->DestBlendCaps =  WINED3DPBLENDCAPS_DESTALPHA       |
                           WINED3DPBLENDCAPS_DESTCOLOR       |
                           WINED3DPBLENDCAPS_INVDESTALPHA    |
                           WINED3DPBLENDCAPS_INVDESTCOLOR    |
                           WINED3DPBLENDCAPS_INVSRCALPHA     |
                           WINED3DPBLENDCAPS_INVSRCCOLOR     |
                           WINED3DPBLENDCAPS_ONE             |
                           WINED3DPBLENDCAPS_SRCALPHA        |
                           WINED3DPBLENDCAPS_SRCCOLOR        |
                           WINED3DPBLENDCAPS_ZERO;
    /* NOTE: WINED3DPBLENDCAPS_SRCALPHASAT is not supported as dest blend factor,
     * according to the glBlendFunc manpage
     *
     * WINED3DPBLENDCAPS_BOTHINVSRCALPHA and WINED3DPBLENDCAPS_BOTHSRCALPHA are
     * legacy settings for srcblend only
     */

    if (gl_info->supported[EXT_BLEND_COLOR])
    {
        caps->SrcBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
    }


    caps->AlphaCmpCaps  = WINED3DPCMPCAPS_ALWAYS       |
                          WINED3DPCMPCAPS_EQUAL        |
                          WINED3DPCMPCAPS_GREATER      |
                          WINED3DPCMPCAPS_GREATEREQUAL |
                          WINED3DPCMPCAPS_LESS         |
                          WINED3DPCMPCAPS_LESSEQUAL    |
                          WINED3DPCMPCAPS_NEVER        |
                          WINED3DPCMPCAPS_NOTEQUAL;

    caps->ShadeCaps      = WINED3DPSHADECAPS_SPECULARGOURAUDRGB |
                           WINED3DPSHADECAPS_COLORGOURAUDRGB    |
                           WINED3DPSHADECAPS_ALPHAFLATBLEND     |
                           WINED3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                           WINED3DPSHADECAPS_COLORFLATRGB       |
                           WINED3DPSHADECAPS_FOGFLAT            |
                           WINED3DPSHADECAPS_FOGGOURAUD         |
                           WINED3DPSHADECAPS_SPECULARFLATRGB;

    caps->TextureCaps   = WINED3DPTEXTURECAPS_ALPHA              |
                          WINED3DPTEXTURECAPS_ALPHAPALETTE       |
                          WINED3DPTEXTURECAPS_TRANSPARENCY       |
                          WINED3DPTEXTURECAPS_BORDER             |
                          WINED3DPTEXTURECAPS_MIPMAP             |
                          WINED3DPTEXTURECAPS_PROJECTED          |
                          WINED3DPTEXTURECAPS_PERSPECTIVE;

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        caps->TextureCaps  |= WINED3DPTEXTURECAPS_POW2 |
                              WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->TextureCaps  |=  WINED3DPTEXTURECAPS_VOLUMEMAP      |
                               WINED3DPTEXTURECAPS_MIPVOLUMEMAP;
        if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
        {
            caps->TextureCaps |= WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;
        }
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        caps->TextureCaps  |= WINED3DPTEXTURECAPS_CUBEMAP     |
                              WINED3DPTEXTURECAPS_MIPCUBEMAP;
        if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
        {
            caps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP_POW2;
        }
    }

    caps->TextureFilterCaps =  WINED3DPTFILTERCAPS_MAGFLINEAR       |
                               WINED3DPTFILTERCAPS_MAGFPOINT        |
                               WINED3DPTFILTERCAPS_MINFLINEAR       |
                               WINED3DPTFILTERCAPS_MINFPOINT        |
                               WINED3DPTFILTERCAPS_MIPFLINEAR       |
                               WINED3DPTFILTERCAPS_MIPFPOINT        |
                               WINED3DPTFILTERCAPS_LINEAR           |
                               WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                               WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                               WINED3DPTFILTERCAPS_MIPLINEAR        |
                               WINED3DPTFILTERCAPS_MIPNEAREST       |
                               WINED3DPTFILTERCAPS_NEAREST;

    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
    {
        caps->TextureFilterCaps  |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                    WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        caps->CubeTextureFilterCaps =  WINED3DPTFILTERCAPS_MAGFLINEAR       |
                                       WINED3DPTFILTERCAPS_MAGFPOINT        |
                                       WINED3DPTFILTERCAPS_MINFLINEAR       |
                                       WINED3DPTFILTERCAPS_MINFPOINT        |
                                       WINED3DPTFILTERCAPS_MIPFLINEAR       |
                                       WINED3DPTFILTERCAPS_MIPFPOINT        |
                                       WINED3DPTFILTERCAPS_LINEAR           |
                                       WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                                       WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                                       WINED3DPTFILTERCAPS_MIPLINEAR        |
                                       WINED3DPTFILTERCAPS_MIPNEAREST       |
                                       WINED3DPTFILTERCAPS_NEAREST;

        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
        {
            caps->CubeTextureFilterCaps  |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                            WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    }
    else
    {
        caps->CubeTextureFilterCaps = 0;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->VolumeTextureFilterCaps  = WINED3DPTFILTERCAPS_MAGFLINEAR       |
                                         WINED3DPTFILTERCAPS_MAGFPOINT        |
                                         WINED3DPTFILTERCAPS_MINFLINEAR       |
                                         WINED3DPTFILTERCAPS_MINFPOINT        |
                                         WINED3DPTFILTERCAPS_MIPFLINEAR       |
                                         WINED3DPTFILTERCAPS_MIPFPOINT        |
                                         WINED3DPTFILTERCAPS_LINEAR           |
                                         WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                                         WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                                         WINED3DPTFILTERCAPS_MIPLINEAR        |
                                         WINED3DPTFILTERCAPS_MIPNEAREST       |
                                         WINED3DPTFILTERCAPS_NEAREST;
    }
    else
    {
        caps->VolumeTextureFilterCaps = 0;
    }

    caps->TextureAddressCaps  =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                 WINED3DPTADDRESSCAPS_CLAMP  |
                                 WINED3DPTADDRESSCAPS_WRAP;

    if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
    }
    if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
    }
    if (gl_info->supported[ATI_TEXTURE_MIRROR_ONCE])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->VolumeTextureAddressCaps =   WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                           WINED3DPTADDRESSCAPS_CLAMP  |
                                           WINED3DPTADDRESSCAPS_WRAP;
        if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
        }
        if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
        }
        if (gl_info->supported[ATI_TEXTURE_MIRROR_ONCE])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
        }
    }
    else
    {
        caps->VolumeTextureAddressCaps = 0;
    }

    caps->LineCaps  = WINED3DLINECAPS_TEXTURE       |
                      WINED3DLINECAPS_ZTEST         |
                      WINED3DLINECAPS_BLEND         |
                      WINED3DLINECAPS_ALPHACMP      |
                      WINED3DLINECAPS_FOG;
    /* WINED3DLINECAPS_ANTIALIAS is not supported on Windows, and dx and gl seem to have a different
     * idea how generating the smoothing alpha values works; the result is different
     */

    caps->MaxTextureWidth = gl_info->limits.texture_size;
    caps->MaxTextureHeight = gl_info->limits.texture_size;

    if (gl_info->supported[EXT_TEXTURE3D])
        caps->MaxVolumeExtent = gl_info->limits.texture3d_size;
    else
        caps->MaxVolumeExtent = 0;

    caps->MaxTextureRepeat = 32768;
    caps->MaxTextureAspectRatio = gl_info->limits.texture_size;
    caps->MaxVertexW = 1.0f;

    caps->GuardBandLeft = 0.0f;
    caps->GuardBandTop = 0.0f;
    caps->GuardBandRight = 0.0f;
    caps->GuardBandBottom = 0.0f;

    caps->ExtentsAdjust = 0.0f;

    caps->StencilCaps   = WINED3DSTENCILCAPS_DECRSAT |
                          WINED3DSTENCILCAPS_INCRSAT |
                          WINED3DSTENCILCAPS_INVERT  |
                          WINED3DSTENCILCAPS_KEEP    |
                          WINED3DSTENCILCAPS_REPLACE |
                          WINED3DSTENCILCAPS_ZERO;
    if (gl_info->supported[EXT_STENCIL_WRAP])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_DECR  |
                              WINED3DSTENCILCAPS_INCR;
    }
    if (gl_info->supported[EXT_STENCIL_TWO_SIDE] || gl_info->supported[ATI_SEPARATE_STENCIL])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    caps->FVFCaps = WINED3DFVFCAPS_PSIZE | 0x0008; /* 8 texture coords */

    caps->MaxUserClipPlanes = gl_info->limits.clipplanes;
    caps->MaxActiveLights = gl_info->limits.lights;

    caps->MaxVertexBlendMatrices = gl_info->limits.blends;
    caps->MaxVertexBlendMatrixIndex   = 0;

    caps->MaxAnisotropy = gl_info->limits.anisotropy;
    caps->MaxPointSize = gl_info->limits.pointsize_max;


    /* FIXME: Add D3DVTXPCAPS_TWEENING, D3DVTXPCAPS_TEXGEN_SPHEREMAP */
    caps->VertexProcessingCaps  = WINED3DVTXPCAPS_DIRECTIONALLIGHTS |
                                  WINED3DVTXPCAPS_MATERIALSOURCE7   |
                                  WINED3DVTXPCAPS_POSITIONALLIGHTS  |
                                  WINED3DVTXPCAPS_LOCALVIEWER       |
                                  WINED3DVTXPCAPS_VERTEXFOG         |
                                  WINED3DVTXPCAPS_TEXGEN;

    caps->MaxPrimitiveCount   = 0xFFFFF; /* For now set 2^20-1 which is used by most >=Geforce3/Radeon8500 cards */
    caps->MaxVertexIndex      = 0xFFFFF;
    caps->MaxStreams          = MAX_STREAMS;
    caps->MaxStreamStride     = 1024;

    /* d3d9.dll sets D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES here because StretchRects is implemented in d3d9 */
    caps->DevCaps2                          = WINED3DDEVCAPS2_STREAMOFFSET |
                                              WINED3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;
    caps->MaxNpatchTessellationLevel        = 0;
    caps->MasterAdapterOrdinal              = 0;
    caps->AdapterOrdinalInGroup             = 0;
    caps->NumberOfAdaptersInGroup           = 1;

    caps->NumSimultaneousRTs = gl_info->limits.buffers;

    caps->StretchRectFilterCaps               = WINED3DPTFILTERCAPS_MINFPOINT  |
                                                WINED3DPTFILTERCAPS_MAGFPOINT  |
                                                WINED3DPTFILTERCAPS_MINFLINEAR |
                                                WINED3DPTFILTERCAPS_MAGFLINEAR;
    caps->VertexTextureFilterCaps             = 0;

    adapter->shader_backend->shader_get_caps(&adapter->gl_info, &shader_caps);
    adapter->fragment_pipe->get_caps(&adapter->gl_info, &fragment_caps);

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    caps->PrimitiveMiscCaps |= fragment_caps.PrimitiveMiscCaps;

    /* This takes care for disabling vertex shader or pixel shader caps while leaving the other one enabled.
     * Ignore shader model capabilities if disabled in config
     */
    if (vs_selected_mode == SHADER_NONE)
    {
        TRACE_(d3d_caps)("Vertex shader disabled in config, reporting version 0.0\n");
        caps->VertexShaderVersion          = 0;
        caps->MaxVertexShaderConst         = 0;
    }
    else
    {
        caps->VertexShaderVersion          = shader_caps.VertexShaderVersion;
        caps->MaxVertexShaderConst         = shader_caps.MaxVertexShaderConst;
    }

    if (ps_selected_mode == SHADER_NONE)
    {
        TRACE_(d3d_caps)("Pixel shader disabled in config, reporting version 0.0\n");
        caps->PixelShaderVersion           = 0;
        caps->PixelShader1xMaxValue        = 0.0f;
    } else {
        caps->PixelShaderVersion           = shader_caps.PixelShaderVersion;
        caps->PixelShader1xMaxValue        = shader_caps.PixelShader1xMaxValue;
    }

    caps->TextureOpCaps                    = fragment_caps.TextureOpCaps;
    caps->MaxTextureBlendStages            = fragment_caps.MaxTextureBlendStages;
    caps->MaxSimultaneousTextures          = fragment_caps.MaxSimultaneousTextures;

    /* The following caps are shader specific, but they are things we cannot detect, or which
     * are the same among all shader models. So to avoid code duplication set the shader version
     * specific, but otherwise constant caps here
     */
    if (caps->VertexShaderVersion >= 3)
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the VS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * VS3.0 value. */
        caps->VS20Caps.caps = WINED3DVS20CAPS_PREDICATION;
        /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        caps->VS20Caps.dynamic_flow_control_depth = WINED3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        caps->VS20Caps.temp_count = max(32, adapter->gl_info.limits.arb_vs_temps);
        /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */
        caps->VS20Caps.static_flow_control_depth = WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH;

        caps->MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        caps->MaxVertexShader30InstructionSlots = max(512, adapter->gl_info.limits.arb_vs_instructions);
    }
    else if (caps->VertexShaderVersion == 2)
    {
        caps->VS20Caps.caps = 0;
        caps->VS20Caps.dynamic_flow_control_depth = WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        caps->VS20Caps.temp_count = max(12, adapter->gl_info.limits.arb_vs_temps);
        caps->VS20Caps.static_flow_control_depth = 1;

        caps->MaxVShaderInstructionsExecuted    = 65535;
        caps->MaxVertexShader30InstructionSlots = 0;
    }
    else
    { /* VS 1.x */
        caps->VS20Caps.caps = 0;
        caps->VS20Caps.dynamic_flow_control_depth = 0;
        caps->VS20Caps.temp_count = 0;
        caps->VS20Caps.static_flow_control_depth = 0;

        caps->MaxVShaderInstructionsExecuted    = 0;
        caps->MaxVertexShader30InstructionSlots = 0;
    }

    if (caps->PixelShaderVersion >= 3)
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the PS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * PS 3.0 value. */

        /* Caps is more or less undocumented on MSDN but it appears to be
         * used for PS20Caps based on results from R9600/FX5900/Geforce6800
         * cards from Windows */
        caps->PS20Caps.caps = WINED3DPS20CAPS_ARBITRARYSWIZZLE |
                WINED3DPS20CAPS_GRADIENTINSTRUCTIONS |
                WINED3DPS20CAPS_PREDICATION          |
                WINED3DPS20CAPS_NODEPENDENTREADLIMIT |
                WINED3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
        /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        caps->PS20Caps.dynamic_flow_control_depth = WINED3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        caps->PS20Caps.temp_count = max(32, adapter->gl_info.limits.arb_ps_temps);
        /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        caps->PS20Caps.static_flow_control_depth = WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH;
        /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */
        caps->PS20Caps.instruction_slot_count = WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS;

        caps->MaxPShaderInstructionsExecuted = 65535;
        caps->MaxPixelShader30InstructionSlots = max(WINED3DMIN30SHADERINSTRUCTIONS,
                adapter->gl_info.limits.arb_ps_instructions);
    }
    else if(caps->PixelShaderVersion == 2)
    {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        caps->PS20Caps.caps = 0;
        caps->PS20Caps.dynamic_flow_control_depth = 0; /* WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        caps->PS20Caps.temp_count = max(12, adapter->gl_info.limits.arb_ps_temps);
        caps->PS20Caps.static_flow_control_depth = WINED3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minimum: 1 */
        /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */
        caps->PS20Caps.instruction_slot_count = WINED3DPS20_MIN_NUMINSTRUCTIONSLOTS;

        caps->MaxPShaderInstructionsExecuted    = 512; /* Minimum value, a GeforceFX uses 1024 */
        caps->MaxPixelShader30InstructionSlots  = 0;
    }
    else /* PS 1.x */
    {
        caps->PS20Caps.caps = 0;
        caps->PS20Caps.dynamic_flow_control_depth = 0;
        caps->PS20Caps.temp_count = 0;
        caps->PS20Caps.static_flow_control_depth = 0;
        caps->PS20Caps.instruction_slot_count = 0;

        caps->MaxPShaderInstructionsExecuted    = 0;
        caps->MaxPixelShader30InstructionSlots  = 0;
    }

    if (caps->VertexShaderVersion >= 2)
    {
        /* OpenGL supports all the formats below, perhaps not always
         * without conversion, but it supports them.
         * Further GLSL doesn't seem to have an official unsigned type so
         * don't advertise it yet as I'm not sure how we handle it.
         * We might need to add some clamping in the shader engine to
         * support it.
         * TODO: WINED3DDTCAPS_USHORT2N, WINED3DDTCAPS_USHORT4N, WINED3DDTCAPS_UDEC3, WINED3DDTCAPS_DEC3N */
        caps->DeclTypes = WINED3DDTCAPS_UBYTE4    |
                          WINED3DDTCAPS_UBYTE4N   |
                          WINED3DDTCAPS_SHORT2N   |
                          WINED3DDTCAPS_SHORT4N;
        if (gl_info->supported[ARB_HALF_FLOAT_VERTEX])
        {
            caps->DeclTypes |= WINED3DDTCAPS_FLOAT16_2 |
                               WINED3DDTCAPS_FLOAT16_4;
        }
    }
    else
    {
        caps->DeclTypes = 0;
    }

    /* Set DirectDraw helper Caps */
    ckey_caps =                         WINEDDCKEYCAPS_DESTBLT              |
                                        WINEDDCKEYCAPS_SRCBLT;
    fx_caps =                           WINEDDFXCAPS_BLTALPHA               |
                                        WINEDDFXCAPS_BLTMIRRORLEFTRIGHT     |
                                        WINEDDFXCAPS_BLTMIRRORUPDOWN        |
                                        WINEDDFXCAPS_BLTROTATION90          |
                                        WINEDDFXCAPS_BLTSHRINKX             |
                                        WINEDDFXCAPS_BLTSHRINKXN            |
                                        WINEDDFXCAPS_BLTSHRINKY             |
                                        WINEDDFXCAPS_BLTSHRINKXN            |
                                        WINEDDFXCAPS_BLTSTRETCHX            |
                                        WINEDDFXCAPS_BLTSTRETCHXN           |
                                        WINEDDFXCAPS_BLTSTRETCHY            |
                                        WINEDDFXCAPS_BLTSTRETCHYN;
    blit_caps =                         WINEDDCAPS_BLT                      |
                                        WINEDDCAPS_BLTCOLORFILL             |
                                        WINEDDCAPS_BLTDEPTHFILL             |
                                        WINEDDCAPS_BLTSTRETCH               |
                                        WINEDDCAPS_CANBLTSYSMEM             |
                                        WINEDDCAPS_CANCLIP                  |
                                        WINEDDCAPS_CANCLIPSTRETCHED         |
                                        WINEDDCAPS_COLORKEY                 |
                                        WINEDDCAPS_COLORKEYHWASSIST         |
                                        WINEDDCAPS_ALIGNBOUNDARYSRC;
    pal_caps =                          WINEDDPCAPS_8BIT                    |
                                        WINEDDPCAPS_PRIMARYSURFACE;

    /* Fill the ddraw caps structure */
    caps->ddraw_caps.caps =             WINEDDCAPS_GDI                      |
                                        WINEDDCAPS_PALETTE                  |
                                        blit_caps;
    caps->ddraw_caps.caps2 =            WINEDDCAPS2_CERTIFIED               |
                                        WINEDDCAPS2_NOPAGELOCKREQUIRED      |
                                        WINEDDCAPS2_PRIMARYGAMMA            |
                                        WINEDDCAPS2_WIDESURFACES            |
                                        WINEDDCAPS2_CANRENDERWINDOWED;
    caps->ddraw_caps.color_key_caps = ckey_caps;
    caps->ddraw_caps.fx_caps = fx_caps;
    caps->ddraw_caps.pal_caps = pal_caps;
    caps->ddraw_caps.svb_caps = blit_caps;
    caps->ddraw_caps.svb_color_key_caps = ckey_caps;
    caps->ddraw_caps.svb_fx_caps = fx_caps;
    caps->ddraw_caps.vsb_caps = blit_caps;
    caps->ddraw_caps.vsb_color_key_caps = ckey_caps;
    caps->ddraw_caps.vsb_fx_caps = fx_caps;
    caps->ddraw_caps.ssb_caps = blit_caps;
    caps->ddraw_caps.ssb_color_key_caps = ckey_caps;
    caps->ddraw_caps.ssb_fx_caps = fx_caps;

    caps->ddraw_caps.dds_caps =         WINEDDSCAPS_ALPHA                   |
                                        WINEDDSCAPS_BACKBUFFER              |
                                        WINEDDSCAPS_FLIP                    |
                                        WINEDDSCAPS_FRONTBUFFER             |
                                        WINEDDSCAPS_OFFSCREENPLAIN          |
                                        WINEDDSCAPS_PALETTE                 |
                                        WINEDDSCAPS_PRIMARYSURFACE          |
                                        WINEDDSCAPS_SYSTEMMEMORY            |
                                        WINEDDSCAPS_VIDEOMEMORY             |
                                        WINEDDSCAPS_VISIBLE;
    caps->ddraw_caps.stride_align = DDRAW_PITCH_ALIGNMENT;

    /* Set D3D caps if OpenGL is available. */
    if (adapter->opengl)
    {
        caps->ddraw_caps.dds_caps |=    WINEDDSCAPS_3DDEVICE                |
                                        WINEDDSCAPS_MIPMAP                  |
                                        WINEDDSCAPS_TEXTURE                 |
                                        WINEDDSCAPS_ZBUFFER;
        caps->ddraw_caps.caps |=        WINEDDCAPS_3D;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_create(struct wined3d *wined3d, UINT adapter_idx, enum wined3d_device_type device_type,
        HWND focus_window, DWORD flags, BYTE surface_alignment, struct wined3d_device_parent *device_parent,
        struct wined3d_device **device)
{
    struct wined3d_device *object;
    HRESULT hr;

    TRACE("wined3d %p, adapter_idx %u, device_type %#x, focus_window %p, flags %#x, device_parent %p, device %p.\n",
            wined3d, adapter_idx, device_type, focus_window, flags, device_parent, device);

    /* Validate the adapter number. If no adapters are available(no GL), ignore the adapter
     * number and create a device without a 3D adapter for 2D only operation. */
    if (wined3d->adapter_count && adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate device memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = device_init(object, wined3d, adapter_idx, device_type,
            focus_window, flags, surface_alignment, device_parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = object;

    device_parent->ops->wined3d_device_created(device_parent, *device);

    return WINED3D_OK;
}

void * CDECL wined3d_get_parent(const struct wined3d *wined3d)
{
    TRACE("wined3d %p.\n", wined3d);

    return wined3d->parent;
}

static void WINE_GLAPI invalid_func(const void *data)
{
    ERR("Invalid vertex attribute function called\n");
    DebugBreak();
}

static void WINE_GLAPI invalid_texcoord_func(GLenum unit, const void *data)
{
    ERR("Invalid texcoord function called\n");
    DebugBreak();
}

/* Helper functions for providing vertex data to opengl. The arrays are initialized based on
 * the extension detection and are used in drawStridedSlow
 */
static void WINE_GLAPI position_d3dcolor(const void *data)
{
    DWORD pos = *((const DWORD *)data);

    FIXME("Add a test for fixed function position from d3dcolor type\n");
    glVertex4s(D3DCOLOR_B_R(pos),
               D3DCOLOR_B_G(pos),
               D3DCOLOR_B_B(pos),
               D3DCOLOR_B_A(pos));
}

static void WINE_GLAPI position_float4(const void *data)
{
    const GLfloat *pos = data;

    if (pos[3] != 0.0f && pos[3] != 1.0f)
    {
        float w = 1.0f / pos[3];

        glVertex4f(pos[0] * w, pos[1] * w, pos[2] * w, w);
    }
    else
    {
        glVertex3fv(pos);
    }
}

static void WINE_GLAPI diffuse_d3dcolor(const void *data)
{
    DWORD diffuseColor = *((const DWORD *)data);

    glColor4ub(D3DCOLOR_B_R(diffuseColor),
               D3DCOLOR_B_G(diffuseColor),
               D3DCOLOR_B_B(diffuseColor),
               D3DCOLOR_B_A(diffuseColor));
}

static void WINE_GLAPI specular_d3dcolor(const void *data)
{
    DWORD specularColor = *((const DWORD *)data);
    GLbyte d[] = {D3DCOLOR_B_R(specularColor),
            D3DCOLOR_B_G(specularColor),
            D3DCOLOR_B_B(specularColor)};

    specular_func_3ubv(d);
}

static void WINE_GLAPI warn_no_specular_func(const void *data)
{
    WARN("GL_EXT_secondary_color not supported\n");
}

static void fillGLAttribFuncs(const struct wined3d_gl_info *gl_info)
{
    position_funcs[WINED3D_FFP_EMIT_FLOAT1]      = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_FLOAT2]      = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_FLOAT3]      = (glAttribFunc)glVertex3fv;
    position_funcs[WINED3D_FFP_EMIT_FLOAT4]      = position_float4;
    position_funcs[WINED3D_FFP_EMIT_D3DCOLOR]    = position_d3dcolor;
    position_funcs[WINED3D_FFP_EMIT_UBYTE4]      = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_SHORT2]      = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_SHORT4]      = (glAttribFunc)glVertex2sv;
    position_funcs[WINED3D_FFP_EMIT_UBYTE4N]     = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_SHORT2N]     = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_SHORT4N]     = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_USHORT2N]    = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_USHORT4N]    = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_UDEC3]       = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_DEC3N]       = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_FLOAT16_2]   = invalid_func;
    position_funcs[WINED3D_FFP_EMIT_FLOAT16_4]   = invalid_func;

    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT1]       = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT2]       = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT3]       = (glAttribFunc)glColor3fv;
    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT4]       = (glAttribFunc)glColor4fv;
    diffuse_funcs[WINED3D_FFP_EMIT_D3DCOLOR]     = diffuse_d3dcolor;
    diffuse_funcs[WINED3D_FFP_EMIT_UBYTE4]       = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_SHORT2]       = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_SHORT4]       = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_UBYTE4N]      = (glAttribFunc)glColor4ubv;
    diffuse_funcs[WINED3D_FFP_EMIT_SHORT2N]      = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_SHORT4N]      = (glAttribFunc)glColor4sv;
    diffuse_funcs[WINED3D_FFP_EMIT_USHORT2N]     = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_USHORT4N]     = (glAttribFunc)glColor4usv;
    diffuse_funcs[WINED3D_FFP_EMIT_UDEC3]        = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_DEC3N]        = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT16_2]    = invalid_func;
    diffuse_funcs[WINED3D_FFP_EMIT_FLOAT16_4]    = invalid_func;

    /* No 4 component entry points here */
    specular_funcs[WINED3D_FFP_EMIT_FLOAT1]      = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_FLOAT2]      = invalid_func;
    if (gl_info->supported[EXT_SECONDARY_COLOR])
    {
        specular_funcs[WINED3D_FFP_EMIT_FLOAT3]  = (glAttribFunc)GL_EXTCALL(glSecondaryColor3fvEXT);
    }
    else
    {
        specular_funcs[WINED3D_FFP_EMIT_FLOAT3]  = warn_no_specular_func;
    }
    specular_funcs[WINED3D_FFP_EMIT_FLOAT4]      = invalid_func;
    if (gl_info->supported[EXT_SECONDARY_COLOR])
    {
        specular_func_3ubv = (glAttribFunc)GL_EXTCALL(glSecondaryColor3ubvEXT);
        specular_funcs[WINED3D_FFP_EMIT_D3DCOLOR] = specular_d3dcolor;
    }
    else
    {
        specular_funcs[WINED3D_FFP_EMIT_D3DCOLOR] = warn_no_specular_func;
    }
    specular_funcs[WINED3D_FFP_EMIT_UBYTE4]      = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_SHORT2]      = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_SHORT4]      = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_UBYTE4N]     = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_SHORT2N]     = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_SHORT4N]     = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_USHORT2N]    = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_USHORT4N]    = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_UDEC3]       = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_DEC3N]       = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_FLOAT16_2]   = invalid_func;
    specular_funcs[WINED3D_FFP_EMIT_FLOAT16_4]   = invalid_func;

    /* Only 3 component entry points here. Test how others behave. Float4 normals are used
     * by one of our tests, trying to pass it to the pixel shader, which fails on Windows.
     */
    normal_funcs[WINED3D_FFP_EMIT_FLOAT1]         = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_FLOAT2]         = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_FLOAT3]         = (glAttribFunc)glNormal3fv;
    normal_funcs[WINED3D_FFP_EMIT_FLOAT4]         = (glAttribFunc)glNormal3fv; /* Just ignore the 4th value */
    normal_funcs[WINED3D_FFP_EMIT_D3DCOLOR]       = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_UBYTE4]         = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_SHORT2]         = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_SHORT4]         = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_UBYTE4N]        = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_SHORT2N]        = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_SHORT4N]        = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_USHORT2N]       = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_USHORT4N]       = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_UDEC3]          = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_DEC3N]          = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_FLOAT16_2]      = invalid_func;
    normal_funcs[WINED3D_FFP_EMIT_FLOAT16_4]      = invalid_func;

    multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT1]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord1fvARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT2]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord2fvARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT3]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord3fvARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT4]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord4fvARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_D3DCOLOR]  = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_UBYTE4]    = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_SHORT2]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord2svARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_SHORT4]    = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord4svARB);
    multi_texcoord_funcs[WINED3D_FFP_EMIT_UBYTE4N]   = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_SHORT2N]   = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_SHORT4N]   = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_USHORT2N]  = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_USHORT4N]  = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_UDEC3]     = invalid_texcoord_func;
    multi_texcoord_funcs[WINED3D_FFP_EMIT_DEC3N]     = invalid_texcoord_func;
    if (gl_info->supported[NV_HALF_FLOAT])
    {
        /* Not supported by ARB_HALF_FLOAT_VERTEX, so check for NV_HALF_FLOAT */
        multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT16_2] = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord2hvNV);
        multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT16_4] = (glMultiTexCoordFunc)GL_EXTCALL(glMultiTexCoord4hvNV);
    } else {
        multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT16_2] = invalid_texcoord_func;
        multi_texcoord_funcs[WINED3D_FFP_EMIT_FLOAT16_4] = invalid_texcoord_func;
    }
}

/* Do not call while under the GL lock. */
static BOOL InitAdapters(struct wined3d *wined3d)
{
    static HMODULE mod_gl;
    BOOL ret;
    int ps_selected_mode, vs_selected_mode;

    /* No need to hold any lock. The calling library makes sure only one thread calls
     * wined3d simultaneously
     */

    TRACE("Initializing adapters\n");

    if(!mod_gl) {
#ifdef USE_WIN32_OPENGL
#define USE_GL_FUNC(pfn) pfn = (void*)GetProcAddress(mod_gl, #pfn);
        mod_gl = LoadLibraryA("opengl32.dll");
        if(!mod_gl) {
            ERR("Can't load opengl32.dll!\n");
            goto nogl_adapter;
        }
#else
#define USE_GL_FUNC(pfn) pfn = (void*)pwglGetProcAddress(#pfn);
        /* To bypass the opengl32 thunks load wglGetProcAddress from gdi32 (glXGetProcAddress wrapper) instead of opengl32's */
        mod_gl = GetModuleHandleA("gdi32.dll");
#endif
    }

/* Load WGL core functions from opengl32.dll */
#define USE_WGL_FUNC(pfn) p##pfn = (void*)GetProcAddress(mod_gl, #pfn);
    WGL_FUNCS_GEN;
#undef USE_WGL_FUNC

    if(!pwglGetProcAddress) {
        ERR("Unable to load wglGetProcAddress!\n");
        goto nogl_adapter;
    }

/* Dynamically load all GL core functions */
    GL_FUNCS_GEN;
#undef USE_GL_FUNC

    /* Load glFinish and glFlush from opengl32.dll even if we're not using WIN32 opengl
     * otherwise because we have to use winex11.drv's override
     */
#ifdef USE_WIN32_OPENGL
    wglFinish = (void*)GetProcAddress(mod_gl, "glFinish");
    wglFlush = (void*)GetProcAddress(mod_gl, "glFlush");
#else
    wglFinish = (void*)pwglGetProcAddress("wglFinish");
    wglFlush = (void*)pwglGetProcAddress("wglFlush");
#endif

    glEnableWINE = glEnable;
    glDisableWINE = glDisable;

    /* For now only one default adapter */
    {
        struct wined3d_adapter *adapter = &wined3d->adapters[0];
        const struct wined3d_gl_info *gl_info = &adapter->gl_info;
        struct wined3d_fake_gl_ctx fake_gl_ctx = {0};
        struct wined3d_pixel_format *cfgs;
        int iPixelFormat;
        int res;
        DISPLAY_DEVICEW DisplayDevice;
        HDC hdc;

        TRACE("Initializing default adapter\n");
        adapter->ordinal = 0;
        adapter->monitorPoint.x = -1;
        adapter->monitorPoint.y = -1;

        if (!AllocateLocallyUniqueId(&adapter->luid))
        {
            DWORD err = GetLastError();
            ERR("Failed to set adapter LUID (%#x).\n", err);
            goto nogl_adapter;
        }
        TRACE("Allocated LUID %08x:%08x for adapter.\n",
                adapter->luid.HighPart, adapter->luid.LowPart);

        if (!WineD3D_CreateFakeGLContext(&fake_gl_ctx))
        {
            ERR("Failed to get a gl context for default adapter\n");
            goto nogl_adapter;
        }

        ret = wined3d_adapter_init_gl_caps(adapter);
        if(!ret) {
            ERR("Failed to initialize gl caps for default adapter\n");
            WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);
            goto nogl_adapter;
        }
        ret = initPixelFormats(&adapter->gl_info, adapter->driver_info.vendor);
        if(!ret) {
            ERR("Failed to init gl formats\n");
            WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);
            goto nogl_adapter;
        }

        hdc = fake_gl_ctx.dc;

        adapter->TextureRam = adapter->driver_info.vidmem;
        adapter->UsedTextureRam = 0;
        TRACE("Emulating %dMB of texture ram\n", adapter->TextureRam/(1024*1024));

        /* Initialize the Adapter's DeviceName which is required for ChangeDisplaySettings and friends */
        DisplayDevice.cb = sizeof(DisplayDevice);
        EnumDisplayDevicesW(NULL, 0 /* Adapter 0 = iDevNum 0 */, &DisplayDevice, 0);
        TRACE("DeviceName: %s\n", debugstr_w(DisplayDevice.DeviceName));
        strcpyW(adapter->DeviceName, DisplayDevice.DeviceName);

        if (gl_info->supported[WGL_ARB_PIXEL_FORMAT])
        {
            GLint cfg_count;
            int attribute;
            int attribs[11];
            int values[11];
            int nAttribs = 0;

            attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
            GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attribute, &cfg_count));
            adapter->cfg_count = cfg_count;

            adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, adapter->cfg_count * sizeof(*adapter->cfgs));
            cfgs = adapter->cfgs;
            attribs[nAttribs++] = WGL_RED_BITS_ARB;
            attribs[nAttribs++] = WGL_GREEN_BITS_ARB;
            attribs[nAttribs++] = WGL_BLUE_BITS_ARB;
            attribs[nAttribs++] = WGL_ALPHA_BITS_ARB;
            attribs[nAttribs++] = WGL_COLOR_BITS_ARB;
            attribs[nAttribs++] = WGL_DEPTH_BITS_ARB;
            attribs[nAttribs++] = WGL_STENCIL_BITS_ARB;
            attribs[nAttribs++] = WGL_DRAW_TO_WINDOW_ARB;
            attribs[nAttribs++] = WGL_PIXEL_TYPE_ARB;
            attribs[nAttribs++] = WGL_DOUBLE_BUFFER_ARB;
            attribs[nAttribs++] = WGL_AUX_BUFFERS_ARB;

            for (iPixelFormat=1; iPixelFormat <= adapter->cfg_count; ++iPixelFormat)
            {
                res = GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, nAttribs, attribs, values));

                if(!res)
                    continue;

                /* Cache the pixel format */
                cfgs->iPixelFormat = iPixelFormat;
                cfgs->redSize = values[0];
                cfgs->greenSize = values[1];
                cfgs->blueSize = values[2];
                cfgs->alphaSize = values[3];
                cfgs->colorSize = values[4];
                cfgs->depthSize = values[5];
                cfgs->stencilSize = values[6];
                cfgs->windowDrawable = values[7];
                cfgs->iPixelType = values[8];
                cfgs->doubleBuffer = values[9];
                cfgs->auxBuffers = values[10];

                cfgs->numSamples = 0;
                /* Check multisample support */
                if (gl_info->supported[ARB_MULTISAMPLE])
                {
                    int attrib[2] = {WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB};
                    int value[2];
                    if(GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, 2, attrib, value))) {
                        /* value[0] = WGL_SAMPLE_BUFFERS_ARB which tells whether multisampling is supported.
                        * value[1] = number of multi sample buffers*/
                        if(value[0])
                            cfgs->numSamples = value[1];
                    }
                }

                TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, "
                        "depth=%d, stencil=%d, samples=%d, windowDrawable=%d\n",
                        cfgs->iPixelFormat, cfgs->iPixelType, cfgs->doubleBuffer,
                        cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize,
                        cfgs->depthSize, cfgs->stencilSize, cfgs->numSamples, cfgs->windowDrawable);
                cfgs++;
            }
        }
        else
        {
            int nCfgs = DescribePixelFormat(hdc, 0, 0, 0);
            adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nCfgs * sizeof(*adapter->cfgs));
            adapter->cfg_count = 0; /* We won't accept all formats e.g. software accelerated ones will be skipped */

            cfgs = adapter->cfgs;
            for(iPixelFormat=1; iPixelFormat<=nCfgs; iPixelFormat++)
            {
                PIXELFORMATDESCRIPTOR ppfd;

                res = DescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &ppfd);
                if(!res)
                    continue;

                /* We only want HW acceleration using an OpenGL ICD driver.
                 * PFD_GENERIC_FORMAT = slow opengl 1.1 gdi software rendering
                 * PFD_GENERIC_ACCELERATED = partial hw acceleration using a MCD driver (e.g. 3dfx minigl)
                 */
                if(ppfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED))
                {
                    TRACE("Skipping iPixelFormat=%d because it isn't ICD accelerated\n", iPixelFormat);
                    continue;
                }

                cfgs->iPixelFormat = iPixelFormat;
                cfgs->redSize = ppfd.cRedBits;
                cfgs->greenSize = ppfd.cGreenBits;
                cfgs->blueSize = ppfd.cBlueBits;
                cfgs->alphaSize = ppfd.cAlphaBits;
                cfgs->colorSize = ppfd.cColorBits;
                cfgs->depthSize = ppfd.cDepthBits;
                cfgs->stencilSize = ppfd.cStencilBits;
                cfgs->windowDrawable = (ppfd.dwFlags & PFD_DRAW_TO_WINDOW) ? 1 : 0;
                cfgs->iPixelType = (ppfd.iPixelType == PFD_TYPE_RGBA) ? WGL_TYPE_RGBA_ARB : WGL_TYPE_COLORINDEX_ARB;
                cfgs->doubleBuffer = (ppfd.dwFlags & PFD_DOUBLEBUFFER) ? 1 : 0;
                cfgs->auxBuffers = ppfd.cAuxBuffers;
                cfgs->numSamples = 0;

                TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, "
                        "depth=%d, stencil=%d, windowDrawable=%d\n",
                        cfgs->iPixelFormat, cfgs->iPixelType, cfgs->doubleBuffer,
                        cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize,
                        cfgs->depthSize, cfgs->stencilSize, cfgs->windowDrawable);
                cfgs++;
                adapter->cfg_count++;
            }

            /* We haven't found any suitable formats. This should only happen
             * in case of GDI software rendering, which is pretty useless
             * anyway. */
            if (!adapter->cfg_count)
            {
                ERR("Disabling Direct3D because no hardware accelerated pixel formats have been found!\n");

                WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);
                HeapFree(GetProcessHeap(), 0, adapter->cfgs);
                goto nogl_adapter;
            }
        }

        WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);

        select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);
        fillGLAttribFuncs(&adapter->gl_info);
        adapter->opengl = TRUE;
    }
    wined3d->adapter_count = 1;
    TRACE("%u adapters successfully initialized.\n", wined3d->adapter_count);

    return TRUE;

nogl_adapter:
    /* Initialize an adapter for ddraw-only memory counting */
    memset(wined3d->adapters, 0, sizeof(wined3d->adapters));
    wined3d->adapters[0].ordinal = 0;
    wined3d->adapters[0].opengl = FALSE;
    wined3d->adapters[0].monitorPoint.x = -1;
    wined3d->adapters[0].monitorPoint.y = -1;

    wined3d->adapters[0].driver_info.name = "Display";
    wined3d->adapters[0].driver_info.description = "WineD3D DirectDraw Emulation";
    if (wined3d_settings.emulated_textureram)
        wined3d->adapters[0].TextureRam = wined3d_settings.emulated_textureram;
    else
        wined3d->adapters[0].TextureRam = 8 * 1024 * 1024; /* This is plenty for a DDraw-only card */

    initPixelFormatsNoGL(&wined3d->adapters[0].gl_info);

    wined3d->adapter_count = 1;
    return FALSE;
}

static void STDMETHODCALLTYPE wined3d_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops wined3d_null_parent_ops =
{
    wined3d_null_wined3d_object_destroyed,
};

/* Do not call while under the GL lock. */
HRESULT wined3d_init(struct wined3d *wined3d, UINT version, DWORD flags, void *parent)
{
    wined3d->dxVersion = version;
    wined3d->ref = 1;
    wined3d->parent = parent;
    wined3d->flags = flags;

    if (!InitAdapters(wined3d))
    {
        WARN("Failed to initialize adapters.\n");
        if (version > 7)
        {
            MESSAGE("Direct3D%u is not available without OpenGL.\n", version);
            return E_FAIL;
        }
    }

    return WINED3D_OK;
}
