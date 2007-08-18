/*
 * IWineD3D implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
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

/* Extension detection */
static const struct {
    const char *extension_string;
    GL_SupportedExt extension;
} EXTENSION_MAP[] = {
    /* APPLE */
    {"GL_APPLE_client_storage",             APPLE_CLIENT_STORAGE},
    {"GL_APPLE_fence",                      APPLE_FENCE},

    /* ATI */
    {"GL_ATI_separate_stencil",             ATI_SEPARATE_STENCIL},
    {"GL_ATI_texture_env_combine3",         ATI_TEXTURE_ENV_COMBINE3},
    {"GL_ATI_texture_mirror_once",          ATI_TEXTURE_MIRROR_ONCE},
    {"GL_ATI_envmap_bumpmap",               ATI_ENVMAP_BUMPMAP},

    /* ARB */
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS},
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM},
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER},
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL},
    {"GL_ARB_imaging",                      ARB_IMAGING},
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE}, /* needs GLX_ARB_MULTISAMPLE as well */
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE},
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY},
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT},
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS},
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE},
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP},
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION},
    {"GL_ARB_texture_cube_map",             ARB_TEXTURE_CUBE_MAP},
    {"GL_ARB_texture_env_add",              ARB_TEXTURE_ENV_ADD},
    {"GL_ARB_texture_env_combine",          ARB_TEXTURE_ENV_COMBINE},
    {"GL_ARB_texture_env_dot3",             ARB_TEXTURE_ENV_DOT3},
    {"GL_ARB_texture_float",                ARB_TEXTURE_FLOAT},
    {"GL_ARB_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT},
    {"GL_ARB_texture_non_power_of_two",     ARB_TEXTURE_NON_POWER_OF_TWO},
    {"GL_ARB_vertex_blend",                 ARB_VERTEX_BLEND},
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT},
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM},
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER},

    /* EXT */
    {"GL_EXT_blend_minmax",                 EXT_BLEND_MINMAX},
    {"GL_EXT_fog_coord",                    EXT_FOG_COORD},
    {"GL_EXT_framebuffer_blit",             EXT_FRAMEBUFFER_BLIT},
    {"GL_EXT_framebuffer_object",           EXT_FRAMEBUFFER_OBJECT},
    {"GL_EXT_paletted_texture",             EXT_PALETTED_TEXTURE},
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS},
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR},
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE},
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP},
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D},
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC},
    {"GL_EXT_texture_env_add",              EXT_TEXTURE_ENV_ADD},
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE},
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3},
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB},
    {"GL_EXT_texture_filter_anisotropic",   EXT_TEXTURE_FILTER_ANISOTROPIC},
    {"GL_EXT_texture_lod",                  EXT_TEXTURE_LOD},
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS},
    {"GL_EXT_vertex_shader",                EXT_VERTEX_SHADER},
    {"GL_EXT_vertex_weighting",             EXT_VERTEX_WEIGHTING},

    /* NV */
    {"GL_NV_half_float",                    NV_HALF_FLOAT},
    {"GL_NV_fence",                         NV_FENCE},
    {"GL_NV_fog_distance",                  NV_FOG_DISTANCE},
    {"GL_NV_fragment_program",              NV_FRAGMENT_PROGRAM},
    {"GL_NV_fragment_program2",             NV_FRAGMENT_PROGRAM2},
    {"GL_NV_register_combiners",            NV_REGISTER_COMBINERS},
    {"GL_NV_register_combiners2",           NV_REGISTER_COMBINERS2},
    {"GL_NV_texgen_reflection",             NV_TEXGEN_REFLECTION},
    {"GL_NV_texture_env_combine4",          NV_TEXTURE_ENV_COMBINE4},
    {"GL_NV_texture_shader",                NV_TEXTURE_SHADER},
    {"GL_NV_texture_shader2",               NV_TEXTURE_SHADER2},
    {"GL_NV_texture_shader3",               NV_TEXTURE_SHADER3},
    {"GL_NV_occlusion_query",               NV_OCCLUSION_QUERY},
    {"GL_NV_vertex_program",                NV_VERTEX_PROGRAM},
    {"GL_NV_vertex_program1_1",             NV_VERTEX_PROGRAM1_1},
    {"GL_NV_vertex_program2",               NV_VERTEX_PROGRAM2},
    {"GL_NV_vertex_program3",               NV_VERTEX_PROGRAM3},
};

/**********************************************************
 * Utility functions follow
 **********************************************************/

/* Adapters */
static int numAdapters = 0;
static struct WineD3DAdapter Adapters[1];

/* lookup tables */
int minLookup[MAX_LOOKUPS];
int maxLookup[MAX_LOOKUPS];
DWORD *stateLookup[MAX_LOOKUPS];

DWORD minMipLookup[WINED3DTEXF_ANISOTROPIC + 1][WINED3DTEXF_LINEAR + 1];


/**
 * Note: GL seems to trap if GetDeviceCaps is called before any HWND's created
 * ie there is no GL Context - Get a default rendering context to enable the
 * function query some info from GL
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
            pwglDeleteContext(glCtx);
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
    LEAVE_GL();
}

static BOOL WineD3D_CreateFakeGLContext(void) {
    HGLRC glCtx = NULL;

    ENTER_GL();
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
    LEAVE_GL();
    return FALSE;
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

static void select_shader_mode(
    WineD3D_GL_Info *gl_info,
    WINED3DDEVTYPE DeviceType,
    int* ps_selected,
    int* vs_selected) {

    if (wined3d_settings.vs_mode == VS_NONE) {
        *vs_selected = SHADER_NONE;
    } else if (gl_info->supported[ARB_VERTEX_SHADER] && wined3d_settings.glslRequested) {
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
BOOL IWineD3DImpl_FillGLCaps(WineD3D_GL_Info *gl_info) {
    const char *GL_Extensions    = NULL;
    const char *WGL_Extensions   = NULL;
    const char *gl_string        = NULL;
    const char *gl_string_cursor = NULL;
    GLint       gl_max;
    GLfloat     gl_floatv[2];
    int         major = 1, minor = 0;
    BOOL        return_value = TRUE;
    int         i;
    HDC         hdc;
    HMODULE     mod_gl;

#ifdef USE_WIN32_OPENGL
#define USE_GL_FUNC(pfn) pfn = (void*)GetProcAddress(mod_gl, #pfn);
    mod_gl = LoadLibraryA("opengl32.dll");
    if(!mod_gl) {
        ERR("Can't load opengl32.dll!\n");
        return FALSE;
    }
#else
#define USE_GL_FUNC(pfn) pfn = (void*)pwglGetProcAddress(#pfn);
    /* To bypass the opengl32 thunks load wglGetProcAddress from gdi32 (glXGetProcAddress wrapper) instead of opengl32's */
    mod_gl = GetModuleHandleA("gdi32.dll");
#endif

/* Load WGL core functions from opengl32.dll */
#define USE_WGL_FUNC(pfn) p##pfn = (void*)GetProcAddress(mod_gl, #pfn);
    WGL_FUNCS_GEN;
#undef USE_WGL_FUNC

    if(!pwglGetProcAddress) {
        ERR("Unable to load wglGetProcAddress!\n");
        return FALSE;
    }

/* Dynamicly load all GL core functions */
    GL_FUNCS_GEN;
