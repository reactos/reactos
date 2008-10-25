/*
 * IWineD3D implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

/* Compile time diagnostics: */

#ifndef DEBUG_SINGLE_MODE
/* Set to 1 to force only a single display mode to be exposed: */
#define DEBUG_SINGLE_MODE 0
#endif

#include "config.h"
#include <assert.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

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
    {"GL_APPLE_flush_render",               APPLE_FLUSH_RENDER,             0                           },
    {"GL_APPLE_ycbcr_422",                  APPLE_YCBCR_422,                0                           },
    {"GL_APPLE_float_pixels",               APPLE_FLOAT_PIXELS,             0                           },

    /* ATI */
    {"GL_ATI_separate_stencil",             ATI_SEPARATE_STENCIL,           0                           },
    {"GL_ATI_texture_env_combine3",         ATI_TEXTURE_ENV_COMBINE3,       0                           },
    {"GL_ATI_texture_mirror_once",          ATI_TEXTURE_MIRROR_ONCE,        0                           },
    {"GL_ATI_envmap_bumpmap",               ATI_ENVMAP_BUMPMAP,             0                           },
    {"GL_ATI_fragment_shader",              ATI_FRAGMENT_SHADER,            0                           },
    {"GL_ATI_texture_compression_3dc",      ATI_TEXTURE_COMPRESSION_3DC,    0                           },

    /* ARB */
    {"GL_ARB_color_buffer_float",           ARB_COLOR_BUFFER_FLOAT,         0                           },
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS,               0                           },
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM,           0                           },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER,            0                           },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL,           0                           },
    {"GL_ARB_imaging",                      ARB_IMAGING,                    0                           },
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE,                0                           }, /* needs GLX_ARB_MULTISAMPLE as well */
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE,               0                           },
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY,            0                           },
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT,        0                           },
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS,           0                           },
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE,               0                           },
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
    {"GL_ARB_vertex_blend",                 ARB_VERTEX_BLEND,               0                           },
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT,       0                           },
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM,             0                           },
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER,              0                           },
    {"GL_ARB_shader_objects",               ARB_SHADER_OBJECTS,             0                           },

    /* EXT */
    {"GL_EXT_blend_color",                  EXT_BLEND_COLOR,                0                           },
    {"GL_EXT_blend_minmax",                 EXT_BLEND_MINMAX,               0                           },
    {"GL_EXT_blend_equation_separate",      EXT_BLEND_EQUATION_SEPARATE,    0                           },
    {"GL_EXT_blend_func_separate",          EXT_BLEND_FUNC_SEPARATE,        0                           },
    {"GL_EXT_fog_coord",                    EXT_FOG_COORD,                  0                           },
    {"GL_EXT_framebuffer_blit",             EXT_FRAMEBUFFER_BLIT,           0                           },
    {"GL_EXT_framebuffer_object",           EXT_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_EXT_paletted_texture",             EXT_PALETTED_TEXTURE,           0                           },
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS,           0                           },
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR,            0                           },
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE,           0                           },
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP,               0                           },
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D,                  MAKEDWORD_VERSION(1, 2)     },
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC,   0                           },
    {"GL_EXT_texture_compression_rgtc",     EXT_TEXTURE_COMPRESSION_RGTC,   0                           },
    {"GL_EXT_texture_env_add",              EXT_TEXTURE_ENV_ADD,            0                           },
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE,        0                           },
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3,           0                           },
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB,               0                           },
    {"GL_EXT_texture_filter_anisotropic",   EXT_TEXTURE_FILTER_ANISOTROPIC, 0                           },
    {"GL_EXT_texture_lod",                  EXT_TEXTURE_LOD,                0                           },
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS,           0                           },
    {"GL_EXT_vertex_shader",                EXT_VERTEX_SHADER,              0                           },
    {"GL_EXT_gpu_program_parameters",       EXT_GPU_PROGRAM_PARAMETERS,     0                           },

    /* NV */
    {"GL_NV_half_float",                    NV_HALF_FLOAT,                  0                           },
    {"GL_NV_fence",                         NV_FENCE,                       0                           },
    {"GL_NV_fog_distance",                  NV_FOG_DISTANCE,                0                           },
    {"GL_NV_fragment_program",              NV_FRAGMENT_PROGRAM,            0                           },
    {"GL_NV_fragment_program2",             NV_FRAGMENT_PROGRAM2,           0                           },
    {"GL_NV_register_combiners",            NV_REGISTER_COMBINERS,          0                           },
    {"GL_NV_register_combiners2",           NV_REGISTER_COMBINERS2,         0                           },
    {"GL_NV_texgen_reflection",             NV_TEXGEN_REFLECTION,           0                           },
    {"GL_NV_texture_env_combine4",          NV_TEXTURE_ENV_COMBINE4,        0                           },
    {"GL_NV_texture_shader",                NV_TEXTURE_SHADER,              0                           },
    {"GL_NV_texture_shader2",               NV_TEXTURE_SHADER2,             0                           },
    {"GL_NV_texture_shader3",               NV_TEXTURE_SHADER3,             0                           },
    {"GL_NV_occlusion_query",               NV_OCCLUSION_QUERY,             0                           },
    {"GL_NV_vertex_program",                NV_VERTEX_PROGRAM,              0                           },
    {"GL_NV_vertex_program1_1",             NV_VERTEX_PROGRAM1_1,           0                           },
    {"GL_NV_vertex_program2",               NV_VERTEX_PROGRAM2,             0                           },
    {"GL_NV_vertex_program3",               NV_VERTEX_PROGRAM3,             0                           },
    {"GL_NV_depth_clamp",                   NV_DEPTH_CLAMP,                 0                           },
    {"GL_NV_light_max_exponent",            NV_LIGHT_MAX_EXPONENT,          0                           },

    /* SGI */
    {"GL_SGIS_generate_mipmap",             SGIS_GENERATE_MIPMAP,           0                           },
};

/**********************************************************
 * Utility functions follow
 **********************************************************/

/* Adapters */
static int numAdapters = 0;
static struct WineD3DAdapter Adapters[1];

static HRESULT WINAPI IWineD3DImpl_CheckDeviceFormat(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DFORMAT AdapterFormat, DWORD Usage, WINED3DRESOURCETYPE RType, WINED3DFORMAT CheckFormat, WINED3DSURFTYPE SurfaceType);
static const struct fragment_pipeline *select_fragment_implementation(UINT Adapter, WINED3DDEVTYPE DeviceType);
static const shader_backend_t *select_shader_backend(UINT Adapter, WINED3DDEVTYPE DeviceType);
static const struct blit_shader *select_blit_implementation(UINT Adapter, WINED3DDEVTYPE DeviceType);

/* lookup tables */
int minLookup[MAX_LOOKUPS];
int maxLookup[MAX_LOOKUPS];
DWORD *stateLookup[MAX_LOOKUPS];

DWORD minMipLookup[WINED3DTEXF_ANISOTROPIC + 1][WINED3DTEXF_LINEAR + 1];
DWORD minMipLookup_noFilter[WINED3DTEXF_ANISOTROPIC + 1][WINED3DTEXF_LINEAR + 1] = {
    {GL_NEAREST, GL_NEAREST, GL_NEAREST},
    {GL_NEAREST, GL_NEAREST, GL_NEAREST},
    {GL_NEAREST, GL_NEAREST, GL_NEAREST},
    {GL_NEAREST, GL_NEAREST, GL_NEAREST},
};

DWORD magLookup[WINED3DTEXF_ANISOTROPIC + 1];
DWORD magLookup_noFilter[WINED3DTEXF_ANISOTROPIC + 1] = {
    GL_NEAREST, GL_NEAREST, GL_NEAREST, GL_NEAREST
};

/* drawStridedSlow attributes */
glAttribFunc position_funcs[WINED3DDECLTYPE_UNUSED];
glAttribFunc diffuse_funcs[WINED3DDECLTYPE_UNUSED];
glAttribFunc specular_funcs[WINED3DDECLTYPE_UNUSED];
glAttribFunc normal_funcs[WINED3DDECLTYPE_UNUSED];
glTexAttribFunc texcoord_funcs[WINED3DDECLTYPE_UNUSED];

/**
 * Note: GL seems to trap if GetDeviceCaps is called before any HWND's created,
 * i.e., there is no GL Context - Get a default rendering context to enable the
 * function query some info from GL.
 */

static int             wined3d_fake_gl_context_ref = 0;
static BOOL            wined3d_fake_gl_context_foreign;
static BOOL            wined3d_fake_gl_context_available = FALSE;
static HDC             wined3d_fake_gl_context_hdc = NULL;
static HWND            wined3d_fake_gl_context_hwnd = NULL;

static CRITICAL_SECTION wined3d_fake_gl_context_cs;
static CRITICAL_SECTION_DEBUG wined3d_fake_gl_context_cs_debug =
{
    0, 0, &wined3d_fake_gl_context_cs,
    { &wined3d_fake_gl_context_cs_debug.ProcessLocksList,
      &wined3d_fake_gl_context_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": wined3d_fake_gl_context_cs") }
};
static CRITICAL_SECTION wined3d_fake_gl_context_cs = { &wined3d_fake_gl_context_cs_debug, -1, 0, 0, 0, 0 };

static void WineD3D_ReleaseFakeGLContext(void) {
    HGLRC glCtx;

    EnterCriticalSection(&wined3d_fake_gl_context_cs);

    if(!wined3d_fake_gl_context_available) {
        TRACE_(d3d_caps)("context not available\n");
        LeaveCriticalSection(&wined3d_fake_gl_context_cs);
        return;
    }

    glCtx = pwglGetCurrentContext();

    TRACE_(d3d_caps)("decrementing ref from %i\n", wined3d_fake_gl_context_ref);
    if (0 == (--wined3d_fake_gl_context_ref) ) {
        if(!wined3d_fake_gl_context_foreign && glCtx) {
            TRACE_(d3d_caps)("destroying fake GL context\n");
            pwglMakeCurrent(NULL, NULL);
            //ros hack, this line does destire the real icd interface in windows and reactos
            // pwglDeleteContext(glCtx);
        }
        if(wined3d_fake_gl_context_hdc)
            ReleaseDC(wined3d_fake_gl_context_hwnd, wined3d_fake_gl_context_hdc);
        wined3d_fake_gl_context_hdc = NULL; /* Make sure we don't think that it is still around */
        if(wined3d_fake_gl_context_hwnd)
            DestroyWindow(wined3d_fake_gl_context_hwnd);
        wined3d_fake_gl_context_hwnd = NULL;
        wined3d_fake_gl_context_available = FALSE;
    }
    assert(wined3d_fake_gl_context_ref >= 0);

    LeaveCriticalSection(&wined3d_fake_gl_context_cs);
}

static BOOL WineD3D_CreateFakeGLContext(void) {
    HGLRC glCtx = NULL;

    EnterCriticalSection(&wined3d_fake_gl_context_cs);

    TRACE("getting context...\n");
    if(wined3d_fake_gl_context_ref > 0) goto ret;
    assert(0 == wined3d_fake_gl_context_ref);

    wined3d_fake_gl_context_foreign = TRUE;

    glCtx = pwglGetCurrentContext();
    if (!glCtx) {
        PIXELFORMATDESCRIPTOR pfd;
        int iPixelFormat;

        wined3d_fake_gl_context_foreign = FALSE;

        /* We need a fake window as a hdc retrieved using GetDC(0) can't be used for much GL purposes */
        wined3d_fake_gl_context_hwnd = CreateWindowA("WineD3D_OpenGL", "WineD3D fake window", WS_OVERLAPPEDWINDOW,        10, 10, 10, 10, NULL, NULL, NULL, NULL);
        if(!wined3d_fake_gl_context_hwnd) {
            ERR("HWND creation failed!\n");
            goto fail;
        }
        wined3d_fake_gl_context_hdc = GetDC(wined3d_fake_gl_context_hwnd);
        if(!wined3d_fake_gl_context_hdc) {
            ERR("GetDC failed!\n");
            goto fail;
        }

        /* PixelFormat selection */
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;/*PFD_GENERIC_ACCELERATED*/
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.iLayerType = PFD_MAIN_PLANE;

        iPixelFormat = ChoosePixelFormat(wined3d_fake_gl_context_hdc, &pfd);
        if(!iPixelFormat) {
            /* If this happens something is very wrong as ChoosePixelFormat barely fails */
            ERR("Can't find a suitable iPixelFormat\n");
            goto fail;
        }
        DescribePixelFormat(wined3d_fake_gl_context_hdc, iPixelFormat, sizeof(pfd), &pfd);
        SetPixelFormat(wined3d_fake_gl_context_hdc, iPixelFormat, &pfd);

        /* Create a GL context */
        glCtx = pwglCreateContext(wined3d_fake_gl_context_hdc);
        if (!glCtx) {
            WARN_(d3d_caps)("Error creating default context for capabilities initialization\n");
            goto fail;
        }

        /* Make it the current GL context */
        if (!pwglMakeCurrent(wined3d_fake_gl_context_hdc, glCtx)) {
            WARN_(d3d_caps)("Error setting default context as current for capabilities initialization\n");
            goto fail;
        }
    }

  ret:
    TRACE("incrementing ref from %i\n", wined3d_fake_gl_context_ref);
    wined3d_fake_gl_context_ref++;
    wined3d_fake_gl_context_available = TRUE;
    LeaveCriticalSection(&wined3d_fake_gl_context_cs);
    return TRUE;
  fail:
    if(wined3d_fake_gl_context_hdc)
        ReleaseDC(wined3d_fake_gl_context_hwnd, wined3d_fake_gl_context_hdc);
    wined3d_fake_gl_context_hdc = NULL;
    if(wined3d_fake_gl_context_hwnd)
        DestroyWindow(wined3d_fake_gl_context_hwnd);
    wined3d_fake_gl_context_hwnd = NULL;
    if(glCtx) pwglDeleteContext(glCtx);
    LeaveCriticalSection(&wined3d_fake_gl_context_cs);
    return FALSE;
}

