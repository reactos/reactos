/*
 * IWineD3D implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define GLINFO_LOCATION (*gl_info)
#define WINE_DEFAULT_VIDMEM (64 * 1024 * 1024)
#define MAKEDWORD_VERSION(maj, min)  ((maj & 0xffff) << 16) | (min & 0xffff)

/* The d3d device ID */
static const GUID IID_D3DDEVICE_D3DUID = { 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };

/* Extension detection */
static const struct {
    const char *extension_string;
    GL_SupportedExt extension;
    DWORD version;
} EXTENSION_MAP[] = {
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
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM,           0                           },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER,            0                           },
    {"GL_ARB_framebuffer_object",           ARB_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_ARB_geometry_shader4",             ARB_GEOMETRY_SHADER4,           0                           },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL,           0                           },
    {"GL_ARB_half_float_vertex",            ARB_HALF_FLOAT_VERTEX,          0                           },
    {"GL_ARB_imaging",                      ARB_IMAGING,                    0                           },
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
    {"GL_ARB_sync",                         ARB_SYNC,                       0                           },
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP,       0                           },
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION,        0                           },
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

static HRESULT WINAPI IWineD3DImpl_CheckDeviceFormat(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DFORMAT AdapterFormat, DWORD Usage, WINED3DRESOURCETYPE RType, WINED3DFORMAT CheckFormat, WINED3DSURFTYPE SurfaceType);

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

static void WineD3D_ReleaseFakeGLContext(struct wined3d_fake_gl_ctx *ctx)
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
    if (!context_set_current(NULL))
    {
        ERR_(d3d_caps)("Failed to clear current D3D context.\n");
    }

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
long WineD3DAdapterChangeGLRam(IWineD3DDeviceImpl *D3DDevice, long glram)
{
    struct wined3d_adapter *adapter = D3DDevice->adapter;

    adapter->UsedTextureRam += glram;
    TRACE("Adjusted gl ram by %ld to %d\n", glram, adapter->UsedTextureRam);
    return adapter->UsedTextureRam;
}