#undef USE_GL_FUNC

    /* Make sure that we've got a context */
    /* TODO: CreateFakeGLContext should really take a display as a parameter  */
    /* Only save the values obtained when a display is provided */
    if (!WineD3D_CreateFakeGLContext() || wined3d_fake_gl_context_foreign)
        return_value = FALSE;

    TRACE_(d3d_caps)("(%p)\n", gl_info);

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
		   strstr(gl_info->gl_renderer, "Intel(R)")) {
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
        gl_info->gl_driver_version = MAKEDWORD_VERSION(major, minor);
        TRACE_(d3d_caps)("found GL_VERSION  (%s)->%i.%i->(0x%08x)\n", debugstr_a(gl_string), major, minor, gl_info->gl_driver_version);
    }

    TRACE_(d3d_caps)("found GL_RENDERER (%s)->(0x%04x)\n", debugstr_a(gl_info->gl_renderer), gl_info->gl_card);

    /*
     * Initialize openGL extension related variables
     *  with Default values
     */
    memset(&gl_info->supported, 0, sizeof(gl_info->supported));
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

/* Now work out what GL support this card really has */
#define USE_GL_FUNC(type, pfn) gl_info->pfn = (type) pwglGetProcAddress(#pfn);
    GL_EXT_FUNCS_GEN;
    WGL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

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
    gl_info->max_pointsize = gl_floatv[1];
    TRACE_(d3d_caps)("Maximum point size support - max point size=%f\n", gl_floatv[1]);

    glGetIntegerv(GL_AUX_BUFFERS, &gl_max);
    gl_info->max_aux_buffers = gl_max;
    TRACE_(d3d_caps)("Offscreen rendering support - number of aux buffers=%d\n", gl_max);

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

        if (gl_info->supported[APPLE_FENCE]) {
            /* GL_NV_fence and GL_APPLE_fence provide the same functionality basically.
             * The apple extension interacts with some other apple exts. Disable the NV
             * extension if the apple one is support to prevent confusion in other parts
             * of the code
             */
            gl_info->supported[NV_FENCE] = FALSE;
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
            GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEMPORARIES_ARB, &gl_max));
            gl_info->ps_arb_max_temps = gl_max;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM temporaries: %d\n", gl_info->ps_arb_max_temps);
            GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_INSTRUCTIONS_ARB, &gl_max));
            gl_info->ps_arb_max_instructions = gl_max;
            TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM instructions: %d\n", gl_info->ps_arb_max_instructions);
        }
        if (gl_info->supported[ARB_VERTEX_PROGRAM]) {
            gl_info->vs_arb_version = VS_VERSION_11;
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
            gl_info->vs_arb_constantsF = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM float constants: %d\n", gl_info->vs_arb_constantsF);
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_TEMPORARIES_ARB, &gl_max));
            gl_info->vs_arb_max_temps = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM temporaries: %d\n", gl_info->vs_arb_max_temps);
            GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_INSTRUCTIONS_ARB, &gl_max));
            gl_info->vs_arb_max_instructions = gl_max;
            TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM instructions: %d\n", gl_info->vs_arb_max_instructions);
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
     */
    switch (gl_info->gl_vendor) {
        case VENDOR_NVIDIA:
            /* Both the GeforceFX, 6xxx and 7xxx series support D3D9. The last two types have more
             * shader capabilities, so we use the shader capabilities to distinguish between FX and 6xxx/7xxx.
             */
            if(WINE_D3D9_CAPABLE(gl_info) && (gl_info->vs_nv_version == VS_VERSION_30)) {
                if (strstr(gl_info->gl_renderer, "7800") ||
                    strstr(gl_info->gl_renderer, "7900") ||
                    strstr(gl_info->gl_renderer, "7950") ||
                    strstr(gl_info->gl_renderer, "Quadro FX 4") ||
                    strstr(gl_info->gl_renderer, "Quadro FX 5"))
                        gl_info->gl_card = CARD_NVIDIA_GEFORCE_7800GT;
                else if(strstr(gl_info->gl_renderer, "6800") ||
                        strstr(gl_info->gl_renderer, "7600"))
                            gl_info->gl_card = CARD_NVIDIA_GEFORCE_6800;
                else if(strstr(gl_info->gl_renderer, "6600") ||
                        strstr(gl_info->gl_renderer, "6610") ||
                        strstr(gl_info->gl_renderer, "6700"))
                            gl_info->gl_card = CARD_NVIDIA_GEFORCE_6600GT;
                else
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE_6200; /* Geforce 6100/6150/6200/7300/7400 */
            } else if(WINE_D3D9_CAPABLE(gl_info)) {
                if (strstr(gl_info->gl_renderer, "5800") ||
                    strstr(gl_info->gl_renderer, "5900") ||
                    strstr(gl_info->gl_renderer, "5950") ||
                    strstr(gl_info->gl_renderer, "Quadro FX"))
                        gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5800;
                else if(strstr(gl_info->gl_renderer, "5600") ||
                        strstr(gl_info->gl_renderer, "5650") ||
                        strstr(gl_info->gl_renderer, "5700") ||
                        strstr(gl_info->gl_renderer, "5750"))
                            gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5600;
                else
                    gl_info->gl_card = CARD_NVIDIA_GEFORCEFX_5200; /* GeforceFX 5100/5200/5250/5300/5500 */
            } else if(WINE_D3D8_CAPABLE(gl_info)) {
                if (strstr(gl_info->gl_renderer, "GeForce4 Ti") || strstr(gl_info->gl_renderer, "Quadro4"))
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE4_TI4200; /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800, Quadro4 */
                else
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE3; /* Geforce3 standard/Ti200/Ti500, Quadro DCC */
            } else if(WINE_D3D7_CAPABLE(gl_info)) {
                if (strstr(gl_info->gl_renderer, "GeForce4 MX"))
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE4_MX; /* MX420/MX440/MX460/MX4000 */
                else if(strstr(gl_info->gl_renderer, "GeForce2 MX") || strstr(gl_info->gl_renderer, "Quadro2 MXR"))
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE2_MX; /* Geforce2 standard/MX100/MX200/MX400, Quadro2 MXR */
                else if(strstr(gl_info->gl_renderer, "GeForce2") || strstr(gl_info->gl_renderer, "Quadro2"))
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE2; /* Geforce2 GTS/Pro/Ti/Ultra, Quadro2 */
                else
                    gl_info->gl_card = CARD_NVIDIA_GEFORCE; /* Geforce 256/DDR, Quadro */
            } else {
                if (strstr(gl_info->gl_renderer, "TNT2"))
                    gl_info->gl_card = CARD_NVIDIA_RIVA_TNT2; /* Riva TNT2 standard/M64/Pro/Ultra */
                else
                    gl_info->gl_card = CARD_NVIDIA_RIVA_TNT; /* Riva TNT, Vanta */
            }
            break;
        case VENDOR_ATI:
            if(WINE_D3D9_CAPABLE(gl_info)) {
                /* Radeon R5xx */
                if (strstr(gl_info->gl_renderer, "X1600") ||
                    strstr(gl_info->gl_renderer, "X1800") ||
                    strstr(gl_info->gl_renderer, "X1900") ||
                    strstr(gl_info->gl_renderer, "X1950"))
                        gl_info->gl_card = CARD_ATI_RADEON_X1600;
                /* Radeon R4xx + X1300/X1400 (lowend R5xx) */
                else if(strstr(gl_info->gl_renderer, "X700") ||
                        strstr(gl_info->gl_renderer, "X800") ||
                        strstr(gl_info->gl_renderer, "X850") ||
                        strstr(gl_info->gl_renderer, "X1300") ||
                        strstr(gl_info->gl_renderer, "X1400"))
                            gl_info->gl_card = CARD_ATI_RADEON_X700;
                /* Radeon R3xx */ 
                else
                    gl_info->gl_card = CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
            } else if(WINE_D3D8_CAPABLE(gl_info)) {
                    gl_info->gl_card = CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
            } else if(WINE_D3D7_CAPABLE(gl_info)) {
                    gl_info->gl_card = CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
            } else
                gl_info->gl_card = CARD_ATI_RAGE_128PRO;
            break;
        case VENDOR_INTEL:
            if (strstr(gl_info->gl_renderer, "915GM")) {
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
    TRACE("FOUND (fake) card: 0x%x (vendor id), 0x%x (device id)\n", gl_info->gl_vendor, gl_info->gl_card);

    /* Load all the lookup tables
    TODO: It may be a good idea to make minLookup and maxLookup const and populate them in wined3d_private.h where they are declared */
    minLookup[WINELOOKUP_WARPPARAM] = WINED3DTADDRESS_WRAP;
    maxLookup[WINELOOKUP_WARPPARAM] = WINED3DTADDRESS_MIRRORONCE;

    minLookup[WINELOOKUP_MAGFILTER] = WINED3DTEXF_NONE;
    maxLookup[WINELOOKUP_MAGFILTER] = WINED3DTEXF_ANISOTROPIC;


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

    stateLookup[WINELOOKUP_MAGFILTER][WINED3DTEXF_NONE        - minLookup[WINELOOKUP_MAGFILTER]]  = GL_NEAREST;
    stateLookup[WINELOOKUP_MAGFILTER][WINED3DTEXF_POINT       - minLookup[WINELOOKUP_MAGFILTER]] = GL_NEAREST;
    stateLookup[WINELOOKUP_MAGFILTER][WINED3DTEXF_LINEAR      - minLookup[WINELOOKUP_MAGFILTER]] = GL_LINEAR;
    stateLookup[WINELOOKUP_MAGFILTER][WINED3DTEXF_ANISOTROPIC - minLookup[WINELOOKUP_MAGFILTER]] =
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
            }
        }
    }


    WineD3D_ReleaseFakeGLContext();
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

    if (Adapter >= IWineD3DImpl_GetAdapterCount(iface)) {
        return NULL;
    }

    TRACE_(d3d_caps)("(%p)->(%d)\n", This, Adapter);
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

            if (Format == WINED3DFMT_UNKNOWN)
            {
                switch (DevModeW.dmBitsPerPel)
                {
                    case 8:
                        pMode->Format = WINED3DFMT_P8;
                        break;
                    case 16:
                        pMode->Format = WINED3DFMT_R5G6B5;
                        break;
                    case 32:
                        pMode->Format = WINED3DFMT_X8R8G8B8;
                        break;
                    default:
                        pMode->Format = WINED3DFMT_UNKNOWN;
                        ERR("Unhandled bit depth (%u) in mode list!\n", DevModeW.dmBitsPerPel);
                }
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

        switch (bpp) {
        case  8: pMode->Format       = WINED3DFMT_R3G3B2;   break;
        case 16: pMode->Format       = WINED3DFMT_R5G6B5;   break;
        case 24: pMode->Format       = WINED3DFMT_X8R8G8B8; break; /* Robots needs 24bit to be X8R8G8B8 */
        case 32: pMode->Format       = WINED3DFMT_X8R8G8B8; break; /* EVE online and the Fur demo need 32bit AdapterDisplatMode to return X8R8G8B8 */
        default: pMode->Format       = WINED3DFMT_UNKNOWN;
        }

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
    /* Current Windows drivers have versions like 6.14.... (some older have an earlier version) */
    pIdentifier->DriverVersion->u.HighPart = MAKEDWORD_VERSION(6, 14);
    pIdentifier->DriverVersion->u.LowPart = Adapters[Adapter].gl_info.gl_driver_version;
    *(pIdentifier->VendorId) = Adapters[Adapter].gl_info.gl_vendor;
    *(pIdentifier->DeviceId) = Adapters[Adapter].gl_info.gl_card;
    *(pIdentifier->SubSysId) = 0;
    *(pIdentifier->Revision) = 0;

    /*FIXME: memcpy(&pIdentifier->DeviceIdentifier, ??, sizeof(??GUID)); */
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
}