/* Adjust the amount of used texture memory */
long WineD3DAdapterChangeGLRam(IWineD3DDeviceImpl *D3DDevice, long glram){
    UINT Adapter = D3DDevice->adapterNo;

    Adapters[Adapter].UsedTextureRam += glram;
    TRACE("Adjusted gl ram by %ld to %d\n", glram, Adapters[Adapter].UsedTextureRam);
    return Adapters[Adapter].UsedTextureRam;
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
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* Set the shader type for this device, depending on the given capabilities,
 * the device type, and the user preferences in wined3d_settings */

void select_shader_mode(
    WineD3D_GL_Info *gl_info,
    WINED3DDEVTYPE DeviceType,
    int* ps_selected,
    int* vs_selected) {

    if (wined3d_settings.vs_mode == VS_NONE) {
        *vs_selected = SHADER_NONE;
    } else if (gl_info->supported[ARB_VERTEX_SHADER] && wined3d_settings.glslRequested) {
        /* Geforce4 cards support GLSL but for vertex shaders only. Further its reported GLSL caps are
         * wrong. This combined with the fact that glsl won't offer more features or performance, use ARB
         * shaders only on this card. */
        if(gl_info->vs_nv_version && gl_info->vs_nv_version < VS_VERSION_20)
            *vs_selected = SHADER_ARB;
        else
            *vs_selected = SHADER_GLSL;
    } else if (gl_info->supported[ARB_VERTEX_PROGRAM]) {
        *vs_selected = SHADER_ARB;
    } else {
        *vs_selected = SHADER_NONE;
    }

    if (wined3d_settings.ps_mode == PS_NONE) {
        *ps_selected = SHADER_NONE;
    } else if (gl_info->supported[ARB_FRAGMENT_SHADER] && wined3d_settings.glslRequested) {
        *ps_selected = SHADER_GLSL;
    } else if (gl_info->supported[ARB_FRAGMENT_PROGRAM]) {
        *ps_selected = SHADER_ARB;
    } else if (gl_info->supported[ATI_FRAGMENT_SHADER]) {
        *ps_selected = SHADER_ATI;
    } else {
        *ps_selected = SHADER_NONE;
    }
}

/** Select the number of report maximum shader constants based on the selected shader modes */
static void select_shader_max_constants(
    int ps_selected_mode,
    int vs_selected_mode,
    WineD3D_GL_Info *gl_info) {

    switch (vs_selected_mode) {
        case SHADER_GLSL:
            /* Subtract the other potential uniforms from the max available (bools, ints, and 1 row of projection matrix) */
            gl_info->max_vshader_constantsF = gl_info->vs_glsl_constantsF - (MAX_CONST_B / 4) - MAX_CONST_I - 1;
            break;
        case SHADER_ARB:
            /* We have to subtract any other PARAMs that we might use in our shader programs.
             * ATI seems to count 2 implicit PARAMs when we use fog and NVIDIA counts 1,
             * and we reference one row of the PROJECTION matrix which counts as 1 PARAM. */
            gl_info->max_vshader_constantsF = gl_info->vs_arb_constantsF - 3;
            break;
        default:
            gl_info->max_vshader_constantsF = 0;
            break;
    }

    switch (ps_selected_mode) {
        case SHADER_GLSL:
            /* Subtract the other potential uniforms from the max available (bools & ints), and 2 states for fog.
             * In theory the texbem instruction may need one more shader constant too. But lets assume
             * that a sm <= 1.3 shader does not need all the uniforms provided by a glsl-capable card,
             * and lets not take away a uniform needlessly from all other shaders.
             */
            gl_info->max_pshader_constantsF = gl_info->ps_glsl_constantsF - (MAX_CONST_B / 4) - MAX_CONST_I - 2;
            break;
        case SHADER_ARB:
            /* The arb shader only loads the bump mapping environment matrix into the shader if it finds
             * a free constant to do that, so only reduce the number of available constants by 2 for the fog states.
             */
            gl_info->max_pshader_constantsF = gl_info->ps_arb_constantsF - 2;
            break;
        default:
            gl_info->max_pshader_constantsF = 0;
            break;
    }
}

/**********************************************************
 * IWineD3D parts follows
 **********************************************************/

#define GLINFO_LOCATION (*gl_info)
static inline BOOL test_arb_vs_offset_limit(WineD3D_GL_Info *gl_info) {
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
    checkGLcall("ARB vp offset limit test cleanup\n");

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

BOOL IWineD3DImpl_FillGLCaps(WineD3D_GL_Info *gl_info) {
    const char *GL_Extensions    = NULL;
    const char *WGL_Extensions   = NULL;
    const char *gl_string        = NULL;
    const char *gl_string_cursor = NULL;
    GLint       gl_max;
    GLfloat     gl_floatv[2];
    int         major = 1, minor = 0;
    BOOL        return_value = TRUE;
    unsigned    i;
    HDC         hdc;
    unsigned int vidmem=0;

    TRACE_(d3d_caps)("(%p)\n", gl_info);

    ENTER_GL();

    gl_string = (const char *) glGetString(GL_RENDERER);
    if (NULL == gl_string)
	gl_string = "None";
    strcpy(gl_info->gl_renderer, gl_string);

    gl_string = (const char *) glGetString(GL_VENDOR);
    TRACE_(d3d_caps)("Filling vendor string %s\n", gl_string);
    if (gl_string != NULL) {
        /* Fill in the GL vendor */
        if (strstr(gl_string, "NVIDIA")) {
            gl_info->gl_vendor = VENDOR_NVIDIA;
        } else if (strstr(gl_string, "ATI")) {
            gl_info->gl_vendor = VENDOR_ATI;
        } else if (strstr(gl_string, "Intel(R)") ||
                   strstr(gl_info->gl_renderer, "Intel(R)") ||
                   strstr(gl_string, "Intel Inc.")) {
            gl_info->gl_vendor = VENDOR_INTEL;
        } else if (strstr(gl_string, "Mesa")) {
            gl_info->gl_vendor = VENDOR_MESA;
        } else {
            gl_info->gl_vendor = VENDOR_WINE;
        }
    } else {
        gl_info->gl_vendor = VENDOR_WINE;
    }


    TRACE_(d3d_caps)("found GL_VENDOR (%s)->(0x%04x)\n", debugstr_a(gl_string), gl_info->gl_vendor);

    /* Parse the GL_VERSION field into major and minor information */
    gl_string = (const char *) glGetString(GL_VERSION);
    if (gl_string != NULL) {

        /* First, parse the generic opengl version. This is supposed not to be convoluted with
         * driver specific information
         */
        gl_string_cursor = gl_string;
        major = atoi(gl_string_cursor);
        if(major <= 0) {
            ERR("Invalid opengl major version: %d\n", major);
        }
        while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
            ++gl_string_cursor;
        }
        if (*gl_string_cursor++ != '.') {
            ERR_(d3d_caps)("Invalid opengl version string: %s\n", debugstr_a(gl_string));
        }
        minor = atoi(gl_string_cursor);
        TRACE_(d3d_caps)("Found OpenGL version: %d.%d\n", major, minor);
        gl_info->gl_version = MAKEDWORD_VERSION(major, minor);

        /* Now parse the driver specific string which we'll report to the app */
        switch (gl_info->gl_vendor) {
        case VENDOR_NVIDIA:
            gl_string_cursor = strstr(gl_string, "NVIDIA");
            if (!gl_string_cursor) {
                ERR_(d3d_caps)("Invalid nVidia version string: %s\n", debugstr_a(gl_string));
                break;
            }

            gl_string_cursor = strstr(gl_string_cursor, " ");
            if (!gl_string_cursor) {
                ERR_(d3d_caps)("Invalid nVidia version string: %s\n", debugstr_a(gl_string));
                break;
            }

            while (*gl_string_cursor == ' ') {
                ++gl_string_cursor;
            }

            if (!*gl_string_cursor) {
                ERR_(d3d_caps)("Invalid nVidia version string: %s\n", debugstr_a(gl_string));
                break;
            }

            major = atoi(gl_string_cursor);
            while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
                ++gl_string_cursor;
            }

            if (*gl_string_cursor++ != '.') {
                ERR_(d3d_caps)("Invalid nVidia version string: %s\n", debugstr_a(gl_string));
                break;
            }

            minor = atoi(gl_string_cursor);
            minor = major*100+minor;
            major = 10;

            break;

        case VENDOR_ATI:
            major = minor = 0;
            gl_string_cursor = strchr(gl_string, '-');
            if (gl_string_cursor) {
                int error = 0;
                gl_string_cursor++;

                /* Check if version number is of the form x.y.z */
                if (*gl_string_cursor > '9' && *gl_string_cursor < '0')
                    error = 1;
                if (!error && *(gl_string_cursor+2) > '9' && *(gl_string_cursor+2) < '0')
                    error = 1;
                if (!error && *(gl_string_cursor+4) > '9' && *(gl_string_cursor+4) < '0')
                    error = 1;
                if (!error && *(gl_string_cursor+1) != '.' && *(gl_string_cursor+3) != '.')
                    error = 1;

                /* Mark version number as malformed */
                if (error)
                    gl_string_cursor = 0;
            }

            if (!gl_string_cursor)
                WARN_(d3d_caps)("malformed GL_VERSION (%s)\n", debugstr_a(gl_string));
            else {
                major = *gl_string_cursor - '0';
                minor = (*(gl_string_cursor+2) - '0') * 256 + (*(gl_string_cursor+4) - '0');
            }
            break;

        case VENDOR_INTEL:
            /* Apple and Mesa version strings look differently, but both provide intel drivers */
            if(strstr(gl_string, "APPLE")) {
                /* [0-9]+.[0-9]+ APPLE-[0-9]+.[0.9]+.[0.9]+
                 * We only need the first part, and use the APPLE as identification
                 * "1.2 APPLE-1.4.56"
                 */
                gl_string_cursor = gl_string;
                major = atoi(gl_string_cursor);
                while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
                    ++gl_string_cursor;
                }

                if (*gl_string_cursor++ != '.') {
                    ERR_(d3d_caps)("Invalid MacOS-Intel version string: %s\n", debugstr_a(gl_string));
                    break;
                }

                minor = atoi(gl_string_cursor);
                break;
            }

        case VENDOR_MESA:
            gl_string_cursor = strstr(gl_string, "Mesa");
            gl_string_cursor = strstr(gl_string_cursor, " ");
            while (*gl_string_cursor && ' ' == *gl_string_cursor) ++gl_string_cursor;
            if (*gl_string_cursor) {
                char tmp[16];
                int cursor = 0;

                while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
                    tmp[cursor++] = *gl_string_cursor;
                    ++gl_string_cursor;
                }
                tmp[cursor] = 0;
                major = atoi(tmp);

                if (*gl_string_cursor != '.') WARN_(d3d_caps)("malformed GL_VERSION (%s)\n", debugstr_a(gl_string));
                ++gl_string_cursor;

                cursor = 0;
                while (*gl_string_cursor <= '9' && *gl_string_cursor >= '0') {
                    tmp[cursor++] = *gl_string_cursor;
                    ++gl_string_cursor;
                }
                tmp[cursor] = 0;
                minor = atoi(tmp);
            }
            break;

        default:
            major = 0;
            minor = 9;
        }
        gl_info->driver_version = MAKEDWORD_VERSION(major, minor);
        TRACE_(d3d_caps)("found driver version (%s)->%i.%i->(0x%08x)\n", debugstr_a(gl_string), major, minor, gl_info->driver_version);
        /* Current Windows drivers have versions like 6.14.... (some older have an earlier version) */
        gl_info->driver_version_hipart = MAKEDWORD_VERSION(6, 14);
    } else {
        FIXME("OpenGL driver did not return version information\n");
        gl_info->driver_version = MAKEDWORD_VERSION(0, 0);
        gl_info->driver_version_hipart = MAKEDWORD_VERSION(6, 14);
    }

    TRACE_(d3d_caps)("found GL_RENDERER (%s)->(0x%04x)\n", debugstr_a(gl_info->gl_renderer), gl_info->gl_card);

    /*
     * Initialize openGL extension related variables
     *  with Default values
     */
    memset(gl_info->supported, 0, sizeof(gl_info->supported));
    gl_info->max_buffers        = 1;
    gl_info->max_textures       = 1;
    gl_info->max_texture_stages = 1;
    gl_info->max_fragment_samplers = 1;
    gl_info->max_vertex_samplers = 0;
    gl_info->max_combined_samplers = 0;
    gl_info->max_sampler_stages = 1;
    gl_info->ps_arb_version = PS_VERSION_NOT_SUPPORTED;
    gl_info->ps_arb_max_temps = 0;
    gl_info->ps_arb_max_instructions = 0;
    gl_info->vs_arb_version = VS_VERSION_NOT_SUPPORTED;
    gl_info->vs_arb_max_temps = 0;
    gl_info->vs_arb_max_instructions = 0;
    gl_info->vs_nv_version  = VS_VERSION_NOT_SUPPORTED;
    gl_info->vs_ati_version = VS_VERSION_NOT_SUPPORTED;
    gl_info->vs_glsl_constantsF = 0;
    gl_info->ps_glsl_constantsF = 0;
    gl_info->vs_arb_constantsF = 0;
    gl_info->ps_arb_constantsF = 0;

    /* Retrieve opengl defaults */
    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    gl_info->max_clipplanes = min(WINED3DMAXUSERCLIPPLANES, gl_max);
    TRACE_(d3d_caps)("ClipPlanes support - num Planes=%d\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    gl_info->max_lights = gl_max;
    TRACE_(d3d_caps)("Lights support - max lights=%d\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    gl_info->max_texture_size = gl_max;
    TRACE_(d3d_caps)("Maximum texture size support - max texture size=%d\n", gl_max);

    glGetFloatv(GL_POINT_SIZE_RANGE, gl_floatv);
    gl_info->max_pointsizemin = gl_floatv[0];
    gl_info->max_pointsize = gl_floatv[1];
    TRACE_(d3d_caps)("Maximum point size support - max point size=%f\n", gl_floatv[1]);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately */
    GL_Extensions = (const char *) glGetString(GL_EXTENSIONS);
    TRACE_(d3d_caps)("GL_Extensions reported:\n");

    if (NULL == GL_Extensions) {
        ERR("   GL_Extensions returns NULL\n");
    } else {
        while (*GL_Extensions != 0x00) {
            const char *Start;
            char        ThisExtn[256];
            size_t      len;

            while (isspace(*GL_Extensions)) GL_Extensions++;
            Start = GL_Extensions;
            while (!isspace(*GL_Extensions) && *GL_Extensions != 0x00) {
                GL_Extensions++;
            }

            len = GL_Extensions - Start;
            if (len == 0 || len >= sizeof(ThisExtn))
                continue;

            memcpy(ThisExtn, Start, len);
            ThisExtn[len] = '\0';
            TRACE_(d3d_caps)("- %s\n", ThisExtn);

            for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i) {
                if (!strcmp(ThisExtn, EXTENSION_MAP[i].extension_string)) {
                    TRACE_(d3d_caps)(" FOUND: %s support\n", EXTENSION_MAP[i].extension_string);
                    gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
                    break;
                }
            }
        }

        LEAVE_GL();

        /* Now work out what GL support this card really has */
#define USE_GL_FUNC(type, pfn, ext, replace) { \
            DWORD ver = ver_for_ext(ext); \
            if(gl_info->supported[ext]) gl_info->pfn = (type) pwglGetProcAddress(#pfn); \
            else if(ver && ver <= gl_info->gl_version) gl_info->pfn = (type) pwglGetProcAddress(#replace); \
            else gl_info->pfn = NULL; \
        }
        GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

#define USE_GL_FUNC(type, pfn, ext, replace) gl_info->pfn = (type) pwglGetProcAddress(#pfn);
        WGL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

        ENTER_GL();
        /* Now mark all the extensions supported which are included in the opengl core version. Do this *after*
         * loading the functions, otherwise the code above will load the extension entry points instead of the
         * core functions, which may not work
         */
        for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i) {
            if (gl_info->supported[EXTENSION_MAP[i].extension] == FALSE &&
                EXTENSION_MAP[i].version <= gl_info->gl_version && EXTENSION_MAP[i].version) {
                TRACE_(d3d_caps)(" GL CORE: %s support\n", EXTENSION_MAP[i].extension_string);
                gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
            }
        }

        if (gl_info->supported[APPLE_FENCE]) {
            /* GL_NV_fence and GL_APPLE_fence provide the same functionality basically.
             * The apple extension interacts with some other apple exts. Disable the NV
             * extension if the apple one is support to prevent confusion in other parts
             * of the code
             */
            gl_info->supported[NV_FENCE] = FALSE;
        }
        if (gl_info->supported[APPLE_FLOAT_PIXELS]) {
            /* GL_APPLE_float_pixels == GL_ARB_texture_float + GL_ARB_half_float_pixel
             *
             * The enums are the same:
             * GL_RGBA16F_ARB     = GL_RGBA_FLOAT16_APPLE = 0x881A
             * GL_RGB16F_ARB      = GL_RGB_FLOAT16_APPLE  = 0x881B
             * GL_RGBA32F_ARB     = GL_RGBA_FLOAT32_APPLE = 0x8814
             * GL_RGB32F_ARB      = GL_RGB_FLOAT32_APPLE  = 0x8815
             * GL_HALF_FLOAT_ARB  = GL_HALF_APPLE         =  0x140B
             */
            if(!gl_info->supported[ARB_TEXTURE_FLOAT]) {
                TRACE_(d3d_caps)(" IMPLIED: GL_ARB_texture_float support(from GL_APPLE_float_pixels\n");
                gl_info->supported[ARB_TEXTURE_FLOAT] = TRUE;
            }
            if(!gl_info->supported[ARB_HALF_FLOAT_PIXEL]) {
                TRACE_(d3d_caps)(" IMPLIED: GL_ARB_half_float_pixel support(from GL_APPLE_float_pixels\n");
                gl_info->supported[ARB_HALF_FLOAT_PIXEL] = TRUE;
            }
        }
        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP]) {
            TRACE_(d3d_caps)(" IMPLIED: NVIDIA (NV) Texture Gen Reflection support\n");
            gl_info->supported[NV_TEXGEN_REFLECTION] = TRUE;
        }
        if (gl_info->supported[NV_TEXTURE_SHADER2]) {
            /* GL_ATI_envmap_bumpmap won't play nice with texture shaders, so disable it
             * Won't occur in any real world situation though
             */
            gl_info->supported[ATI_ENVMAP_BUMPMAP] = FALSE;
            if(gl_info->supported[NV_REGISTER_COMBINERS]) {
                /* Also disable ATI_FRAGMENT_SHADER if register combiners and texture_shader2
                 * are supported. The nv extensions provide the same functionality as the
                 * ATI one, and a bit more(signed pixelformats)
                 */
                gl_info->supported[ATI_FRAGMENT_SHADER] = FALSE;
            }
        }
        if (gl_info->supported[ARB_DRAW_BUFFERS]) {
            glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &gl_max);
            gl_info->max_buffers = gl_max;
            TRACE_(d3d_caps)("Max draw buffers: %u\n", gl_max);
        }
        if (gl_info->supported[ARB_MULTITEXTURE]) {
            glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
            gl_info->max_textures = min(MAX_TEXTURES, gl_max);
            TRACE_(d3d_caps)("Max textures: %d\n", gl_info->max_textures);

            if (gl_info->supported[NV_REGISTER_COMBINERS]) {
                GLint tmp;
                glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &tmp);
                gl_info->max_texture_stages = min(MAX_TEXTURES, tmp);
            } else {
                gl_info->max_texture_stages = min(MAX_TEXTURES, gl_max);
            }
            TRACE_(d3d_caps)("Max texture stages: %d\n", gl_info->max_texture_stages);

            if (gl_info->supported[ARB_FRAGMENT_PROGRAM]) {
                GLint tmp;
                glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
                gl_info->max_fragment_samplers = min(MAX_FRAGMENT_SAMPLERS, tmp);
            } else {
                gl_info->max_fragment_samplers = max(gl_info->max_fragment_samplers, gl_max);
            }
            TRACE_(d3d_caps)("Max fragment samplers: %d\n", gl_info->max_fragment_samplers);

            if (gl_info->supported[ARB_VERTEX_SHADER]) {
                GLint tmp;
                glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
                gl_info->max_vertex_samplers = tmp;
                glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &tmp);
                gl_info->max_combined_samplers = tmp;

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
                 * and reduce the number of vertex samplers or probably disable vertex texture fetch.
                 */
                if(gl_info->max_vertex_samplers &&
                   MAX_TEXTURES + gl_info->max_vertex_samplers > gl_info->max_combined_samplers) {
                    FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers\n",
                          gl_info->max_vertex_samplers, gl_info->max_combined_samplers);
                    FIXME("Expected vertex samplers + MAX_TEXTURES(=8) > combined_samplers\n");
                    if( gl_info->max_combined_samplers > MAX_TEXTURES )
                        gl_info->max_vertex_samplers =
                            gl_info->max_combined_samplers - MAX_TEXTURES;
                    else
                        gl_info->max_vertex_samplers = 0;
                }
            } else {
                gl_info->max_combined_samplers = gl_info->max_fragment_samplers;
            }
            TRACE_(d3d_caps)("Max vertex samplers: %u\n", gl_info->max_vertex_samplers);
            TRACE_(d3d_caps)("Max combined samplers: %u\n", gl_info->max_combined_samplers);
        }
        if (gl_info->supported[ARB_VERTEX_BLEND]) {
            glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
            gl_info->max_blends = gl_max;
            TRACE_(d3d_caps)("Max blends: %u\n", gl_info->max_blends);
        }
        if (gl_info->supported[EXT_TEXTURE3D]) {
            glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &gl_max);
            gl_info->max_texture3d_size = gl_max;
            TRACE_(d3d_caps)("Max texture3D size: %d\n", gl_info->max_texture3d_size);
        }
        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC]) {
            glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
            gl_info->max_anisotropy = gl_max;
            TRACE_(d3d_caps)("Max anisotropy: %d\n", gl_info->max_anisotropy);
        }
        if (gl_info->supported[ARB_FRAGMENT_PROGRAM]) {
            gl_info->ps_arb_version = PS_VERSION_11;
            GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
            gl_info->ps_arb_constantsF = gl_max;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM float constants: %d\n", gl_info->ps_arb_constantsF);
            GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
            gl_info->ps_arb_max_temps = gl_max;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native temporaries: %d\n", gl_info->ps_arb_max_temps);
            GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
            gl_info->ps_arb_max_instructions = gl_max;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native instructions: %d\n", gl_info->ps_arb_max_instructions);
        }
        if (gl_info->supported[ARB_VERTEX_PROGRAM]) {
            gl_info->vs_arb_version = VS_VERSION_11;
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
            gl_info->vs_arb_constantsF = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM float constants: %d\n", gl_info->vs_arb_constantsF);
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
            gl_info->vs_arb_max_temps = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native temporaries: %d\n", gl_info->vs_arb_max_temps);
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
            gl_info->vs_arb_max_instructions = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native instructions: %d\n", gl_info->vs_arb_max_instructions);

            gl_info->arb_vs_offset_limit = test_arb_vs_offset_limit(gl_info);
        }
        if (gl_info->supported[ARB_VERTEX_SHADER]) {
            glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &gl_max);
            gl_info->vs_glsl_constantsF = gl_max / 4;
            TRACE_(d3d_caps)("Max ARB_VERTEX_SHADER float constants: %u\n", gl_info->vs_glsl_constantsF);
        }
        if (gl_info->supported[ARB_FRAGMENT_SHADER]) {
            glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &gl_max);
            gl_info->ps_glsl_constantsF = gl_max / 4;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_SHADER float constants: %u\n", gl_info->ps_glsl_constantsF);
            glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max);
            gl_info->max_glsl_varyings = gl_max;
            TRACE_(d3d_caps)("Max GLSL varyings: %u (%u 4 component varyings)\n", gl_max, gl_max / 4);
        }
        if (gl_info->supported[EXT_VERTEX_SHADER]) {
            gl_info->vs_ati_version = VS_VERSION_11;
        }
        if (gl_info->supported[NV_VERTEX_PROGRAM3]) {
            gl_info->vs_nv_version = VS_VERSION_30;
        } else if (gl_info->supported[NV_VERTEX_PROGRAM2]) {
            gl_info->vs_nv_version = VS_VERSION_20;
        } else if (gl_info->supported[NV_VERTEX_PROGRAM1_1]) {
            gl_info->vs_nv_version = VS_VERSION_11;
        } else if (gl_info->supported[NV_VERTEX_PROGRAM]) {
            gl_info->vs_nv_version = VS_VERSION_10;
        }
        if (gl_info->supported[NV_FRAGMENT_PROGRAM2]) {
            gl_info->ps_nv_version = PS_VERSION_30;
        } else if (gl_info->supported[NV_FRAGMENT_PROGRAM]) {
            gl_info->ps_nv_version = PS_VERSION_20;
        }
        if (gl_info->supported[NV_LIGHT_MAX_EXPONENT]) {
            glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->max_shininess);
        } else {
            gl_info->max_shininess = 128.0;
        }
        if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO]) {
            /* If we have full NP2 texture support, disable GL_ARB_texture_rectangle because we will never use it.
             * This saves a few redundant glDisable calls
             */
            gl_info->supported[ARB_TEXTURE_RECTANGLE] = FALSE;
        }
        if(gl_info->supported[ATI_FRAGMENT_SHADER]) {
            /* Disable NV_register_combiners and fragment shader if this is supported.
             * generally the NV extensions are preferred over the ATI ones, and this
             * extension is disabled if register_combiners and texture_shader2 are both
             * supported. So we reach this place only if we have incomplete NV dxlevel 8
             * fragment processing support
             */
            gl_info->supported[NV_REGISTER_COMBINERS] = FALSE;
            gl_info->supported[NV_REGISTER_COMBINERS2] = FALSE;
            gl_info->supported[NV_TEXTURE_SHADER] = FALSE;
            gl_info->supported[NV_TEXTURE_SHADER2] = FALSE;
            gl_info->supported[NV_TEXTURE_SHADER3] = FALSE;
        }

    }
    checkGLcall("extension detection\n");

    /* In some cases the number of texture stages can be larger than the number
     * of samplers. The GF4 for example can use only 2 samplers (no fragment
     * shaders), but 8 texture stages (register combiners). */
    gl_info->max_sampler_stages = max(gl_info->max_fragment_samplers, gl_info->max_texture_stages);

    /* We can only use ORM_FBO when the hardware supports it. */
    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && !gl_info->supported[EXT_FRAMEBUFFER_OBJECT]) {
        WARN_(d3d_caps)("GL_EXT_framebuffer_object not supported, falling back to PBuffer offscreen rendering mode.\n");
        wined3d_settings.offscreen_rendering_mode = ORM_PBUFFER;
    }

    /* MRTs are currently only supported when FBOs are used. */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) {
        gl_info->max_buffers = 1;
    }

    /* Below is a list of Nvidia and ATI GPUs. Both vendors have dozens of different GPUs with roughly the same
     * features. In most cases GPUs from a certain family differ in clockspeeds, the amount of video memory and
     * in case of the latest videocards in the number of pixel/vertex pipelines.
     *
     * A Direct3D device object contains the PCI id (vendor + device) of the videocard which is used for
     * rendering. Various games use this information to get a rough estimation of the features of the card
     * and some might use it for enabling 3d effects only on certain types of videocards. In some cases
     * games might even use it to work around bugs which happen on certain videocards/driver combinations.
     * The problem is that OpenGL only exposes a rendering string containing the name of the videocard and
     * not the PCI id.
     *
     * Various games depend on the PCI id, so somehow we need to provide one. A simple option is to parse
     * the renderer string and translate this to the right PCI id. This is a lot of work because there are more
     * than 200 GPUs just for Nvidia. Various cards share the same renderer string, so the amount of code might
     * be 'small' but there are quite a number of exceptions which would make this a pain to maintain.
     * Another way would be to query the PCI id from the operating system (assuming this is the videocard which
     * is used for rendering which is not always the case). This would work but it is not very portable. Second
     * it would not work well in, let's say, a remote X situation in which the amount of 3d features which can be used
     * is limited.
     *
     * As said most games only use the PCI id to get an indication of the capabilities of the card.
     * It doesn't really matter if the given id is the correct one if we return the id of a card with
     * similar 3d features.
     *
     * The code below checks the OpenGL capabilities of a videocard and matches that to a certain level of
     * Direct3D functionality. Once a card passes the Direct3D9 check, we know that the card (in case of Nvidia)
     * is at least a GeforceFX. To give a better estimate we do a basic check on the renderer string but if that
     * won't pass we return a default card. This way is better than maintaining a full card database as even
     * without a full database we can return a card with similar features. Second the size of the database
     * can be made quite small because when you know what type of 3d functionality a card has, you know to which
     * GPU family the GPU must belong. Because of this you only have to check a small part of the renderer string
     * to distinguishes between different models from that family.
     *
     * The code also selects a default amount of video memory which we will use for an estimation of the amount
     * of free texture memory. In case of real D3D the amount of texture memory includes video memory and system
     * memory (to be specific AGP memory or in case of PCIE TurboCache/HyperMemory). We don't know how much
     * system memory can be addressed by the system but we can make a reasonable estimation about the amount of
     * video memory. If the value is slightly wrong it doesn't matter as we didn't include AGP-like memory which
     * makes the amount of addressable memory higher and second OpenGL isn't that critical it moves to system
     * memory behind our backs if really needed.
     * Note that the amount of video memory can be overruled using a registry setting.
     */
    switch (gl_info->gl_vendor) {
        case VENDOR_NVIDIA:
            /* Both the GeforceFX, 6xxx and 7xxx series support D3D9. The last two types have more
             * shader capabilities, so we use the shader capabilities to distinguish between FX and 6xxx/7xxx.
             */
            if(WINE_D3D9_CAPABLE(gl_info) && (gl_info->vs_nv_version == VS_VERSION_30)) {
                /* Geforce9 - highend */
                if(strstr(gl_info->gl_renderer, "9800")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_9800GT;
                    vidmem = 512;
                }
                /* Geforce9 - midend */
                else if(strstr(gl_info->gl_renderer, "9600")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_9600GT;
                    vidmem = 384; /* The 9600GSO has 384MB, the 9600GT has 512-1024MB */
                }
                /* Geforce8 - highend */
                else if (strstr(gl_info->gl_renderer, "8800")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_8800GTS;
                    vidmem = 320; /* The 8800GTS uses 320MB, a 8800GTX can have 768MB */
                }
                /* Geforce8 - midend mobile */
                else if(strstr(gl_info->gl_renderer, "8600 M")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_8600MGT;
                    vidmem = 512;
                }
                /* Geforce8 - midend */
                else if(strstr(gl_info->gl_renderer, "8600") ||
                        strstr(gl_info->gl_renderer, "8700"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_8600GT;
                    vidmem = 256;
                }
                /* Geforce8 - lowend */
                else if(strstr(gl_info->gl_renderer, "8300") ||
                        strstr(gl_info->gl_renderer, "8400") ||
                        strstr(gl_info->gl_renderer, "8500"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_8300GS;
                    vidmem = 128; /* 128-256MB for a 8300, 256-512MB for a 8400 */
                }
                /* Geforce7 - highend */
                else if(strstr(gl_info->gl_renderer, "7800") ||
                        strstr(gl_info->gl_renderer, "7900") ||
                        strstr(gl_info->gl_renderer, "7950") ||
                        strstr(gl_info->gl_renderer, "Quadro FX 4") ||
                        strstr(gl_info->gl_renderer, "Quadro FX 5"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_7800GT;
                    vidmem = 256; /* A 7800GT uses 256MB while highend 7900 cards can use 512MB */
                }
                /* Geforce7 midend */
                else if(strstr(gl_info->gl_renderer, "7600") ||
                        strstr(gl_info->gl_renderer, "7700")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_7600;
                    vidmem = 256; /* The 7600 uses 256-512MB */
                /* Geforce7 lower medium */
                } else if(strstr(gl_info->gl_renderer, "7400")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_7400;
                    vidmem = 256; /* The 7400 uses 256-512MB */
                }
                /* Geforce7 lowend */
                else if(strstr(gl_info->gl_renderer, "7300")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_7300;
                    vidmem = 256; /* Mac Pros with this card have 256 MB */
                }
                /* Geforce6 highend */
                else if(strstr(gl_info->gl_renderer, "6800"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_6800;
                    vidmem = 128; /* The 6800 uses 128-256MB, the 7600 uses 256-512MB */
                }
                /* Geforce6 - midend */
                else if(strstr(gl_info->gl_renderer, "6600") ||
                        strstr(gl_info->gl_renderer, "6610") ||
                        strstr(gl_info->gl_renderer, "6700"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_6600GT;
                    vidmem = 128; /* A 6600GT has 128-256MB */
                }
                /* Geforce6/7 lowend */
                else {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_6200; /* Geforce 6100/6150/6200/7300/7400/7500 */
                    vidmem = 64; /* */
                }
            } else if(WINE_D3D9_CAPABLE(gl_info)) {
                /* GeforceFX - highend */
                if (strstr(gl_info->gl_renderer, "5800") ||
                    strstr(gl_info->gl_renderer, "5900") ||
                    strstr(gl_info->gl_renderer, "5950") ||
                    strstr(gl_info->gl_renderer, "Quadro FX"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5800;
                    vidmem = 256; /* 5800-5900 cards use 256MB */
                }
                /* GeforceFX - midend */
                else if(strstr(gl_info->gl_renderer, "5600") ||
                        strstr(gl_info->gl_renderer, "5650") ||
                        strstr(gl_info->gl_renderer, "5700") ||
                        strstr(gl_info->gl_renderer, "5750"))
                {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5600;
                    vidmem = 128; /* A 5600 uses 128-256MB */
                }
                /* GeforceFX - lowend */
                else {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5200; /* GeforceFX 5100/5200/5250/5300/5500 */
                    vidmem = 64; /* Normal FX5200 cards use 64-256MB; laptop (non-standard) can have less */
                }
            } else if(WINE_D3D8_CAPABLE(gl_info)) {
                if (strstr(gl_info->gl_renderer, "GeForce4 Ti") || strstr(gl_info->gl_renderer, "Quadro4")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE4_TI4200; /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800, Quadro4 */
                    vidmem = 64; /* Geforce4 Ti cards have 64-128MB */
                }
                else {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE3; /* Geforce3 standard/Ti200/Ti500, Quadro DCC */
                    vidmem = 64; /* Geforce3 cards have 64-128MB */
                }
            } else if(WINE_D3D7_CAPABLE(gl_info)) {
                if (strstr(gl_info->gl_renderer, "GeForce4 MX")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE4_MX; /* MX420/MX440/MX460/MX4000 */
                    vidmem = 64; /* Most Geforce4MX GPUs have at least 64MB of memory, some early models had 32MB but most have 64MB or even 128MB */
                }
                else if(strstr(gl_info->gl_renderer, "GeForce2 MX") || strstr(gl_info->gl_renderer, "Quadro2 MXR")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE2_MX; /* Geforce2 standard/MX100/MX200/MX400, Quadro2 MXR */
                    vidmem = 32; /* Geforce2MX GPUs have 32-64MB of video memory */
                }
                else if(strstr(gl_info->gl_renderer, "GeForce2") || strstr(gl_info->gl_renderer, "Quadro2")) {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE2; /* Geforce2 GTS/Pro/Ti/Ultra, Quadro2 */
                    vidmem = 32; /* Geforce2 GPUs have 32-64MB of video memory */
                }
                else {
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE; /* Geforce 256/DDR, Quadro */
                    vidmem = 32; /* Most Geforce1 cards have 32MB, there are also some rare 16 and 64MB (Dell) models */
                }
            } else {
                if (strstr(gl_info->gl_renderer, "TNT2")) {
                    gl_info->gl_card = CARD_NVIDIA_RIVA_TNT2; /* Riva TNT2 standard/M64/Pro/Ultra */
                    vidmem = 32; /* Most TNT2 boards have 32MB, though there are 16MB boards too */
                }
                else {
                    gl_info->gl_card = CARD_NVIDIA_RIVA_TNT; /* Riva TNT, Vanta */
                    vidmem = 16; /* Most TNT boards have 16MB, some rare models have 8MB */
                }
            }
            break;
        case VENDOR_ATI:
            if(WINE_D3D9_CAPABLE(gl_info)) {
                /* Radeon R6xx HD2900/HD3800 - highend */
                if (strstr(gl_info->gl_renderer, "HD 2900") ||
                    strstr(gl_info->gl_renderer, "HD 3870") ||
                    strstr(gl_info->gl_renderer, "HD 3850"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_HD2900;
                    vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
                }
                /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
                else if (strstr(gl_info->gl_renderer, "HD 2600") ||
                         strstr(gl_info->gl_renderer, "HD 3830") ||
                         strstr(gl_info->gl_renderer, "HD 3690") ||
                         strstr(gl_info->gl_renderer, "HD 3650"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_HD2600;
                    vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
                }
                /* Radeon R6xx HD2300/HD2400/HD3400 - lowend */
                else if (strstr(gl_info->gl_renderer, "HD 2300") ||
                         strstr(gl_info->gl_renderer, "HD 2400") ||
                         strstr(gl_info->gl_renderer, "HD 3470") ||
                         strstr(gl_info->gl_renderer, "HD 3450") ||
                         strstr(gl_info->gl_renderer, "HD 3430"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_HD2300;
                    vidmem = 128; /* HD2300 uses at least 128MB, HD2400 uses 256MB */
                }
                /* Radeon R6xx/R7xx integrated */
                else if (strstr(gl_info->gl_renderer, "HD 3100") ||
                         strstr(gl_info->gl_renderer, "HD 3200") ||
                         strstr(gl_info->gl_renderer, "HD 3300"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_HD3200;
                    vidmem = 128; /* 128MB */
                }
                /* Radeon R5xx */
                else if (strstr(gl_info->gl_renderer, "X1600") ||
                         strstr(gl_info->gl_renderer, "X1650") ||
                         strstr(gl_info->gl_renderer, "X1800") ||
                         strstr(gl_info->gl_renderer, "X1900") ||
                         strstr(gl_info->gl_renderer, "X1950"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_X1600;
                    vidmem = 128; /* X1600 uses 128-256MB, >=X1800 uses 256MB */
                }
                /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300 (lowend R5xx) */
                else if(strstr(gl_info->gl_renderer, "X700") ||
                        strstr(gl_info->gl_renderer, "X800") ||
                        strstr(gl_info->gl_renderer, "X850") ||
                        strstr(gl_info->gl_renderer, "X1300") ||
                        strstr(gl_info->gl_renderer, "X1400") ||
                        strstr(gl_info->gl_renderer, "X1450") ||
                        strstr(gl_info->gl_renderer, "X1550"))
                {
                    gl_info->gl_card = CARD_ATI_RADEON_X700;
                    vidmem = 128; /* x700/x8*0 use 128-256MB, >=x1300 128-512MB */
                }
                /* Radeon R3xx */ 
                else {
                    gl_info->gl_card = CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
                    vidmem = 64; /* Radeon 9500 uses 64MB, higher models use up to 256MB */
                }
            } else if(WINE_D3D8_CAPABLE(gl_info)) {
                gl_info->gl_card = CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
                vidmem = 64; /* 8500/9000 cards use mostly 64MB, though there are 32MB and 128MB models */
            } else if(WINE_D3D7_CAPABLE(gl_info)) {
                gl_info->gl_card = CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
                vidmem = 32; /* There are models with up to 64MB */
            } else {
                gl_info->gl_card = CARD_ATI_RAGE_128PRO;
                vidmem = 16; /* There are 16-32MB models */
            }
            break;
        case VENDOR_INTEL:
            if (strstr(gl_info->gl_renderer, "GMA 950")) {
                /* MacOS calls the card GMA 950, but everywhere else the PCI ID is named 945GM */
                gl_info->gl_card = CARD_INTEL_I945GM;
                vidmem = 64;
            } else if (strstr(gl_info->gl_renderer, "915GM")) {
                gl_info->gl_card = CARD_INTEL_I915GM;
            } else if (strstr(gl_info->gl_renderer, "915G")) {
                gl_info->gl_card = CARD_INTEL_I915G;
            } else if (strstr(gl_info->gl_renderer, "865G")) {
                gl_info->gl_card = CARD_INTEL_I865G;
            } else if (strstr(gl_info->gl_renderer, "855G")) {
                gl_info->gl_card = CARD_INTEL_I855G;
            } else if (strstr(gl_info->gl_renderer, "830G")) {
                gl_info->gl_card = CARD_INTEL_I830G;
            } else {
                gl_info->gl_card = CARD_INTEL_I915G;
            }
            break;
        case VENDOR_MESA:
        case VENDOR_WINE:
        default:
            /* Default to generic Nvidia hardware based on the supported OpenGL extensions. The choice 
             * for Nvidia was because the hardware and drivers they make are of good quality. This makes
             * them a good generic choice.
             */
            gl_info->gl_vendor = VENDOR_NVIDIA;
            if(WINE_D3D9_CAPABLE(gl_info))
                gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5600;
            else if(WINE_D3D8_CAPABLE(gl_info))
                gl_info->gl_card = CARD_NVIDIA_GEFORCE3;
            else if(WINE_D3D7_CAPABLE(gl_info))
                gl_info->gl_card = CARD_NVIDIA_GEFORCE;
            else if(WINE_D3D6_CAPABLE(gl_info))
                gl_info->gl_card = CARD_NVIDIA_RIVA_TNT;
            else
                gl_info->gl_card = CARD_NVIDIA_RIVA_128;
    }
    TRACE_(d3d_caps)("FOUND (fake) card: 0x%x (vendor id), 0x%x (device id)\n", gl_info->gl_vendor, gl_info->gl_card);

    /* If we have an estimate use it, else default to 64MB;  */
    if(vidmem)
        gl_info->vidmem = vidmem*1024*1024; /* convert from MBs to bytes */
    else
        gl_info->vidmem = WINE_DEFAULT_VIDMEM;

    /* Load all the lookup tables
    TODO: It may be a good idea to make minLookup and maxLookup const and populate them in wined3d_private.h where they are declared */
    minLookup[WINELOOKUP_WARPPARAM] = WINED3DTADDRESS_WRAP;
    maxLookup[WINELOOKUP_WARPPARAM] = WINED3DTADDRESS_MIRRORONCE;

    for (i = 0; i < MAX_LOOKUPS; i++) {
        stateLookup[i] = HeapAlloc(GetProcessHeap(), 0, sizeof(*stateLookup[i]) * (1 + maxLookup[i] - minLookup[i]) );
    }

    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_WRAP   - minLookup[WINELOOKUP_WARPPARAM]] = GL_REPEAT;
    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_CLAMP  - minLookup[WINELOOKUP_WARPPARAM]] = GL_CLAMP_TO_EDGE;
    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_BORDER - minLookup[WINELOOKUP_WARPPARAM]] =
             gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_BORDER - minLookup[WINELOOKUP_WARPPARAM]] =
             gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_MIRROR - minLookup[WINELOOKUP_WARPPARAM]] =
             gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    stateLookup[WINELOOKUP_WARPPARAM][WINED3DTADDRESS_MIRRORONCE - minLookup[WINELOOKUP_WARPPARAM]] =
             gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] ? GL_MIRROR_CLAMP_TO_EDGE_ATI : GL_REPEAT;

    magLookup[WINED3DTEXF_NONE        - WINED3DTEXF_NONE]  = GL_NEAREST;
    magLookup[WINED3DTEXF_POINT       - WINED3DTEXF_NONE] = GL_NEAREST;
    magLookup[WINED3DTEXF_LINEAR      - WINED3DTEXF_NONE] = GL_LINEAR;
    magLookup[WINED3DTEXF_ANISOTROPIC - WINED3DTEXF_NONE] =
             gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC] ? GL_LINEAR : GL_NEAREST;


    minMipLookup[WINED3DTEXF_NONE][WINED3DTEXF_NONE]     = GL_LINEAR;
    minMipLookup[WINED3DTEXF_NONE][WINED3DTEXF_POINT]    = GL_LINEAR;
    minMipLookup[WINED3DTEXF_NONE][WINED3DTEXF_LINEAR]   = GL_LINEAR;
    minMipLookup[WINED3DTEXF_POINT][WINED3DTEXF_NONE]    = GL_NEAREST;
    minMipLookup[WINED3DTEXF_POINT][WINED3DTEXF_POINT]   = GL_NEAREST_MIPMAP_NEAREST;
    minMipLookup[WINED3DTEXF_POINT][WINED3DTEXF_LINEAR]  = GL_NEAREST_MIPMAP_LINEAR;
    minMipLookup[WINED3DTEXF_LINEAR][WINED3DTEXF_NONE]   = GL_LINEAR;
    minMipLookup[WINED3DTEXF_LINEAR][WINED3DTEXF_POINT]  = GL_LINEAR_MIPMAP_NEAREST;
    minMipLookup[WINED3DTEXF_LINEAR][WINED3DTEXF_LINEAR] = GL_LINEAR_MIPMAP_LINEAR;
    minMipLookup[WINED3DTEXF_ANISOTROPIC][WINED3DTEXF_NONE]   = gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC] ?
    GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    minMipLookup[WINED3DTEXF_ANISOTROPIC][WINED3DTEXF_POINT]  = gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC] ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;
    minMipLookup[WINED3DTEXF_ANISOTROPIC][WINED3DTEXF_LINEAR] = gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC] ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

/* TODO: config lookups */

    /* Make sure there's an active HDC else the WGL extensions will fail */
    hdc = pwglGetCurrentDC();
    if (hdc) {
        WGL_Extensions = GL_EXTCALL(wglGetExtensionsStringARB(hdc));
        TRACE_(d3d_caps)("WGL_Extensions reported:\n");

        if (NULL == WGL_Extensions) {
            ERR("   WGL_Extensions returns NULL\n");
        } else {
            while (*WGL_Extensions != 0x00) {
                const char *Start;
                char ThisExtn[256];
                size_t len;

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
                TRACE_(d3d_caps)("- %s\n", ThisExtn);

                if (!strcmp(ThisExtn, "WGL_ARB_pbuffer")) {
                    gl_info->supported[WGL_ARB_PBUFFER] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_ARB_pbuffer support\n");
                }
                if (!strcmp(ThisExtn, "WGL_WINE_pixel_format_passthrough")) {
                    gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_WINE_pixel_format_passthrough support\n");
                }
            }
        }
    }
    LEAVE_GL();

    return return_value;
}
#undef GLINFO_LOCATION