static void wined3d_adapter_cleanup(struct wined3d_adapter *adapter)
{
    HeapFree(GetProcessHeap(), 0, adapter->gl_info.gl_formats);
    HeapFree(GetProcessHeap(), 0, adapter->cfgs);
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

static HRESULT WINAPI IWineD3DImpl_QueryInterface(IWineD3D *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DDevice)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DImpl_AddRef(IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG WINAPI IWineD3DImpl_Release(IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        unsigned int i;

        for (i = 0; i < This->adapter_count; ++i)
        {
            wined3d_adapter_cleanup(&This->adapters[i]);
        }
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/**********************************************************
 * IWineD3D parts follows
 **********************************************************/

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
    if(glGetError() != 0) {
        TRACE("OpenGL implementation does not allow indirect addressing offsets > 63\n");
        TRACE("error: %s\n", debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
    } else TRACE("OpenGL implementation allows offsets > 63\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("ARB vp offset limit test cleanup");

    return ret;
}

static DWORD ver_for_ext(GL_SupportedExt ext)
{
    unsigned int i;
    for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i) {
        if(EXTENSION_MAP[i].extension == ext) {
            return EXTENSION_MAP[i].version;
        }
    }
    return 0;
}

static BOOL match_ati_r300_to_500(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_ATI) return FALSE;
    if (device == CARD_ATI_RADEON_9500) return TRUE;
    if (device == CARD_ATI_RADEON_X700) return TRUE;
    if (device == CARD_ATI_RADEON_X1600) return TRUE;
    return FALSE;
}

static BOOL match_geforce5(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor == HW_VENDOR_NVIDIA)
    {
        if (device == CARD_NVIDIA_GEFORCEFX_5800 || device == CARD_NVIDIA_GEFORCEFX_5600)
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
    wglFinish(); /* just to be sure */

    memset(check, 0, sizeof(check));
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
    if (card_vendor != HW_VENDOR_ATI) return FALSE;
    if (device == CARD_ATI_RADEON_X1600) return FALSE;
    return TRUE;
}

static BOOL match_fglrx(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return (gl_vendor == GL_VENDOR_ATI);

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

static BOOL match_apple_nvts(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (!match_apple(gl_info, gl_renderer, gl_vendor, card_vendor, device)) return FALSE;
    return gl_info->supported[NV_TEXTURE_SHADER];
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

/* fglrx crashes with a very bad kernel panic if GL_POINT_SPRITE_ARB is set to GL_COORD_REPLACE_ARB
 * on more than one texture unit. This means that the d3d9 visual point size test will cause a
 * kernel panic on any machine running fglrx 9.3(latest that supports r300 to r500 cards). This
 * quirk only enables point sprites on the first texture unit. This keeps point sprites working in
 * most games, but avoids the crash
 *
 * A more sophisticated way would be to find all units that need texture coordinates and enable
 * point sprites for one if only one is found, and software emulate point sprites in drawStridedSlow
 * if more than one unit needs texture coordinates(This requires software ffp and vertex shaders though)
 *
 * Note that disabling the extension entirely does not gain predictability because there is no point
 * sprite capability flag in d3d, so the potential rendering bugs are the same if we disable the extension. */
static void quirk_one_point_sprite(struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        TRACE("Limiting point sprites to one texture unit.\n");
        gl_info->limits.point_sprite_units = 1;
    }
}

static void quirk_ati_dx9(struct wined3d_gl_info *gl_info)
{
    quirk_arb_constants(gl_info);

    /* MacOS advertises GL_ARB_texture_non_power_of_two on ATI r500 and earlier cards, although
     * these cards only support GL_ARB_texture_rectangle(D3DPTEXTURECAPS_NONPOW2CONDITIONAL).
     * If real NP2 textures are used, the driver falls back to software. We could just remove the
     * extension and use GL_ARB_texture_rectangle instead, but texture_rectangle is inconventient
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
        gl_info->supported[WINE_NORMALIZED_TEXRECT] = TRUE;
    }

    /* fglrx has the same structural issues as the one described in quirk_apple_glsl_constants, although
     * it is generally more efficient. Reserve just 8 constants. */
    TRACE_(d3d_caps)("Reserving 8 GLSL constants for compiler private use.\n");
    gl_info->reserved_glsl_constants = max(gl_info->reserved_glsl_constants, 8);
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
     *  in IWineD3DImpl_FillGLCaps).
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

static void quirk_apple_nvts(struct wined3d_gl_info *gl_info)
{
    gl_info->supported[NV_TEXTURE_SHADER] = FALSE;
    gl_info->supported[NV_TEXTURE_SHADER2] = FALSE;
}

static void quirk_disable_nvvp_clip(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_NV_CLIP_BROKEN;
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
        match_ati_r300_to_500,
        quirk_ati_dx9,
        "ATI GLSL constant and normalized texrect quirk"
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
        match_fglrx,
        quirk_one_point_sprite,
        "Fglrx point sprite crash workaround"
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
        /* The pixel formats provided by GL_NV_texture_shader are broken on OSX
         * (rdar://5682521).
         */
        match_apple_nvts,
        quirk_apple_nvts,
        "Apple NV_texture_shader disable"
    },
    {
        match_broken_nv_clip,
        quirk_disable_nvvp_clip,
        "Apple NV_vertex_program clip bug quirk"
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
 * "y" is the Direct3D level the driver supports:
 * 11 -> d3d6
 * 12 -> d3d7
 * 13 -> d3d8
 * 14 -> d3d9
 * 15 -> d3d10
 *
 * "z" is unknown, possibly vendor specific.
 *
 * "w" is the vendor specific driver version.
 */
struct driver_version_information
{
    WORD vendor;                    /* reported PCI card vendor ID  */
    WORD card;                      /* reported PCI card device ID  */
    const char *description;        /* Description of the card e.g. NVIDIA RIVA TNT */
    WORD d3d_level;                 /* driver hiword to report      */
    WORD lopart_hi, lopart_lo;      /* driver loword to report      */
};

static const struct driver_version_information driver_version_table[] =
{
    /* Nvidia drivers. Geforce6 and newer cards are supported by the current driver (180.x)
     * GeforceFX support is up to 173.x, - driver uses numbering x.y.11.7341 for 173.41 where x is the windows revision (6=2000/xp, 7=vista), y is unknown
     * Geforce2MX/3/4 up to 96.x - driver uses numbering 9.6.8.9 for 96.89
     * TNT/Geforce1/2 up to 71.x - driver uses numbering 7.1.8.6 for 71.86
     *
     * All version numbers used below are from the Linux nvidia drivers. */
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT,           "NVIDIA RIVA TNT",                  1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT2,          "NVIDIA RIVA TNT2/TNT2 Pro",        1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE,            "NVIDIA GeForce 256",               1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2_MX,        "NVIDIA GeForce2 MX/MX 400",        6,  4,  3      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2,           "NVIDIA GeForce2 GTS/GeForce2 Pro", 1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE3,           "NVIDIA GeForce3",                  6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_MX,        "NVIDIA GeForce4 MX 460",           6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_TI4200,    "NVIDIA GeForce4 Ti 4200",          6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5200,     "NVIDIA GeForce FX 5200",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5600,     "NVIDIA GeForce FX 5600",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5800,     "NVIDIA GeForce FX 5800",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6200,       "NVIDIA GeForce 6200",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6600GT,     "NVIDIA GeForce 6600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6800,       "NVIDIA GeForce 6800",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7300,       "NVIDIA GeForce Go 7300",           15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7400,       "NVIDIA GeForce Go 7400",           15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7600,       "NVIDIA GeForce 7600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7800GT,     "NVIDIA GeForce 7800 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            15, 11, 8618   },

    /* ATI cards. The driver versions are somewhat similar, but not quite the same. Let's hardcode. */
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_9500,           "ATI Radeon 9500",                  14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_X700,           "ATI Radeon X700 SE",               14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_X1600,          "ATI Radeon X1600 Series",          14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2300,         "ATI Mobility Radeon HD 2300",      14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2600,         "ATI Mobility Radeon HD 2600",      14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2900,         "ATI Radeon HD 2900 XT",            14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4350,         "ATI Radeon HD 4350",               14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4600,         "ATI Radeon HD 4600 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4700,         "ATI Radeon HD 4700 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4800,         "ATI Radeon HD 4800 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD5700,         "ATI Radeon HD 5700 Series",        14, 10, 8681    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD5800,         "ATI Radeon HD 5800 Series",        14, 10, 8681    },

    /* TODO: Add information about legacy ATI hardware, Intel and other cards. */
};

static void init_driver_info(struct wined3d_driver_info *driver_info,
        enum wined3d_pci_vendor vendor, enum wined3d_pci_device device)
{
    OSVERSIONINFOW os_version;
    WORD driver_os_version;
    unsigned int i;

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

    switch (vendor)
    {
        case HW_VENDOR_ATI:
            driver_info->name = "ati2dvag.dll";
            break;

        case HW_VENDOR_NVIDIA:
            driver_info->name = "nv4_disp.dll";
            break;

        case HW_VENDOR_INTEL:
        default:
            FIXME_(d3d_caps)("Unhandled vendor %04x.\n", vendor);
            driver_info->name = "Display";
            break;
    }

    memset(&os_version, 0, sizeof(os_version));
    os_version.dwOSVersionInfoSize = sizeof(os_version);
    if (!GetVersionExW(&os_version))
    {
        ERR("Failed to get OS version, reporting 2000/XP.\n");
        driver_os_version = 6;
    }
    else
    {
        TRACE("OS version %u.%u.\n", os_version.dwMajorVersion, os_version.dwMinorVersion);
        switch (os_version.dwMajorVersion)
        {
            case 4:
                driver_os_version = 4;
                break;

            case 5:
                driver_os_version = 6;
                break;

            case 6:
                if (os_version.dwMinorVersion == 0)
                {
                    driver_os_version = 7;
                }
                else
                {
                    if (os_version.dwMinorVersion > 1)
                    {
                        FIXME("Unhandled OS version %u.%u, reporting Win 7.\n",
                                os_version.dwMajorVersion, os_version.dwMinorVersion);
                    }
                    driver_os_version = 8;
                }
                break;

            default:
                FIXME("Unhandled OS version %u.%u, reporting 2000/XP.\n",
                        os_version.dwMajorVersion, os_version.dwMinorVersion);
                driver_os_version = 6;
                break;
        }
    }

    driver_info->description = "Direct3D HAL";
    driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, 15);
    driver_info->version_low = MAKEDWORD_VERSION(8, 6); /* Nvidia RIVA TNT, arbitrary */

    for (i = 0; i < (sizeof(driver_version_table) / sizeof(driver_version_table[0])); ++i)
    {
        if (vendor == driver_version_table[i].vendor && device == driver_version_table[i].card)
        {
            TRACE_(d3d_caps)("Found card %04x:%04x in driver DB.\n", vendor, device);

            driver_info->description = driver_version_table[i].description;
            driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, driver_version_table[i].d3d_level);
            driver_info->version_low = MAKEDWORD_VERSION(driver_version_table[i].lopart_hi,
                    driver_version_table[i].lopart_lo);
            break;
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

static enum wined3d_gl_vendor wined3d_guess_gl_vendor(struct wined3d_gl_info *gl_info, const char *gl_vendor_string, const char *gl_renderer)
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
        return GL_VENDOR_ATI;

    if (strstr(gl_vendor_string, "Intel(R)")
            || strstr(gl_renderer, "Intel(R)")
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

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning GL_VENDOR_WINE.\n", debugstr_a(gl_vendor_string));

    return GL_VENDOR_WINE;
}

static enum wined3d_pci_vendor wined3d_guess_card_vendor(const char *gl_vendor_string, const char *gl_renderer)
{
    if (strstr(gl_vendor_string, "NVIDIA"))
        return HW_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "X.Org R300 Project")
            || strstr(gl_vendor_string, "DRI R300 Project"))
        return HW_VENDOR_ATI;

    if (strstr(gl_vendor_string, "Intel(R)")
            || strstr(gl_renderer, "Intel(R)")
            || strstr(gl_vendor_string, "Intel Inc."))
        return HW_VENDOR_INTEL;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc."))
        return HW_VENDOR_WINE;

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning HW_VENDOR_NVIDIA.\n", debugstr_a(gl_vendor_string));

    return HW_VENDOR_NVIDIA;
}



enum wined3d_pci_device select_card_nvidia_binary(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            unsigned int *vidmem )
{
    /* Both the GeforceFX, 6xxx and 7xxx series support D3D9. The last two types have more
     * shader capabilities, so we use the shader capabilities to distinguish between FX and 6xxx/7xxx.
     */
    if (WINE_D3D9_CAPABLE(gl_info) && gl_info->supported[NV_VERTEX_PROGRAM3])
    {
        /* Geforce 200 - highend */
        if (strstr(gl_renderer, "GTX 280")
                || strstr(gl_renderer, "GTX 285")
                || strstr(gl_renderer, "GTX 295"))
        {
            *vidmem = 1024;
            return CARD_NVIDIA_GEFORCE_GTX280;
        }

        /* Geforce 200 - midend high */
        if (strstr(gl_renderer, "GTX 275"))
        {
            *vidmem = 896;
            return CARD_NVIDIA_GEFORCE_GTX275;
        }

        /* Geforce 200 - midend */
        if (strstr(gl_renderer, "GTX 260"))
        {
            *vidmem = 1024;
            return CARD_NVIDIA_GEFORCE_GTX260;
        }
        /* Geforce 200 - midend */
        if (strstr(gl_renderer, "GT 240"))
        {
           *vidmem = 512;
           return CARD_NVIDIA_GEFORCE_GT240;
        }

        /* Geforce9 - highend / Geforce 200 - midend (GTS 150/250 are based on the same core) */
        if (strstr(gl_renderer, "9800")
                || strstr(gl_renderer, "GTS 150")
                || strstr(gl_renderer, "GTS 250"))
        {
            *vidmem = 512;
            return CARD_NVIDIA_GEFORCE_9800GT;
        }

        /* Geforce9 - midend */
        if (strstr(gl_renderer, "9600"))
        {
            *vidmem = 384; /* The 9600GSO has 384MB, the 9600GT has 512-1024MB */
            return CARD_NVIDIA_GEFORCE_9600GT;
        }

        /* Geforce9 - midend low / Geforce 200 - low */
        if (strstr(gl_renderer, "9500")
                || strstr(gl_renderer, "GT 120")
                || strstr(gl_renderer, "GT 130"))
        {
            *vidmem = 256; /* The 9500GT has 256-1024MB */
            return CARD_NVIDIA_GEFORCE_9500GT;
        }

        /* Geforce9 - lowend */
        if (strstr(gl_renderer, "9400"))
        {
            *vidmem = 256; /* The 9400GT has 256-1024MB */
            return CARD_NVIDIA_GEFORCE_9400GT;
        }

        /* Geforce9 - lowend low */
        if (strstr(gl_renderer, "9100")
                || strstr(gl_renderer, "9200")
                || strstr(gl_renderer, "9300")
                || strstr(gl_renderer, "G 100"))
        {
            *vidmem = 256; /* The 9100-9300 cards have 256MB */
            return CARD_NVIDIA_GEFORCE_9200;
        }

        /* Geforce8 - highend */
        if (strstr(gl_renderer, "8800"))
        {
            *vidmem = 320; /* The 8800GTS uses 320MB, a 8800GTX can have 768MB */
            return CARD_NVIDIA_GEFORCE_8800GTS;
        }

        /* Geforce8 - midend mobile */
        if (strstr(gl_renderer, "8600 M"))
        {
            *vidmem = 512;
            return CARD_NVIDIA_GEFORCE_8600MGT;
        }

        /* Geforce8 - midend */
        if (strstr(gl_renderer, "8600")
                || strstr(gl_renderer, "8700"))
        {
            *vidmem = 256;
            return CARD_NVIDIA_GEFORCE_8600GT;
        }

        /* Geforce8 - lowend */
        if (strstr(gl_renderer, "8100")
                || strstr(gl_renderer, "8200")
                || strstr(gl_renderer, "8300")
                || strstr(gl_renderer, "8400")
                || strstr(gl_renderer, "8500"))
        {
            *vidmem = 128; /* 128-256MB for a 8300, 256-512MB for a 8400 */
            return CARD_NVIDIA_GEFORCE_8300GS;
        }

        /* Geforce7 - highend */
        if (strstr(gl_renderer, "7800")
                || strstr(gl_renderer, "7900")
                || strstr(gl_renderer, "7950")
                || strstr(gl_renderer, "Quadro FX 4")
                || strstr(gl_renderer, "Quadro FX 5"))
        {
            *vidmem = 256; /* A 7800GT uses 256MB while highend 7900 cards can use 512MB */
            return CARD_NVIDIA_GEFORCE_7800GT;
        }

        /* Geforce7 midend */
        if (strstr(gl_renderer, "7600")
                || strstr(gl_renderer, "7700"))
        {
            *vidmem = 256; /* The 7600 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_7600;
        }

        /* Geforce7 lower medium */
        if (strstr(gl_renderer, "7400"))
        {
            *vidmem = 256; /* The 7400 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_7400;
        }

        /* Geforce7 lowend */
        if (strstr(gl_renderer, "7300"))
        {
            *vidmem = 256; /* Mac Pros with this card have 256 MB */
            return CARD_NVIDIA_GEFORCE_7300;
        }

        /* Geforce6 highend */
        if (strstr(gl_renderer, "6800"))
        {
            *vidmem = 128; /* The 6800 uses 128-256MB, the 7600 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_6800;
        }

        /* Geforce6 - midend */
        if (strstr(gl_renderer, "6600")
                || strstr(gl_renderer, "6610")
                || strstr(gl_renderer, "6700"))
        {
            *vidmem = 128; /* A 6600GT has 128-256MB */
            return CARD_NVIDIA_GEFORCE_6600GT;
        }

        /* Geforce6/7 lowend */
        *vidmem = 64; /* */
        return CARD_NVIDIA_GEFORCE_6200; /* Geforce 6100/6150/6200/7300/7400/7500 */
    }

    if (WINE_D3D9_CAPABLE(gl_info))
    {
        /* GeforceFX - highend */
        if (strstr(gl_renderer, "5800")
                || strstr(gl_renderer, "5900")
                || strstr(gl_renderer, "5950")
                || strstr(gl_renderer, "Quadro FX"))
        {
            *vidmem = 256; /* 5800-5900 cards use 256MB */
            return CARD_NVIDIA_GEFORCEFX_5800;
        }

        /* GeforceFX - midend */
        if (strstr(gl_renderer, "5600")
                || strstr(gl_renderer, "5650")
                || strstr(gl_renderer, "5700")
                || strstr(gl_renderer, "5750"))
        {
            *vidmem = 128; /* A 5600 uses 128-256MB */
            return CARD_NVIDIA_GEFORCEFX_5600;
        }

        /* GeforceFX - lowend */
        *vidmem = 64; /* Normal FX5200 cards use 64-256MB; laptop (non-standard) can have less */
        return CARD_NVIDIA_GEFORCEFX_5200; /* GeforceFX 5100/5200/5250/5300/5500 */
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        if (strstr(gl_renderer, "GeForce4 Ti") || strstr(gl_renderer, "Quadro4"))
        {
            *vidmem = 64; /* Geforce4 Ti cards have 64-128MB */
            return CARD_NVIDIA_GEFORCE4_TI4200; /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800, Quadro4 */
        }

        *vidmem = 64; /* Geforce3 cards have 64-128MB */
        return CARD_NVIDIA_GEFORCE3; /* Geforce3 standard/Ti200/Ti500, Quadro DCC */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        if (strstr(gl_renderer, "GeForce4 MX"))
        {
            /* Most Geforce4MX GPUs have at least 64MB of memory, some
             * early models had 32MB but most have 64MB or even 128MB. */
            *vidmem = 64;
            return CARD_NVIDIA_GEFORCE4_MX; /* MX420/MX440/MX460/MX4000 */
        }

        if (strstr(gl_renderer, "GeForce2 MX") || strstr(gl_renderer, "Quadro2 MXR"))
        {
            *vidmem = 32; /* Geforce2MX GPUs have 32-64MB of video memory */
            return CARD_NVIDIA_GEFORCE2_MX; /* Geforce2 standard/MX100/MX200/MX400, Quadro2 MXR */
        }

        if (strstr(gl_renderer, "GeForce2") || strstr(gl_renderer, "Quadro2"))
        {
            *vidmem = 32; /* Geforce2 GPUs have 32-64MB of video memory */
            return CARD_NVIDIA_GEFORCE2; /* Geforce2 GTS/Pro/Ti/Ultra, Quadro2 */
        }

        /* Most Geforce1 cards have 32MB, there are also some rare 16
         * and 64MB (Dell) models. */
        *vidmem = 32;
        return CARD_NVIDIA_GEFORCE; /* Geforce 256/DDR, Quadro */
    }

    if (strstr(gl_renderer, "TNT2"))
    {
        *vidmem = 32; /* Most TNT2 boards have 32MB, though there are 16MB boards too */
        return CARD_NVIDIA_RIVA_TNT2; /* Riva TNT2 standard/M64/Pro/Ultra */
    }

    *vidmem = 16; /* Most TNT boards have 16MB, some rare models have 8MB */
    return CARD_NVIDIA_RIVA_TNT; /* Riva TNT, Vanta */

}

enum wined3d_pci_device select_card_ati_binary(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
    unsigned int *vidmem )
{
    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
    if (WINE_D3D9_CAPABLE(gl_info))
    {
        /* Radeon EG CYPRESS XT / PRO HD5800 - highend */
        if (strstr(gl_renderer, "HD 5800")          /* Radeon EG CYPRESS HD58xx generic renderer string */
                || strstr(gl_renderer, "HD 5850")   /* Radeon EG CYPRESS XT */
                || strstr(gl_renderer, "HD 5870"))  /* Radeon EG CYPRESS PRO */
        {
            *vidmem = 1024; /* note: HD58xx cards use 1024MB  */
            return CARD_ATI_RADEON_HD5800;
        }

        /* Radeon EG JUNIPER XT / LE HD5700 - midend */
        if (strstr(gl_renderer, "HD 5700")          /* Radeon EG JUNIPER HD57xx generic renderer string */
                || strstr(gl_renderer, "HD 5750")   /* Radeon EG JUNIPER LE */
                || strstr(gl_renderer, "HD 5770"))  /* Radeon EG JUNIPER XT */
        {
            *vidmem = 512; /* note: HD5770 cards use 1024MB and HD5750 cards use 512MB or 1024MB  */
            return CARD_ATI_RADEON_HD5700;
        }

        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "HD 4800")          /* Radeon RV7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "HD 4830")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4850")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4870")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4890"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "HD 4700")          /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4770"))  /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "HD 4600")          /* Radeon RV730 */
                || strstr(gl_renderer, "HD 4650")   /* Radeon RV730 */
                || strstr(gl_renderer, "HD 4670"))  /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "HD 4350")          /* Radeon RV710 */
                || strstr(gl_renderer, "HD 4550"))  /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "HD 2900")
                || strstr(gl_renderer, "HD 3870")
                || strstr(gl_renderer, "HD 3850"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "HD 2600")
                || strstr(gl_renderer, "HD 3830")
                || strstr(gl_renderer, "HD 3690")
                || strstr(gl_renderer, "HD 3650"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2300/HD2400/HD3400 - lowend */
        if (strstr(gl_renderer, "HD 2300")
                || strstr(gl_renderer, "HD 2400")
                || strstr(gl_renderer, "HD 3470")
                || strstr(gl_renderer, "HD 3450")
                || strstr(gl_renderer, "HD 3430")
                || strstr(gl_renderer, "HD 3400"))
        {
            *vidmem = 128; /* HD2300 uses at least 128MB, HD2400 uses 256MB */
            return CARD_ATI_RADEON_HD2300;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "HD 3100")
                || strstr(gl_renderer, "HD 3200")
                || strstr(gl_renderer, "HD 3300"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }

        /* Radeon R5xx */
        if (strstr(gl_renderer, "X1600")
                || strstr(gl_renderer, "X1650")
                || strstr(gl_renderer, "X1800")
                || strstr(gl_renderer, "X1900")
                || strstr(gl_renderer, "X1950"))
        {
            *vidmem = 128; /* X1600 uses 128-256MB, >=X1800 uses 256MB */
            return CARD_ATI_RADEON_X1600;
        }

        /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300 (lowend R5xx) */
        if (strstr(gl_renderer, "X700")
                || strstr(gl_renderer, "X800")
                || strstr(gl_renderer, "X850")
                || strstr(gl_renderer, "X1300")
                || strstr(gl_renderer, "X1400")
                || strstr(gl_renderer, "X1450")
                || strstr(gl_renderer, "X1550"))
        {
            *vidmem = 128; /* x700/x8*0 use 128-256MB, >=x1300 128-512MB */
            return CARD_ATI_RADEON_X700;
        }

        /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400MHz */
        if (strstr(gl_renderer, "Radeon Xpress"))
        {
            *vidmem = 64; /* Shared RAM, BIOS configurable, 64-256M */
            return CARD_ATI_RADEON_XPRESS_200M;
        }

        /* Radeon R3xx */
        *vidmem = 64; /* Radeon 9500 uses 64MB, higher models use up to 256MB */
        return CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        *vidmem = 64; /* 8500/9000 cards use mostly 64MB, though there are 32MB and 128MB models */
        return CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        *vidmem = 32; /* There are models with up to 64MB */
        return CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
    }

    *vidmem = 16; /* There are 16-32MB models */
    return CARD_ATI_RAGE_128PRO;

}

enum wined3d_pci_device select_card_intel_binary(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
    unsigned int *vidmem )
{
    if (strstr(gl_renderer, "X3100"))
    {
        /* MacOS calls the card GMA X3100, Google findings also suggest the name GM965 */
        *vidmem = 128;
        return CARD_INTEL_X3100;
    }

    if (strstr(gl_renderer, "GMA 950") || strstr(gl_renderer, "945GM"))
    {
        /* MacOS calls the card GMA 950, but everywhere else the PCI ID is named 945GM */
        *vidmem = 64;
        return CARD_INTEL_I945GM;
    }

    if (strstr(gl_renderer, "915GM")) return CARD_INTEL_I915GM;
    if (strstr(gl_renderer, "915G")) return CARD_INTEL_I915G;
    if (strstr(gl_renderer, "865G")) return CARD_INTEL_I865G;
    if (strstr(gl_renderer, "855G")) return CARD_INTEL_I855G;
    if (strstr(gl_renderer, "830G")) return CARD_INTEL_I830G;
    return CARD_INTEL_I915G;

}

enum wined3d_pci_device (select_card_ati_mesa)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            unsigned int *vidmem )
{
    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
    if (strstr(gl_renderer, "Gallium"))
    {
        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "R700")          /* Radeon R7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "RV770")  /* Radeon RV770 */
                || strstr(gl_renderer, "RV790"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "RV740"))          /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "RV730"))        /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "RV710"))          /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "R600")
                || strstr(gl_renderer, "RV670")
                || strstr(gl_renderer, "R680"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "RV630")
                || strstr(gl_renderer, "RV635"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2300/HD2400/HD3400 - lowend */
        if (strstr(gl_renderer, "RV610")
                || strstr(gl_renderer, "RV620"))
        {
            *vidmem = 128; /* HD2300 uses at least 128MB, HD2400 uses 256MB */
            return CARD_ATI_RADEON_HD2300;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "RS780")
                || strstr(gl_renderer, "RS880"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }

        /* Radeon R5xx */
        if (strstr(gl_renderer, "RV530")
                || strstr(gl_renderer, "RV535")
                || strstr(gl_renderer, "RV560")
                || strstr(gl_renderer, "R520")
                || strstr(gl_renderer, "RV570")
                || strstr(gl_renderer, "R580"))
        {
            *vidmem = 128; /* X1600 uses 128-256MB, >=X1800 uses 256MB */
            return CARD_ATI_RADEON_X1600;
        }

        /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300 (lowend R5xx) */
        if (strstr(gl_renderer, "R410")
                || strstr(gl_renderer, "R420")
                || strstr(gl_renderer, "R423")
                || strstr(gl_renderer, "R430")
                || strstr(gl_renderer, "R480")
                || strstr(gl_renderer, "R481")
                || strstr(gl_renderer, "RV410")
                || strstr(gl_renderer, "RV515")
                || strstr(gl_renderer, "RV516"))
        {
            *vidmem = 128; /* x700/x8*0 use 128-256MB, >=x1300 128-512MB */
            return CARD_ATI_RADEON_X700;
        }

        /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400MHz */
        if (strstr(gl_renderer, "RS400")
                || strstr(gl_renderer, "RS480")
                || strstr(gl_renderer, "RS482")
                || strstr(gl_renderer, "RS485")
                || strstr(gl_renderer, "RS600")
                || strstr(gl_renderer, "RS690")
                || strstr(gl_renderer, "RS740"))
        {
            *vidmem = 64; /* Shared RAM, BIOS configurable, 64-256M */
            return CARD_ATI_RADEON_XPRESS_200M;
        }

        /* Radeon R3xx */
        if (strstr(gl_renderer, "R300")
                || strstr(gl_renderer, "RV350")
                || strstr(gl_renderer, "RV351")
                || strstr(gl_renderer, "RV360")
                || strstr(gl_renderer, "RV370")
                || strstr(gl_renderer, "R350")
                || strstr(gl_renderer, "R360"))
        {
            *vidmem = 64; /* Radeon 9500 uses 64MB, higher models use up to 256MB */
            return CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
        }
    }

    if (WINE_D3D9_CAPABLE(gl_info))
    {
        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "(R700")          /* Radeon R7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "(RV770")  /* Radeon RV770 */
                || strstr(gl_renderer, "(RV790"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "(RV740"))          /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "(RV730"))        /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "(RV710"))          /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "(R600")
                || strstr(gl_renderer, "(RV670")
                || strstr(gl_renderer, "(R680"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "(RV630")
                || strstr(gl_renderer, "(RV635"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2300/HD2400/HD3400 - lowend */
        if (strstr(gl_renderer, "(RV610")
                || strstr(gl_renderer, "(RV620"))
        {
            *vidmem = 128; /* HD2300 uses at least 128MB, HD2400 uses 256MB */
            return CARD_ATI_RADEON_HD2300;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "(RS780")
                || strstr(gl_renderer, "(RS880"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        *vidmem = 64; /* 8500/9000 cards use mostly 64MB, though there are 32MB and 128MB models */
        return CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        *vidmem = 32; /* There are models with up to 64MB */
        return CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
    }

    *vidmem = 16; /* There are 16-32MB models */
    return CARD_ATI_RAGE_128PRO;

}