static BOOL IWineD3DImpl_IsPixelFormatCompatibleWithDepthFmt(const WineD3D_PixelFormat *cfg, WINED3DFORMAT Format) {
    short depthSize, stencilSize;

    if(!cfg)
        return FALSE;

    if(!getDepthStencilBits(Format, &depthSize, &stencilSize)) {
        ERR("Unable to check compatibility for Format=%s\n", debug_d3dformat(Format));
        return FALSE;
    }

    if(cfg->depthSize < depthSize)
        return FALSE;

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
    TRACE_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, DevType:(%x,%s), SurfFmt:(%x,%s), Win?%d, MultiSamp:%x, pQual:%p)\n",
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

    /* TODO: Store in Adapter structure */
    if (pQualityLevels != NULL) {
        static int s_single_shot = 0;
        if (!s_single_shot) {
            FIXME("Quality levels unsupported at present\n");
            s_single_shot = 1;
        }
        *pQualityLevels = 1; /* Guess at a value! */
    }

    if (WINED3DMULTISAMPLE_NONE == MultiSampleType) return WINED3D_OK;
    return WINED3DERR_NOTAVAILABLE;
}

static HRESULT WINAPI IWineD3DImpl_CheckDeviceType(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE CheckType,
                                            WINED3DFORMAT DisplayFormat, WINED3DFORMAT BackBufferFormat, BOOL Windowed) {

    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    int nCfgs = 0;
    WineD3D_PixelFormat *cfgs;
    int it;
    HRESULT hr = WINED3DERR_NOTAVAILABLE;

    TRACE_(d3d_caps)("(%p)-> (STUB) (Adptr:%d, CheckType:(%x,%s), DispFmt:(%x,%s), BackBuf:(%x,%s), Win?%d): stub\n",
          This,
          Adapter,
          CheckType, debug_d3ddevicetype(CheckType),
          DisplayFormat, debug_d3dformat(DisplayFormat),
          BackBufferFormat, debug_d3dformat(BackBufferFormat),
          Windowed);

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        WARN_(d3d_caps)("Adapter >= IWineD3D_GetAdapterCount(iface), returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    cfgs = Adapters[Adapter].cfgs;
    nCfgs = Adapters[Adapter].nCfgs;
    for (it = 0; it < nCfgs; ++it) {
        if (IWineD3DImpl_IsPixelFormatCompatibleWithRenderFmt(&cfgs[it], DisplayFormat)) {
            hr = WINED3D_OK;
            TRACE_(d3d_caps)("OK\n");
            break ;
        }
    }

    if(hr != WINED3D_OK)
        ERR("unsupported format %s\n", debug_d3dformat(DisplayFormat));

    if(hr != WINED3D_OK)
        TRACE_(d3d_caps)("returning something different from WINED3D_OK\n");

    return hr;
}

#define GLINFO_LOCATION Adapters[Adapter].gl_info
static HRESULT WINAPI IWineD3DImpl_CheckDeviceFormat(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, 
                                              WINED3DFORMAT AdapterFormat, DWORD Usage, WINED3DRESOURCETYPE RType, WINED3DFORMAT CheckFormat) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
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

    if (Usage & WINED3DUSAGE_QUERY_FILTER) {
        switch (CheckFormat) {
            /* Filtering not supported */
            case WINED3DFMT_A32B32G32R32F:
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
            default:
                break;
        }
    }

    /* TODO: Check support against more of the WINED3DUSAGE_QUERY_* constants
     * See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/IDirect3D9__CheckDeviceFormat.asp
     * and http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/D3DUSAGE_QUERY.asp */
    if (Usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE) {
        if (!GL_LIMITS(vertex_samplers)) {
            TRACE_(d3d_caps)("[FAILED]\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        switch (CheckFormat) {
            case WINED3DFMT_A32B32G32R32F:
                if (!GL_SUPPORT(ARB_TEXTURE_FLOAT)) {
                    TRACE_(d3d_caps)("[FAILED]\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;

            default:
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
        }
    }

    if(Usage & WINED3DUSAGE_DEPTHSTENCIL) {
        switch (CheckFormat) {
            /* In theory we could do all formats, just fetch them accordingly should the buffer be locked.
             * Windows supports only those 3, and enumerating the other formats confuses applications
             */
            case WINED3DFMT_D24S8:
            case WINED3DFMT_D24X8:
            case WINED3DFMT_D16:
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;
            case WINED3DFMT_D16_LOCKABLE:
            case WINED3DFMT_D24FS8:
            case WINED3DFMT_D32F_LOCKABLE:
            case WINED3DFMT_D24X4S4:
            case WINED3DFMT_D15S1:
            case WINED3DFMT_D32:
                TRACE_(d3d_caps)("[FAILED]. Disabled because not enumerated on windows\n");
                return WINED3DERR_NOTAVAILABLE;
            default:
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
        }
    } else if(Usage & WINED3DUSAGE_RENDERTARGET) {
        switch (CheckFormat) {
            case WINED3DFMT_R8G8B8:
            case WINED3DFMT_A8R8G8B8:
            case WINED3DFMT_X8R8G8B8:
            case WINED3DFMT_R5G6B5:
            case WINED3DFMT_X1R5G5B5:
            case WINED3DFMT_A1R5G5B5:
            case WINED3DFMT_A4R4G4B4:
            case WINED3DFMT_R3G3B2:
            case WINED3DFMT_X4R4G4B4:
            case WINED3DFMT_A8B8G8R8:
            case WINED3DFMT_X8B8G8R8:
            case WINED3DFMT_P8:
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;
            case WINED3DFMT_R16F:
            case WINED3DFMT_A16B16G16R16F:
                if (!GL_SUPPORT(ARB_HALF_FLOAT_PIXEL) || !GL_SUPPORT(ARB_TEXTURE_FLOAT)) {
                    TRACE_(d3d_caps)("[FAILED]\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;
            case WINED3DFMT_A32B32G32R32F:
               if (!GL_SUPPORT(ARB_TEXTURE_FLOAT)) {
                    TRACE_(d3d_caps)("[FAILED]\n");
                    return WINED3DERR_NOTAVAILABLE;
                }
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;
            default:
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
        }
    } else if(Usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP) {
        if(GL_SUPPORT(NV_REGISTER_COMBINERS) && GL_SUPPORT(NV_TEXTURE_SHADER2)) {
            switch (CheckFormat) {
                case WINED3DFMT_V8U8:
                    TRACE_(d3d_caps)("[OK]\n");
                    return WINED3D_OK;
                /* TODO: Other bump map formats */
                default:
                    TRACE_(d3d_caps)("[FAILED]\n");
                    return WINED3DERR_NOTAVAILABLE;
            }
        }
        if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
            switch (CheckFormat) {
                case WINED3DFMT_V8U8:
                    TRACE_(d3d_caps)("[OK]\n");
                    return WINED3D_OK;
                default:
                    TRACE_(d3d_caps)("[FAILED]\n");
                    return WINED3DERR_NOTAVAILABLE;
            }
        }
        TRACE_(d3d_caps)("[FAILED]\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
        switch (CheckFormat) {
        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
          TRACE_(d3d_caps)("[OK]\n");
          return WINED3D_OK;
        default:
            break; /* Avoid compiler warnings */
        }
    }

    /* Check for supported sRGB formats (Texture loading and framebuffer) */
    if (GL_SUPPORT(EXT_TEXTURE_SRGB) && (Usage & WINED3DUSAGE_QUERY_SRGBREAD)) {
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
                return WINED3D_OK;

            default:
                TRACE_(d3d_caps)("[FAILED] Gamma texture format %s not supported.\n", debug_d3dformat(CheckFormat));
                return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (GL_SUPPORT(ARB_TEXTURE_FLOAT)) {

        BOOL half_pixel_support = GL_SUPPORT(ARB_HALF_FLOAT_PIXEL);

        switch (CheckFormat) {
            case WINED3DFMT_R16F:
            case WINED3DFMT_A16B16G16R16F:
                if (!half_pixel_support) break;
            case WINED3DFMT_R32F:
            case WINED3DFMT_A32B32G32R32F:
                TRACE_(d3d_caps)("[OK]\n");
                return WINED3D_OK;
            default:
                break; /* Avoid compiler warnings */
        }
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
        case WINED3DFMT_R3G3B2:
        case WINED3DFMT_A8:
        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_A8B8G8R8:
        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_A2B10G10R10:
            TRACE_(d3d_caps)("[OK]\n");
            return WINED3D_OK;

        /*****
         *  supported: Palettized
         */
        case WINED3DFMT_P8:
            TRACE_(d3d_caps)("[OK]\n");
            return WINED3D_OK;

        /*****
         *  Supported: (Alpha)-Luminance
         */
        case WINED3DFMT_L8:
        case WINED3DFMT_A8L8:
        case WINED3DFMT_A4L4:
            TRACE_(d3d_caps)("[OK]\n");
            return WINED3D_OK;

        /*****
         *  Not supported for now: Bump mapping formats
         *  Enable some because games often fail when they are not available
         *  and are still playable even without bump mapping
         */
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
        case WINED3DFMT_L6V5U5:
        case WINED3DFMT_X8L8V8U8:
        case WINED3DFMT_Q8W8V8U8:
        case WINED3DFMT_W11V11U10:
        case WINED3DFMT_A2W10V10U10:
            WARN_(d3d_caps)("[Not supported, but pretended to do]\n");
            return WINED3D_OK;

        /*****
         *  DXTN Formats: Handled above
         * WINED3DFMT_DXT1
         * WINED3DFMT_DXT2
         * WINED3DFMT_DXT3
         * WINED3DFMT_DXT4
         * WINED3DFMT_DXT5
         */

        /*****
         *  Odd formats - not supported
         */
        case WINED3DFMT_VERTEXDATA:
        case WINED3DFMT_INDEX16:
        case WINED3DFMT_INDEX32:
        case WINED3DFMT_Q16W16V16U16:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return WINED3DERR_NOTAVAILABLE;

        /*****
         *  Float formats: Not supported right now
         */
        case WINED3DFMT_G16R16F:
        case WINED3DFMT_G32R32F:
        case WINED3DFMT_CxV8U8:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return WINED3DERR_NOTAVAILABLE;

            /* Not supported */
        case WINED3DFMT_G16R16:
        case WINED3DFMT_A16B16G16R16:
        case WINED3DFMT_A8R3G3B2:
            TRACE_(d3d_caps)("[FAILED]\n"); /* Enable when implemented */
            return WINED3DERR_NOTAVAILABLE;

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
                return WINED3D_OK;
            } else {
                TRACE_(d3d_caps)("[FAILED]\n");
                return WINED3DERR_NOTAVAILABLE;
            }

        default:
            break;
    }

    TRACE_(d3d_caps)("[FAILED]\n");
    return WINED3DERR_NOTAVAILABLE;
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

/* Note: d3d8 passes in a pointer to a D3DCAPS8 structure, which is a true
      subset of a D3DCAPS9 structure. However, it has to come via a void *
      as the d3d8 interface cannot import the d3d9 header                  */
static HRESULT WINAPI IWineD3DImpl_GetDeviceCaps(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, WINED3DCAPS* pCaps) {

    IWineD3DImpl    *This = (IWineD3DImpl *)iface;
    int vs_selected_mode;
    int ps_selected_mode;

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
    *pCaps->DeviceType              = (DeviceType == WINED3DDEVTYPE_HAL) ? WINED3DDEVTYPE_HAL : WINED3DDEVTYPE_REF;  /* Not quite true, but use h/w supported by opengl I suppose */
    *pCaps->AdapterOrdinal          = Adapter;

    *pCaps->Caps                    = 0;
    *pCaps->Caps2                   = WINED3DCAPS2_CANRENDERWINDOWED |
                                      WINED3DCAPS2_FULLSCREENGAMMA |
                                      WINED3DCAPS2_DYNAMICTEXTURES;
    *pCaps->Caps3                   = WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD;
    *pCaps->PresentationIntervals   = WINED3DPRESENT_INTERVAL_IMMEDIATE;

    *pCaps->CursorCaps              = WINED3DCURSORCAPS_COLOR            |
                                      WINED3DCURSORCAPS_LOWRES;

    *pCaps->DevCaps                 = WINED3DDEVCAPS_FLOATTLVERTEX       |
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

    *pCaps->PrimitiveMiscCaps       = WINED3DPMISCCAPS_CULLNONE              |
                                      WINED3DPMISCCAPS_CULLCCW               |
                                      WINED3DPMISCCAPS_CULLCW                |
                                      WINED3DPMISCCAPS_COLORWRITEENABLE      |
                                      WINED3DPMISCCAPS_CLIPTLVERTS           |
                                      WINED3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                                      WINED3DPMISCCAPS_MASKZ                 |
                                      WINED3DPMISCCAPS_BLENDOP;
                                    /* TODO:
                                        WINED3DPMISCCAPS_NULLREFERENCE
                                        WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS
                                        WINED3DPMISCCAPS_FOGANDSPECULARALPHA
                                        WINED3DPMISCCAPS_SEPARATEALPHABLEND
                                        WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                                        WINED3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING
                                        WINED3DPMISCCAPS_FOGVERTEXCLAMPED */

/* The caps below can be supported but aren't handled yet in utils.c 'd3dta_to_combiner_input', disable them until support is fixed */
#if 0
    if (GL_SUPPORT(NV_REGISTER_COMBINERS))
        *pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_TSSARGTEMP;
    if (GL_SUPPORT(NV_REGISTER_COMBINERS2))
        *pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_PERSTAGECONSTANT;
#endif

    *pCaps->RasterCaps              = WINED3DPRASTERCAPS_DITHER    |
                                      WINED3DPRASTERCAPS_PAT       |
                                      WINED3DPRASTERCAPS_WFOG      |
                                      WINED3DPRASTERCAPS_ZFOG      |
                                      WINED3DPRASTERCAPS_FOGVERTEX |
                                      WINED3DPRASTERCAPS_FOGTABLE  |
                                      WINED3DPRASTERCAPS_FOGRANGE  |
                                      WINED3DPRASTERCAPS_STIPPLE   |
                                      WINED3DPRASTERCAPS_SUBPIXEL  |
                                      WINED3DPRASTERCAPS_ZTEST     |
                                      WINED3DPRASTERCAPS_SCISSORTEST   |
                                      WINED3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |
                                      WINED3DPRASTERCAPS_DEPTHBIAS;

    if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
      *pCaps->RasterCaps |= WINED3DPRASTERCAPS_ANISOTROPY    |
                            WINED3DPRASTERCAPS_ZBIAS         |
                            WINED3DPRASTERCAPS_MIPMAPLODBIAS;
    }
                        /* FIXME Add:
			   WINED3DPRASTERCAPS_COLORPERSPECTIVE
			   WINED3DPRASTERCAPS_STRETCHBLTMULTISAMPLE
			   WINED3DPRASTERCAPS_ANTIALIASEDGES
			   WINED3DPRASTERCAPS_ZBUFFERLESSHSR
			   WINED3DPRASTERCAPS_WBUFFER */

    *pCaps->ZCmpCaps = WINED3DPCMPCAPS_ALWAYS       |
                       WINED3DPCMPCAPS_EQUAL        |
                       WINED3DPCMPCAPS_GREATER      |
                       WINED3DPCMPCAPS_GREATEREQUAL |
                       WINED3DPCMPCAPS_LESS         |
                       WINED3DPCMPCAPS_LESSEQUAL    |
                       WINED3DPCMPCAPS_NEVER        |
                       WINED3DPCMPCAPS_NOTEQUAL;

    *pCaps->SrcBlendCaps  = WINED3DPBLENDCAPS_BLENDFACTOR     |
                            WINED3DPBLENDCAPS_BOTHINVSRCALPHA |
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

    *pCaps->DestBlendCaps = WINED3DPBLENDCAPS_BLENDFACTOR     |
                            WINED3DPBLENDCAPS_BOTHINVSRCALPHA |
                            WINED3DPBLENDCAPS_BOTHSRCALPHA    |
                            WINED3DPBLENDCAPS_DESTALPHA       |
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
     */

    *pCaps->AlphaCmpCaps = WINED3DPCMPCAPS_ALWAYS       |
                           WINED3DPCMPCAPS_EQUAL        |
                           WINED3DPCMPCAPS_GREATER      |
                           WINED3DPCMPCAPS_GREATEREQUAL |
                           WINED3DPCMPCAPS_LESS         |
                           WINED3DPCMPCAPS_LESSEQUAL    |
                           WINED3DPCMPCAPS_NEVER        |
                           WINED3DPCMPCAPS_NOTEQUAL;

    *pCaps->ShadeCaps     = WINED3DPSHADECAPS_SPECULARGOURAUDRGB |
                            WINED3DPSHADECAPS_COLORGOURAUDRGB    |
                            WINED3DPSHADECAPS_ALPHAFLATBLEND     |
                            WINED3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                            WINED3DPSHADECAPS_COLORFLATRGB       |
                            WINED3DPSHADECAPS_FOGFLAT            |
                            WINED3DPSHADECAPS_FOGGOURAUD         |
                            WINED3DPSHADECAPS_SPECULARFLATRGB;

    *pCaps->TextureCaps =  WINED3DPTEXTURECAPS_ALPHA              |
                           WINED3DPTEXTURECAPS_ALPHAPALETTE       |
                           WINED3DPTEXTURECAPS_BORDER             |
                           WINED3DPTEXTURECAPS_MIPMAP             |
                           WINED3DPTEXTURECAPS_PROJECTED          |
                           WINED3DPTEXTURECAPS_PERSPECTIVE        |
                           WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;

    if( GL_SUPPORT(EXT_TEXTURE3D)) {
        *pCaps->TextureCaps |=  WINED3DPTEXTURECAPS_VOLUMEMAP      |
                                WINED3DPTEXTURECAPS_MIPVOLUMEMAP   |
                                WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;
    }

    if (GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        *pCaps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP     |
                             WINED3DPTEXTURECAPS_MIPCUBEMAP    |
                             WINED3DPTEXTURECAPS_CUBEMAP_POW2;

    }

    *pCaps->TextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
        *pCaps->TextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                     WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        *pCaps->CubeTextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
            *pCaps->CubeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                            WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    } else
        *pCaps->CubeTextureFilterCaps = 0;

    if (GL_SUPPORT(EXT_TEXTURE3D)) {
        *pCaps->VolumeTextureFilterCaps = WINED3DPTFILTERCAPS_MAGFLINEAR       |
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
        *pCaps->VolumeTextureFilterCaps = 0;

    *pCaps->TextureAddressCaps =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                  WINED3DPTADDRESSCAPS_CLAMP  |
                                  WINED3DPTADDRESSCAPS_WRAP;

    if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
        *pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
    }
    if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
        *pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
    }
    if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
        *pCaps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
    }

    if (GL_SUPPORT(EXT_TEXTURE3D)) {
        *pCaps->VolumeTextureAddressCaps =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                            WINED3DPTADDRESSCAPS_CLAMP  |
                                            WINED3DPTADDRESSCAPS_WRAP;
        if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
            *pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
        }
        if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
            *pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
        }
        if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
            *pCaps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
        }
    } else
        *pCaps->VolumeTextureAddressCaps = 0;

    *pCaps->LineCaps = WINED3DLINECAPS_TEXTURE |
                       WINED3DLINECAPS_ZTEST;
                      /* FIXME: Add
                        WINED3DLINECAPS_BLEND
                        WINED3DLINECAPS_ALPHACMP
                        WINED3DLINECAPS_FOG */

    *pCaps->MaxTextureWidth  = GL_LIMITS(texture_size);
    *pCaps->MaxTextureHeight = GL_LIMITS(texture_size);

    if(GL_SUPPORT(EXT_TEXTURE3D))
        *pCaps->MaxVolumeExtent = GL_LIMITS(texture3d_size);
    else
        *pCaps->MaxVolumeExtent = 0;

    *pCaps->MaxTextureRepeat = 32768;
    *pCaps->MaxTextureAspectRatio = GL_LIMITS(texture_size);
    *pCaps->MaxVertexW = 1.0;

    *pCaps->GuardBandLeft = 0;
    *pCaps->GuardBandTop = 0;
    *pCaps->GuardBandRight = 0;
    *pCaps->GuardBandBottom = 0;

    *pCaps->ExtentsAdjust = 0;

    *pCaps->StencilCaps =  WINED3DSTENCILCAPS_DECRSAT |
                           WINED3DSTENCILCAPS_INCRSAT |
                           WINED3DSTENCILCAPS_INVERT  |
                           WINED3DSTENCILCAPS_KEEP    |
                           WINED3DSTENCILCAPS_REPLACE |
                           WINED3DSTENCILCAPS_ZERO;
    if (GL_SUPPORT(EXT_STENCIL_WRAP)) {
      *pCaps->StencilCaps |= WINED3DSTENCILCAPS_DECR  |
                             WINED3DSTENCILCAPS_INCR;
    }
    if ( This->dxVersion > 8 &&
        ( GL_SUPPORT(EXT_STENCIL_TWO_SIDE) ||
            GL_SUPPORT(ATI_SEPARATE_STENCIL) ) ) {
        *pCaps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    *pCaps->FVFCaps = WINED3DFVFCAPS_PSIZE | 0x0008; /* 8 texture coords */

    *pCaps->TextureOpCaps =  WINED3DTEXOPCAPS_ADD         |
                             WINED3DTEXOPCAPS_ADDSIGNED   |
                             WINED3DTEXOPCAPS_ADDSIGNED2X |
                             WINED3DTEXOPCAPS_MODULATE    |
                             WINED3DTEXOPCAPS_MODULATE2X  |
                             WINED3DTEXOPCAPS_MODULATE4X  |
                             WINED3DTEXOPCAPS_SELECTARG1  |
                             WINED3DTEXOPCAPS_SELECTARG2  |
                             WINED3DTEXOPCAPS_DISABLE;

    if (GL_SUPPORT(ARB_TEXTURE_ENV_COMBINE) ||
        GL_SUPPORT(EXT_TEXTURE_ENV_COMBINE) ||
        GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA |
                                WINED3DTEXOPCAPS_BLENDTEXTUREALPHA  |
                                WINED3DTEXOPCAPS_BLENDFACTORALPHA   |
                                WINED3DTEXOPCAPS_BLENDCURRENTALPHA  |
                                WINED3DTEXOPCAPS_LERP               |
                                WINED3DTEXOPCAPS_SUBTRACT;
    }
    if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3) ||
         GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_ADDSMOOTH             |
                                WINED3DTEXOPCAPS_MULTIPLYADD            |
                                WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR |
                                WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
                                WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM;
    }
    if (GL_SUPPORT(ARB_TEXTURE_ENV_DOT3))
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_DOTPRODUCT3;

    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
                                 WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA;
    }

    if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAP;
    } else if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        /* Bump mapping is supported already in NV_TEXTURE_SHADER, but that extension does
         * not support 3D textures. This asks for trouble if an app uses both bump mapping
         * and 3D textures. It also allows us to keep the code simpler by having texture
         * shaders constantly enabled.
         */
        *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAP;
        /* TODO: Luminance bump map? */
    }