/**********************************************************
 * IWineD3D implementation follows
 **********************************************************/

static UINT     WINAPI IWineD3DImpl_GetAdapterCount (IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    TRACE_(d3d_caps)("(%p): Reporting %d adapters\n", This, numAdapters);
    return numAdapters;
}

static HRESULT  WINAPI IWineD3DImpl_RegisterSoftwareDevice(IWineD3D *iface, void* pInitializeFunction) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    FIXME("(%p)->(%p): stub\n", This, pInitializeFunction);
    return WINED3D_OK;
}

static HMONITOR WINAPI IWineD3DImpl_GetAdapterMonitor(IWineD3D *iface, UINT Adapter) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    TRACE_(d3d_caps)("(%p)->(%d)\n", This, Adapter);

    if (Adapter >= IWineD3DImpl_GetAdapterCount(iface)) {
        return NULL;
    }

    return MonitorFromPoint(Adapters[Adapter].monitorPoint, MONITOR_DEFAULTTOPRIMARY);
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
        int i = 0;
        int j = 0;

        if (!DEBUG_SINGLE_MODE) {
            DEVMODEW DevModeW;

            ZeroMemory(&DevModeW, sizeof(DevModeW));
            DevModeW.dmSize = sizeof(DevModeW);
            while (EnumDisplaySettingsExW(NULL, j, &DevModeW, 0)) {
                j++;
                switch (Format)
                {
                    case WINED3DFMT_UNKNOWN:
                        /* This is for D3D8, do not enumerate P8 here */
                        if (DevModeW.dmBitsPerPel == 32 ||
                            DevModeW.dmBitsPerPel == 16) i++;
                        break;
                    case WINED3DFMT_X8R8G8B8:
                        if (DevModeW.dmBitsPerPel == 32) i++;
                        break;
                    case WINED3DFMT_R5G6B5:
                        if (DevModeW.dmBitsPerPel == 16) i++;
                        break;
                    case WINED3DFMT_P8:
                        if (DevModeW.dmBitsPerPel == 8) i++;
                        break;
                    default:
                        /* Skip other modes as they do not match the requested format */
                        break;
                }
            }
        } else {
            i = 1;
            j = 1;
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
    if (Adapter == 0 && !DEBUG_SINGLE_MODE) { /* Display */
        DEVMODEW DevModeW;
        int ModeIdx = 0;
        int i = 0;
        int j = 0;

        ZeroMemory(&DevModeW, sizeof(DevModeW));
        DevModeW.dmSize = sizeof(DevModeW);

        /* If we are filtering to a specific format (D3D9), then need to skip
           all unrelated modes, but if mode is irrelevant (D3D8), then we can
           just count through the ones with valid bit depths */
        while ((i<=Mode) && EnumDisplaySettingsExW(NULL, j++, &DevModeW, 0)) {
            switch (Format)
            {
                case WINED3DFMT_UNKNOWN:
                    /* This is D3D8. Do not enumerate P8 here */
                    if (DevModeW.dmBitsPerPel == 32 ||
                        DevModeW.dmBitsPerPel == 16) i++;
                    break;
                case WINED3DFMT_X8R8G8B8:
                    if (DevModeW.dmBitsPerPel == 32) i++;
                    break;
                case WINED3DFMT_R5G6B5:
                    if (DevModeW.dmBitsPerPel == 16) i++;
                    break;
                case WINED3DFMT_P8:
                    if (DevModeW.dmBitsPerPel == 8) i++;
                    break;
                default:
                    /* Modes that don't match what we support can get an early-out */
                    TRACE_(d3d_caps)("Searching for %s, returning D3DERR_INVALIDCALL\n", debug_d3dformat(Format));
                    return WINED3DERR_INVALIDCALL;
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
            pMode->RefreshRate  = WINED3DADAPTER_DEFAULT;
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

    } else if (DEBUG_SINGLE_MODE) {
        /* Return one setting of the format requested */
        if (Mode > 0) return WINED3DERR_INVALIDCALL;
        pMode->Width        = 800;
        pMode->Height       = 600;
        pMode->RefreshRate  = 60;
        pMode->Format       = (Format == WINED3DFMT_UNKNOWN) ? WINED3DFMT_X8R8G8B8 : Format;
    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DImpl_GetAdapterDisplayMode(IWineD3D *iface, UINT Adapter, WINED3DDISPLAYMODE* pMode) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter: %d, pMode: %p)\n", This, Adapter, pMode);

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
        pMode->RefreshRate  = WINED3DADAPTER_DEFAULT;
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

    TRACE_(d3d_caps)("(%p}->(Adapter: %d, Flags: %x, pId=%p)\n", This, Adapter, Flags, pIdentifier);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    /* Return the information requested */
    TRACE_(d3d_caps)("device/Vendor Name and Version detection using FillGLCaps\n");
    strcpy(pIdentifier->Driver, Adapters[Adapter].driver);
    strcpy(pIdentifier->Description, Adapters[Adapter].description);

    /* Note dx8 doesn't supply a DeviceName */
    if (NULL != pIdentifier->DeviceName) strcpy(pIdentifier->DeviceName, "\\\\.\\DISPLAY"); /* FIXME: May depend on desktop? */
    pIdentifier->DriverVersion->u.HighPart = Adapters[Adapter].gl_info.driver_version_hipart;
    pIdentifier->DriverVersion->u.LowPart = Adapters[Adapter].gl_info.driver_version;
    *(pIdentifier->VendorId) = Adapters[Adapter].gl_info.gl_vendor;
    *(pIdentifier->DeviceId) = Adapters[Adapter].gl_info.gl_card;
    *(pIdentifier->SubSysId) = 0;
    *(pIdentifier->Revision) = 0;
    *pIdentifier->DeviceIdentifier = IID_D3DDEVICE_D3DUID;

    if (Flags & WINED3DENUM_NO_WHQL_LEVEL) {
        *(pIdentifier->WHQLLevel) = 0;
    } else {
        *(pIdentifier->WHQLLevel) = 1;
    }

    return WINED3D_OK;
}

static BOOL IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(const WineD3D_PixelFormat *cfg, WINED3DFORMAT Format) {
    short redSize, greenSize, blueSize, alphaSize, colorBits;

    if(!cfg)
        return FALSE;

    if(cfg->iPixelType == WGL_TYPE_RGBA_ARB) { /* Integer RGBA formats */
        if(!getColorBits(Format, &redSize, &greenSize, &blueSize, &alphaSize, &colorBits)) {
            ERR("Unable to check compatibility for Format=%s\n", debug_d3dformat(Format));
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
        if(Format == WINED3DFMT_R16F)
            return (cfg->redSize == 16 && cfg->greenSize == 0 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if(Format == WINED3DFMT_G16R16F)
            return (cfg->redSize == 16 && cfg->greenSize == 16 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if(Format == WINED3DFMT_A16B16G16R16F)
            return (cfg->redSize == 16 && cfg->greenSize == 16 && cfg->blueSize == 16 && cfg->alphaSize == 16);
        if(Format == WINED3DFMT_R32F)
            return (cfg->redSize == 32 && cfg->greenSize == 0 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if(Format == WINED3DFMT_G32R32F)
            return (cfg->redSize == 32 && cfg->greenSize == 32 && cfg->blueSize == 0 && cfg->alphaSize == 0);
        if(Format == WINED3DFMT_A32B32G32R32F)
            return (cfg->redSize == 32 && cfg->greenSize == 32 && cfg->blueSize == 32 && cfg->alphaSize == 32);
    } else {
        /* Probably a color index mode */
        return FALSE;
    }

    return FALSE;
}

static BOOL IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(const WineD3D_PixelFormat *cfg, WINED3DFORMAT Format) {
    short depthSize, stencilSize;
    BOOL lockable = FALSE;

    if(!cfg)
        return FALSE;

    if(!getDepthStencilBits(Format, &depthSize, &stencilSize)) {
        ERR("Unable to check compatibility for Format=%s\n", debug_d3dformat(Format));
        return FALSE;
    }

    if((Format == WINED3DFMT_D16_LOCKABLE) || (Format == WINED3DFMT_D32F_LOCKABLE))
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
    WineD3D_PixelFormat *cfgs;
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

    cfgs = Adapters[Adapter].cfgs;
    nCfgs = Adapters[Adapter].nCfgs;
    for (it = 0; it < nCfgs; ++it) {
        if (IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&cfgs[it], RenderTargetFormat)) {
            if (IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&cfgs[it], DepthStencilFormat)) {
                TRACE_(d3d_caps)("(%p) : Formats matched\n", This);
                return WINED3D_OK;
            }
        }
    }
    WARN_(d3d_caps)("unsupported format pair: %s and %s\n", debug_d3dformat(RenderTargetFormat), debug_d3dformat(DepthStencilFormat));

    return WINED3DERR_NOTAVAILABLE;
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceMultiSampleType(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, 
                                                       WINED3DFORMAT SurfaceFormat,
                                                       BOOL Windowed, WINED3DMULTISAMPLE_TYPE MultiSampleType, DWORD*   pQualityLevels) {

    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    const GlPixelFormatDesc *glDesc;
    const StaticPixelFormatDesc *desc;

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

    if (WINED3DMULTISAMPLE_NONE == MultiSampleType) return WINED3D_OK;

    /* By default multisampling is disabled right now as it causes issues
     * on some Nvidia driver versions and it doesn't work well in combination
     * with FBOs yet. */
    if(!wined3d_settings.allow_multisampling)
        return WINED3DERR_NOTAVAILABLE;

    desc = getFormatDescEntry(SurfaceFormat, &Adapters[Adapter].gl_info, &glDesc);
    if(!desc || !glDesc) {
        return WINED3DERR_INVALIDCALL;
    }

    if(glDesc->Flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)) {
        int i, nCfgs;
        WineD3D_PixelFormat *cfgs;

        cfgs = Adapters[Adapter].cfgs;
        nCfgs = Adapters[Adapter].nCfgs;
        for(i=0; i<nCfgs; i++) {
            if(cfgs[i].numSamples != MultiSampleType)
                continue;

            if(!IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&cfgs[i], SurfaceFormat))
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
        WineD3D_PixelFormat *cfgs;

        if(!getColorBits(SurfaceFormat, &redSize, &greenSize, &blueSize, &alphaSize, &colorBits)) {
            ERR("Unable to color bits for format %#x, can't check multisampling capability!\n", SurfaceFormat);
            return WINED3DERR_NOTAVAILABLE;
        }

        cfgs = Adapters[Adapter].cfgs;
        nCfgs = Adapters[Adapter].nCfgs;
        for(i=0; i<nCfgs; i++) {
            if(cfgs[i].numSamples != MultiSampleType)
                continue;
            if(cfgs[i].redSize != redSize)
                continue;
            if(cfgs[i].greenSize != greenSize)
                continue;
            if(cfgs[i].blueSize != blueSize)
                continue;
            if(cfgs[i].alphaSize != alphaSize)
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
                                            WINED3DFORMAT DisplayFormat, WINED3DFORMAT BackBufferFormat, BOOL Windowed) {

    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    HRESULT hr = WINED3DERR_NOTAVAILABLE;
    UINT nmodes;

    TRACE_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, CheckType:(%x,%s), DispFmt:(%x,%s), BackBuf:(%x,%s), Win?%d): stub\n",
          This,
          Adapter,
          DeviceType, debug_d3ddevicetype(DeviceType),
          DisplayFormat, debug_d3dformat(DisplayFormat),
          BackBufferFormat, debug_d3dformat(BackBufferFormat),
          Windowed);

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
    if(!((DisplayFormat == WINED3DFMT_R5G6B5) ||
         (DisplayFormat == WINED3DFMT_X1R5G5B5) ||
         (DisplayFormat == WINED3DFMT_X8R8G8B8) ||
         (DisplayFormat == WINED3DFMT_A2R10G10B10)))
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
    if( (DisplayFormat == WINED3DFMT_R5G6B5) && (BackBufferFormat != WINED3DFMT_R5G6B5) ) {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode X1R5G5B5 can only be mixed with backbuffer format *1R5G5B5 */
    if( (DisplayFormat == WINED3DFMT_X1R5G5B5) && !((BackBufferFormat == WINED3DFMT_X1R5G5B5) || (BackBufferFormat == WINED3DFMT_A1R5G5B5)) ) {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* In FULLSCREEN mode X8R8G8B8 can only be mixed with backbuffer format *8R8G8B8 */
    if( (DisplayFormat == WINED3DFMT_X8R8G8B8) && !((BackBufferFormat == WINED3DFMT_X8R8G8B8) || (BackBufferFormat == WINED3DFMT_A8R8G8B8)) ) {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* A2R10G10B10 is only allowed in fullscreen mode and it can only be mixed with backbuffer format A2R10G10B10 */
    if( (DisplayFormat == WINED3DFMT_A2R10G10B10) && ((BackBufferFormat != WINED3DFMT_A2R10G10B10) || Windowed)) {
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Use CheckDeviceFormat to see if the BackBufferFormat is usable with the given DisplayFormat */
    hr = IWineD3DImpl_CheckDeviceFormat(iface, Adapter, DeviceType, DisplayFormat, WINED3DUSAGE_RENDERTARGET, WINED3DRTYPE_SURFACE, BackBufferFormat, SURFACE_OPENGL);
    if(FAILED(hr))
        TRACE_(d3d_caps)("Unsupported display/backbuffer format combination %s/%s\n", debug_d3dformat(DisplayFormat), debug_d3dformat(BackBufferFormat));

    return hr;
}


#define GLINFO_LOCATION Adapters[Adapter].gl_info
/* Check if we support bumpmapping for a format */
static BOOL CheckBumpMapCapability(UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DFORMAT CheckFormat)
{
    const struct fragment_pipeline *fp;
    const GlPixelFormatDesc *glDesc;

    switch(CheckFormat) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
        case WINED3DFMT_L6V5U5:
        case WINED3DFMT_X8L8V8U8:
        case WINED3DFMT_Q8W8V8U8:
            getFormatDescEntry(CheckFormat, &GLINFO_LOCATION, &glDesc);
            if(glDesc->conversion_group == WINED3DFMT_UNKNOWN) {
                /* We have a GL extension giving native support */
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }

            /* No native support: Ask the fixed function pipeline implementation if it
             * can deal with the conversion
             */
            fp = select_fragment_implementation(Adapter, DeviceType);
            if(fp->conv_supported(CheckFormat)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            } else {
                TRACE_(d3d_caps)("[FAILED]\n");
                return FALSE;
            }

        default:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
    }
}

/* Check if the given DisplayFormat + DepthStencilFormat combination is valid for the Adapter */
static BOOL CheckDepthStencilCapability(UINT Adapter, WINED3DFORMAT DisplayFormat, WINED3DFORMAT DepthStencilFormat)
{
    int it=0;
    WineD3D_PixelFormat *cfgs = Adapters[Adapter].cfgs;
    const GlPixelFormatDesc *glDesc;
    const StaticPixelFormatDesc *desc = getFormatDescEntry(DepthStencilFormat, &GLINFO_LOCATION, &glDesc);

    /* Fail if we weren't able to get a description of the format */
    if(!desc || !glDesc)
        return FALSE;

    /* Only allow depth/stencil formats */
    if(!(glDesc->Flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        return FALSE;

    /* Walk through all WGL pixel formats to find a match */
    cfgs = Adapters[Adapter].cfgs;
    for (it = 0; it < Adapters[Adapter].nCfgs; ++it) {
        if (IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&cfgs[it], DisplayFormat)) {
            if (IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(&cfgs[it], DepthStencilFormat)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static BOOL CheckFilterCapability(UINT Adapter, WINED3DFORMAT CheckFormat)
{
    const GlPixelFormatDesc *glDesc;
    const StaticPixelFormatDesc *desc = getFormatDescEntry(CheckFormat, &GLINFO_LOCATION, &glDesc);

    /* Fail if we weren't able to get a description of the format */
    if(!desc || !glDesc)
        return FALSE;

    /* The flags entry of a format contains the filtering capability */
    if(glDesc->Flags & WINED3DFMT_FLAG_FILTERING)
        return TRUE;

    return FALSE;
}

/* Check the render target capabilities of a format */
static BOOL CheckRenderTargetCapability(WINED3DFORMAT AdapterFormat, WINED3DFORMAT CheckFormat)
{
    UINT Adapter = 0;
    const GlPixelFormatDesc *glDesc;
    const StaticPixelFormatDesc *desc = getFormatDescEntry(CheckFormat, &GLINFO_LOCATION, &glDesc);

    /* Fail if we weren't able to get a description of the format */
    if(!desc || !glDesc)
        return FALSE;

    /* Filter out non-RT formats */
    if(!(glDesc->Flags & WINED3DFMT_FLAG_RENDERTARGET))
        return FALSE;

    if(wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER) {
        WineD3D_PixelFormat *cfgs = Adapters[Adapter].cfgs;
        int it;
        short AdapterRed, AdapterGreen, AdapterBlue, AdapterAlpha, AdapterTotalSize;
        short CheckRed, CheckGreen, CheckBlue, CheckAlpha, CheckTotalSize;

        getColorBits(AdapterFormat, &AdapterRed, &AdapterGreen, &AdapterBlue, &AdapterAlpha, &AdapterTotalSize);
        getColorBits(CheckFormat, &CheckRed, &CheckGreen, &CheckBlue, &CheckAlpha, &CheckTotalSize);

        /* In backbuffer mode the front and backbuffer share the same WGL pixelformat.
         * The format must match in RGB, alpha is allowed to be different. (Only the backbuffer can have alpha) */
        if(!((AdapterRed == CheckRed) && (AdapterGreen == CheckGreen) && (AdapterBlue == CheckBlue))) {
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
        }

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        for (it = 0; it < Adapters[Adapter].nCfgs; ++it) {
            if (cfgs[it].windowDrawable && IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&cfgs[it], CheckFormat)) {
                TRACE_(d3d_caps)("iPixelFormat=%d is compatible with CheckFormat=%s\n", cfgs[it].iPixelFormat, debug_d3dformat(CheckFormat));
                return TRUE;
            }
        }
    } else if(wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER) {
        /* We can probably use this function in FBO mode too on some drivers to get some basic indication of the capabilities. */
        WineD3D_PixelFormat *cfgs = Adapters[Adapter].cfgs;
        int it;

        /* Check if there is a WGL pixel format matching the requirements, the pixel format should also be usable with pbuffers */
        for (it = 0; it < Adapters[Adapter].nCfgs; ++it) {
            if (cfgs[it].pbufferDrawable && IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&cfgs[it], CheckFormat)) {
                TRACE_(d3d_caps)("iPixelFormat=%d is compatible with CheckFormat=%s\n", cfgs[it].iPixelFormat, debug_d3dformat(CheckFormat));
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

static BOOL CheckSrgbReadCapability(UINT Adapter, WINED3DFORMAT CheckFormat)
{
    /* Check for supported sRGB formats (Texture loading and framebuffer) */
    if(!GL_SUPPORT(EXT_TEXTURE_SRGB)) {
        TRACE_(d3d_caps)("[FAILED] GL_EXT_texture_sRGB not supported\n");
        return FALSE;
    }

    switch (CheckFormat) {
        case WINED3DFMT_A8R8G8B8:
        case WINED3DFMT_X8R8G8B8:
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_L8:
        case WINED3DFMT_A8L8:
        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        default:
            TRACE_(d3d_caps)("[FAILED] Gamma texture format %s not supported.\n", debug_d3dformat(CheckFormat));
            return FALSE;
    }
    return FALSE;
}

static BOOL CheckSrgbWriteCapability(UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DFORMAT CheckFormat)
{
    /* Only offer SRGB writing on X8R8G8B8/A8R8G8B8 when we use ARB or GLSL shaders as we are
     * doing the color fixup in shaders.
     * Note Windows drivers (at least on the Geforce 8800) also offer this on R5G6B5. */
    if((CheckFormat == WINED3DFMT_X8R8G8B8) || (CheckFormat == WINED3DFMT_A8R8G8B8)) {
        int vs_selected_mode;
        int ps_selected_mode;
        select_shader_mode(&GLINFO_LOCATION, DeviceType, &ps_selected_mode, &vs_selected_mode);

        if((ps_selected_mode == SHADER_ARB) || (ps_selected_mode == SHADER_GLSL)) {
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;
        }
    }

    TRACE_(d3d_caps)("[FAILED] - no SRGB writing support on format=%s\n", debug_d3dformat(CheckFormat));
    return FALSE;
}

/* Check if a format support blending in combination with pixel shaders */
static BOOL CheckPostPixelShaderBlendingCapability(UINT Adapter, WINED3DFORMAT CheckFormat)
{
    const GlPixelFormatDesc *glDesc;
    const StaticPixelFormatDesc *desc = getFormatDescEntry(CheckFormat, &GLINFO_LOCATION, &glDesc);

    /* Fail if we weren't able to get a description of the format */
    if(!desc || !glDesc)
        return FALSE;

    /* The flags entry of a format contains the post pixel shader blending capability */
    if(glDesc->Flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING)
        return TRUE;

    return FALSE;
}

static BOOL CheckWrapAndMipCapability(UINT Adapter, WINED3DFORMAT CheckFormat) {
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
static BOOL CheckTextureCapability(UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DFORMAT CheckFormat)
{
    const shader_backend_t *shader_backend;
    const struct fragment_pipeline *fp;
    const GlPixelFormatDesc *glDesc;

    switch (CheckFormat) {

        /*****
         *  supported: RGB(A) formats
         */
        case WINED3DFMT_R8G8B8: /* Enable for dx7, blacklisted for 8 and 9 above */
        case WINED3DFMT_A8R8G8B8:
        case WINED3DFMT_X8R8G8B8:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_A8:
        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_A8B8G8R8:
        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_A2B10G10R10:
        case WINED3DFMT_G16R16:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        case WINED3DFMT_R3G3B2:
            TRACE_(d3d_caps)("[FAILED] - Not supported on Windows\n");
            return FALSE;

        /*****
         *  supported: Palettized
         */
        case WINED3DFMT_P8:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;
        /* No Windows driver offers A8P8, so don't offer it either */
        case WINED3DFMT_A8P8:
            return FALSE;

        /*****
         *  Supported: (Alpha)-Luminance
         */
        case WINED3DFMT_L8:
        case WINED3DFMT_A8L8:
        case WINED3DFMT_L16:
            TRACE_(d3d_caps)("[OK]\n");
            return TRUE;

        /* Not supported on Windows, thus disabled */
        case WINED3DFMT_A4L4:
            TRACE_(d3d_caps)("[FAILED] - not supported on windows\n");
            return FALSE;

        /*****
         *  Supported: Depth/Stencil formats
         */
        case WINED3DFMT_D16_LOCKABLE:
        case WINED3DFMT_D16:
        case WINED3DFMT_D15S1:
        case WINED3DFMT_D24X8:
        case WINED3DFMT_D24X4S4:
        case WINED3DFMT_D24S8:
        case WINED3DFMT_D24FS8:
        case WINED3DFMT_D32:
        case WINED3DFMT_D32F_LOCKABLE:
            return TRUE;

        /*****
         *  Not supported everywhere(depends on GL_ATI_envmap_bumpmap or
         *  GL_NV_texture_shader). Emulated by shaders
         */
        case WINED3DFMT_V8U8:
        case WINED3DFMT_X8L8V8U8:
        case WINED3DFMT_L6V5U5:
        case WINED3DFMT_Q8W8V8U8:
        case WINED3DFMT_V16U16:
        case WINED3DFMT_W11V11U10:
            getFormatDescEntry(CheckFormat, &GLINFO_LOCATION, &glDesc);
            if(glDesc->conversion_group == WINED3DFMT_UNKNOWN) {
                /* We have a GL extension giving native support */
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }

            /* No native support: Ask the fixed function pipeline implementation if it
             * can deal with the conversion
             */
            shader_backend = select_shader_backend(Adapter, DeviceType);
            if(shader_backend->shader_conv_supported(CheckFormat)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            } else {
                TRACE_(d3d_caps)("[FAILED]\n");
                return FALSE;
            }

        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
            if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;


        /*****
         *  Odd formats - not supported
         */
        case WINED3DFMT_VERTEXDATA:
        case WINED3DFMT_INDEX16:
        case WINED3DFMT_INDEX32:
        case WINED3DFMT_Q16W16V16U16:
        case WINED3DFMT_A2W10V10U10:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

        /*****
         *  WINED3DFMT_CxV8U8: Not supported right now
         */
        case WINED3DFMT_CxV8U8:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

        /* YUV formats */
        case WINED3DFMT_UYVY:
        case WINED3DFMT_YUY2:
            if(GL_SUPPORT(APPLE_YCBCR_422)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;
        case WINED3DFMT_YV12:
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

            /* Not supported */
        case WINED3DFMT_A16B16G16R16:
        case WINED3DFMT_A8R3G3B2:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return FALSE;

            /* Floating point formats */
        case WINED3DFMT_R16F:
        case WINED3DFMT_A16B16G16R16F:
            if(GL_SUPPORT(ARB_TEXTURE_FLOAT) && GL_SUPPORT(ARB_HALF_FLOAT_PIXEL)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_R32F:
        case WINED3DFMT_A32B32G32R32F:
            if (GL_SUPPORT(ARB_TEXTURE_FLOAT)) {
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            }
            TRACE_(d3d_caps)("[FAILED]\n");
            return FALSE;

        case WINED3DFMT_G16R16F:
        case WINED3DFMT_G32R32F:
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
        case WINEMAKEFOURCC('I','N','S','T'):
            TRACE("ATI Instancing check hack\n");
            if(GL_SUPPORT(ARB_VERTEX_PROGRAM) || GL_SUPPORT(ARB_VERTEX_SHADER)) {
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
            if(GL_SUPPORT(ATI_TEXTURE_COMPRESSION_3DC) || GL_SUPPORT(EXT_TEXTURE_COMPRESSION_RGTC)) {
                shader_backend = select_shader_backend(Adapter, DeviceType);
                fp = select_fragment_implementation(Adapter, DeviceType);
                if(shader_backend->shader_conv_supported(CheckFormat) &&
                   fp->conv_supported(CheckFormat)) {
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
            ERR("Unhandled format=%s\n", debug_d3dformat(CheckFormat));
            break;
    }
    return FALSE;
}

static BOOL CheckSurfaceCapability(UINT Adapter, WINED3DFORMAT AdapterFormat, WINED3DDEVTYPE DeviceType, WINED3DFORMAT CheckFormat, WINED3DSURFTYPE SurfaceType) {
    const struct blit_shader *blitter;

    if(SurfaceType == SURFACE_GDI) {
        switch(CheckFormat) {
            case WINED3DFMT_R8G8B8:
            case WINED3DFMT_A8R8G8B8:
            case WINED3DFMT_X8R8G8B8:
            case WINED3DFMT_R5G6B5:
            case WINED3DFMT_X1R5G5B5:
            case WINED3DFMT_A1R5G5B5:
            case WINED3DFMT_A4R4G4B4:
            case WINED3DFMT_R3G3B2:
            case WINED3DFMT_A8:
            case WINED3DFMT_A8R3G3B2:
            case WINED3DFMT_X4R4G4B4:
            case WINED3DFMT_A2B10G10R10:
            case WINED3DFMT_A8B8G8R8:
            case WINED3DFMT_X8B8G8R8:
            case WINED3DFMT_G16R16:
            case WINED3DFMT_A2R10G10B10:
            case WINED3DFMT_A16B16G16R16:
            case WINED3DFMT_P8:
                TRACE_(d3d_caps)("[OK]\n");
                return TRUE;
            default:
                TRACE_(d3d_caps)("[FAILED] - not available on GDI surfaces\n");
                return FALSE;
        }
    }

    /* All format that are supported for textures are supported for surfaces as well */
    if(CheckTextureCapability(Adapter, DeviceType, CheckFormat)) return TRUE;
    /* All depth stencil formats are supported on surfaces */
    if(CheckDepthStencilCapability(Adapter, AdapterFormat, CheckFormat)) return TRUE;

    /* If opengl can't process the format natively, the blitter may be able to convert it */
    blitter = select_blit_implementation(Adapter, DeviceType);
    if(blitter->conv_supported(CheckFormat)) {
        TRACE_(d3d_caps)("[OK]\n");
        return TRUE;
    }

    /* Reject other formats */
    TRACE_(d3d_caps)("[FAILED]\n");
    return FALSE;
}

static BOOL CheckVertexTextureCapability(UINT Adapter, WINED3DFORMAT CheckFormat)
{
    if (!GL_LIMITS(vertex_samplers)) {
        TRACE_(d3d_caps)("[FAILED]\n");
        return FALSE;
    }

    switch (CheckFormat) {
        case WINED3DFMT_A32B32G32R32F:
            if (!GL_SUPPORT(ARB_TEXTURE_FLOAT)) {
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
        WINED3DSURFTYPE SurfaceType) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
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
        if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
            /* Check if the texture format is around */
            if(CheckTextureCapability(Adapter, DeviceType, CheckFormat)) {
                if(Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
                    /* Check for automatic mipmap generation support */
                    if(GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
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
                    if(CheckRenderTargetCapability(AdapterFormat, CheckFormat)) {
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
                    if(CheckFilterCapability(Adapter, CheckFormat)) {
                        UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_POSTPIXELSHADER_BLENDING support */
                if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                    if(CheckPostPixelShaderBlendingCapability(Adapter, CheckFormat)) {
                        UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_SRGBREAD support */
                if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                    if(CheckSrgbReadCapability(Adapter, CheckFormat)) {
                        UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_SRGBWRITE support */
                if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                    if(CheckSrgbWriteCapability(Adapter, DeviceType, CheckFormat)) {
                        UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_VERTEXTEXTURE support */
                if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                    if(CheckVertexTextureCapability(Adapter, CheckFormat)) {
                        UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                    } else {
                        TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                        return WINED3DERR_NOTAVAILABLE;
                    }
                }

                /* Check QUERY_WRAPANDMIP support */
                if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                    if(CheckWrapAndMipCapability(Adapter, CheckFormat)) {
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

        if(CheckSurfaceCapability(Adapter, AdapterFormat, DeviceType, CheckFormat, SurfaceType)) {
            if(Usage & WINED3DUSAGE_DEPTHSTENCIL) {
                if(CheckDepthStencilCapability(Adapter, AdapterFormat, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_DEPTHSTENCIL;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No depthstencil support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            if(Usage & WINED3DUSAGE_RENDERTARGET) {
                if(CheckRenderTargetCapability(AdapterFormat, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_RENDERTARGET;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No rendertarget support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if(CheckPostPixelShaderBlendingCapability(Adapter, CheckFormat)) {
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
        if(CheckTextureCapability(Adapter, DeviceType, CheckFormat)) {
            if(Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
                /* Check for automatic mipmap generation support */
                if(GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
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
                if(CheckRenderTargetCapability(AdapterFormat, CheckFormat)) {
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
                if(CheckFilterCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_LEGACYBUMPMAP support */
            if(Usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP) {
                if(CheckBumpMapCapability(Adapter, DeviceType, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_LEGACYBUMPMAP;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No legacy bumpmap support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if(CheckPostPixelShaderBlendingCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBREAD support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                if(CheckSrgbReadCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBWRITE support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                if(CheckSrgbWriteCapability(Adapter, DeviceType, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_VERTEXTEXTURE support */
            if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                if(CheckVertexTextureCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_WRAPANDMIP support */
            if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                if(CheckWrapAndMipCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_WRAPANDMIP;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No wrapping and mipmapping support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            if(Usage & WINED3DUSAGE_DEPTHSTENCIL) {
                if(CheckDepthStencilCapability(Adapter, AdapterFormat, CheckFormat)) {
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
        if(GL_SUPPORT(EXT_TEXTURE3D)) {
            if(CheckTextureCapability(Adapter, DeviceType, CheckFormat) == FALSE) {
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
                if(CheckFilterCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_FILTER;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query filter support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_POSTPIXELSHADER_BLENDING support */
            if(Usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
                if(CheckPostPixelShaderBlendingCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query post pixelshader blending support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBREAD support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBREAD) {
                if(CheckSrgbReadCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBREAD;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbread support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_SRGBWRITE support */
            if(Usage & WINED3DUSAGE_QUERY_SRGBWRITE) {
                if(CheckSrgbWriteCapability(Adapter, DeviceType, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_SRGBWRITE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query srgbwrite support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_VERTEXTEXTURE support */
            if(Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
                if(CheckVertexTextureCapability(Adapter, CheckFormat)) {
                    UsageCaps |= WINED3DUSAGE_QUERY_VERTEXTEXTURE;
                } else {
                    TRACE_(d3d_caps)("[FAILED] - No query vertextexture support\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
            }

            /* Check QUERY_WRAPANDMIP support */
            if(Usage & WINED3DUSAGE_QUERY_WRAPANDMIP) {
                if(CheckWrapAndMipCapability(Adapter, CheckFormat)) {
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
            case WINED3DFMT_P8:
            case WINED3DFMT_A4L4:
            case WINED3DFMT_R32F:
            case WINED3DFMT_R16F:
            case WINED3DFMT_X8L8V8U8:
            case WINED3DFMT_L6V5U5:
            case WINED3DFMT_G16R16:
                TRACE_(d3d_caps)("[FAILED] - No converted formats on volumes\n");
                return WINED3DERR_NOTAVAILABLE;

            case WINED3DFMT_Q8W8V8U8:
            case WINED3DFMT_V16U16:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                TRACE_(d3d_caps)("[FAILED] - No converted formats on volumes\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            break;

            case WINED3DFMT_V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER) || !GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
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
    } else if((RType == WINED3DRTYPE_INDEXBUFFER) || (RType == WINED3DRTYPE_VERTEXBUFFER)){
        /* For instance vertexbuffer/indexbuffer aren't supported yet because no Windows drivers seem to offer it */
        TRACE_(d3d_caps)("Unhandled resource type D3DRTYPE_INDEXBUFFER / D3DRTYPE_VERTEXBUFFER\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    /* This format is nothing special and it is supported perfectly.
     * However, ati and nvidia driver on windows do not mark this format as
     * supported (tested with the dxCapsViewer) and pretending to
     * support this format uncovers a bug in Battlefield 1942 (fonts are missing)
     * So do the same as Windows drivers and pretend not to support it on dx8 and 9
     * Enable it on dx7. It will need additional checking on dx10 when we support it.
     */
    if(This->dxVersion > 7 && CheckFormat == WINED3DFMT_R8G8B8) {
        TRACE_(d3d_caps)("[FAILED]\n");
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

static HRESULT  WINAPI IWineD3DImpl_CheckDeviceFormatConversion(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType,
                                                          WINED3DFORMAT SourceFormat, WINED3DFORMAT TargetFormat) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    FIXME_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, DevType:(%u,%s), SrcFmt:(%u,%s), TgtFmt:(%u,%s))\n",
          This,
          Adapter,
          DeviceType, debug_d3ddevicetype(DeviceType),
          SourceFormat, debug_d3dformat(SourceFormat),
          TargetFormat, debug_d3dformat(TargetFormat));
    return WINED3D_OK;
}

static const shader_backend_t *select_shader_backend(UINT Adapter, WINED3DDEVTYPE DeviceType) {
    const shader_backend_t *ret;
    int vs_selected_mode;
    int ps_selected_mode;

    select_shader_mode(&GLINFO_LOCATION, DeviceType, &ps_selected_mode, &vs_selected_mode);
    if (vs_selected_mode == SHADER_GLSL || ps_selected_mode == SHADER_GLSL) {
        ret = &glsl_shader_backend;
    } else if (vs_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_ARB) {
        ret = &arb_program_shader_backend;
    } else {
        ret = &none_shader_backend;
    }
    return ret;
}

static const struct fragment_pipeline *select_fragment_implementation(UINT Adapter, WINED3DDEVTYPE DeviceType) {
    int vs_selected_mode;
    int ps_selected_mode;

    select_shader_mode(&GLINFO_LOCATION, DeviceType, &ps_selected_mode, &vs_selected_mode);
    if((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL) && GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
        return &arbfp_fragment_pipeline;
    } else if(ps_selected_mode == SHADER_ATI) {
        return &atifs_fragment_pipeline;
    } else if(GL_SUPPORT(NV_REGISTER_COMBINERS) && GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        return &nvts_fragment_pipeline;
    } else if(GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        return &nvrc_fragment_pipeline;
    } else {
        return &ffp_fragment_pipeline;
    }
}

static const struct blit_shader *select_blit_implementation(UINT Adapter, WINED3DDEVTYPE DeviceType) {
    int vs_selected_mode;
    int ps_selected_mode;

    select_shader_mode(&GLINFO_LOCATION, DeviceType, &ps_selected_mode, &vs_selected_mode);
    if((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL) && GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
        return &arbfp_blit;
    } else {
        return &ffp_blit;
    }
}

/* Note: d3d8 passes in a pointer to a D3DCAPS8 structure, which is a true
      subset of a D3DCAPS9 structure. However, it has to come via a void *
      as the d3d8 interface cannot import the d3d9 header                  */
static HRESULT WINAPI IWineD3DImpl_GetDeviceCaps(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DCAPS* pCaps) {

    IWineD3DImpl    *This = (IWineD3DImpl *)iface;
    int vs_selected_mode;
    int ps_selected_mode;
    struct shader_caps shader_caps;
    struct fragment_caps fragment_caps;
    const shader_backend_t *shader_backend;
    const struct fragment_pipeline *frag_pipeline = NULL;
    DWORD ckey_caps, blit_caps, fx_caps;

    TRACE_(d3d_caps)("(%p)->(Adptr:%d, DevType: %x, pCaps: %p)\n", This, Adapter, DeviceType, pCaps);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    select_shader_mode(&Adapters[Adapter].gl_info, DeviceType, &ps_selected_mode, &vs_selected_mode);

    /* This function should *not* be modifying GL caps
     * TODO: move the functionality where it belongs */
    select_shader_max_constants(ps_selected_mode, vs_selected_mode, &Adapters[Adapter].gl_info);

    /* ------------------------------------------------
       The following fields apply to both d3d8 and d3d9
       ------------------------------------------------ */
    pCaps->DeviceType              = (DeviceType == WINED3DDEVTYPE_HAL) ? WINED3DDEVTYPE_HAL : WINED3DDEVTYPE_REF;  /* Not quite true, but use h/w supported by opengl I suppose */
    pCaps->AdapterOrdinal          = Adapter;

    pCaps->Caps                    = 0;
    pCaps->Caps2                   = WINED3DCAPS2_CANRENDERWINDOWED |
                                     WINED3DCAPS2_FULLSCREENGAMMA |
                                     WINED3DCAPS2_DYNAMICTEXTURES;
    if(GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
        pCaps->Caps2 |= WINED3DCAPS2_CANAUTOGENMIPMAP;
    }
    pCaps->Caps3                   = WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD;
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

    if(GL_SUPPORT(EXT_BLEND_EQUATION_SEPARATE) && GL_SUPPORT(EXT_BLEND_FUNC_SEPARATE))
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

    if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
        pCaps->RasterCaps |= WINED3DPRASTERCAPS_ANISOTROPY    |
                             WINED3DPRASTERCAPS_ZBIAS         |
                             WINED3DPRASTERCAPS_MIPMAPLODBIAS;
    }
    if(GL_SUPPORT(NV_FOG_DISTANCE)) {
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

    if( GL_SUPPORT(EXT_BLEND_COLOR)) {
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

    if( !GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO)) {
        pCaps->TextureCaps |= WINED3DPTEXTURECAPS_POW2 |
                              WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }

    if( GL_SUPPORT(EXT_TEXTURE3D)) {
        pCaps->TextureCaps |=  WINED3DPTEXTURECAPS_VOLUMEMAP      |
                               WINED3DPTEXTURECAPS_MIPVOLUMEMAP   |
                               WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;
    }

    if (GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
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

    if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
        pCaps->TextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                    WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
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

        if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
            pCaps->CubeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                            WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    } else
        pCaps->CubeTextureFilterCaps = 0;

    if (GL_SUPPORT(EXT_TEXTURE3D)) {
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

    if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
    }
    if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
    }
    if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
        pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
    }

    if (GL_SUPPORT(EXT_TEXTURE3D)) {
        pCaps->VolumeTextureAddressCaps =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                           WINED3DPTADDRESSCAPS_CLAMP  |
                                           WINED3DPTADDRESSCAPS_WRAP;
        if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
            pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
        }
        if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
            pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
        }
        if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
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

    pCaps->MaxTextureWidth  = GL_LIMITS(texture_size);
    pCaps->MaxTextureHeight = GL_LIMITS(texture_size);

    if(GL_SUPPORT(EXT_TEXTURE3D))
        pCaps->MaxVolumeExtent = GL_LIMITS(texture3d_size);
    else
        pCaps->MaxVolumeExtent = 0;

    pCaps->MaxTextureRepeat = 32768;
    pCaps->MaxTextureAspectRatio = GL_LIMITS(texture_size);
    pCaps->MaxVertexW = 1.0;

    pCaps->GuardBandLeft = 0;
    pCaps->GuardBandTop = 0;
    pCaps->GuardBandRight = 0;
    pCaps->GuardBandBottom = 0;

    pCaps->ExtentsAdjust = 0;

    pCaps->StencilCaps =  WINED3DSTENCILCAPS_DECRSAT |
                          WINED3DSTENCILCAPS_INCRSAT |
                          WINED3DSTENCILCAPS_INVERT  |
                          WINED3DSTENCILCAPS_KEEP    |
                          WINED3DSTENCILCAPS_REPLACE |
                          WINED3DSTENCILCAPS_ZERO;
    if (GL_SUPPORT(EXT_STENCIL_WRAP)) {
        pCaps->StencilCaps |= WINED3DSTENCILCAPS_DECR  |
                              WINED3DSTENCILCAPS_INCR;
    }
    if ( This->dxVersion > 8 &&
        ( GL_SUPPORT(EXT_STENCIL_TWO_SIDE) ||
            GL_SUPPORT(ATI_SEPARATE_STENCIL) ) ) {
        pCaps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    pCaps->FVFCaps = WINED3DFVFCAPS_PSIZE | 0x0008; /* 8 texture coords */

    pCaps->MaxUserClipPlanes       = GL_LIMITS(clipplanes);
    pCaps->MaxActiveLights         = GL_LIMITS(lights);

    pCaps->MaxVertexBlendMatrices      = GL_LIMITS(blends);
    pCaps->MaxVertexBlendMatrixIndex   = 0;

    pCaps->MaxAnisotropy   = GL_LIMITS(anisotropy);
    pCaps->MaxPointSize    = GL_LIMITS(pointsize);


    pCaps->VertexProcessingCaps = WINED3DVTXPCAPS_DIRECTIONALLIGHTS |
                                  WINED3DVTXPCAPS_MATERIALSOURCE7   |
                                  WINED3DVTXPCAPS_POSITIONALLIGHTS  |
                                  WINED3DVTXPCAPS_LOCALVIEWER       |
                                  WINED3DVTXPCAPS_VERTEXFOG         |
                                  WINED3DVTXPCAPS_TEXGEN;
                                  /* FIXME: Add 
                                     D3DVTXPCAPS_TWEENING, D3DVTXPCAPS_TEXGEN_SPHEREMAP */

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

    pCaps->NumSimultaneousRTs = GL_LIMITS(buffers);

    pCaps->StretchRectFilterCaps             = WINED3DPTFILTERCAPS_MINFPOINT  |
                                                WINED3DPTFILTERCAPS_MAGFPOINT  |
                                                WINED3DPTFILTERCAPS_MINFLINEAR |
                                                WINED3DPTFILTERCAPS_MAGFLINEAR;
    pCaps->VertexTextureFilterCaps           = 0;

    memset(&shader_caps, 0, sizeof(shader_caps));
    shader_backend = select_shader_backend(Adapter, DeviceType);
    shader_backend->shader_get_caps(DeviceType, &GLINFO_LOCATION, &shader_caps);

    memset(&fragment_caps, 0, sizeof(fragment_caps));
    frag_pipeline = select_fragment_implementation(Adapter, DeviceType);
    frag_pipeline->get_caps(DeviceType, &GLINFO_LOCATION, &fragment_caps);

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
        pCaps->PixelShader1xMaxValue        = 0.0;
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
        pCaps->VS20Caps.NumTemps                 = max(32, GLINFO_LOCATION.vs_arb_max_temps);
        pCaps->VS20Caps.StaticFlowControlDepth   = WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH ; /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */

        pCaps->MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        pCaps->MaxVertexShader30InstructionSlots = max(512, GLINFO_LOCATION.vs_arb_max_instructions);
    } else if(pCaps->VertexShaderVersion == WINED3DVS_VERSION(2,0)) {
        pCaps->VS20Caps.Caps                     = 0;
        pCaps->VS20Caps.DynamicFlowControlDepth  = WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        pCaps->VS20Caps.NumTemps                 = max(12, GLINFO_LOCATION.vs_arb_max_temps);
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
        pCaps->PS20Caps.NumTemps                 = max(32, GLINFO_LOCATION.ps_arb_max_temps);
        pCaps->PS20Caps.StaticFlowControlDepth   = WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH; /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        pCaps->PS20Caps.NumInstructionSlots      = WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS; /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */

        pCaps->MaxPShaderInstructionsExecuted    = 65535;
        pCaps->MaxPixelShader30InstructionSlots  = max(WINED3DMIN30SHADERINSTRUCTIONS, GLINFO_LOCATION.ps_arb_max_instructions);
    } else if(pCaps->PixelShaderVersion == WINED3DPS_VERSION(2,0)) {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        pCaps->PS20Caps.Caps                     = 0;
        pCaps->PS20Caps.DynamicFlowControlDepth  = 0; /* WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        pCaps->PS20Caps.NumTemps                 = max(12, GLINFO_LOCATION.ps_arb_max_temps);
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
        if (GL_SUPPORT(NV_HALF_FLOAT)) {
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
    if(Adapters[Adapter].opengl) {
        pCaps->DirectDrawCaps.ddsCaps |=WINEDDSCAPS_3DDEVICE                |
                                        WINEDDSCAPS_MIPMAP                  |
                                        WINEDDSCAPS_TEXTURE                 |
                                        WINEDDSCAPS_ZBUFFER;
        pCaps->DirectDrawCaps.Caps |=   WINEDDCAPS_3D;
    }

    return WINED3D_OK;
}

/* Note due to structure differences between dx8 and dx9 D3DPRESENT_PARAMETERS,
   and fields being inserted in the middle, a new structure is used in place    */
static HRESULT  WINAPI IWineD3DImpl_CreateDevice(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, HWND hFocusWindow,
                                           DWORD BehaviourFlags, IWineD3DDevice** ppReturnedDeviceInterface,
                                           IUnknown *parent) {

    IWineD3DDeviceImpl *object  = NULL;
    IWineD3DImpl       *This    = (IWineD3DImpl *)iface;
    WINED3DDISPLAYMODE  mode;
    const struct fragment_pipeline *frag_pipeline = NULL;
    int i;
    struct fragment_caps ffp_caps;

    /* Validate the adapter number. If no adapters are available(no GL), ignore the adapter
     * number and create a device without a 3D adapter for 2D only operation.
     */
    if (IWineD3D_GetAdapterCount(iface) && Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return WINED3DERR_INVALIDCALL;
    }

    /* Create a WineD3DDevice object */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DDeviceImpl));
    *ppReturnedDeviceInterface = (IWineD3DDevice *)object;
    TRACE("Created WineD3DDevice object @ %p\n", object);
    if (NULL == object) {
      return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    /* Set up initial COM information */
    object->lpVtbl  = &IWineD3DDevice_Vtbl;
    object->ref     = 1;
    object->wineD3D = iface;
    object->adapter = numAdapters ? &Adapters[Adapter] : NULL;
    IWineD3D_AddRef(object->wineD3D);
    object->parent  = parent;
    list_init(&object->resources);
    list_init(&object->shaders);

    if(This->dxVersion == 7) {
        object->surface_alignment = DDRAW_PITCH_ALIGNMENT;
    } else {
        object->surface_alignment = D3D8_PITCH_ALIGNMENT;
    }
    object->posFixup[0] = 1.0; /* This is needed to get the x coord unmodified through a MAD */

    /* Set the state up as invalid until the device is fully created */
    object->state   = WINED3DERR_DRIVERINTERNALERROR;

    TRACE("(%p)->(Adptr:%d, DevType: %x, FocusHwnd: %p, BehFlags: %x, RetDevInt: %p)\n", This, Adapter, DeviceType,
          hFocusWindow, BehaviourFlags, ppReturnedDeviceInterface);

    /* Save the creation parameters */
    object->createParms.AdapterOrdinal = Adapter;
    object->createParms.DeviceType     = DeviceType;
    object->createParms.hFocusWindow   = hFocusWindow;
    object->createParms.BehaviorFlags  = BehaviourFlags;

    /* Initialize other useful values */
    object->adapterNo                    = Adapter;
    object->devType                      = DeviceType;

    select_shader_mode(&GLINFO_LOCATION, DeviceType, &object->ps_selected_mode, &object->vs_selected_mode);
    object->shader_backend = select_shader_backend(Adapter, DeviceType);

    memset(&ffp_caps, 0, sizeof(ffp_caps));
    frag_pipeline = select_fragment_implementation(Adapter, DeviceType);
    object->frag_pipe = frag_pipeline;
    frag_pipeline->get_caps(DeviceType, &GLINFO_LOCATION, &ffp_caps);
    object->max_ffp_textures = ffp_caps.MaxSimultaneousTextures;
    object->max_ffp_texture_stages = ffp_caps.MaxTextureBlendStages;
    compile_state_table(object->StateTable, object->multistate_funcs, &GLINFO_LOCATION,
                        ffp_vertexstate_template, frag_pipeline, misc_state_template);

    object->blitter = select_blit_implementation(Adapter, DeviceType);

    /* Prefer the vtable with functions optimized for single dirtifyable objects if the shader
     * model can deal with that. It is essentially the same, just with adjusted
     * Set*ShaderConstantF implementations
     */
    if(object->shader_backend->shader_dirtifyable_constants((IWineD3DDevice *) object)) {
        object->lpVtbl  = &IWineD3DDevice_DirtyConst_Vtbl;
    }

    /* set the state of the device to valid */
    object->state = WINED3D_OK;

    /* Get the initial screen setup for ddraw */
    IWineD3DImpl_GetAdapterDisplayMode(iface, Adapter, &mode);

    object->ddraw_width = mode.Width;
    object->ddraw_height = mode.Height;
    object->ddraw_format = mode.Format;

    for(i = 0; i < PATCHMAP_SIZE; i++) {
        list_init(&object->patches[i]);
    }
    return WINED3D_OK;
}
#undef GLINFO_LOCATION

static HRESULT WINAPI IWineD3DImpl_GetParent(IWineD3D *iface, IUnknown **pParent) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    IUnknown_AddRef(This->parent);
    *pParent = This->parent;
    return WINED3D_OK;
}

ULONG WINAPI D3DCB_DefaultDestroySurface(IWineD3DSurface *pSurface) {
    IUnknown* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    /* Now, release the parent, which will take care of cleaning up the surface for us */
    IWineD3DSurface_GetParent(pSurface, &surfaceParent);
    IUnknown_Release(surfaceParent);
    return IUnknown_Release(surfaceParent);
}

ULONG WINAPI D3DCB_DefaultDestroyVolume(IWineD3DVolume *pVolume) {
    IUnknown* volumeParent;
    TRACE("(%p) call back\n", pVolume);

    /* Now, release the parent, which will take care of cleaning up the volume for us */
    IWineD3DVolume_GetParent(pVolume, &volumeParent);
    IUnknown_Release(volumeParent);
    return IUnknown_Release(volumeParent);
}

static BOOL implementation_is_apple(WineD3D_GL_Info *gl_info) {
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
     */
    if(gl_info->supported[APPLE_FENCE] &&
       gl_info->supported[APPLE_CLIENT_STORAGE] &&
       gl_info->supported[APPLE_FLUSH_RENDER] &&
       gl_info->supported[APPLE_YCBCR_422]) {
        TRACE_(d3d_caps)("GL_APPLE_fence, GL_APPLE_client_storage, GL_APPLE_flush_render and GL_ycbcr_422 are supported\n");
        TRACE_(d3d_caps)("Activating MacOS fixups\n");
        return TRUE;
    } else {
        TRACE_(d3d_caps)("Apple extensions are not supported\n");
        TRACE_(d3d_caps)("Not activating MacOS fixups\n");
        return FALSE;
    }
}

#define GLINFO_LOCATION (*gl_info)
static void test_pbo_functionality(WineD3D_GL_Info *gl_info) {
    /* Some OpenGL implementations, namely Apple's Geforce 8 driver, advertises PBOs,
     * but glTexSubImage from a PBO fails miserably, with the first line repeated over
     * all the texture. This function detects this bug by its symptom and disables PBOs
     * if the test fails.
     *
     * The test uploads a 4x4 texture via the PBO in the "native" format GL_BGRA,
     * GL_UNSIGNED_INT_8_8_8_8_REV. This format triggers the bug, and it is what we use
     * for D3DFMT_A8R8G8B8. Then the texture is read back without any PBO and the data
     * read back is compared to the original. If they are equal PBOs are assumed to work,
     * otherwise the PBO extension is disabled.
     */
    GLuint texture, pbo;
    static const unsigned int pattern[] = {
        0x00000000, 0x000000ff, 0x0000ff00, 0x40ff0000,
        0x80ffffff, 0x40ffff00, 0x00ff00ff, 0x0000ffff,
        0x00ffff00, 0x00ff00ff, 0x0000ffff, 0x000000ff,
        0x80ff00ff, 0x0000ffff, 0x00ff00ff, 0x40ff00ff
    };
    unsigned int check[sizeof(pattern) / sizeof(pattern[0])];

    if(!gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]) {
        /* No PBO -> No point in testing them */
        return;
    }

    while(glGetError());
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    checkGLcall("Specifying the PBO test texture\n");

    GL_EXTCALL(glGenBuffersARB(1, &pbo));
    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo));
    GL_EXTCALL(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, sizeof(pattern), pattern, GL_STREAM_DRAW_ARB));
    checkGLcall("Specifying the PBO test pbo\n");

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("Loading the PBO test texture\n");

    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
    glFinish(); /* just to be sure */

    memset(check, 0, sizeof(check));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, check);
    checkGLcall("Reading back the PBO test texture\n");

    glDeleteTextures(1, &texture);
    GL_EXTCALL(glDeleteBuffersARB(1, &pbo));
    checkGLcall("PBO test cleanup\n");

    if(memcmp(check, pattern, sizeof(check)) != 0) {
        WARN_(d3d_caps)("PBO test failed, read back data doesn't match original\n");
        WARN_(d3d_caps)("Disabling PBOs. This may result in slower performance\n");
        gl_info->supported[ARB_PIXEL_BUFFER_OBJECT] = FALSE;
    } else {
        TRACE_(d3d_caps)("PBO test successful\n");
    }
}
#undef GLINFO_LOCATION

/* Certain applications(Steam) complain if we report an outdated driver version. In general,
 * reporting a driver version is moot because we are not the Windows driver, and we have different
 * bugs, features, etc.
 *
 * If a card is not found in this table, the gl driver version is reported
 */
struct driver_version_information {
    WORD vendor;                        /* reported PCI card vendor ID  */
    WORD card;                          /* reported PCI card device ID  */
    WORD hipart_hi, hipart_lo;          /* driver hiword to report      */
    WORD lopart_hi, lopart_lo;          /* driver loword to report      */
};

static const struct driver_version_information driver_version_table[] = {
    /* Nvidia drivers. Geforce6 and newer cards are supported by the current driver (177.x)*/
    /* GeforceFX support is up to 173.x, Geforce2MX/3/4 up to 96.x, TNT/Geforce1/2 up to 71.x */
    /* Note that version numbers >100 lets say 123.45 use >= x.y.11.2345 and not x.y.10.12345 */
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5200,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5600,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5800,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6200,       7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6600GT,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6800,       7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7400,       7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7300,       7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7600,       7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7800GT,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    7,  15, 11, 7341   },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     7,  15, 11, 7341    },
    {VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     7,  15, 11, 7341    },

    /* ATI cards. The driver versions are somewhat similar, but not quite the same. Let's hardcode */
    {VENDOR_ATI,        CARD_ATI_RADEON_9500,           6,  14, 10, 6764    },
    {VENDOR_ATI,        CARD_ATI_RADEON_X700,           6,  14, 10, 6764    },
    {VENDOR_ATI,        CARD_ATI_RADEON_X1600,          6,  14, 10, 6764    },
    {VENDOR_ATI,        CARD_ATI_RADEON_HD2300,         6,  14, 10, 6764    },
    {VENDOR_ATI,        CARD_ATI_RADEON_HD2600,         6,  14, 10, 6764    },
    {VENDOR_ATI,        CARD_ATI_RADEON_HD2900,         6,  14, 10, 6764    },

    /* TODO: Add information about legacy nvidia and ATI hardware, Intel and other cards */
};

static void fixup_extensions(WineD3D_GL_Info *gl_info) {
    unsigned int i;
    BOOL apple = implementation_is_apple(gl_info);

    if(apple) {
        /* MacOS advertises more GLSL vertex shader uniforms than supported by the hardware, and if more are
         * used it falls back to software. While the compiler can detect if the shader uses all declared
         * uniforms, the optimization fails if the shader uses relative addressing. So any GLSL shader
         * using relative addressing falls back to software.
         *
         * ARB vp gives the correct amount of uniforms, so use it instead of GLSL
         */
        if(gl_info->vs_glsl_constantsF <= gl_info->vs_arb_constantsF) {
            FIXME("GLSL doesn't advertise more vertex shader uniforms than ARB. Driver fixup outdated?\n");
        } else {
            TRACE("Driver claims %u GLSL vs uniforms, replacing with %u ARB vp uniforms\n",
                  gl_info->vs_glsl_constantsF, gl_info->vs_arb_constantsF);
            gl_info->vs_glsl_constantsF = gl_info->vs_arb_constantsF;
        }

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
         * performance negatively.
         */
        if(gl_info->gl_vendor == VENDOR_INTEL ||
           (gl_info->gl_vendor == VENDOR_ATI && gl_info->gl_card != CARD_ATI_RADEON_X1600)) {
            TRACE("Enabling vertex texture coord fixes in vertex shaders\n");
            gl_info->set_texcoord_w = TRUE;
        }
    }

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
     * due to that, so this code works on fglrx as well.
     */
    if(gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] && gl_info->gl_vendor == VENDOR_ATI) {
        if(gl_info->gl_card == CARD_ATI_RADEON_X700 || gl_info->gl_card == CARD_ATI_RADEON_X1600 ||
            gl_info->gl_card == CARD_ATI_RADEON_9500 || gl_info->gl_card == CARD_ATI_RADEON_8500  ||
            gl_info->gl_card == CARD_ATI_RADEON_7200 || gl_info->gl_card == CARD_ATI_RAGE_128PRO) {
            TRACE("GL_ARB_texture_non_power_of_two advertised on R500 or earlier card, removing\n");
            gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
            gl_info->supported[WINE_NORMALIZED_TEXRECT] = TRUE;
        }
    }

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
     *  The behaviour can be verified through a simple test app attached in bugreport #14724.
     */
    if(gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] && gl_info->gl_vendor == VENDOR_NVIDIA) {
        if(gl_info->gl_card == CARD_NVIDIA_GEFORCEFX_5800 || gl_info->gl_card == CARD_NVIDIA_GEFORCEFX_5600) {
            TRACE("GL_ARB_texture_non_power_of_two advertised through OpenGL 2.0 on NV FX card, removing\n");
            gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
            gl_info->supported[ARB_TEXTURE_RECTANGLE] = TRUE;
        }
    }

    /* Find out if PBOs work as they are supposed to */
    test_pbo_functionality(gl_info);

    /* Fixup the driver version */
    for(i = 0; i < (sizeof(driver_version_table) / sizeof(driver_version_table[0])); i++) {
        if(gl_info->gl_vendor == driver_version_table[i].vendor &&
           gl_info->gl_card   == driver_version_table[i].card) {
            TRACE_(d3d_caps)("Found card 0x%04x, 0x%04x in driver version DB\n", gl_info->gl_vendor, gl_info->gl_card);

            gl_info->driver_version        = MAKEDWORD_VERSION(driver_version_table[i].lopart_hi,
                                                               driver_version_table[i].lopart_lo);
            gl_info->driver_version_hipart = MAKEDWORD_VERSION(driver_version_table[i].hipart_hi,
                                                               driver_version_table[i].hipart_lo);
            break;
        }
    }
}

static void WINE_GLAPI invalid_func(void *data) {
    ERR("Invalid vertex attribute function called\n");
    DebugBreak();
}

#define GLINFO_LOCATION (Adapters[0].gl_info)

/* Helper functions for providing vertex data to opengl. The arrays are initialized based on
 * the extension detection and are used in drawStridedSlow
 */
static void WINE_GLAPI position_d3dcolor(void *data) {
    DWORD pos = *((DWORD *) data);

    FIXME("Add a test for fixed function position from d3dcolor type\n");
    glVertex4s(D3DCOLOR_B_R(pos),
               D3DCOLOR_B_G(pos),
               D3DCOLOR_B_B(pos),
               D3DCOLOR_B_A(pos));
}
static void WINE_GLAPI position_float4(void *data) {
    GLfloat *pos = (float *) data;

    if (pos[3] < eps && pos[3] > -eps)
        glVertex3fv(pos);
    else {
        float w = 1.0 / pos[3];

        glVertex4f(pos[0] * w, pos[1] * w, pos[2] * w, w);
    }
}

static void WINE_GLAPI diffuse_d3dcolor(void *data) {
    DWORD diffuseColor = *((DWORD *) data);

    glColor4ub(D3DCOLOR_B_R(diffuseColor),
               D3DCOLOR_B_G(diffuseColor),
               D3DCOLOR_B_B(diffuseColor),
               D3DCOLOR_B_A(diffuseColor));
}

static void WINE_GLAPI specular_d3dcolor(void *data) {
    DWORD specularColor = *((DWORD *) data);

    GL_EXTCALL(glSecondaryColor3ubEXT)(D3DCOLOR_B_R(specularColor),
                                       D3DCOLOR_B_G(specularColor),
                                       D3DCOLOR_B_B(specularColor));
}
static void WINE_GLAPI warn_no_specular_func(void *data) {
    WARN("GL_EXT_secondary_color not supported\n");
}

void fillGLAttribFuncs(WineD3D_GL_Info *gl_info) {
    position_funcs[WINED3DDECLTYPE_FLOAT1]      = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_FLOAT2]      = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_FLOAT3]      = (void *) glVertex3fv;
    position_funcs[WINED3DDECLTYPE_FLOAT4]      = (void *) position_float4;
    position_funcs[WINED3DDECLTYPE_D3DCOLOR]    = (void *) position_d3dcolor;
    position_funcs[WINED3DDECLTYPE_UBYTE4]      = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_SHORT2]      = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_SHORT4]      = (void *) glVertex2sv;
    position_funcs[WINED3DDECLTYPE_UBYTE4N]     = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_SHORT2N]     = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_SHORT4N]     = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_USHORT2N]    = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_USHORT4N]    = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_UDEC3]       = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_DEC3N]       = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_FLOAT16_2]   = (void *) invalid_func;
    position_funcs[WINED3DDECLTYPE_FLOAT16_4]   = (void *) invalid_func;

    diffuse_funcs[WINED3DDECLTYPE_FLOAT1]       = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_FLOAT2]       = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_FLOAT3]       = (void *) glColor3fv;
    diffuse_funcs[WINED3DDECLTYPE_FLOAT4]       = (void *) glColor4fv;
    diffuse_funcs[WINED3DDECLTYPE_D3DCOLOR]     = (void *) diffuse_d3dcolor;
    diffuse_funcs[WINED3DDECLTYPE_UBYTE4]       = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_SHORT2]       = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_SHORT4]       = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_UBYTE4N]      = (void *) glColor4ubv;
    diffuse_funcs[WINED3DDECLTYPE_SHORT2N]      = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_SHORT4N]      = (void *) glColor4sv;
    diffuse_funcs[WINED3DDECLTYPE_USHORT2N]     = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_USHORT4N]     = (void *) glColor4usv;
    diffuse_funcs[WINED3DDECLTYPE_UDEC3]        = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_DEC3N]        = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_FLOAT16_2]    = (void *) invalid_func;
    diffuse_funcs[WINED3DDECLTYPE_FLOAT16_4]    = (void *) invalid_func;

    /* No 4 component entry points here */
    specular_funcs[WINED3DDECLTYPE_FLOAT1]      = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_FLOAT2]      = (void *) invalid_func;
    if(GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        specular_funcs[WINED3DDECLTYPE_FLOAT3]      = (void *) GL_EXTCALL(glSecondaryColor3fvEXT);
    } else {
        specular_funcs[WINED3DDECLTYPE_FLOAT3]      = (void *) warn_no_specular_func;
    }
    specular_funcs[WINED3DDECLTYPE_FLOAT4]      = (void *) invalid_func;
    if(GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        specular_funcs[WINED3DDECLTYPE_D3DCOLOR]    = (void *) specular_d3dcolor;
    } else {
        specular_funcs[WINED3DDECLTYPE_D3DCOLOR]      = (void *) warn_no_specular_func;
    }
    specular_funcs[WINED3DDECLTYPE_UBYTE4]      = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_SHORT2]      = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_SHORT4]      = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_UBYTE4N]     = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_SHORT2N]     = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_SHORT4N]     = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_USHORT2N]    = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_USHORT4N]    = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_UDEC3]       = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_DEC3N]       = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_FLOAT16_2]   = (void *) invalid_func;
    specular_funcs[WINED3DDECLTYPE_FLOAT16_4]   = (void *) invalid_func;

    /* Only 3 component entry points here. Test how others behave. Float4 normals are used
     * by one of our tests, trying to pass it to the pixel shader, which fails on Windows.
     */
    normal_funcs[WINED3DDECLTYPE_FLOAT1]         = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_FLOAT2]         = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_FLOAT3]         = (void *) glNormal3fv;
    normal_funcs[WINED3DDECLTYPE_FLOAT4]         = (void *) glNormal3fv; /* Just ignore the 4th value */
    normal_funcs[WINED3DDECLTYPE_D3DCOLOR]       = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_UBYTE4]         = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_SHORT2]         = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_SHORT4]         = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_UBYTE4N]        = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_SHORT2N]        = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_SHORT4N]        = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_USHORT2N]       = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_USHORT4N]       = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_UDEC3]          = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_DEC3N]          = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_FLOAT16_2]      = (void *) invalid_func;
    normal_funcs[WINED3DDECLTYPE_FLOAT16_4]      = (void *) invalid_func;
}

#define PUSH1(att)        attribs[nAttribs++] = (att);
BOOL InitAdapters(void) {
    static HMODULE mod_gl, mod_win32gl;
    BOOL ret;
    int ps_selected_mode, vs_selected_mode;

    /* No need to hold any lock. The calling library makes sure only one thread calls
     * wined3d simultaneously
     */
    if(numAdapters > 0) return Adapters[0].opengl;

    TRACE("Initializing adapters\n");

    if(!mod_gl) {
#ifdef USE_WIN32_OPENGL
#define USE_GL_FUNC(pfn) pfn = (void*)GetProcAddress(mod_gl, #pfn);
        mod_gl = LoadLibraryA("opengl32.dll");
        if(!mod_gl) {
            ERR("Can't load opengl32.dll!\n");
            goto nogl_adapter;
        }
        mod_win32gl = mod_gl;
#else
#define USE_GL_FUNC(pfn) pfn = (void*)pwglGetProcAddress(#pfn);
        /* To bypass the opengl32 thunks load wglGetProcAddress from gdi32 (glXGetProcAddress wrapper) instead of opengl32's */
        mod_gl = GetModuleHandleA("gdi32.dll");
        mod_win32gl = LoadLibraryA("opengl32.dll");
        if(!mod_win32gl) {
            ERR("Can't load opengl32.dll!\n");
            goto nogl_adapter;
        }
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
    glFinish = (void*)GetProcAddress(mod_win32gl, "glFinish");
    glFlush = (void*)GetProcAddress(mod_win32gl, "glFlush");

    /* For now only one default adapter */
    {
        int iPixelFormat;
        int attribs[10];
        int values[10];
        int nAttribs = 0;
        int res;
        int i;
        WineD3D_PixelFormat *cfgs;
        int attribute;
        DISPLAY_DEVICEW DisplayDevice;
        HDC hdc;

        TRACE("Initializing default adapter\n");
        Adapters[0].num = 0;
        Adapters[0].monitorPoint.x = -1;
        Adapters[0].monitorPoint.y = -1;

        if (!WineD3D_CreateFakeGLContext()) {
            ERR("Failed to get a gl context for default adapter\n");
            WineD3D_ReleaseFakeGLContext();
            goto nogl_adapter;
        }

        ret = IWineD3DImpl_FillGLCaps(&Adapters[0].gl_info);
        if(!ret) {
            ERR("Failed to initialize gl caps for default adapter\n");
            WineD3D_ReleaseFakeGLContext();
            goto nogl_adapter;
        }
        ret = initPixelFormats(&Adapters[0].gl_info);
        if(!ret) {
            ERR("Failed to init gl formats\n");
            WineD3D_ReleaseFakeGLContext();
            goto nogl_adapter;
        }

        hdc = pwglGetCurrentDC();
        if(!hdc) {
            ERR("Failed to get gl HDC\n");
            WineD3D_ReleaseFakeGLContext();
            goto nogl_adapter;
        }

        Adapters[0].driver = "Display";
        Adapters[0].description = "Direct3D HAL";

        /* Use the VideoRamSize registry setting when set */
        if(wined3d_settings.emulated_textureram)
            Adapters[0].TextureRam = wined3d_settings.emulated_textureram;
        else
            Adapters[0].TextureRam = Adapters[0].gl_info.vidmem;
        Adapters[0].UsedTextureRam = 0;
        TRACE("Emulating %dMB of texture ram\n", Adapters[0].TextureRam/(1024*1024));

        /* Initialize the Adapter's DeviceName which is required for ChangeDisplaySettings and friends */
        DisplayDevice.cb = sizeof(DisplayDevice);
        EnumDisplayDevicesW(NULL, 0 /* Adapter 0 = iDevNum 0 */, &DisplayDevice, 0);
        TRACE("DeviceName: %s\n", debugstr_w(DisplayDevice.DeviceName));
        strcpyW(Adapters[0].DeviceName, DisplayDevice.DeviceName);

        attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
        GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attribute, &Adapters[0].nCfgs));

        Adapters[0].cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Adapters[0].nCfgs *sizeof(WineD3D_PixelFormat));
        cfgs = Adapters[0].cfgs;
        PUSH1(WGL_RED_BITS_ARB)
        PUSH1(WGL_GREEN_BITS_ARB)
        PUSH1(WGL_BLUE_BITS_ARB)
        PUSH1(WGL_ALPHA_BITS_ARB)
        PUSH1(WGL_DEPTH_BITS_ARB)
        PUSH1(WGL_STENCIL_BITS_ARB)
        PUSH1(WGL_DRAW_TO_WINDOW_ARB)
        PUSH1(WGL_PIXEL_TYPE_ARB)
        PUSH1(WGL_DOUBLE_BUFFER_ARB)
        PUSH1(WGL_AUX_BUFFERS_ARB)

        for(iPixelFormat=1; iPixelFormat<=Adapters[0].nCfgs; iPixelFormat++) {
            res = GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, nAttribs, attribs, values));

            if(!res)
                continue;

            /* Cache the pixel format */
            cfgs->iPixelFormat = iPixelFormat;
            cfgs->redSize = values[0];
            cfgs->greenSize = values[1];
            cfgs->blueSize = values[2];
            cfgs->alphaSize = values[3];
            cfgs->depthSize = values[4];
            cfgs->stencilSize = values[5];
            cfgs->windowDrawable = values[6];
            cfgs->iPixelType = values[7];
            cfgs->doubleBuffer = values[8];
            cfgs->auxBuffers = values[9];

            cfgs->pbufferDrawable = FALSE;
            /* Check for pbuffer support when it is around as wglGetPixelFormatAttribiv fails for unknown attributes. */
            if(GL_SUPPORT(WGL_ARB_PBUFFER)) {
                int attrib = WGL_DRAW_TO_PBUFFER_ARB;
                int value;
                if(GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, 1, &attrib, &value)))
                    cfgs->pbufferDrawable = value;
            }

            cfgs->numSamples = 0;
            /* Check multisample support */
            if(GL_SUPPORT(ARB_MULTISAMPLE)) {
                int attrib[2] = {WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB};
                int value[2];
                if(GL_EXTCALL(wglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0, 2, attrib, value))) {
                    /* value[0] = WGL_SAMPLE_BUFFERS_ARB which tells whether multisampling is supported.
                     * value[1] = number of multi sample buffers*/
                    if(value[0])
                        cfgs->numSamples = value[1];
                }
            }

            TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, depth=%d, stencil=%d, windowDrawable=%d, pbufferDrawable=%d\n", cfgs->iPixelFormat, cfgs->iPixelType, cfgs->doubleBuffer, cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize, cfgs->depthSize, cfgs->stencilSize, cfgs->windowDrawable, cfgs->pbufferDrawable);
            cfgs++;
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
        Adapters[0].brokenStencil = TRUE;
        for(i=0, cfgs=Adapters[0].cfgs; i<Adapters[0].nCfgs; i++) {
            /* Nearly all drivers offer depth formats without stencil, only on i915 this if-statement won't be entered. */
            if(cfgs[i].depthSize && !cfgs[i].stencilSize) {
                Adapters[0].brokenStencil = FALSE;
                break;
            }
        }

        fixup_extensions(&Adapters[0].gl_info);

        WineD3D_ReleaseFakeGLContext();

        select_shader_mode(&Adapters[0].gl_info, WINED3DDEVTYPE_HAL, &ps_selected_mode, &vs_selected_mode);
        select_shader_max_constants(ps_selected_mode, vs_selected_mode, &Adapters[0].gl_info);
        fillGLAttribFuncs(&Adapters[0].gl_info);
        init_type_lookup(&Adapters[0].gl_info);
        Adapters[0].opengl = TRUE;
    }
    numAdapters = 1;
    TRACE("%d adapters successfully initialized\n", numAdapters);

    return TRUE;

nogl_adapter:
    /* Initialize an adapter for ddraw-only memory counting */
    memset(Adapters, 0, sizeof(Adapters));
    Adapters[0].num = 0;
    Adapters[0].opengl = FALSE;
    Adapters[0].monitorPoint.x = -1;
    Adapters[0].monitorPoint.y = -1;

    Adapters[0].driver = "Display";
    Adapters[0].description = "WineD3D DirectDraw Emulation";
    if(wined3d_settings.emulated_textureram) {
        Adapters[0].TextureRam = wined3d_settings.emulated_textureram;
    } else {
        Adapters[0].TextureRam = 8 * 1024 * 1024; /* This is plenty for a DDraw-only card */
    }

    numAdapters = 1;
    return FALSE;
}
#undef PUSH1
#undef GLINFO_LOCATION

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