enum wined3d_pci_device (select_card_nvidia_mesa)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
    unsigned int *vidmem )
{
    FIXME_(d3d_caps)("Card selection not handled for Mesa Nouveau driver\n");
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCEFX_5600;
    if (WINE_D3D8_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE3;
    if (WINE_D3D7_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE;
    if (WINE_D3D6_CAPABLE(gl_info)) return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}

enum wined3d_pci_device (select_card_intel_mesa)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
    unsigned int *vidmem )
{
    FIXME_(d3d_caps)("Card selection not handled for Mesa Intel driver\n");
    return CARD_INTEL_I915G;
}


struct vendor_card_selection
{
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_vendor card_vendor;
    const char *description;        /* Description of the card selector i.e. Apple OS/X Intel */
    enum wined3d_pci_device (*select_card)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            unsigned int *vidmem );
};

static const struct vendor_card_selection vendor_card_select_table[] =
{
    {GL_VENDOR_NVIDIA, HW_VENDOR_NVIDIA,  "Nvidia binary driver",     select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_NVIDIA,  "Apple OSX NVidia binary driver",   select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_ATI,     "Apple OSX AMD/ATI binary driver",  select_card_ati_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_INTEL,   "Apple OSX Intel binary driver",    select_card_intel_binary},
    {GL_VENDOR_ATI,    HW_VENDOR_ATI,     "AMD/ATI binary driver",    select_card_ati_binary},
    {GL_VENDOR_MESA,   HW_VENDOR_ATI,     "Mesa AMD/ATI driver",      select_card_ati_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_NVIDIA,  "Mesa Nouveau driver",      select_card_nvidia_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_INTEL,   "Mesa Intel driver",        select_card_intel_mesa}
};


static enum wined3d_pci_device wined3d_guess_card(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor *gl_vendor, enum wined3d_pci_vendor *card_vendor, unsigned int *vidmem)
{
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
        return vendor_card_select_table[i].select_card(gl_info, gl_renderer, vidmem);
    }

    FIXME_(d3d_caps)("No card selector available for GL vendor %d and card vendor %04x.\n",
                     *gl_vendor, *card_vendor);

    /* Default to generic Nvidia hardware based on the supported OpenGL extensions. The choice
     * for Nvidia was because the hardware and drivers they make are of good quality. This makes
     * them a good generic choice. */
    *card_vendor = HW_VENDOR_NVIDIA;
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCEFX_5600;
    if (WINE_D3D8_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE3;
    if (WINE_D3D7_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE;
    if (WINE_D3D6_CAPABLE(gl_info)) return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}

static const struct fragment_pipeline *select_fragment_implementation(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
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

static const shader_backend_t *select_shader_backend(struct wined3d_adapter *adapter)
{
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);
    if (vs_selected_mode == SHADER_GLSL || ps_selected_mode == SHADER_GLSL) return &glsl_shader_backend;
    if (vs_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_ARB) return &arb_program_shader_backend;
    return &none_shader_backend;
}

static const struct blit_shader *select_blit_implementation(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if ((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM]) return &arbfp_blit;
    else return &ffp_blit;
}

/* Context activation is done by the caller. */
static BOOL IWineD3DImpl_FillGLCaps(struct wined3d_adapter *adapter)
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
    unsigned int vidmem=0;
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
    gl_info->limits.buffers = 1;
    gl_info->limits.textures = 1;
    gl_info->limits.fragment_samplers = 1;
    gl_info->limits.vertex_samplers = 0;
    gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers + gl_info->limits.vertex_samplers;
    gl_info->limits.sampler_stages = 1;
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
        char current_ext[256];

        while (isspace(*GL_Extensions)) ++GL_Extensions;
        start = GL_Extensions;
        while (!isspace(*GL_Extensions) && *GL_Extensions) ++GL_Extensions;

        len = GL_Extensions - start;
        if (!len || len >= sizeof(current_ext)) continue;

        memcpy(current_ext, start, len);
        current_ext[len] = '\0';
        TRACE_(d3d_caps)("- %s\n", debugstr_a(current_ext));

        for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i)
        {
            if (!strcmp(current_ext, EXTENSION_MAP[i].extension_string))
            {
                TRACE_(d3d_caps)(" FOUND: %s support.\n", EXTENSION_MAP[i].extension_string);
                gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
                break;
            }
        }
    }

    /* Now work out what GL support this card really has */
#define USE_GL_FUNC(type, pfn, ext, replace) \
{ \
    DWORD ver = ver_for_ext(ext); \
    if (gl_info->supported[ext]) gl_info->pfn = (type)pwglGetProcAddress(#pfn); \
    else if (ver && ver <= gl_version) gl_info->pfn = (type)pwglGetProcAddress(#replace); \
    else gl_info->pfn = NULL; \
}
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

#define USE_GL_FUNC(type, pfn, ext, replace) gl_info->pfn = (type)pwglGetProcAddress(#pfn);
    WGL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

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
        TRACE_(d3d_caps)("GLSL version string: %s.\n", debugstr_a(str));
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
    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        gl_info->limits.point_sprite_units = gl_info->limits.textures;
    }
    else
    {
        gl_info->limits.point_sprite_units = 0;
    }
    checkGLcall("extension detection");

    LEAVE_GL();

    adapter->fragment_pipe = select_fragment_implementation(adapter);
    adapter->shader_backend = select_shader_backend(adapter);
    adapter->blitter = select_blit_implementation(adapter);

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

    device = wined3d_guess_card(gl_info, gl_renderer_str, &gl_vendor, &card_vendor, &vidmem);
    TRACE_(d3d_caps)("FOUND (fake) card: 0x%x (vendor id), 0x%x (device id)\n", card_vendor, device);

    /* If we have an estimate use it, else default to 64MB;  */
    if(vidmem)
        gl_info->vidmem = vidmem*1024*1024; /* convert from MBs to bytes */
    else
        gl_info->vidmem = WINE_DEFAULT_VIDMEM;

    gl_info->wrap_lookup[WINED3DTADDRESS_WRAP - WINED3DTADDRESS_WRAP] = GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_MIRROR - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_CLAMP - WINED3DTADDRESS_WRAP] = GL_CLAMP_TO_EDGE;
    gl_info->wrap_lookup[WINED3DTADDRESS_BORDER - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_MIRRORONCE - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] ? GL_MIRROR_CLAMP_TO_EDGE_ATI : GL_REPEAT;

    /* Make sure there's an active HDC else the WGL extensions will fail */
    hdc = pwglGetCurrentDC();
    if (hdc) {
        /* Not all GL drivers might offer WGL extensions e.g. VirtualBox */
        if(GL_EXTCALL(wglGetExtensionsStringARB))
            WGL_Extensions = GL_EXTCALL(wglGetExtensionsStringARB(hdc));

        if (NULL == WGL_Extensions) {
            ERR("   WGL_Extensions returns NULL\n");
        } else {
            TRACE_(d3d_caps)("WGL_Extensions reported:\n");
            while (*WGL_Extensions != 0x00) {
                const char *Start;
                char ThisExtn[256];

                while (isspace(*WGL_Extensions)) WGL_Extensions++;
                Start = WGL_Extensions;
                while (!isspace(*WGL_Extensions) && *WGL_Extensions != 0x00) {
                    WGL_Extensions++;
                }

                len = WGL_Extensions - Start;
                if (len == 0 || len >= sizeof(ThisExtn))
                    continue;

                memcpy(ThisExtn, Start, len);
                ThisExtn[len] = '\0';
                TRACE_(d3d_caps)("- %s\n", debugstr_a(ThisExtn));

                if (!strcmp(ThisExtn, "WGL_ARB_pbuffer")) {
                    gl_info->supported[WGL_ARB_PBUFFER] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_ARB_pbuffer support\n");
                }
                if (!strcmp(ThisExtn, "WGL_ARB_pixel_format")) {
                    gl_info->supported[WGL_ARB_PIXEL_FORMAT] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_ARB_pixel_format support\n");
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

/**********************************************************
 * IWineD3D implementation follows
 **********************************************************/

static UINT     WINAPI IWineD3DImpl_GetAdapterCount (IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    TRACE_(d3d_caps)("(%p): Reporting %u adapters\n", This, This->adapter_count);

    return This->adapter_count;
}

static HRESULT WINAPI IWineD3DImpl_RegisterSoftwareDevice(IWineD3D *iface, void *init_function)
{
    FIXME("iface %p, init_function %p stub!\n", iface, init_function);

    return WINED3D_OK;
}

static HMONITOR WINAPI IWineD3DImpl_GetAdapterMonitor(IWineD3D *iface, UINT Adapter) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    TRACE_(d3d_caps)("(%p)->(%d)\n", This, Adapter);

    if (Adapter >= IWineD3DImpl_GetAdapterCount(iface)) {
        return NULL;
    }

    return MonitorFromPoint(This->adapters[Adapter].monitorPoint, MONITOR_DEFAULTTOPRIMARY);
}

/* FIXME: GetAdapterModeCount and EnumAdapterModes currently only returns modes
     of the same bpp but different resolutions                                  */

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
static UINT     WINAPI IWineD3DImpl_GetAdapterModeCount(IWineD3D *iface, UINT Adapter, WINED3DFORMAT Format) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter: %d, Format: %s)\n", This, Adapter, debug_d3dformat(Format));

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return 0;
    }

    /* TODO: Store modes per adapter and read it from the adapter structure */
    if (Adapter == 0) { /* Display */
        const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapters[Adapter].gl_info);
        UINT format_bits = format_desc->byte_count * CHAR_BIT;
        unsigned int i = 0;
        unsigned int j = 0;
        DEVMODEW mode;

        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);

        while (EnumDisplaySettingsExW(NULL, j, &mode, 0))
        {
            ++j;

            if (Format == WINED3DFMT_UNKNOWN)
            {
                /* This is for D3D8, do not enumerate P8 here */
                if (mode.dmBitsPerPel == 32 || mode.dmBitsPerPel == 16) ++i;
            }
            else if (mode.dmBitsPerPel == format_bits)
            {
                ++i;
            }
        }

        TRACE_(d3d_caps)("(%p}->(Adapter: %d) => %d (out of %d)\n", This, Adapter, i, j);
        return i;
    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }
    return 0;
}

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
static HRESULT WINAPI IWineD3DImpl_EnumAdapterModes(IWineD3D *iface, UINT Adapter, WINED3DFORMAT Format, UINT Mode, WINED3DDISPLAYMODE* pMode) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter:%d, mode:%d, pMode:%p, format:%s)\n", This, Adapter, Mode, pMode, debug_d3dformat(Format));

    /* Validate the parameters as much as possible */
    if (NULL == pMode ||
        Adapter >= IWineD3DImpl_GetAdapterCount(iface) ||
        Mode    >= IWineD3DImpl_GetAdapterModeCount(iface, Adapter, Format)) {
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: Store modes per adapter and read it from the adapter structure */
    if (Adapter == 0)
    {
        const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapters[Adapter].gl_info);
        UINT format_bits = format_desc->byte_count * CHAR_BIT;
        DEVMODEW DevModeW;
        int ModeIdx = 0;
        UINT i = 0;
        int j = 0;

        ZeroMemory(&DevModeW, sizeof(DevModeW));
        DevModeW.dmSize = sizeof(DevModeW);

        /* If we are filtering to a specific format (D3D9), then need to skip
           all unrelated modes, but if mode is irrelevant (D3D8), then we can
           just count through the ones with valid bit depths */
        while ((i<=Mode) && EnumDisplaySettingsExW(NULL, j++, &DevModeW, 0))
        {
            if (Format == WINED3DFMT_UNKNOWN)
            {
                /* This is for D3D8, do not enumerate P8 here */
                if (DevModeW.dmBitsPerPel == 32 || DevModeW.dmBitsPerPel == 16) ++i;
            }
            else if (DevModeW.dmBitsPerPel == format_bits)
            {
                ++i;
            }
        }

        if (i == 0) {
            TRACE_(d3d_caps)("No modes found for format (%x - %s)\n", Format, debug_d3dformat(Format));
            return WINED3DERR_INVALIDCALL;
        }
        ModeIdx = j - 1;

        /* Now get the display mode via the calculated index */
        if (EnumDisplaySettingsExW(NULL, ModeIdx, &DevModeW, 0)) {
            pMode->Width        = DevModeW.dmPelsWidth;
            pMode->Height       = DevModeW.dmPelsHeight;
            pMode->RefreshRate  = DEFAULT_REFRESH_RATE;
            if (DevModeW.dmFields & DM_DISPLAYFREQUENCY)
                pMode->RefreshRate = DevModeW.dmDisplayFrequency;

            if (Format == WINED3DFMT_UNKNOWN) {
                pMode->Format = pixelformat_for_depth(DevModeW.dmBitsPerPel);
            } else {
                pMode->Format = Format;
            }
        } else {
            TRACE_(d3d_caps)("Requested mode out of range %d\n", Mode);
            return WINED3DERR_INVALIDCALL;
        }

        TRACE_(d3d_caps)("W %d H %d rr %d fmt (%x - %s) bpp %u\n", pMode->Width, pMode->Height,
                pMode->RefreshRate, pMode->Format, debug_d3dformat(pMode->Format),
                DevModeW.dmBitsPerPel);

    }
    else
    {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DImpl_GetAdapterDisplayMode(IWineD3D *iface, UINT Adapter, WINED3DDISPLAYMODE *pMode)
{
    TRACE("iface %p, adapter_idx %u, display_mode %p.\n", iface, Adapter, pMode);

    if (NULL == pMode ||
        Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        int bpp = 0;
        DEVMODEW DevModeW;

        ZeroMemory(&DevModeW, sizeof(DevModeW));
        DevModeW.dmSize = sizeof(DevModeW);

        EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &DevModeW, 0);
        pMode->Width        = DevModeW.dmPelsWidth;
        pMode->Height       = DevModeW.dmPelsHeight;
        bpp                 = DevModeW.dmBitsPerPel;
        pMode->RefreshRate  = DEFAULT_REFRESH_RATE;
        if (DevModeW.dmFields&DM_DISPLAYFREQUENCY)
        {
            pMode->RefreshRate = DevModeW.dmDisplayFrequency;
        }

        pMode->Format = pixelformat_for_depth(bpp);
    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    TRACE_(d3d_caps)("returning w:%d, h:%d, ref:%d, fmt:%s\n", pMode->Width,
          pMode->Height, pMode->RefreshRate, debug_d3dformat(pMode->Format));
    return WINED3D_OK;
}

/* NOTE: due to structure differences between dx8 and dx9 D3DADAPTER_IDENTIFIER,
   and fields being inserted in the middle, a new structure is used in place    */
static HRESULT WINAPI IWineD3DImpl_GetAdapterIdentifier(IWineD3D *iface, UINT Adapter, DWORD Flags,
                                                   WINED3DADAPTER_IDENTIFIER* pIdentifier) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    struct wined3d_adapter *adapter;
    size_t len;

    TRACE_(d3d_caps)("(%p}->(Adapter: %d, Flags: %x, pId=%p)\n", This, Adapter, Flags, pIdentifier);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    adapter = &This->adapters[Adapter];

    /* Return the information requested */
    TRACE_(d3d_caps)("device/Vendor Name and Version detection using FillGLCaps\n");

    if (pIdentifier->driver_size)
    {
        const char *name = adapter->driver_info.name;
        len = min(strlen(name), pIdentifier->driver_size - 1);
        memcpy(pIdentifier->driver, name, len);
        pIdentifier->driver[len] = '\0';
    }

    if (pIdentifier->description_size)
    {
        const char *description = adapter->driver_info.description;
        len = min(strlen(description), pIdentifier->description_size - 1);
        memcpy(pIdentifier->description, description, len);
        pIdentifier->description[len] = '\0';
    }

    /* Note that d3d8 doesn't supply a device name. */
    if (pIdentifier->device_name_size)
    {
        static const char *device_name = "\\\\.\\DISPLAY1"; /* FIXME: May depend on desktop? */

        len = strlen(device_name);
        if (len >= pIdentifier->device_name_size)
        {
            ERR("Device name size too small.\n");
            return WINED3DERR_INVALIDCALL;
        }

        memcpy(pIdentifier->device_name, device_name, len);
        pIdentifier->device_name[len] = '\0';
    }

    pIdentifier->driver_version.u.HighPart = adapter->driver_info.version_high;
    pIdentifier->driver_version.u.LowPart = adapter->driver_info.version_low;
    pIdentifier->vendor_id = adapter->driver_info.vendor;
    pIdentifier->device_id = adapter->driver_info.device;
    pIdentifier->subsystem_id = 0;
    pIdentifier->revision = 0;
    memcpy(&pIdentifier->device_identifier, &IID_D3DDEVICE_D3DUID, sizeof(pIdentifier->device_identifier));
    pIdentifier->whql_level = (Flags & WINED3DENUM_NO_WHQL_LEVEL) ? 0 : 1;
    memcpy(&pIdentifier->adapter_luid, &adapter->luid, sizeof(pIdentifier->adapter_luid));
    pIdentifier->video_memory = adapter->TextureRam;

    return WINED3D_OK;
}