#if 0
    /* FIXME: Add
    *pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE
                             WINED3DTEXOPCAPS_PREMODULATE */
#endif

    *pCaps->MaxTextureBlendStages   = GL_LIMITS(texture_stages);
    *pCaps->MaxSimultaneousTextures = GL_LIMITS(textures);
    *pCaps->MaxUserClipPlanes       = GL_LIMITS(clipplanes);
    *pCaps->MaxActiveLights         = GL_LIMITS(lights);

    *pCaps->MaxVertexBlendMatrices      = GL_LIMITS(blends);
    *pCaps->MaxVertexBlendMatrixIndex   = 0;

    *pCaps->MaxAnisotropy   = GL_LIMITS(anisotropy);
    *pCaps->MaxPointSize    = GL_LIMITS(pointsize);


    *pCaps->VertexProcessingCaps = WINED3DVTXPCAPS_DIRECTIONALLIGHTS |
                                   WINED3DVTXPCAPS_MATERIALSOURCE7   |
                                   WINED3DVTXPCAPS_POSITIONALLIGHTS  |
                                   WINED3DVTXPCAPS_LOCALVIEWER       |
                                   WINED3DVTXPCAPS_VERTEXFOG         |
                                   WINED3DVTXPCAPS_TEXGEN;
                                  /* FIXME: Add 
                                     D3DVTXPCAPS_TWEENING, D3DVTXPCAPS_TEXGEN_SPHEREMAP */

    *pCaps->MaxPrimitiveCount   = 0xFFFFF; /* For now set 2^20-1 which is used by most >=Geforce3/Radeon8500 cards */
    *pCaps->MaxVertexIndex      = 0xFFFFF;
    *pCaps->MaxStreams          = MAX_STREAMS;
    *pCaps->MaxStreamStride     = 1024;

    if (vs_selected_mode == SHADER_GLSL) {
        /* Nvidia Geforce6/7 or Ati R4xx/R5xx cards with GLSL support, support VS 3.0 but older Nvidia/Ati
           models with GLSL support only support 2.0. In case of nvidia we can detect VS 2.0 support using
           vs_nv_version which is based on NV_vertex_program. For Ati cards there's no easy way, so for
           now only support 2.0/3.0 detection on Nvidia GeforceFX cards and default to 3.0 for everything else */
        if(GLINFO_LOCATION.vs_nv_version == VS_VERSION_20)
            *pCaps->VertexShaderVersion = WINED3DVS_VERSION(2,0);
        else
            *pCaps->VertexShaderVersion = WINED3DVS_VERSION(3,0);
        TRACE_(d3d_caps)("Hardware vertex shader version 3.0 enabled (GLSL)\n");
    } else if (vs_selected_mode == SHADER_ARB) {
        *pCaps->VertexShaderVersion = WINED3DVS_VERSION(1,1);
        TRACE_(d3d_caps)("Hardware vertex shader version 1.1 enabled (ARB_PROGRAM)\n");
    } else {
        *pCaps->VertexShaderVersion  = 0;
        TRACE_(d3d_caps)("Vertex shader functionality not available\n");
    }

    *pCaps->MaxVertexShaderConst = GL_LIMITS(vshader_constantsF);

    if (ps_selected_mode == SHADER_GLSL) {
        /* See the comment about VS2.0/VS3.0 detection as we do the same here but then based on NV_fragment_program
           in case of GeforceFX cards. */
        if(GLINFO_LOCATION.ps_nv_version == PS_VERSION_20)
            *pCaps->PixelShaderVersion = WINED3DPS_VERSION(2,0);
        else
            *pCaps->PixelShaderVersion = WINED3DPS_VERSION(3,0);
        /* FIXME: The following line is card dependent. -1.0 to 1.0 is a safe default clamp range for now */
        *pCaps->PixelShader1xMaxValue = 1.0;
        TRACE_(d3d_caps)("Hardware pixel shader version 3.0 enabled (GLSL)\n");
    } else if (ps_selected_mode == SHADER_ARB) {
        *pCaps->PixelShaderVersion    = WINED3DPS_VERSION(1,4);
        *pCaps->PixelShader1xMaxValue = 1.0;
        TRACE_(d3d_caps)("Hardware pixel shader version 1.4 enabled (ARB_PROGRAM)\n");
    } else {
        *pCaps->PixelShaderVersion    = 0;
        *pCaps->PixelShader1xMaxValue = 0.0;
        TRACE_(d3d_caps)("Pixel shader functionality not available\n");
    }

    /* ------------------------------------------------
       The following fields apply to d3d9 only
       ------------------------------------------------ */
    if (This->dxVersion > 8) {
        /* d3d9.dll sets D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES here because StretchRects is implemented in d3d9 */
        *pCaps->DevCaps2                          = WINED3DDEVCAPS2_STREAMOFFSET;
        /* TODO: VS3.0 needs at least D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET */
        *pCaps->MaxNpatchTessellationLevel        = 0;
        *pCaps->MasterAdapterOrdinal              = 0;
        *pCaps->AdapterOrdinalInGroup             = 0;
        *pCaps->NumberOfAdaptersInGroup           = 1;

        if(*pCaps->VertexShaderVersion >= WINED3DVS_VERSION(2,0)) {
            /* OpenGL supports all the formats below, perhaps not always
             * without conversion, but it supports them.
             * Further GLSL doesn't seem to have an official unsigned type so
             * don't advertise it yet as I'm not sure how we handle it.
             * We might need to add some clamping in the shader engine to
             * support it.
             * TODO: WINED3DDTCAPS_USHORT2N, WINED3DDTCAPS_USHORT4N, WINED3DDTCAPS_UDEC3, WINED3DDTCAPS_DEC3N */
            *pCaps->DeclTypes = WINED3DDTCAPS_UBYTE4    |
                                WINED3DDTCAPS_UBYTE4N   |
                                WINED3DDTCAPS_SHORT2N   |
                                WINED3DDTCAPS_SHORT4N;
            if (GL_SUPPORT(NV_HALF_FLOAT)) {
                *pCaps->DeclTypes |=
                                WINED3DDTCAPS_FLOAT16_2 |
                                WINED3DDTCAPS_FLOAT16_4;
            }
        } else
            *pCaps->DeclTypes                         = 0;

        *pCaps->NumSimultaneousRTs = GL_LIMITS(buffers);

            
        *pCaps->StretchRectFilterCaps             = WINED3DPTFILTERCAPS_MINFPOINT  |
                                                    WINED3DPTFILTERCAPS_MAGFPOINT  |
                                                    WINED3DPTFILTERCAPS_MINFLINEAR |
                                                    WINED3DPTFILTERCAPS_MAGFLINEAR;
        *pCaps->VertexTextureFilterCaps           = 0;
        
        if(*pCaps->VertexShaderVersion == WINED3DVS_VERSION(3,0)) {
            /* Where possible set the caps based on OpenGL extensions and if they aren't set (in case of software rendering)
               use the VS 3.0 from MSDN or else if there's OpenGL spec use a hardcoded value minimum VS3.0 value. */
            *pCaps->VS20Caps.Caps                     = WINED3DVS20CAPS_PREDICATION;
            *pCaps->VS20Caps.DynamicFlowControlDepth  = WINED3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH; /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
            *pCaps->VS20Caps.NumTemps                 = max(32, GLINFO_LOCATION.vs_arb_max_temps);
            *pCaps->VS20Caps.StaticFlowControlDepth   = WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH ; /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */

            *pCaps->MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
            *pCaps->MaxVertexShader30InstructionSlots = max(512, GLINFO_LOCATION.vs_arb_max_instructions);
        } else if(*pCaps->VertexShaderVersion == WINED3DVS_VERSION(2,0)) {
            *pCaps->VS20Caps.Caps                     = 0;
            *pCaps->VS20Caps.DynamicFlowControlDepth  = WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
            *pCaps->VS20Caps.NumTemps                 = max(12, GLINFO_LOCATION.vs_arb_max_temps);
            *pCaps->VS20Caps.StaticFlowControlDepth   = 1;    

            *pCaps->MaxVShaderInstructionsExecuted    = 65535;
            *pCaps->MaxVertexShader30InstructionSlots = 0;
        } else { /* VS 1.x */
            *pCaps->VS20Caps.Caps                     = 0;
            *pCaps->VS20Caps.DynamicFlowControlDepth  = 0;
            *pCaps->VS20Caps.NumTemps                 = 0;
            *pCaps->VS20Caps.StaticFlowControlDepth   = 0;    

            *pCaps->MaxVShaderInstructionsExecuted    = 0;
            *pCaps->MaxVertexShader30InstructionSlots = 0;        
        }

        if(*pCaps->PixelShaderVersion == WINED3DPS_VERSION(3,0)) {
            /* Where possible set the caps based on OpenGL extensions and if they aren't set (in case of software rendering)
               use the PS 3.0 from MSDN or else if there's OpenGL spec use a hardcoded value minimum PS 3.0 value. */
            
            /* Caps is more or less undocumented on MSDN but it appears to be used for PS20Caps based on results from R9600/FX5900/Geforce6800 cards from Windows */
            *pCaps->PS20Caps.Caps                     = WINED3DPS20CAPS_ARBITRARYSWIZZLE     |
                                                        WINED3DPS20CAPS_GRADIENTINSTRUCTIONS |
                                                        WINED3DPS20CAPS_PREDICATION          |
                                                        WINED3DPS20CAPS_NODEPENDENTREADLIMIT |
                                                        WINED3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
            *pCaps->PS20Caps.DynamicFlowControlDepth  = WINED3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH; /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
            *pCaps->PS20Caps.NumTemps                 = max(32, GLINFO_LOCATION.ps_arb_max_temps);
            *pCaps->PS20Caps.StaticFlowControlDepth   = WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH; /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
            *pCaps->PS20Caps.NumInstructionSlots      = WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS; /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */

            *pCaps->MaxPShaderInstructionsExecuted    = 65535;
            *pCaps->MaxPixelShader30InstructionSlots  = max(WINED3DMIN30SHADERINSTRUCTIONS, GLINFO_LOCATION.ps_arb_max_instructions);
        } else if(*pCaps->PixelShaderVersion == WINED3DPS_VERSION(2,0)) {
            /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
            *pCaps->PS20Caps.Caps                     = 0;
            *pCaps->PS20Caps.DynamicFlowControlDepth  = 0; /* WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
            *pCaps->PS20Caps.NumTemps                 = max(12, GLINFO_LOCATION.ps_arb_max_temps);
            *pCaps->PS20Caps.StaticFlowControlDepth   = WINED3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minumum: 1 */
            *pCaps->PS20Caps.NumInstructionSlots      = WINED3DPS20_MIN_NUMINSTRUCTIONSLOTS; /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */

            *pCaps->MaxPShaderInstructionsExecuted    = 512; /* Minimum value, a GeforceFX uses 1024 */
            *pCaps->MaxPixelShader30InstructionSlots  = 0;
        } else { /* PS 1.x */
            *pCaps->PS20Caps.Caps                     = 0;
            *pCaps->PS20Caps.DynamicFlowControlDepth  = 0;
            *pCaps->PS20Caps.NumTemps                 = 0;
            *pCaps->PS20Caps.StaticFlowControlDepth   = 0;
            *pCaps->PS20Caps.NumInstructionSlots      = 0;

            *pCaps->MaxPShaderInstructionsExecuted    = 0;
            *pCaps->MaxPixelShader30InstructionSlots  = 0;
        }
    }

    return WINED3D_OK;
}

static unsigned int glsl_program_key_hash(void *key) {
    glsl_program_key_t *k = (glsl_program_key_t *)key;

    unsigned int hash = k->vshader | k->pshader << 16;
    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    hash ^=  (hash >> 16);

    return hash;
}

static BOOL glsl_program_key_compare(void *keya, void *keyb) {
    glsl_program_key_t *ka = (glsl_program_key_t *)keya;
    glsl_program_key_t *kb = (glsl_program_key_t *)keyb;

    return ka->vshader == kb->vshader && ka->pshader == kb->pshader;
}

/* Note due to structure differences between dx8 and dx9 D3DPRESENT_PARAMETERS,
   and fields being inserted in the middle, a new structure is used in place    */
static HRESULT  WINAPI IWineD3DImpl_CreateDevice(IWineD3D *iface, UINT Adapter, WINED3DDEVTYPE DeviceType, HWND hFocusWindow,
                                           DWORD BehaviourFlags, IWineD3DDevice** ppReturnedDeviceInterface,
                                           IUnknown *parent) {

    IWineD3DDeviceImpl *object  = NULL;
    IWineD3DImpl       *This    = (IWineD3DImpl *)iface;
    HDC hDC;
    int i;

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

    if(This->dxVersion == 7) {
        object->surface_alignment = 8;
    } else {
        object->surface_alignment = 4;
    }

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
    if (object->ps_selected_mode == SHADER_GLSL || object->vs_selected_mode == SHADER_GLSL) {
        object->shader_backend = &glsl_shader_backend;
        object->glsl_program_lookup = hash_table_create(&glsl_program_key_hash, &glsl_program_key_compare);
    } else if (object->ps_selected_mode == SHADER_ARB || object->vs_selected_mode == SHADER_ARB) {
        object->shader_backend = &arb_program_shader_backend;
    } else {
        object->shader_backend = &none_shader_backend;
    }

    /* set the state of the device to valid */
    object->state = WINED3D_OK;

    /* Get the initial screen setup for ddraw */
    object->ddraw_width = GetSystemMetrics(SM_CXSCREEN);
    object->ddraw_height = GetSystemMetrics(SM_CYSCREEN);
    hDC = GetDC(0);
    object->ddraw_format = pixelformat_for_depth(GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES));
    ReleaseDC(0, hDC);

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