static BOOL IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(const struct wined3d_gl_info *gl_info,
        const WineD3D_PixelFormat *cfg, const struct GlPixelFormatDesc *format_desc)
{
    short redSize, greenSize, blueSize, alphaSize, colorBits;

    if(!cfg)
        return FALSE;

    if(cfg->iPixelType == WGL_TYPE_RGBA_ARB) { /* Integer RGBA formats */
        if (!getColorBits(format_desc, &redSize, &greenSize, &blueSize, &alphaSize, &colorBits))
        {
            ERR("Unable to check compatibility for Format=%s\n", debug_d3dformat(format_desc->format));
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
    } else if(cfg->iPixelType == WGL_TYPE_RGBA_FLOAT_ARB) { /* Float RGBA formats; TODO: WGL_NV_float_buffer */
        if (format_desc->format == WINED3DFMT_R16_FLOAT)
            return (cfg->redSize == 16 && cfg->greenSize == 0 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if (format_desc->format == WINED3DFMT_R16G16_FLOAT)
            return (cfg->redSize == 16 && cfg->greenSize == 16 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if (format_desc->format == WINED3DFMT_R16G16B16A16_FLOAT)
            return (cfg->redSize == 16 && cfg->greenSize == 16 && cfg->blueSize == 16 && cfg->alphaSize == 16);
        if (format_desc->format == WINED3DFMT_R32_FLOAT)
            return (cfg->redSize == 32 && cfg->greenSize == 0 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if (format_desc->format == WINED3DFMT_R32G32_FLOAT)
            return (cfg->redSize == 32 && cfg->greenSize == 32 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if (format_desc->format == WINED3DFMT_R32G32B32A32_FLOAT)
            return (cfg->redSize == 32 && cfg->greenSize == 32 && cfg->blueSize == 32 && cfg->alphaSize == 32);
    } else {
        /* Probably a color index mode */
        return FALSE;
    }

    return FALSE;
}

static BOOL IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(const struct wined3d_gl_info *gl_info,
        const WineD3D_PixelFormat *cfg, const struct GlPixelFormatDesc *format_desc)
{
    short depthSize, stencilSize;
    BOOL lockable = FALSE;

    if(!cfg)
        return FALSE;

    if (!getDepthStencilBits(format_desc, &depthSize, &stencilSize))
    {
        ERR("Unable to check compatibility for Format=%s\n", debug_d3dformat(format_desc->format));
        return FALSE;
    }

    if ((format_desc->format == WINED3DFMT_D16_LOCKABLE) || (format_desc->format == WINED3DFMT_D32_FLOAT))
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

static HRESULT WINAPI IWineD3DImpl_CheckDepthStencilMatch(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType,
                                                   WINED3DFORMAT AdapterFormat,
                                                   WINED3DFORMAT RenderTargetFormat,
                                                   WINED3DFORMAT DepthStencilFormat) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    int nCfgs;
    const WineD3D_PixelFormat *cfgs;
    const struct wined3d_adapter *adapter;
    const struct GlPixelFormatDesc *rt_format_desc;
    const struct GlPixelFormatDesc *ds_format_desc;
    int it;

    WARN_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, DevType:(%x,%s), AdptFmt:(%x,%s), RendrTgtFmt:(%x,%s), DepthStencilFmt:(%x,%s))\n",
           This, Adapter,
           DeviceType, debug_d3ddevicetype(DeviceType),
           AdapterFormat, debug_d3dformat(AdapterFormat),
           RenderTargetFormat, debug_d3dformat(RenderTargetFormat),
           DepthStencilFormat, debug_d3dformat(DepthStencilFormat));

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        TRACE("(%p) Failed: Atapter (%u) higher than supported adapters (%u) returning WINED3DERR_INVALIDCALL\n", This, Adapter, IWineD3D_GetAdapterCount(iface));
        return WINED3DERR_INVALIDCALL;
    }

    adapter = &This->adapters[Adapter];
    rt_format_desc = getFormatDescEntry(RenderTargetFormat, &adapter->gl_info);
    ds_format_desc = getFormatDescEntry(DepthStencilFormat, &adapter->gl_info);
    cfgs = adapter->cfgs;
    nCfgs = adapter->nCfgs;
    for (it = 0; it < nCfgs; ++it) {
        if (IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&adapter->gl_info, &cfgs[it], rt_format_desc))
        {
            if (IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&adapter->gl_info, &cfgs[it], ds_format_desc))
            {
                TRACE_(d3d_caps)("(%p) : Formats matched\n", This);
                return WINED3D_OK;
            }
        }
    }
    WARN_(d3d_caps)("unsupported format pair: %s and %s\n", debug_d3dformat(RenderTargetFormat), debug_d3dformat(DepthStencilFormat));

    return WINED3DERR_NOTAVAILABLE;
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceMultiSampleType(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType,
        WINED3DFORMAT SurfaceFormat, BOOL Windowed, WINED3DMULTISAMPLE_TYPE MultiSampleType, DWORD *pQualityLevels)
{
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    const struct GlPixelFormatDesc *glDesc;
    const struct wined3d_adapter *adapter;

    TRACE_(d3d_caps)("(%p)-> (Adptr:%d, DevType:(%x,%s), SurfFmt:(%x,%s), Win?%d, MultiSamp:%x, pQual:%p)\n",
          This,
          Adapter,
          DeviceType, debug_d3ddevicetype(DeviceType),
          SurfaceFormat, debug_d3dformat(SurfaceFormat),
          Windowed,
          MultiSampleType,
          pQualityLevels);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: handle Windowed, add more quality levels */

    if (WINED3DMULTISAMPLE_NONE == MultiSampleType) {
        if(pQualityLevels) *pQualityLevels = 1;
        return WINED3D_OK;
    }

    /* By default multisampling is disabled right now as it causes issues
     * on some Nvidia driver versions and it doesn't work well in combination
     * with FBOs yet. */
    if(!wined3d_settings.allow_multisampling)
        return WINED3DERR_NOTAVAILABLE;

    adapter = &This->adapters[Adapter];
    glDesc = getFormatDescEntry(SurfaceFormat, &adapter->gl_info);
    if (!glDesc) return WINED3DERR_INVALIDCALL;

    if(glDesc->Flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)) {
        int i, nCfgs;
        const WineD3D_PixelFormat *cfgs;

        cfgs = adapter->cfgs;
        nCfgs = adapter->nCfgs;
        for(i=0; i<nCfgs; i++) {
            if(cfgs[i].numSamples != MultiSampleType)
                continue;

            if (!IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&adapter->gl_info, &cfgs[i], glDesc))
                continue;

            TRACE("Found iPixelFormat=%d to support MultiSampleType=%d for format %s\n", cfgs[i].iPixelFormat, MultiSampleType, debug_d3dformat(SurfaceFormat));

            if(pQualityLevels)
                *pQualityLevels = 1; /* Guess at a value! */
            return WINED3D_OK;
        }
    }
    else if(glDesc->Flags & WINED3DFMT_FLAG_RENDERTARGET) {
        short redSize, greenSize, blueSize, alphaSize, colorBits;
        int i, nCfgs;
        const WineD3D_PixelFormat *cfgs;

        if (!getColorBits(glDesc, &redSize, &greenSize, &blueSize, &alphaSize, &colorBits))
        {
            ERR("Unable to color bits for format %#x, can't check multisampling capability!\n", SurfaceFormat);
            return WINED3DERR_NOTAVAILABLE;
        }

        cfgs = adapter->cfgs;
        nCfgs = adapter->nCfgs;
        for(i=0; i<nCfgs; i++) {
            if(cfgs[i].numSamples != MultiSampleType)
                continue;
            if(cfgs[i].redSize != redSize)
                continue;
            if(cfgs[i].greenSize != greenSize)
                continue;
            if(cfgs[i].blueSize != blueSize)
                continue;
            /* Not all drivers report alpha-less formats since they use 32-bit anyway, so accept alpha even if we didn't ask for it. */
            if(alphaSize && cfgs[i].alphaSize != alphaSize)
                continue;
            if(cfgs[i].colorSize != (glDesc->byte_count << 3))
                continue;

            TRACE("Found iPixelFormat=%d to support MultiSampleType=%d for format %s\n", cfgs[i].iPixelFormat, MultiSampleType, debug_d3dformat(SurfaceFormat));

            if(pQualityLevels)
                *pQualityLevels = 1; /* Guess at a value! */
            return WINED3D_OK;
        }
    }
    return WINED3DERR_NOTAVAILABLE;
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceType(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType,
        WINED3DFORMAT DisplayFormat, WINED3DFORMAT BackBufferFormat, BOOL Windowed)
{
    HRESULT hr = WINED3DERR_NOTAVAILABLE;
    UINT nmodes;

    TRACE("iface %p, adapter_idx %u, device_type %s, display_format %s, backbuffer_format %s, windowed %#x.\n",
            iface, Adapter, debug_d3ddevicetype(DeviceType), debug_d3dformat(DisplayFormat),
            debug_d3dformat(BackBufferFormat), Windowed);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        WARN_(d3d_caps)("Adapter >= IWineD3D_GetAdapterCount(iface), returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* The task of this function is to check whether a certain display / backbuffer format
     * combination is available on the given adapter. In fullscreen mode microsoft specified
     * that the display format shouldn't provide alpha and that ignoring alpha the backbuffer
     * and display format should match exactly.
     * In windowed mode format conversion can occur and this depends on the driver. When format
     * conversion is done, this function should nevertheless fail and applications need to use
     * CheckDeviceFormatConversion.
     * At the moment we assume that fullscreen and windowed have the same capabilities */

    /* There are only 4 display formats */
    if (!(DisplayFormat == WINED3DFMT_B5G6R5_UNORM
            || DisplayFormat == WINED3DFMT_B5G5R5X1_UNORM
            || DisplayFormat == WINED3DFMT_B8G8R8X8_UNORM
            || DisplayFormat == WINED3DFMT_B10G10R10A2_UNORM))
    {
        TRACE_(d3d_caps)("Format %s unsupported as display format\n", debug_d3dformat(DisplayFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* If the requested DisplayFormat is not available, don't continue */
    nmodes = IWineD3DImpl_GetAdapterModeCount(iface, Adapter, DisplayFormat);
    if(!nmodes) {
        TRACE_(d3d_caps)("No available modes for display format %s\n", debug_d3dformat(DisplayFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Windowed mode allows you to specify WINED3DFMT_UNKNOWN for the backbufferformat, it means 'reuse' the display format for the backbuffer */
    if(!Windowed && BackBufferFormat == WINED3DFMT_UNKNOWN) {
        TRACE_(d3d_caps)("BackBufferFormat WINED3FMT_UNKNOWN not available in Windowed mode\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode R5G6B5 can only be mixed with backbuffer format R5G6B5 */
    if (DisplayFormat == WINED3DFMT_B5G6R5_UNORM && BackBufferFormat != WINED3DFMT_B5G6R5_UNORM)
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode X1R5G5B5 can only be mixed with backbuffer format *1R5G5B5 */
    if (DisplayFormat == WINED3DFMT_B5G5R5X1_UNORM
            && !(BackBufferFormat == WINED3DFMT_B5G5R5X1_UNORM || BackBufferFormat == WINED3DFMT_B5G5R5A1_UNORM))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode X8R8G8B8 can only be mixed with backbuffer format *8R8G8B8 */
    if (DisplayFormat == WINED3DFMT_B8G8R8X8_UNORM
            && !(BackBufferFormat == WINED3DFMT_B8G8R8X8_UNORM || BackBufferFormat == WINED3DFMT_B8G8R8A8_UNORM))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* A2R10G10B10 is only allowed in fullscreen mode and it can only be mixed with backbuffer format A2R10G10B10 */
    if (DisplayFormat == WINED3DFMT_B10G10R10A2_UNORM
            && (BackBufferFormat != WINED3DFMT_B10G10R10A2_UNORM || Windowed))
    {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Use CheckDeviceFormat to see if the BackBufferFormat is usable with the given DisplayFormat */
    hr = IWineD3DImpl_CheckDeviceFormat(iface, Adapter, DeviceType, DisplayFormat, WINED3DUSAGE_RENDERTARGET, WINED3DRTYPE_SURFACE, BackBufferFormat, SURFACE_OPENGL);
    if(FAILED(hr))
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));

    return hr;
}


/* Check if we support bumpmapping for a format */
static BOOL CheckBumpMapCapability(struct wined3d_adapter *adapter,
        WINED3DDEVTYPE DeviceType, const struct GlPixelFormatDesc *format_desc)
{
    switch(format_desc->format)
    {
        case WINED3DFMT_R8G8_SNORM:
        case WINED3DFMT_R16G16_SNORM:
        case WINED3DFMT_R5G5_SNORM_L6_UNORM:
        case WINED3DFMT_R8G8_SNORM_L8X8_UNORM:
        case WINED3DFMT_R8G8B8A8_SNORM:
            /* Ask the fixed function pipeline implementation if it can deal
             * with the conversion. If we've got a GL extension giving native
             * support this will be an identity conversion. */
            if (adapter->fragment_pipe->color_fixup_supported(format_desc->color_fixup))
            {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        default:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
    }
}

/* Check if the given DisplayFormat + DepthStencilFormat combination is valid for the Adapter */
static BOOL CheckDepthStencilCapability(struct wined3d_adapter *adapter,
        const struct GlPixelFormatDesc *display_format_desc, const struct GlPixelFormatDesc *ds_format_desc)
{
    int it=0;

    /* Only allow depth/stencil formats */
    if (!(ds_format_desc->depth_size || ds_format_desc->stencil_size)) return FALSE;

    /* Walk through all WGL pixel formats to find a match */
    for (it = 0; it < adapter->nCfgs; ++it)
    {
        WineD3D_PixelFormat *cfg = &adapter->cfgs[it];
        if (IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&adapter->gl_info, cfg, display_format_desc))
        {
            if (IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&adapter->gl_info, cfg, ds_format_desc))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static BOOL CheckFilterCapability(struct wined3d_adapter *adapter, const struct GlPixelFormatDesc *format_desc)
{
    /* The flags entry of a format contains the filtering capability */
    if (format_desc->Flags & WINED3DFMT_FLAG_FILTERING) return TRUE;

    return FALSE;
}

/* Check the render target capabilities of a format */
static BOOL CheckRenderTargetCapability(struct wined3d_adapter *adapter,
        const struct GlPixelFormatDesc *adapter_format_desc, const struct GlPixelFormatDesc *check_format_desc)
{
    /* Filter out non-RT formats */
    if (!(check_format_desc->Flags & WINED3DFMT_FLAG_RENDERTARGET)) return FALSE;

    if(wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER) {
        WineD3D_PixelFormat *cfgs = adapter->cfgs;
        int it;
        short AdapterRed, AdapterGreen, AdapterBlue, AdapterAlpha, AdapterTotalSize;
        short CheckRed, CheckGreen, CheckBlue, CheckAlpha, CheckTotalSize;

        getColorBits(adapter_format_desc, &AdapterRed, &AdapterGreen, &AdapterBlue, &AdapterAlpha, &AdapterTotalSize);
        getColorBits(check_format_desc, &CheckRed, &CheckGreen, &CheckBlue, &CheckAlpha, &CheckTotalSize);

        /* In backbuffer mode the front and backbuffer share the same WGL pixelformat.
         * The format must match in RGB, alpha is allowed to be different. (Only the backbuffer can have alpha) */
        if(!((AdapterRed == CheckRed) && (AdapterGreen == CheckGreen) && (AdapterBlue == CheckBlue))) {
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
        }

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        for (it = 0; it < adapter->nCfgs; ++it)
        {
            if (cfgs[it].windowDrawable && IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&adapter->gl_info,
                    &cfgs[it], check_format_desc))
            {
                TRACE_(d3d_caps)("iPixelFormat=%d is compatible with CheckFormat=%s\n",
                        cfgs[it].iPixelFormat, debug_d3dformat(check_format_desc->format));
                return TRUE;
            }
        }
    } else if(wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER) {
        /* We can probably use this function in FBO mode too on some drivers to get some basic indication of the capabilities. */
        WineD3D_PixelFormat *cfgs = adapter->cfgs;
        int it;

        /* Check if there is a WGL pixel format matching the requirements, the pixel format should also be usable with pbuffers */
        for (it = 0; it < adapter->nCfgs; ++it)
        {
            if (cfgs[it].pbufferDrawable && IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&adapter->gl_info,
                    &cfgs[it], check_format_desc))
            {
                TRACE_(d3d_caps)("iPixelFormat=%d is compatible with CheckFormat=%s\n",
                        cfgs[it].iPixelFormat, debug_d3dformat(check_format_desc->format));
                return TRUE;
            }
        }
    } else if(wined3d_settings.offscreen_rendering_mode == ORM_FBO){
        /* For now return TRUE for FBOs until we have some proper checks.
         * Note that this function will only be called when the format is around for texturing. */
        return TRUE;
    }
    return FALSE;
}

static BOOL CheckSrgbReadCapability(struct wined3d_adapter *adapter, const struct GlPixelFormatDesc *format_desc)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    /* Check for supported sRGB formats (Texture loading and framebuffer) */
    if (!gl_info->supported[EXT_TEXTURE_SRGB])
    {
        TRACE_(d3d_caps)("[FAILED] GL_EXT_texture_sRGB not supported\n");
        return FALSE;
    }

    switch (format_desc->format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
        case WINED3DFMT_B8G8R8X8_UNORM:
        case WINED3DFMT_B4G4R4A4_UNORM:
        case WINED3DFMT_L8_UNORM:
        case WINED3DFMT_L8A8_UNORM:
        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        default:
            TRACE_(d3d_caps)("[FAILED] Gamma texture format %s not supported.\n", debug_d3dformat(format_desc->format));
            return FALSE;
    }
    return FALSE;
}

static BOOL CheckSrgbWriteCapability(struct wined3d_adapter *adapter,
        WINED3DDEVTYPE DeviceType, const struct GlPixelFormatDesc *format_desc)
{
    /* Only offer SRGB writing on X8R8G8B8/A8R8G8B8 when we use ARB or GLSL shaders as we are
     * doing the color fixup in shaders.
     * Note Windows drivers (at least on the Geforce 8800) also offer this on R5G6B5. */
    if ((format_desc->format == WINED3DFMT_B8G8R8X8_UNORM) || (format_desc->format == WINED3DFMT_B8G8R8A8_UNORM))
    {
        int vs_selected_mode;
        int ps_selected_mode;
        select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);

        if((ps_selected_mode == SHADER_ARB) || (ps_selected_mode == SHADER_GLSL)) {
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;
        }
    }

    TRACE_(d3d_caps)("[FAILED] - no SRGB writing support on format=%s\n", debug_d3dformat(format_desc->format));
    return FALSE;
}

/* Check if a format support blending in combination with pixel shaders */
static BOOL CheckPostPixelShaderBlendingCapability(struct wined3d_adapter *adapter,
        const struct GlPixelFormatDesc *format_desc)
{
    /* The flags entry of a format contains the post pixel shader blending capability */
    if (format_desc->Flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING) return TRUE;

    return FALSE;
}

static BOOL CheckWrapAndMipCapability(struct wined3d_adapter *adapter, const struct GlPixelFormatDesc *format_desc)
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
static BOOL CheckTextureCapability(struct wined3d_adapter *adapter,
        WINED3DDEVTYPE DeviceType, const struct GlPixelFormatDesc *format_desc)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    switch (format_desc->format)
    {
        /*****
         *  supported: RGB(A) formats
         */
        case WINED3DFMT_B8G8R8_UNORM: /* Enable for dx7, blacklisted for 8 and 9 above */
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
         *  supported: Palettized
         */
        case WINED3DFMT_P8_UINT:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;
        /* No Windows driver offers WINED3DFMT_P8_UINT_A8_UNORM, so don't offer it either */
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
        case WINED3DFMT_S1_UINT_D15_UNORM:
        case WINED3DFMT_X8D24_UNORM:
        case WINED3DFMT_S4X4_UINT_D24_UNORM:
        case WINED3DFMT_D24_UNORM_S8_UINT:
        case WINED3DFMT_S8_UINT_D24_FLOAT:
        case WINED3DFMT_D32_UNORM:
        case WINED3DFMT_D32_FLOAT:
            return TRUE;

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
            if (adapter->shader_backend->shader_color_fixup_supported(format_desc->color_fixup))
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

            /* Not supported */
        case WINED3DFMT_R16G16B16A16_UNORM:
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
                    || gl_info->supported[EXT_TEXTURE_COMPRESSION_RGTC])
            {
                if (adapter->shader_backend->shader_color_fixup_supported(format_desc->color_fixup)
                        && adapter->fragment_pipe->color_fixup_supported(format_desc->color_fixup))
                {
                    TRACE_(d3d_caps)("[OK]\n");
                    return TRUE;
                }

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
             * ATI refused to support formats which can easilly be emulated with pixel shaders, so
             * Applications have to deal with not having NVHS and NVHU.
             */
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_UNKNOWN:
            return FALSE;

        default:
            ERR("Unhandled format=%s\n", debug_d3dformat(format_desc->format));
            break;
    }
    return FALSE;
}

static BOOL CheckSurfaceCapability(struct wined3d_adapter *adapter, const struct GlPixelFormatDesc *adapter_format_desc,
        WINED3DDEVTYPE DeviceType, const struct GlPixelFormatDesc *check_format_desc, WINED3DSURFTYPE SurfaceType)
{
    if(SurfaceType == SURFACE_GDI) {
        switch(check_format_desc->format)
        {
            case WINED3DFMT_B8G8R8_UNORM:
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
    if (CheckTextureCapability(adapter, DeviceType, check_format_desc)) return TRUE;
    /* All depth stencil formats are supported on surfaces */
    if (CheckDepthStencilCapability(adapter, adapter_format_desc, check_format_desc)) return TRUE;

    /* If opengl can't process the format natively, the blitter may be able to convert it */
    if (adapter->blitter->color_fixup_supported(check_format_desc->color_fixup))
    {
        TRACE_(d3d_caps)("[OK]\n");
        return TRUE;
    }

    /* Reject other formats */
    TRACE_(d3d_caps)("[FAILED]\n");
    return FALSE;
}

static BOOL CheckVertexTextureCapability(struct wined3d_adapter *adapter, const struct GlPixelFormatDesc *format_desc)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    if (!gl_info->limits.vertex_samplers)
    {
        TRACE_(d3d_caps)("[FAILED]\n");
        return FALSE;
    }

    switch (format_desc->format)
    {
        case WINED3DFMT_R32G32B32A32_FLOAT:
            if (!gl_info->supported[ARB_TEXTURE_FLOAT])
            {
                TRACE_(d3d_caps)("[FAILED]\n");
                return FALSE;
            }
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        default:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
    }
    return FALSE;
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceFormat(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType,
        WINED3DFORMAT AdapterFormat, DWORD Usage, WINED3DRESOURCETYPE RType, WINED3DFORMAT CheckFormat,
        WINED3DSURFTYPE SurfaceType)
{
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    struct wined3d_adapter *adapter = &This->adapters[Adapter];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(CheckFormat, gl_info);
    const struct GlPixelFormatDesc *adapter_format_desc = getFormatDescEntry(AdapterFormat, gl_info);
    DWORD UsageCaps = 0;

    TRACE_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, DevType:(%u,%s), AdptFmt:(%u,%s), Use:(%u,%s,%s), ResTyp:(%x,%s), CheckFmt:(%u,%s))\n",
          This,
          Adapter,
          DeviceType, debug_d3ddevicetype(DeviceType),
          AdapterFormat, debug_d3dformat(AdapterFormat),
          Usage, debug_d3dusage(Usage), debug_d3dusagequery(Usage),
          RType, debug_d3dresourcetype(RType),
          CheckFormat, debug_d3dformat(CheckFormat));

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    if(RType == WINED3DRTYPE_CUBETEXTURE) {

        if(SurfaceType != SURFACE_OPENGL) {
            TRACE("[FAILED]\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* Cubetexture allows:
         *                    - D3DUSAGE_AUTOGENMIPMAP
         *                    - D3DUSAGE_DEPTHSTENCIL
         *                    - D3DUSAGE_DYNAMIC
         *                    - D3DUSAGE_NONSECURE (d3d9ex)
         *                    - D3DUSAGE_RENDERTARGET
         *                    - D3DUSAGE_SOFTWAREPROCESSING
         *                    - D3DUSAGE_QUERY_WRAPANDMIP
         */
        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            /* Check if the texture format is around */
            if (CheckTextureCapability(adapter, DeviceType, format_desc))
            {
                if(Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
                    /* Check for automatic mipmap generation support */
                    if (gl_info->supported[SGIS_GENERATE_MIPMAP])
                    {
                        UsageCaps |= WINED3DUSAGE_AUTOGENMIPMAP;
                    } else {
                        /* When autogenmipmap isn't around continue and return WINED3DOK_NOAUTOGEN instead of D3D_OK */
                        TRACE_(d3d_caps)("[FAILED] - No autogenmipmap support, but continuing\n");
                    }
                }

                /* Always report dynamic locking */
                if(Usage & WINED3DUSAGE_DYNAMIC)
                    UsageCaps |= WINED3DUSAGE_DYNAMIC;

                if(Usage & WINED3DUSAGE_RENDERTARGET) {
                    if(CheckRenderTargetCapability(adapter, adapter_format_desc, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_RENDERTARGET;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Always report software processing */
                if(Usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                    UsageCaps |= WINED3DUSAGE_SOFTWAREPROCESSING;

                /* Check QUERY_FILTER support */
                if(Usage & WINED3DUSAGE_QUERY_FILTER) {
                    if (CheckFilterCapability(adapter, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_POSTPIXELSHADER_BLENDING support */
                if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                    if (CheckPostPixelShaderBlendingCapability(adapter, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_SRGBREAD support */
                if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                    if (CheckSrgbReadCapability(adapter, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_SRGBWRITE support */
                if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                    if (CheckSrgbWriteCapability(adapter, DeviceType, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_VERTEXTEXTURE support */
                if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                    if (CheckVertexTextureCapability(adapter, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_WRAPANDMIP support */
                if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                    if (CheckWrapAndMipCapability(adapter, format_desc))
                    {
                        UsageCaps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }
            } else {
                TRACE_(d3d_caps)("[FAILED] - Cube texture format not supported\n");
                return WINED3DERR_NOTAVAILABLE;
            }
        } else {
            TRACE_(d3d_caps)("[FAILED] - No cube texture support\n");
            return WINED3DERR_NOTAVAILABLE;
        }
    } else if(RType == WINED3DRTYPE_SURFACE) {
        /* Surface allows:
         *                - D3DUSAGE_DEPTHSTENCIL
         *                - D3DUSAGE_NONSECURE (d3d9ex)
         *                - D3DUSAGE_RENDERTARGET
         */

        if (CheckSurfaceCapability(adapter, adapter_format_desc, DeviceType, format_desc, SurfaceType))
        {
            if(Usage & WINED3DUSAGE_DEPTHSTENCIL) {
                if (CheckDepthStencilCapability(adapter, adapter_format_desc, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_DEPTHSTENCIL;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No depthstencil support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            if(Usage & WINED3DUSAGE_RENDERTARGET) {
                if (CheckRenderTargetCapability(adapter, adapter_format_desc, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_RENDERTARGET;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if (CheckPostPixelShaderBlendingCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }
        } else {
            TRACE_(d3d_caps)("[FAILED] - Not supported for plain surfaces\n");
            return WINED3DERR_NOTAVAILABLE;
        }

    } else if(RType == WINED3DRTYPE_TEXTURE) {
        /* Texture allows:
         *                - D3DUSAGE_AUTOGENMIPMAP
         *                - D3DUSAGE_DEPTHSTENCIL
         *                - D3DUSAGE_DMAP
         *                - D3DUSAGE_DYNAMIC
         *                - D3DUSAGE_NONSECURE (d3d9ex)
         *                - D3DUSAGE_RENDERTARGET
         *                - D3DUSAGE_SOFTWAREPROCESSING
         *                - D3DUSAGE_TEXTAPI (d3d9ex)
         *                - D3DUSAGE_QUERY_WRAPANDMIP
         */

        if(SurfaceType != SURFACE_OPENGL) {
            TRACE("[FAILED]\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* Check if the texture format is around */
        if (CheckTextureCapability(adapter, DeviceType, format_desc))
        {
            if(Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
                /* Check for automatic mipmap generation support */
                if (gl_info->supported[SGIS_GENERATE_MIPMAP])
                {
                    UsageCaps |= WINED3DUSAGE_AUTOGENMIPMAP;
                } else {
                    /* When autogenmipmap isn't around continue and return WINED3DOK_NOAUTOGEN instead of D3D_OK */
                    TRACE_(d3d_caps)("[FAILED] - No autogenmipmap support, but continuing\n");
                }
            }

            /* Always report dynamic locking */
            if(Usage & WINED3DUSAGE_DYNAMIC)
                UsageCaps |= WINED3DUSAGE_DYNAMIC;

            if(Usage & WINED3DUSAGE_RENDERTARGET) {
                if (CheckRenderTargetCapability(adapter, adapter_format_desc, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_RENDERTARGET;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                     return WINED3DERR_NOTAVAILABLE;
                 }
            }

            /* Always report software processing */
            if(Usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                UsageCaps |= WINED3DUSAGE_SOFTWAREPROCESSING;

            /* Check QUERY_FILTER support */
            if(Usage & WINED3DUSAGE_QUERY_FILTER) {
                if (CheckFilterCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_LEGACYBUMPMAP support */
            if(Usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP) {
                if (CheckBumpMapCapability(adapter, DeviceType, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_LEGACYBUMPMAP;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No legacy bumpmap support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if (CheckPostPixelShaderBlendingCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBREAD support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                if (CheckSrgbReadCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBWRITE support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                if (CheckSrgbWriteCapability(adapter, DeviceType, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_VERTEXTEXTURE support */
            if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                if (CheckVertexTextureCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_WRAPANDMIP support */
            if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                if (CheckWrapAndMipCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            if(Usage & WINED3DUSAGE_DEPTHSTENCIL) {
                if (CheckDepthStencilCapability(adapter, adapter_format_desc, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_DEPTHSTENCIL;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No depth stencil support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }
        } else {
            TRACE_(d3d_caps)("[FAILED] - Texture format not supported\n");
            return WINED3DERR_NOTAVAILABLE;
        }
    } else if((RType == WINED3DRTYPE_VOLUME) || (RType == WINED3DRTYPE_VOLUMETEXTURE)) {
        /* Volume is to VolumeTexture what Surface is to Texture but its usage caps are not documented.
         * Most driver seem to offer (nearly) the same on Volume and VolumeTexture, so do that too.
         *
         * Volumetexture allows:
         *                      - D3DUSAGE_DYNAMIC
         *                      - D3DUSAGE_NONSECURE (d3d9ex)
         *                      - D3DUSAGE_SOFTWAREPROCESSING
         *                      - D3DUSAGE_QUERY_WRAPANDMIP
         */

        if(SurfaceType != SURFACE_OPENGL) {
            TRACE("[FAILED]\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* Check volume texture and volume usage caps */
        if (gl_info->supported[EXT_TEXTURE3D])
        {
            if (!CheckTextureCapability(adapter, DeviceType, format_desc))
            {
                TRACE_(d3d_caps)("[FAILED] - Format not supported\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            /* Always report dynamic locking */
            if(Usage & WINED3DUSAGE_DYNAMIC)
                UsageCaps |= WINED3DUSAGE_DYNAMIC;

            /* Always report software processing */
            if(Usage & WINED3DUSAGE_SOFTWAREPROCESSING)
                UsageCaps |= WINED3DUSAGE_SOFTWAREPROCESSING;

            /* Check QUERY_FILTER support */
            if(Usage & WINED3DUSAGE_QUERY_FILTER) {
                if (CheckFilterCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if (CheckPostPixelShaderBlendingCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBREAD support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                if (CheckSrgbReadCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBWRITE support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                if (CheckSrgbWriteCapability(adapter, DeviceType, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_VERTEXTEXTURE support */
            if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                if (CheckVertexTextureCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_WRAPANDMIP support */
            if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                if (CheckWrapAndMipCapability(adapter, format_desc))
                {
                    UsageCaps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }
        } else {
            TRACE_(d3d_caps)("[FAILED] - No volume texture support\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* Filter formats that need conversion; For one part, this conversion is unimplemented,
         * and volume textures are huge, so it would be a big performance hit. Unless we hit an
         * app needing one of those formats, don't advertize them to avoid leading apps into
         * temptation. The windows drivers don't support most of those formats on volumes anyway,
         * except of R32F.
         */
        switch(CheckFormat) {
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
                /* The GL_EXT_texture_compression_s3tc spec requires that loading an s3tc
                 * compressed texture results in an error. While the D3D refrast does
                 * support s3tc volumes, at least the nvidia windows driver does not, so
                 * we're free not to support this format.
                 */
                TRACE_(d3d_caps)("[FAILED] - DXTn does not support 3D textures\n");
                return WINED3DERR_NOTAVAILABLE;

            default:
                /* Do nothing, continue with checking the format below */
                break;
        }
    } else if(RType == WINED3DRTYPE_BUFFER){
        /* For instance vertexbuffer/indexbuffer aren't supported yet because no Windows drivers seem to offer it */
        TRACE_(d3d_caps)("Unhandled resource type D3DRTYPE_INDEXBUFFER / D3DRTYPE_VERTEXBUFFER\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    /* When the UsageCaps exactly matches Usage return WINED3D_OK except for the situation in which
     * WINED3DUSAGE_AUTOGENMIPMAP isn't around, then WINED3DOK_NOAUTOGEN is returned if all the other
     * usage flags match. */
    if(UsageCaps == Usage) {
        return WINED3D_OK;
    } else if((UsageCaps == (Usage & ~WINED3DUSAGE_AUTOGENMIPMAP)) && (Usage & WINED3DUSAGE_AUTOGENMIPMAP)){
        return WINED3DOK_NOAUTOGEN;
    } else {
        TRACE_(d3d_caps)("[FAILED] - Usage=%#08x requested for CheckFormat=%s and RType=%d but only %#08x is available\n", Usage, debug_d3dformat(CheckFormat), RType, UsageCaps);
        return WINED3DERR_NOTAVAILABLE;
    }
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceFormatConversion(IWineD3D *iface, UINT adapter_idx,
        WINED3DDEVTYPE device_type, WINED3DFORMAT src_format, WINED3DFORMAT dst_format)
{
    FIXME("iface %p, adapter_idx %u, device_type %s, src_format %s, dst_format %s stub!\n",
            iface, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(src_format),
            debug_d3dformat(dst_format));

    return WINED3D_OK;
}

/* Note: d3d8 passes in a pointer to a D3DCAPS8 structure, which is a true
      subset of a D3DCAPS9 structure. However, it has to come via a void *
      as the d3d8 interface cannot import the d3d9 header                  */
static HRESULT WINAPI IWineD3DImpl_GetDeviceCaps(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DCAPS* pCaps) {

    IWineD3DImpl    *This = (IWineD3DImpl *)iface;
    struct wined3d_adapter *adapter = &This->adapters[Adapter];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int vs_selected_mode;
    int ps_selected_mode;
    struct shader_caps shader_caps;
    struct fragment_caps fragment_caps;
    DWORD ckey_caps, blit_caps, fx_caps;

    TRACE_(d3d_caps)("(%p)->(Adptr:%d, DevType: %x, pCaps: %p)\n", This, Adapter, DeviceType, pCaps);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);

    /* ------------------------------------------------
       The following fields apply to both d3d8 and d3d9
       ------------------------------------------------ */
    pCaps->DeviceType              = (DeviceType == WINED3DDEVTYPE_HAL) ? WINED3DDEVTYPE_HAL : WINED3DDEVTYPE_REF;  /* Not quite true, but use h/w supported by opengl I suppose */
    pCaps->AdapterOrdinal          = Adapter;

    pCaps->Caps                    = 0;
    pCaps->Caps2                   = WINED3DCAPS2_CANRENDERWINDOWED |
                                     WINED3DCAPS2_FULLSCREENGAMMA |
                                     WINED3DCAPS2_DYNAMICTEXTURES;
    if (gl_info->supported[SGIS_GENERATE_MIPMAP])
    {
        pCaps->Caps2 |= WINED3DCAPS2_CANAUTOGENMIPMAP;
    }

    pCaps->Caps3                   = WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |
                                     WINED3DCAPS3_COPY_TO_VIDMEM                   |
                                     WINED3DCAPS3_COPY_TO_SYSTEMMEM;

    pCaps->PresentationIntervals   = WINED3DPRESENT_INTERVAL_IMMEDIATE  |
                                     WINED3DPRESENT_INTERVAL_ONE;

    pCaps->CursorCaps              = WINED3DCURSORCAPS_COLOR            |
                                     WINED3DCURSORCAPS_LOWRES;

    pCaps->DevCaps                 = WINED3DDEVCAPS_FLOATTLVERTEX       |
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

    pCaps->PrimitiveMiscCaps       = WINED3DPMISCCAPS_CULLNONE              |
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
                                        WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS
                                        WINED3DPMISCCAPS_FOGANDSPECULARALPHA
                                        WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                                        WINED3DPMISCCAPS_FOGVERTEXCLAMPED */

    if (gl_info->supported[EXT_BLEND_EQUATION_SEPARATE] && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        pCaps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_SEPARATEALPHABLEND;

    pCaps->RasterCaps              = WINED3DPRASTERCAPS_DITHER    |
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
        pCaps->RasterCaps |= WINED3DPRASTERCAPS_ANISOTROPY    |
                             WINED3DPRASTERCAPS_ZBIAS         |
                             WINED3DPRASTERCAPS_MIPMAPLODBIAS;
    }
    if (gl_info->supported[NV_FOG_DISTANCE])
    {
        pCaps->RasterCaps         |= WINED3DPRASTERCAPS_FOGRANGE;
    }
                        /* FIXME Add:
                           WINED3DPRASTERCAPS_COLORPERSPECTIVE
                           WINED3DPRASTERCAPS_STRETCHBLTMULTISAMPLE
                           WINED3DPRASTERCAPS_ANTIALIASEDGES
                           WINED3DPRASTERCAPS_ZBUFFERLESSHSR
                           WINED3DPRASTERCAPS_WBUFFER */

    pCaps->ZCmpCaps = WINED3DPCMPCAPS_ALWAYS       |
                      WINED3DPCMPCAPS_EQUAL        |
                      WINED3DPCMPCAPS_GREATER      |
                      WINED3DPCMPCAPS_GREATEREQUAL |
                      WINED3DPCMPCAPS_LESS         |
                      WINED3DPCMPCAPS_LESSEQUAL    |
                      WINED3DPCMPCAPS_NEVER        |
                      WINED3DPCMPCAPS_NOTEQUAL;

    pCaps->SrcBlendCaps  = WINED3DPBLENDCAPS_BOTHINVSRCALPHA |
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

    pCaps->DestBlendCaps = WINED3DPBLENDCAPS_DESTALPHA       |
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
        pCaps->SrcBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
        pCaps->DestBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
    }


    pCaps->AlphaCmpCaps = WINED3DPCMPCAPS_ALWAYS       |
                          WINED3DPCMPCAPS_EQUAL        |
                          WINED3DPCMPCAPS_GREATER      |
                          WINED3DPCMPCAPS_GREATEREQUAL |
                          WINED3DPCMPCAPS_LESS         |
                          WINED3DPCMPCAPS_LESSEQUAL    |
                          WINED3DPCMPCAPS_NEVER        |
                          WINED3DPCMPCAPS_NOTEQUAL;

    pCaps->ShadeCaps     = WINED3DPSHADECAPS_SPECULARGOURAUDRGB |
                           WINED3DPSHADECAPS_COLORGOURAUDRGB    |
                           WINED3DPSHADECAPS_ALPHAFLATBLEND     |
                           WINED3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                           WINED3DPSHADECAPS_COLORFLATRGB       |
                           WINED3DPSHADECAPS_FOGFLAT            |
                           WINED3DPSHADECAPS_FOGGOURAUD         |
                           WINED3DPSHADECAPS_SPECULARFLATRGB;

    pCaps->TextureCaps =  WINED3DPTEXTURECAPS_ALPHA              |
                          WINED3DPTEXTURECAPS_ALPHAPALETTE       |
                          WINED3DPTEXTURECAPS_TRANSPARENCY       |
                          WINED3DPTEXTURECAPS_BORDER             |
                          WINED3DPTEXTURECAPS_MIPMAP             |
                          WINED3DPTEXTURECAPS_PROJECTED          |
                          WINED3DPTEXTURECAPS_PERSPECTIVE;

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        pCaps->TextureCaps |= WINED3DPTEXTURECAPS_POW2 |
                              WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        pCaps->TextureCaps |=  WINED3DPTEXTURECAPS_VOLUMEMAP      |
                               WINED3DPTEXTURECAPS_MIPVOLUMEMAP   |
                               WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        pCaps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP     |
                              WINED3DPTEXTURECAPS_MIPCUBEMAP    |
                              WINED3DPTEXTURECAPS_CUBEMAP_POW2;

    }

    pCaps->TextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
        pCaps->TextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                    WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        pCaps->CubeTextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
            pCaps->CubeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                            WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    } else
        pCaps->CubeTextureFilterCaps = 0;

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        pCaps->VolumeTextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
    } else
        pCaps->VolumeTextureFilterCaps = 0;

    pCaps->TextureAddressCaps =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                 WINED3DPTADDRESSCAPS_CLAMP  |
                                 WINED3DPTADDRESSCAPS_WRAP;

    if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
    {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
    }
    if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
    {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
    }
    if (gl_info->supported[ATI_TEXTURE_MIRROR_ONCE])
    {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        pCaps->VolumeTextureAddressCaps =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                           WINED3DPTADDRESSCAPS_CLAMP  |
                                           WINED3DPTADDRESSCAPS_WRAP;
        if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
        {
            pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
        }
        if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
        {
            pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
        }
        if (gl_info->supported[ATI_TEXTURE_MIRROR_ONCE])
        {
            pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
        }
    } else
        pCaps->VolumeTextureAddressCaps = 0;

    pCaps->LineCaps = WINED3DLINECAPS_TEXTURE       |
                      WINED3DLINECAPS_ZTEST         |
                      WINED3DLINECAPS_BLEND         |
                      WINED3DLINECAPS_ALPHACMP      |
                      WINED3DLINECAPS_FOG;
    /* WINED3DLINECAPS_ANTIALIAS is not supported on Windows, and dx and gl seem to have a different
     * idea how generating the smoothing alpha values works; the result is different
     */

    pCaps->MaxTextureWidth = gl_info->limits.texture_size;
    pCaps->MaxTextureHeight = gl_info->limits.texture_size;

    if (gl_info->supported[EXT_TEXTURE3D])
        pCaps->MaxVolumeExtent = gl_info->limits.texture3d_size;
    else
        pCaps->MaxVolumeExtent = 0;

    pCaps->MaxTextureRepeat = 32768;
    pCaps->MaxTextureAspectRatio = gl_info->limits.texture_size;
    pCaps->MaxVertexW = 1.0f;

    pCaps->GuardBandLeft = 0.0f;
    pCaps->GuardBandTop = 0.0f;
    pCaps->GuardBandRight = 0.0f;
    pCaps->GuardBandBottom = 0.0f;

    pCaps->ExtentsAdjust = 0.0f;

    pCaps->StencilCaps =  WINED3DSTENCILCAPS_DECRSAT |
                          WINED3DSTENCILCAPS_INCRSAT |
                          WINED3DSTENCILCAPS_INVERT  |
                          WINED3DSTENCILCAPS_KEEP    |
                          WINED3DSTENCILCAPS_REPLACE |
                          WINED3DSTENCILCAPS_ZERO;
    if (gl_info->supported[EXT_STENCIL_WRAP])
    {
        pCaps->StencilCaps |= WINED3DSTENCILCAPS_DECR  |
                              WINED3DSTENCILCAPS_INCR;
    }
    if (gl_info->supported[EXT_STENCIL_TWO_SIDE] || gl_info->supported[ATI_SEPARATE_STENCIL])
    {
        pCaps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    pCaps->FVFCaps = WINED3DFVFCAPS_PSIZE | 0x0008; /* 8 texture coords */

    pCaps->MaxUserClipPlanes = gl_info->limits.clipplanes;
    pCaps->MaxActiveLights = gl_info->limits.lights;

    pCaps->MaxVertexBlendMatrices = gl_info->limits.blends;
    pCaps->MaxVertexBlendMatrixIndex   = 0;

    pCaps->MaxAnisotropy = gl_info->limits.anisotropy;
    pCaps->MaxPointSize = gl_info->limits.pointsize_max;


    /* FIXME: Add D3DVTXPCAPS_TWEENING, D3DVTXPCAPS_TEXGEN_SPHEREMAP */
    pCaps->VertexProcessingCaps = WINED3DVTXPCAPS_DIRECTIONALLIGHTS |
                                  WINED3DVTXPCAPS_MATERIALSOURCE7   |
                                  WINED3DVTXPCAPS_POSITIONALLIGHTS  |
                                  WINED3DVTXPCAPS_LOCALVIEWER       |
                                  WINED3DVTXPCAPS_VERTEXFOG         |
                                  WINED3DVTXPCAPS_TEXGEN;

    pCaps->MaxPrimitiveCount   = 0xFFFFF; /* For now set 2^20-1 which is used by most >=Geforce3/Radeon8500 cards */
    pCaps->MaxVertexIndex      = 0xFFFFF;
    pCaps->MaxStreams          = MAX_STREAMS;
    pCaps->MaxStreamStride     = 1024;

    /* d3d9.dll sets D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES here because StretchRects is implemented in d3d9 */
    pCaps->DevCaps2                          = WINED3DDEVCAPS2_STREAMOFFSET |
                                               WINED3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;
    pCaps->MaxNpatchTessellationLevel        = 0;
    pCaps->MasterAdapterOrdinal              = 0;
    pCaps->AdapterOrdinalInGroup             = 0;
    pCaps->NumberOfAdaptersInGroup           = 1;

    pCaps->NumSimultaneousRTs = gl_info->limits.buffers;

    pCaps->StretchRectFilterCaps             = WINED3DPTFILTERCAPS_MINFPOINT  |
                                                WINED3DPTFILTERCAPS_MAGFPOINT  |
                                                WINED3DPTFILTERCAPS_MINFLINEAR |
                                                WINED3DPTFILTERCAPS_MAGFLINEAR;
    pCaps->VertexTextureFilterCaps           = 0;

    memset(&shader_caps, 0, sizeof(shader_caps));
    adapter->shader_backend->shader_get_caps(&adapter->gl_info, &shader_caps);

    memset(&fragment_caps, 0, sizeof(fragment_caps));
    adapter->fragment_pipe->get_caps(&adapter->gl_info, &fragment_caps);

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    pCaps->PrimitiveMiscCaps |= fragment_caps.PrimitiveMiscCaps;

    /* This takes care for disabling vertex shader or pixel shader caps while leaving the other one enabled.
     * Ignore shader model capabilities if disabled in config
     */
    if(vs_selected_mode == SHADER_NONE) {
        TRACE_(d3d_caps)("Vertex shader disabled in config, reporting version 0.0\n");
        pCaps->VertexShaderVersion          = WINED3DVS_VERSION(0,0);
        pCaps->MaxVertexShaderConst         = 0;
    } else {
        pCaps->VertexShaderVersion          = shader_caps.VertexShaderVersion;
        pCaps->MaxVertexShaderConst         = shader_caps.MaxVertexShaderConst;
    }

    if(ps_selected_mode == SHADER_NONE) {
        TRACE_(d3d_caps)("Pixel shader disabled in config, reporting version 0.0\n");
        pCaps->PixelShaderVersion           = WINED3DPS_VERSION(0,0);
        pCaps->PixelShader1xMaxValue        = 0.0f;
    } else {
        pCaps->PixelShaderVersion           = shader_caps.PixelShaderVersion;
        pCaps->PixelShader1xMaxValue        = shader_caps.PixelShader1xMaxValue;
    }

    pCaps->TextureOpCaps                    = fragment_caps.TextureOpCaps;
    pCaps->MaxTextureBlendStages            = fragment_caps.MaxTextureBlendStages;
    pCaps->MaxSimultaneousTextures          = fragment_caps.MaxSimultaneousTextures;

    pCaps->VS20Caps                         = shader_caps.VS20Caps;
    pCaps->MaxVShaderInstructionsExecuted   = shader_caps.MaxVShaderInstructionsExecuted;
    pCaps->MaxVertexShader30InstructionSlots= shader_caps.MaxVertexShader30InstructionSlots;
    pCaps->PS20Caps                         = shader_caps.PS20Caps;
    pCaps->MaxPShaderInstructionsExecuted   = shader_caps.MaxPShaderInstructionsExecuted;
    pCaps->MaxPixelShader30InstructionSlots = shader_caps.MaxPixelShader30InstructionSlots;

    /* The following caps are shader specific, but they are things we cannot detect, or which
     * are the same among all shader models. So to avoid code duplication set the shader version
     * specific, but otherwise constant caps here
     */
    if(pCaps->VertexShaderVersion == WINED3DVS_VERSION(3,0)) {
        /* Where possible set the caps based on OpenGL extensions and if they aren't set (in case of software rendering)
        use the VS 3.0 from MSDN or else if there's OpenGL spec use a hardcoded value minimum VS3.0 value. */
        pCaps->VS20Caps.Caps                     = WINED3DVS20CAPS_PREDICATION;
        pCaps->VS20Caps.DynamicFlowControlDepth  = WINED3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH; /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        pCaps->VS20Caps.NumTemps = max(32, adapter->gl_info.limits.arb_vs_temps);
        pCaps->VS20Caps.StaticFlowControlDepth   = WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH ; /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */

        pCaps->MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        pCaps->MaxVertexShader30InstructionSlots = max(512, adapter->gl_info.limits.arb_vs_instructions);
    }
    else if (pCaps->VertexShaderVersion == WINED3DVS_VERSION(2,0))
    {
        pCaps->VS20Caps.Caps                     = 0;
        pCaps->VS20Caps.DynamicFlowControlDepth  = WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        pCaps->VS20Caps.NumTemps = max(12, adapter->gl_info.limits.arb_vs_temps);
        pCaps->VS20Caps.StaticFlowControlDepth   = 1;

        pCaps->MaxVShaderInstructionsExecuted    = 65535;
        pCaps->MaxVertexShader30InstructionSlots = 0;
    } else { /* VS 1.x */
        pCaps->VS20Caps.Caps                     = 0;
        pCaps->VS20Caps.DynamicFlowControlDepth  = 0;
        pCaps->VS20Caps.NumTemps                 = 0;
        pCaps->VS20Caps.StaticFlowControlDepth   = 0;

        pCaps->MaxVShaderInstructionsExecuted    = 0;
        pCaps->MaxVertexShader30InstructionSlots = 0;
    }

    if(pCaps->PixelShaderVersion == WINED3DPS_VERSION(3,0)) {
        /* Where possible set the caps based on OpenGL extensions and if they aren't set (in case of software rendering)
        use the PS 3.0 from MSDN or else if there's OpenGL spec use a hardcoded value minimum PS 3.0 value. */

        /* Caps is more or less undocumented on MSDN but it appears to be used for PS20Caps based on results from R9600/FX5900/Geforce6800 cards from Windows */
        pCaps->PS20Caps.Caps                     = WINED3DPS20CAPS_ARBITRARYSWIZZLE     |
                WINED3DPS20CAPS_GRADIENTINSTRUCTIONS |
                WINED3DPS20CAPS_PREDICATION          |
                WINED3DPS20CAPS_NODEPENDENTREADLIMIT |
                WINED3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
        pCaps->PS20Caps.DynamicFlowControlDepth  = WINED3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH; /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        pCaps->PS20Caps.NumTemps = max(32, adapter->gl_info.limits.arb_ps_temps);
        pCaps->PS20Caps.StaticFlowControlDepth   = WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH; /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        pCaps->PS20Caps.NumInstructionSlots      = WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS; /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */

        pCaps->MaxPShaderInstructionsExecuted    = 65535;
        pCaps->MaxPixelShader30InstructionSlots = max(WINED3DMIN30SHADERINSTRUCTIONS,
                adapter->gl_info.limits.arb_ps_instructions);
    }
    else if(pCaps->PixelShaderVersion == WINED3DPS_VERSION(2,0))
    {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        pCaps->PS20Caps.Caps                     = 0;
        pCaps->PS20Caps.DynamicFlowControlDepth  = 0; /* WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        pCaps->PS20Caps.NumTemps = max(12, adapter->gl_info.limits.arb_ps_temps);
        pCaps->PS20Caps.StaticFlowControlDepth   = WINED3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minimum: 1 */
        pCaps->PS20Caps.NumInstructionSlots      = WINED3DPS20_MIN_NUMINSTRUCTIONSLOTS; /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */

        pCaps->MaxPShaderInstructionsExecuted    = 512; /* Minimum value, a GeforceFX uses 1024 */
        pCaps->MaxPixelShader30InstructionSlots  = 0;
    } else { /* PS 1.x */
        pCaps->PS20Caps.Caps                     = 0;
        pCaps->PS20Caps.DynamicFlowControlDepth  = 0;
        pCaps->PS20Caps.NumTemps                 = 0;
        pCaps->PS20Caps.StaticFlowControlDepth   = 0;
        pCaps->PS20Caps.NumInstructionSlots      = 0;

        pCaps->MaxPShaderInstructionsExecuted    = 0;
        pCaps->MaxPixelShader30InstructionSlots  = 0;
    }

    if(pCaps->VertexShaderVersion >= WINED3DVS_VERSION(2,0)) {
        /* OpenGL supports all the formats below, perhaps not always
         * without conversion, but it supports them.
         * Further GLSL doesn't seem to have an official unsigned type so
         * don't advertise it yet as I'm not sure how we handle it.
         * We might need to add some clamping in the shader engine to
         * support it.
         * TODO: WINED3DDTCAPS_USHORT2N, WINED3DDTCAPS_USHORT4N, WINED3DDTCAPS_UDEC3, WINED3DDTCAPS_DEC3N */
        pCaps->DeclTypes = WINED3DDTCAPS_UBYTE4    |
                           WINED3DDTCAPS_UBYTE4N   |
                           WINED3DDTCAPS_SHORT2N   |
                           WINED3DDTCAPS_SHORT4N;
        if (gl_info->supported[ARB_HALF_FLOAT_VERTEX])
        {
            pCaps->DeclTypes |= WINED3DDTCAPS_FLOAT16_2 |
                                WINED3DDTCAPS_FLOAT16_4;
        }
    } else
        pCaps->DeclTypes                         = 0;

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

    /* Fill the ddraw caps structure */
    pCaps->DirectDrawCaps.Caps =        WINEDDCAPS_GDI                      |
                                        WINEDDCAPS_PALETTE                  |
                                        blit_caps;
    pCaps->DirectDrawCaps.Caps2 =       WINEDDCAPS2_CERTIFIED                |
                                        WINEDDCAPS2_NOPAGELOCKREQUIRED       |
                                        WINEDDCAPS2_PRIMARYGAMMA             |
                                        WINEDDCAPS2_WIDESURFACES             |
                                        WINEDDCAPS2_CANRENDERWINDOWED;
    pCaps->DirectDrawCaps.SVBCaps =     blit_caps;
    pCaps->DirectDrawCaps.SVBCKeyCaps = ckey_caps;
    pCaps->DirectDrawCaps.SVBFXCaps =   fx_caps;
    pCaps->DirectDrawCaps.VSBCaps =     blit_caps;
    pCaps->DirectDrawCaps.VSBCKeyCaps = ckey_caps;
    pCaps->DirectDrawCaps.VSBFXCaps =   fx_caps;
    pCaps->DirectDrawCaps.SSBCaps =     blit_caps;
    pCaps->DirectDrawCaps.SSBCKeyCaps = ckey_caps;
    pCaps->DirectDrawCaps.SSBFXCaps =   fx_caps;

    pCaps->DirectDrawCaps.ddsCaps =     WINEDDSCAPS_ALPHA                   |
                                        WINEDDSCAPS_BACKBUFFER              |
                                        WINEDDSCAPS_FLIP                    |
                                        WINEDDSCAPS_FRONTBUFFER             |
                                        WINEDDSCAPS_OFFSCREENPLAIN          |
                                        WINEDDSCAPS_PALETTE                 |
                                        WINEDDSCAPS_PRIMARYSURFACE          |
                                        WINEDDSCAPS_SYSTEMMEMORY            |
                                        WINEDDSCAPS_VIDEOMEMORY             |
                                        WINEDDSCAPS_VISIBLE;
    pCaps->DirectDrawCaps.StrideAlign = DDRAW_PITCH_ALIGNMENT;

    /* Set D3D caps if OpenGL is available. */
    if (adapter->opengl)
    {
        pCaps->DirectDrawCaps.ddsCaps |=WINEDDSCAPS_3DDEVICE                |
                                        WINEDDSCAPS_MIPMAP                  |
                                        WINEDDSCAPS_TEXTURE                 |
                                        WINEDDSCAPS_ZBUFFER;
        pCaps->DirectDrawCaps.Caps |=   WINEDDCAPS_3D;
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DImpl_CreateDevice(IWineD3D *iface, UINT adapter_idx,
        WINED3DDEVTYPE device_type, HWND focus_window, DWORD flags, IUnknown *parent,
        IWineD3DDeviceParent *device_parent, IWineD3DDevice **device)
{
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    IWineD3DDeviceImpl *object;
    HRESULT hr;

    TRACE("iface %p, adapter_idx %u, device_type %#x, focus_window %p, flags %#x.\n"
            "parent %p, device_parent %p, device %p.\n",
            iface, adapter_idx, device_type, focus_window, flags,
            parent, device_parent, device);

    /* Validate the adapter number. If no adapters are available(no GL), ignore the adapter
     * number and create a device without a 3D adapter for 2D only operation. */
    if (IWineD3D_GetAdapterCount(iface) && adapter_idx >= IWineD3D_GetAdapterCount(iface))
    {
        return WINED3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate device memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = device_init(object, This, adapter_idx, device_type, focus_window, flags, parent, device_parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = (IWineD3DDevice *)object;

    IWineD3DDeviceParent_WineD3DDeviceCreated(device_parent, *device);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DImpl_GetParent(IWineD3D *iface, IUnknown **pParent) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    IUnknown_AddRef(This->parent);
    *pParent = This->parent;
    return WINED3D_OK;
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

BOOL InitAdapters(IWineD3DImpl *This)
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
        struct wined3d_adapter *adapter = &This->adapters[0];
        const struct wined3d_gl_info *gl_info = &adapter->gl_info;
        struct wined3d_fake_gl_ctx fake_gl_ctx = {0};
        int iPixelFormat;
        int res;
        int i;
        WineD3D_PixelFormat *cfgs;
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

        ret = IWineD3DImpl_FillGLCaps(adapter);
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

        /* Use the VideoRamSize registry setting when set */
        if(wined3d_settings.emulated_textureram)
            adapter->TextureRam = wined3d_settings.emulated_textureram;
        else
            adapter->TextureRam = adapter->gl_info.vidmem;
        adapter->UsedTextureRam = 0;
        TRACE("Emulating %dMB of texture ram\n", adapter->TextureRam/(1024*1024));

        /* Initialize the Adapter's DeviceName which is required for ChangeDisplaySettings and friends */
        DisplayDevice.cb = sizeof(DisplayDevice);
        EnumDisplayDevicesW(NULL, 0 /* Adapter 0 = iDevNum 0 */, &DisplayDevice, 0);
        TRACE("DeviceName: %s\n", debugstr_w(DisplayDevice.DeviceName));
        strcpyW(adapter->DeviceName, DisplayDevice.DeviceName);

        if (gl_info->supported[WGL_ARB_PIXEL_FORMAT])
        {
            int attribute;
            int attribs[11];
            int values[11];
            int nAttribs = 0;

            attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
            GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attribute, &adapter->nCfgs));

            adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, adapter->nCfgs *sizeof(WineD3D_PixelFormat));
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

            for (iPixelFormat=1; iPixelFormat <= adapter->nCfgs; ++iPixelFormat)
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

                cfgs->pbufferDrawable = FALSE;
                /* Check for pbuffer support when it is around as
                 * wglGetPixelFormatAttribiv fails for unknown attributes. */
                if (gl_info->supported[WGL_ARB_PBUFFER])
                {
                    int attrib = WGL_DRAW_TO_PBUFFER_ARB;
                    int value;
                    if(GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, 1, &attrib, &value)))
                        cfgs->pbufferDrawable = value;
                }

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

                TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, depth=%d, stencil=%d, samples=%d, windowDrawable=%d, pbufferDrawable=%d\n", cfgs->iPixelFormat, cfgs->iPixelType, cfgs->doubleBuffer, cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize, cfgs->depthSize, cfgs->stencilSize, cfgs->numSamples, cfgs->windowDrawable, cfgs->pbufferDrawable);
                cfgs++;
            }
        }
        else
        {
            int nCfgs = DescribePixelFormat(hdc, 0, 0, 0);
            adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nCfgs*sizeof(WineD3D_PixelFormat));
            adapter->nCfgs = 0; /* We won't accept all formats e.g. software accelerated ones will be skipped */

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
                cfgs->pbufferDrawable = 0;
                cfgs->windowDrawable = (ppfd.dwFlags & PFD_DRAW_TO_WINDOW) ? 1 : 0;
                cfgs->iPixelType = (ppfd.iPixelType == PFD_TYPE_RGBA) ? WGL_TYPE_RGBA_ARB : WGL_TYPE_COLORINDEX_ARB;
                cfgs->doubleBuffer = (ppfd.dwFlags & PFD_DOUBLEBUFFER) ? 1 : 0;
                cfgs->auxBuffers = ppfd.cAuxBuffers;
                cfgs->numSamples = 0;

                TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, depth=%d, stencil=%d, windowDrawable=%d, pbufferDrawable=%d\n", cfgs->iPixelFormat, cfgs->iPixelType, cfgs->doubleBuffer, cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize, cfgs->depthSize, cfgs->stencilSize, cfgs->windowDrawable, cfgs->pbufferDrawable);
                cfgs++;
                adapter->nCfgs++;
            }

            /* Yikes we haven't found any suitable formats. This should only happen in case of GDI software rendering which we can't use anyway as its 3D functionality is very, very limited */
            if(!adapter->nCfgs)
            {
                ERR("Disabling Direct3D because no hardware accelerated pixel formats have been found!\n");

                WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);
                HeapFree(GetProcessHeap(), 0, adapter->cfgs);
                goto nogl_adapter;
            }
        }

        /* D16, D24X8 and D24S8 are common depth / depth+stencil formats. All drivers support them though this doesn't
         * mean that the format is offered in hardware. For instance Geforce8 cards don't have offer D16 in hardware
         * but just fake it using D24(X8?) which is fine. D3D also allows that.
         * Some display drivers (i915 on Linux) only report mixed depth+stencil formats like D24S8. MSDN clearly mentions
         * that only on lockable formats (e.g. D16_locked) the bit order is guaranteed and that on other formats the
         * driver is allowed to consume more bits EXCEPT for stencil bits.
         *
         * Mark an adapter with this broken stencil behavior.
         */
        adapter->brokenStencil = TRUE;
        for (i = 0, cfgs = adapter->cfgs; i < adapter->nCfgs; ++i)
        {
            /* Nearly all drivers offer depth formats without stencil, only on i915 this if-statement won't be entered. */
            if(cfgs[i].depthSize && !cfgs[i].stencilSize) {
                adapter->brokenStencil = FALSE;
                break;
            }
        }

        WineD3D_ReleaseFakeGLContext(&fake_gl_ctx);

        select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);
        fillGLAttribFuncs(&adapter->gl_info);
        adapter->opengl = TRUE;
    }
    This->adapter_count = 1;
    TRACE("%u adapters successfully initialized\n", This->adapter_count);

    return TRUE;

nogl_adapter:
    /* Initialize an adapter for ddraw-only memory counting */
    memset(This->adapters, 0, sizeof(This->adapters));
    This->adapters[0].ordinal = 0;
    This->adapters[0].opengl = FALSE;
    This->adapters[0].monitorPoint.x = -1;
    This->adapters[0].monitorPoint.y = -1;

    This->adapters[0].driver_info.name = "Display";
    This->adapters[0].driver_info.description = "WineD3D DirectDraw Emulation";
    if(wined3d_settings.emulated_textureram) {
        This->adapters[0].TextureRam = wined3d_settings.emulated_textureram;
    } else {
        This->adapters[0].TextureRam = 8 * 1024 * 1024; /* This is plenty for a DDraw-only card */
    }

    initPixelFormatsNoGL(&This->adapters[0].gl_info);

    This->adapter_count = 1;
    return FALSE;
}

/**********************************************************
 * IWineD3D VTbl follows
 **********************************************************/

const IWineD3DVtbl IWineD3D_Vtbl =
{
    /* IUnknown */
    IWineD3DImpl_QueryInterface,
    IWineD3DImpl_AddRef,
    IWineD3DImpl_Release,
    /* IWineD3D */
    IWineD3DImpl_GetParent,
    IWineD3DImpl_GetAdapterCount,
    IWineD3DImpl_RegisterSoftwareDevice,
    IWineD3DImpl_GetAdapterMonitor,
    IWineD3DImpl_GetAdapterModeCount,
    IWineD3DImpl_EnumAdapterModes,
    IWineD3DImpl_GetAdapterDisplayMode,
    IWineD3DImpl_GetAdapterIdentifier,
    IWineD3DImpl_CheckDeviceMultiSampleType,
    IWineD3DImpl_CheckDepthStencilMatch,
    IWineD3DImpl_CheckDeviceType,
    IWineD3DImpl_CheckDeviceFormat,
    IWineD3DImpl_CheckDeviceFormatConversion,
    IWineD3DImpl_GetDeviceCaps,
    IWineD3DImpl_CreateDevice
};

static void STDMETHODCALLTYPE wined3d_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops wined3d_null_parent_ops =
{
    wined3d_null_wined3d_object_destroyed,
};