#define PUSH1(att)        attribs[nAttribs++] = (att);
#define GLINFO_LOCATION (Adapters[0].gl_info)
BOOL InitAdapters(void) {
    BOOL ret;
    int ps_selected_mode, vs_selected_mode;

    /* No need to hold any lock. The calling library makes sure only one thread calls
     * wined3d simultaneously
     */
    if(numAdapters > 0) return TRUE;

    TRACE("Initializing adapters\n");
    /* For now only one default adapter */
    {
        int attribute;
        DISPLAY_DEVICEW DisplayDevice;

        TRACE("Initializing default adapter\n");
        Adapters[0].monitorPoint.x = -1;
        Adapters[0].monitorPoint.y = -1;

        ret = IWineD3DImpl_FillGLCaps(&Adapters[0].gl_info);
        if(!ret) {
            ERR("Failed to initialize gl caps for default adapter\n");
            HeapFree(GetProcessHeap(), 0, Adapters);
            return FALSE;
        }
        ret = initPixelFormats(&Adapters[0].gl_info);
        if(!ret) {
            ERR("Failed to init gl formats\n");
            HeapFree(GetProcessHeap(), 0, Adapters);
            return FALSE;
        }

        Adapters[0].driver = "Display";
        Adapters[0].description = "Direct3D HAL";

        /* Initialize the Adapter's DeviceName which is required for ChangeDisplaySettings and friends */
        DisplayDevice.cb = sizeof(DisplayDevice);
        EnumDisplayDevicesW(NULL, 0 /* Adapter 0 = iDevNum 0 */, &DisplayDevice, 0);
        TRACE("DeviceName: %s\n", debugstr_w(DisplayDevice.DeviceName));
        strcpyW(Adapters[0].DeviceName, DisplayDevice.DeviceName);

        if (WineD3D_CreateFakeGLContext()) {
            int iPixelFormat;
            int attribs[8];
            int values[8];
            int nAttribs = 0;
            int res;
            WineD3D_PixelFormat *cfgs;

            attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
            GL_EXTCALL(wglGetPixelFormatAttribivARB(wined3d_fake_gl_context_hdc, 0, 0, 1, &attribute, &Adapters[0].nCfgs));

            Adapters[0].cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Adapters[0].nCfgs *sizeof(WineD3D_PixelFormat));
            cfgs = Adapters[0].cfgs;
            PUSH1(WGL_RED_BITS_ARB)
            PUSH1(WGL_GREEN_BITS_ARB)
            PUSH1(WGL_BLUE_BITS_ARB)
            PUSH1(WGL_ALPHA_BITS_ARB)
            PUSH1(WGL_DEPTH_BITS_ARB)
            PUSH1(WGL_STENCIL_BITS_ARB)

            for(iPixelFormat=1; iPixelFormat<=Adapters[0].nCfgs; iPixelFormat++) {
                res = GL_EXTCALL(wglGetPixelFormatAttribivARB(wined3d_fake_gl_context_hdc, iPixelFormat, 0, nAttribs, attribs, values));

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

                TRACE("iPixelFormat=%d, RGBA=%d/%d/%d/%d, depth=%d, stencil=%d\n", cfgs->iPixelFormat, cfgs->redSize, cfgs->greenSize, cfgs->blueSize, cfgs->alphaSize, cfgs->depthSize, cfgs->stencilSize);
                cfgs++;
            }
            WineD3D_ReleaseFakeGLContext();
        }

        select_shader_mode(&Adapters[0].gl_info, WINED3DDEVTYPE_HAL, &ps_selected_mode, &vs_selected_mode);
        select_shader_max_constants(ps_selected_mode, vs_selected_mode, &Adapters[0].gl_info);

    }
    numAdapters = 1;
    TRACE("%d adapters successfully initialized\n", numAdapters);

    return TRUE;
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
