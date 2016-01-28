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

#include "wined3d_private.h"
#include <winternl.h>

#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINE_DEFAULT_VIDMEM (64 * 1024 * 1024)
#define DEFAULT_REFRESH_RATE 0

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
    DRIVER_NVIDIA_GEFORCE8,
    DRIVER_VMWARE,
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
    GL_VENDOR_MESA,
    GL_VENDOR_NVIDIA,
};

enum wined3d_d3d_level
{
    WINED3D_D3D_LEVEL_5,
    WINED3D_D3D_LEVEL_6,
    WINED3D_D3D_LEVEL_7,
    WINED3D_D3D_LEVEL_8,
    WINED3D_D3D_LEVEL_9_SM2,
    WINED3D_D3D_LEVEL_9_SM3,
    WINED3D_D3D_LEVEL_10,
    WINED3D_D3D_LEVEL_11,
    WINED3D_D3D_LEVEL_COUNT
};

/* The d3d device ID */
static const GUID IID_D3DDEVICE_D3DUID = { 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };

/* Extension detection */
struct wined3d_extension_map
{
    const char *extension_string;
    enum wined3d_gl_extension extension;
};

static const struct wined3d_extension_map gl_extension_map[] =
{
    /* APPLE */
    {"GL_APPLE_client_storage",             APPLE_CLIENT_STORAGE          },
    {"GL_APPLE_fence",                      APPLE_FENCE                   },
    {"GL_APPLE_float_pixels",               APPLE_FLOAT_PIXELS            },
    {"GL_APPLE_flush_buffer_range",         APPLE_FLUSH_BUFFER_RANGE      },
    {"GL_APPLE_ycbcr_422",                  APPLE_YCBCR_422               },

    /* ARB */
    {"GL_ARB_blend_func_extended",          ARB_BLEND_FUNC_EXTENDED       },
    {"GL_ARB_color_buffer_float",           ARB_COLOR_BUFFER_FLOAT        },
    {"GL_ARB_debug_output",                 ARB_DEBUG_OUTPUT              },
    {"GL_ARB_depth_buffer_float",           ARB_DEPTH_BUFFER_FLOAT        },
    {"GL_ARB_depth_texture",                ARB_DEPTH_TEXTURE             },
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS              },
    {"GL_ARB_draw_elements_base_vertex",    ARB_DRAW_ELEMENTS_BASE_VERTEX },
    {"GL_ARB_draw_instanced",               ARB_DRAW_INSTANCED            },
    {"GL_ARB_ES2_compatibility",            ARB_ES2_COMPATIBILITY         },
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM          },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER           },
    {"GL_ARB_framebuffer_object",           ARB_FRAMEBUFFER_OBJECT        },
    {"GL_ARB_framebuffer_sRGB",             ARB_FRAMEBUFFER_SRGB          },
    {"GL_ARB_geometry_shader4",             ARB_GEOMETRY_SHADER4          },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL          },
    {"GL_ARB_half_float_vertex",            ARB_HALF_FLOAT_VERTEX         },
    {"GL_ARB_instanced_arrays",             ARB_INSTANCED_ARRAYS,         },
    {"GL_ARB_internalformat_query2",        ARB_INTERNALFORMAT_QUERY2,    },
    {"GL_ARB_map_buffer_alignment",         ARB_MAP_BUFFER_ALIGNMENT      },
    {"GL_ARB_map_buffer_range",             ARB_MAP_BUFFER_RANGE          },
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE               },
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE              },
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY           },
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT       },
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS          },
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE              },
    {"GL_ARB_provoking_vertex",             ARB_PROVOKING_VERTEX          },
    {"GL_ARB_sampler_objects",              ARB_SAMPLER_OBJECTS           },
    {"GL_ARB_shader_bit_encoding",          ARB_SHADER_BIT_ENCODING       },
    {"GL_ARB_shader_texture_lod",           ARB_SHADER_TEXTURE_LOD        },
    {"GL_ARB_shading_language_100",         ARB_SHADING_LANGUAGE_100      },
    {"GL_ARB_shadow",                       ARB_SHADOW                    },
    {"GL_ARB_sync",                         ARB_SYNC                      },
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP      },
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION       },
    {"GL_ARB_texture_compression_rgtc",     ARB_TEXTURE_COMPRESSION_RGTC  },
    {"GL_ARB_texture_cube_map",             ARB_TEXTURE_CUBE_MAP          },
    {"GL_ARB_texture_env_combine",          ARB_TEXTURE_ENV_COMBINE       },
    {"GL_ARB_texture_env_dot3",             ARB_TEXTURE_ENV_DOT3          },
    {"GL_ARB_texture_float",                ARB_TEXTURE_FLOAT             },
    {"GL_ARB_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT   },
    {"GL_ARB_texture_mirror_clamp_to_edge", ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE},
    {"GL_ARB_texture_non_power_of_two",     ARB_TEXTURE_NON_POWER_OF_TWO  },
    {"GL_ARB_texture_rectangle",            ARB_TEXTURE_RECTANGLE         },
    {"GL_ARB_texture_rg",                   ARB_TEXTURE_RG                },
    {"GL_ARB_timer_query",                  ARB_TIMER_QUERY               },
    {"GL_ARB_uniform_buffer_object",        ARB_UNIFORM_BUFFER_OBJECT     },
    {"GL_ARB_vertex_array_bgra",            ARB_VERTEX_ARRAY_BGRA         },
    {"GL_ARB_vertex_blend",                 ARB_VERTEX_BLEND              },
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT      },
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM            },
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER             },

    /* ATI */
    {"GL_ATI_fragment_shader",              ATI_FRAGMENT_SHADER           },
    {"GL_ATI_separate_stencil",             ATI_SEPARATE_STENCIL          },
    {"GL_ATI_texture_compression_3dc",      ATI_TEXTURE_COMPRESSION_3DC   },
    {"GL_ATI_texture_env_combine3",         ATI_TEXTURE_ENV_COMBINE3      },
    {"GL_ATI_texture_mirror_once",          ATI_TEXTURE_MIRROR_ONCE       },

    /* EXT */
    {"GL_EXT_blend_color",                  EXT_BLEND_COLOR               },
    {"GL_EXT_blend_equation_separate",      EXT_BLEND_EQUATION_SEPARATE   },
    {"GL_EXT_blend_func_separate",          EXT_BLEND_FUNC_SEPARATE       },
    {"GL_EXT_blend_minmax",                 EXT_BLEND_MINMAX              },
    {"GL_EXT_blend_subtract",               EXT_BLEND_SUBTRACT            },
    {"GL_EXT_depth_bounds_test",            EXT_DEPTH_BOUNDS_TEST         },
    {"GL_EXT_draw_buffers2",                EXT_DRAW_BUFFERS2             },
    {"GL_EXT_fog_coord",                    EXT_FOG_COORD                 },
    {"GL_EXT_framebuffer_blit",             EXT_FRAMEBUFFER_BLIT          },
    {"GL_EXT_framebuffer_multisample",      EXT_FRAMEBUFFER_MULTISAMPLE   },
    {"GL_EXT_framebuffer_object",           EXT_FRAMEBUFFER_OBJECT        },
    {"GL_EXT_gpu_program_parameters",       EXT_GPU_PROGRAM_PARAMETERS    },
    {"GL_EXT_gpu_shader4",                  EXT_GPU_SHADER4               },
    {"GL_EXT_packed_depth_stencil",         EXT_PACKED_DEPTH_STENCIL      },
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS          },
    {"GL_EXT_provoking_vertex",             EXT_PROVOKING_VERTEX          },
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR           },
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE          },
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP              },
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D                 },
    {"GL_EXT_texture_compression_rgtc",     EXT_TEXTURE_COMPRESSION_RGTC  },
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC  },
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE       },
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3          },
    {"GL_EXT_texture_filter_anisotropic",   EXT_TEXTURE_FILTER_ANISOTROPIC},
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS          },
    {"GL_EXT_texture_mirror_clamp",         EXT_TEXTURE_MIRROR_CLAMP      },
    {"GL_EXT_texture_snorm",                EXT_TEXTURE_SNORM             },
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB              },
    {"GL_EXT_texture_sRGB_decode",          EXT_TEXTURE_SRGB_DECODE       },
    {"GL_EXT_vertex_array_bgra",            EXT_VERTEX_ARRAY_BGRA         },

    /* NV */
    {"GL_NV_fence",                         NV_FENCE                      },
    {"GL_NV_fog_distance",                  NV_FOG_DISTANCE               },
    {"GL_NV_fragment_program",              NV_FRAGMENT_PROGRAM           },
    {"GL_NV_fragment_program2",             NV_FRAGMENT_PROGRAM2          },
    {"GL_NV_fragment_program_option",       NV_FRAGMENT_PROGRAM_OPTION    },
    {"GL_NV_half_float",                    NV_HALF_FLOAT                 },
    {"GL_NV_light_max_exponent",            NV_LIGHT_MAX_EXPONENT         },
    {"GL_NV_point_sprite",                  NV_POINT_SPRITE               },
    {"GL_NV_register_combiners",            NV_REGISTER_COMBINERS         },
    {"GL_NV_register_combiners2",           NV_REGISTER_COMBINERS2        },
    {"GL_NV_texgen_reflection",             NV_TEXGEN_REFLECTION          },
    {"GL_NV_texture_env_combine4",          NV_TEXTURE_ENV_COMBINE4       },
    {"GL_NV_texture_shader",                NV_TEXTURE_SHADER             },
    {"GL_NV_texture_shader2",               NV_TEXTURE_SHADER2            },
    {"GL_NV_vertex_program",                NV_VERTEX_PROGRAM             },
    {"GL_NV_vertex_program1_1",             NV_VERTEX_PROGRAM1_1          },
    {"GL_NV_vertex_program2",               NV_VERTEX_PROGRAM2            },
    {"GL_NV_vertex_program2_option",        NV_VERTEX_PROGRAM2_OPTION     },
    {"GL_NV_vertex_program3",               NV_VERTEX_PROGRAM3            },
    {"GL_NVX_gpu_memory_info",              NVX_GPU_MEMORY_INFO           },

    /* SGI */
    {"GL_SGIS_generate_mipmap",             SGIS_GENERATE_MIPMAP          },
};

static const struct wined3d_extension_map wgl_extension_map[] =
{
    {"WGL_ARB_pixel_format",                WGL_ARB_PIXEL_FORMAT             },
    {"WGL_EXT_swap_control",                WGL_EXT_SWAP_CONTROL             },
    {"WGL_WINE_pixel_format_passthrough",   WGL_WINE_PIXEL_FORMAT_PASSTHROUGH},
    {"WGL_WINE_gpu_info",                   WGL_WINE_GPU_INFO                },
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

const GLenum magLookup[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_LINEAR,
};

static void wined3d_caps_gl_ctx_destroy(const struct wined3d_caps_gl_ctx *ctx)
{
    const struct wined3d_gl_info *gl_info = ctx->gl_info;

    TRACE("Destroying caps GL context.\n");

    /* Both glDeleteProgram and glDeleteBuffers silently ignore 0 IDs but
     * this function might be called before the relevant function pointers
     * in gl_info are initialized. */
    if (ctx->test_program_id || ctx->test_vbo)
    {
        GL_EXTCALL(glDeleteProgram(ctx->test_program_id));
        GL_EXTCALL(glDeleteBuffers(1, &ctx->test_vbo));
    }

    if (!wglMakeCurrent(NULL, NULL))
        ERR("Failed to disable caps GL context.\n");

    if (!wglDeleteContext(ctx->gl_ctx))
    {
        DWORD err = GetLastError();
        ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx->gl_ctx, err);
    }

    wined3d_release_dc(ctx->wnd, ctx->dc);
    DestroyWindow(ctx->wnd);

    if (ctx->restore_gl_ctx && !wglMakeCurrent(ctx->restore_dc, ctx->restore_gl_ctx))
        ERR("Failed to restore previous GL context.\n");
}

static BOOL wined3d_caps_gl_ctx_create_attribs(struct wined3d_caps_gl_ctx *caps_gl_ctx,
        struct wined3d_gl_info *gl_info)
{
    HGLRC new_ctx;

    if (!(gl_info->p_wglCreateContextAttribsARB = (void *)wglGetProcAddress("wglCreateContextAttribsARB")))
        return TRUE;

    if (!(new_ctx = context_create_wgl_attribs(gl_info, caps_gl_ctx->dc, NULL)))
    {
        gl_info->p_wglCreateContextAttribsARB = NULL;
        return FALSE;
    }

    if (!wglMakeCurrent(caps_gl_ctx->dc, new_ctx))
    {
        ERR("Failed to make new context current, last error %#x.\n", GetLastError());
        if (!wglDeleteContext(new_ctx))
            ERR("Failed to delete new context, last error %#x.\n", GetLastError());
        gl_info->p_wglCreateContextAttribsARB = NULL;
        return TRUE;
    }

    if (!wglDeleteContext(caps_gl_ctx->gl_ctx))
        ERR("Failed to delete old context, last error %#x.\n", GetLastError());
    caps_gl_ctx->gl_ctx = new_ctx;

    return TRUE;
}

static BOOL wined3d_caps_gl_ctx_create(struct wined3d_adapter *adapter, struct wined3d_caps_gl_ctx *ctx)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormat;

    TRACE("getting context...\n");

    ctx->restore_dc = wglGetCurrentDC();
    ctx->restore_gl_ctx = wglGetCurrentContext();

    /* We need a fake window as a hdc retrieved using GetDC(0) can't be used for much GL purposes. */
    ctx->wnd = CreateWindowA(WINED3D_OPENGL_WINDOW_CLASS_NAME, "WineD3D fake window",
            WS_OVERLAPPEDWINDOW, 10, 10, 10, 10, NULL, NULL, NULL, NULL);
    if (!ctx->wnd)
    {
        ERR("Failed to create a window.\n");
        goto fail;
    }

    ctx->dc = GetDC(ctx->wnd);
    if (!ctx->dc)
    {
        ERR("Failed to get a DC.\n");
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

    if (!(iPixelFormat = ChoosePixelFormat(ctx->dc, &pfd)))
    {
        /* If this happens something is very wrong as ChoosePixelFormat barely fails. */
        ERR("Failed to find a suitable pixel format.\n");
        goto fail;
    }
    DescribePixelFormat(ctx->dc, iPixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(ctx->dc, iPixelFormat, &pfd);

    /* Create a GL context. */
    if (!(ctx->gl_ctx = wglCreateContext(ctx->dc)))
    {
        WARN("Failed to create default context for capabilities initialization.\n");
        goto fail;
    }

    /* Make it the current GL context. */
    if (!wglMakeCurrent(ctx->dc, ctx->gl_ctx))
    {
        ERR("Failed to make caps GL context current.\n");
        goto fail;
    }

    ctx->gl_info = &adapter->gl_info;
    return TRUE;

fail:
    if (ctx->gl_ctx) wglDeleteContext(ctx->gl_ctx);
    ctx->gl_ctx = NULL;
    if (ctx->dc) ReleaseDC(ctx->wnd, ctx->dc);
    ctx->dc = NULL;
    if (ctx->wnd) DestroyWindow(ctx->wnd);
    ctx->wnd = NULL;
    if (ctx->restore_gl_ctx && !wglMakeCurrent(ctx->restore_dc, ctx->restore_gl_ctx))
        ERR("Failed to restore previous GL context.\n");

    return FALSE;
}

/* Adjust the amount of used texture memory */
UINT64 adapter_adjust_memory(struct wined3d_adapter *adapter, INT64 amount)
{
    adapter->vram_bytes_used += amount;
    TRACE("Adjusted used adapter memory by 0x%s to 0x%s.\n",
            wine_dbgstr_longlong(amount),
            wine_dbgstr_longlong(adapter->vram_bytes_used));
    return adapter->vram_bytes_used;
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

/* Context activation is done by the caller. */
static BOOL test_arb_vs_offset_limit(const struct wined3d_gl_info *gl_info)
{
    GLuint prog;
    BOOL ret = FALSE;
    static const char testcode[] =
        "!!ARBvp1.0\n"
        "PARAM C[66] = { program.env[0..65] };\n"
        "ADDRESS A0;"
        "PARAM zero = {0.0, 0.0, 0.0, 0.0};\n"
        "ARL A0.x, zero.x;\n"
        "MOV result.position, C[A0.x + 65];\n"
        "END\n";

    while (gl_info->gl_ops.gl.p_glGetError());
    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog) {
        ERR("Failed to create an ARB offset limit test program\n");
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  strlen(testcode), testcode));
    if (gl_info->gl_ops.gl.p_glGetError())
    {
        TRACE("OpenGL implementation does not allow indirect addressing offsets > 63\n");
        TRACE("error: %s\n", debugstr_a((const char *)gl_info->gl_ops.gl.p_glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
    } else TRACE("OpenGL implementation allows offsets > 63\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("ARB vp offset limit test cleanup");

    return ret;
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

    while (gl_info->gl_ops.gl.p_glGetError());
    gl_info->gl_ops.gl.p_glGenTextures(1, &texture);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, texture);

    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    checkGLcall("Specifying the PBO test texture");

    GL_EXTCALL(glGenBuffers(1, &pbo));
    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo));
    GL_EXTCALL(glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(pattern), pattern, GL_STREAM_DRAW));
    checkGLcall("Specifying the PBO test pbo");

    gl_info->gl_ops.gl.p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("Loading the PBO test texture");

    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

    gl_info->gl_ops.gl.p_glFinish(); /* just to be sure */

    memset(check, 0, sizeof(check));
    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, check);
    checkGLcall("Reading back the PBO test texture");

    gl_info->gl_ops.gl.p_glDeleteTextures(1, &texture);
    GL_EXTCALL(glDeleteBuffers(1, &pbo));
    checkGLcall("PBO test cleanup");

    if (memcmp(check, pattern, sizeof(check)))
    {
        WARN_(d3d_perf)("PBO test failed, read back data doesn't match original.\n"
                "Disabling PBOs. This may result in slower performance.\n");
        gl_info->supported[ARB_PIXEL_BUFFER_OBJECT] = FALSE;
    }
    else
    {
        TRACE("PBO test successful.\n");
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
     * varyings and we subtract one in dx9 shaders it's not going to hurt us because the dx9 limit is
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

    if (!gl_info->supported[EXT_SECONDARY_COLOR])
        return FALSE;

    while (gl_info->gl_ops.gl.p_glGetError());
    GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE, 4, data);
    error = gl_info->gl_ops.gl.p_glGetError();

    if (error == GL_NO_ERROR)
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
    static const char testcode[] =
        "!!ARBvp1.0\n"
        "OPTION NV_vertex_program2;\n"
        "MOV result.clip[0], 0.0;\n"
        "MOV result.position, 0.0;\n"
        "END\n";

    if (!gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]) return FALSE;

    while (gl_info->gl_ops.gl.p_glGetError());

    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog)
    {
        ERR("Failed to create the NVvp clip test program\n");
        return FALSE;
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  strlen(testcode), testcode));
    gl_info->gl_ops.gl.p_glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if(pos != -1)
    {
        WARN("GL_NV_vertex_program2_option result.clip[] test failed\n");
        TRACE("error: %s\n", debugstr_a((const char *)gl_info->gl_ops.gl.p_glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
        while (gl_info->gl_ops.gl.p_glGetError());
    }
    else TRACE("GL_NV_vertex_program2_option result.clip[] test passed\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("GL_NV_vertex_program2_option result.clip[] test cleanup");

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

    gl_info->gl_ops.gl.p_glGenTextures(1, &tex);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("glTexImage2D");

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    checkGLcall("glFramebufferTexture2D");

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) ERR("FBO status %#x\n", status);
    checkGLcall("glCheckFramebufferStatus");

    memset(data, 0x11, sizeof(data));
    gl_info->gl_ops.gl.p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glTexSubImage2D");

    gl_info->gl_ops.gl.p_glClearColor(0.996f, 0.729f, 0.745f, 0.792f);
    gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");

    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glGetTexImage");

    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, 0);
    checkGLcall("glBindTexture");

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex);
    checkGLcall("glDeleteTextures");

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

    gl_info->gl_ops.gl.p_glGenTextures(1, &tex);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
    checkGLcall("glTexImage2D");

    gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &size);
    checkGLcall("glGetTexLevelParameteriv");
    TRACE("Real color depth is %d\n", size);

    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, 0);
    checkGLcall("glBindTexture");
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex);
    checkGLcall("glDeleteTextures");

    return size < 16;
}

static BOOL match_fglrx(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return gl_vendor == GL_VENDOR_FGLRX;
}

static BOOL match_r200(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_AMD) return FALSE;
    if (device == CARD_AMD_RADEON_8500) return TRUE;
    return FALSE;
}

static BOOL match_broken_arb_fog(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    DWORD data[4];
    GLuint tex, fbo;
    GLenum status;
    float color[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    GLuint prog;
    GLint err_pos;
    static const char program_code[] =
        "!!ARBfp1.0\n"
        "OPTION ARB_fog_linear;\n"
        "MOV result.color, {1.0, 0.0, 0.0, 0.0};\n"
        "END\n";

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return FALSE;
    if (!gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return FALSE;

    gl_info->gl_ops.gl.p_glGenTextures(1, &tex);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 1, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("glTexImage2D");

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    checkGLcall("glFramebufferTexture2D");

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) ERR("FBO status %#x\n", status);
    checkGLcall("glCheckFramebufferStatus");

    gl_info->gl_ops.gl.p_glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");
    gl_info->gl_ops.gl.p_glViewport(0, 0, 4, 1);
    checkGLcall("glViewport");

    gl_info->gl_ops.gl.p_glEnable(GL_FOG);
    gl_info->gl_ops.gl.p_glFogf(GL_FOG_START, 0.5f);
    gl_info->gl_ops.gl.p_glFogf(GL_FOG_END, 0.5f);
    gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_LINEAR);
    gl_info->gl_ops.gl.p_glHint(GL_FOG_HINT, GL_NICEST);
    gl_info->gl_ops.gl.p_glFogfv(GL_FOG_COLOR, color);
    checkGLcall("fog setup");

    GL_EXTCALL(glGenProgramsARB(1, &prog));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(program_code), program_code));
    gl_info->gl_ops.gl.p_glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("Test fragment program setup");

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err_pos);
    if (err_pos != -1)
    {
        const char *error_str;
        error_str = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        FIXME("Fog test program error at position %d: %s\n\n", err_pos, debugstr_a(error_str));
    }

    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glVertex3f(-1.0f, -1.0f,  0.0f);
    gl_info->gl_ops.gl.p_glVertex3f( 1.0f, -1.0f,  1.0f);
    gl_info->gl_ops.gl.p_glVertex3f(-1.0f,  1.0f,  0.0f);
    gl_info->gl_ops.gl.p_glVertex3f( 1.0f,  1.0f,  1.0f);
    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("ARBfp fog test draw");

    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glGetTexImage");
    data[0] &= 0x00ffffff;
    data[1] &= 0x00ffffff;
    data[2] &= 0x00ffffff;
    data[3] &= 0x00ffffff;

    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, 0);

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex);
    gl_info->gl_ops.gl.p_glDisable(GL_FOG);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0));
    gl_info->gl_ops.gl.p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("ARBfp fog test teardown");

    TRACE("Fog test data: %08x %08x %08x %08x\n", data[0], data[1], data[2], data[3]);
    return data[0] != 0x00ff0000 || data[3] != 0x0000ff00;
}

static void quirk_apple_glsl_constants(struct wined3d_gl_info *gl_info)
{
    /* MacOS needs uniforms for relative addressing offsets. This can accumulate to quite a few uniforms.
     * Beyond that the general uniform isn't optimal, so reserve a number of uniforms. 12 vec4's should
     * allow 48 different offsets or other helper immediate values. */
    TRACE("Reserving 12 GLSL constants for compiler private use.\n");
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
     *  software-emulated extension and re-enable ARB_tex_rect (which was previously disabled
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

static void quirk_r200_constants(struct wined3d_gl_info *gl_info)
{
    /* The Mesa r200 driver (and there is no other driver for this GPU Wine would run on)
     * loads some fog parameters (start, end, exponent, but not the color) into the
     * program.
     *
     * Apparently the fog hardware is only able to handle linear fog with a range of 0.0;1.0,
     * and it is the responsibility of the vertex pipeline to handle non-linear fog and
     * linear fog with start and end other than 0.0 and 1.0. */
    TRACE("Reserving 1 ARB constant for compiler private use.\n");
    gl_info->reserved_arb_constants = max(gl_info->reserved_arb_constants, 1);
}

static void quirk_broken_arb_fog(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_BROKEN_ARB_FOG;
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
    {
        match_r200,
        quirk_r200_constants,
        "r200 vertex shader constants"
    },
    {
        match_broken_arb_fog,
        quirk_broken_arb_fog,
        "ARBfp fogstart == fogend workaround"
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
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 17, 10, 1280},
    {DRIVER_AMD_R300,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 14, 10, 741 },
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 17, 10, 1280 },

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
     * - Geforce8 and newer is supported by the current 340.52 driver on XP-Win8
     * - Geforce6 and 7 support is up to 307.83 on XP-Win8
     * - GeforceFX support is up to 173.x on <= XP
     * - Geforce2MX/3/4 up to 96.x on <= XP
     * - TNT/Geforce1/2 up to 71.x on <= XP
     * All version numbers used below are from the Linux nvidia drivers. */
    {DRIVER_NVIDIA_TNT,         DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 7186},
    {DRIVER_NVIDIA_GEFORCE2MX,  DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 9371},
    {DRIVER_NVIDIA_GEFORCEFX,   DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 11, 7516},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT5X,  "nv4_disp.dll", 18, 13,  783},
    {DRIVER_NVIDIA_GEFORCE8,    DRIVER_MODEL_NT5X,  "nv4_disp.dll", 18, 13, 4052},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT6X,  "nvd3dum.dll",  18, 13,  783},
    {DRIVER_NVIDIA_GEFORCE8,    DRIVER_MODEL_NT6X,  "nvd3dum.dll",  18, 13, 4052},

    /* VMware */
    {DRIVER_VMWARE,             DRIVER_MODEL_NT5X,  "vm3dum.dll",   14, 1,  1134},
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
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_128,           "NVIDIA RIVA 128",                  DRIVER_NVIDIA_TNT,       4   },
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
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8200,       "NVIDIA GeForce 8200",              DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8400GS,     "NVIDIA GeForce 8400 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8500GT,     "NVIDIA GeForce 8500 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          DRIVER_NVIDIA_GEFORCE8,  320 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTX,    "NVIDIA GeForce 8800 GTX",          DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9300,       "NVIDIA GeForce 9300",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400M,      "NVIDIA GeForce 9400M",             DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           DRIVER_NVIDIA_GEFORCE8,  384 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_210,        "NVIDIA GeForce 210",               DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT220,      "NVIDIA GeForce GT 220",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           DRIVER_NVIDIA_GEFORCE8,  896 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_315M,       "NVIDIA GeForce 315M",              DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_320M,       "NVIDIA GeForce 320M",              DRIVER_NVIDIA_GEFORCE8,  256},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT320M,     "NVIDIA GeForce GT 320M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT325M,     "NVIDIA GeForce GT 325M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT330,      "NVIDIA GeForce GT 330",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS350M,    "NVIDIA GeForce GTS 350M",          DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_410M,       "NVIDIA GeForce 410M",              DRIVER_NVIDIA_GEFORCE8,  512},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT420,      "NVIDIA GeForce GT 420",            DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT425M,     "NVIDIA GeForce GT 425M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT430,      "NVIDIA GeForce GT 430",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT440,      "NVIDIA GeForce GT 440",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS450,     "NVIDIA GeForce GTS 450",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460,     "NVIDIA GeForce GTX 460",           DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460M,    "NVIDIA GeForce GTX 460M",          DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX465,     "NVIDIA GeForce GTX 465",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX470,     "NVIDIA GeForce GTX 470",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX480,     "NVIDIA GeForce GTX 480",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT520,      "NVIDIA GeForce GT 520",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT540M,     "NVIDIA GeForce GT 540M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX550,     "NVIDIA GeForce GTX 550 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT555M,     "NVIDIA GeForce GT 555M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560TI,   "NVIDIA GeForce GTX 560 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560,     "NVIDIA GeForce GTX 560",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX570,     "NVIDIA GeForce GTX 570",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX580,     "NVIDIA GeForce GTX 580",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT610,      "NVIDIA GeForce GT 610",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630,      "NVIDIA GeForce GT 630",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630M,     "NVIDIA GeForce GT 630M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT640M,     "NVIDIA GeForce GT 640M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT650M,     "NVIDIA GeForce GT 650M",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650,     "NVIDIA GeForce GTX 650",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650TI,   "NVIDIA GeForce GTX 650 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660,     "NVIDIA GeForce GTX 660",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660M,    "NVIDIA GeForce GTX 660M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660TI,   "NVIDIA GeForce GTX 660 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670,     "NVIDIA GeForce GTX 670",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670MX,   "NVIDIA GeForce GTX 670MX",         DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX680,     "NVIDIA GeForce GTX 680",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT750M,     "NVIDIA GeForce GT 750M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750,     "NVIDIA GeForce GTX 750",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750TI,   "NVIDIA GeForce GTX 750 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX760,     "NVIDIA Geforce GTX 760",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX765M,    "NVIDIA GeForce GTX 765M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770M,    "NVIDIA GeForce GTX 770M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770,     "NVIDIA GeForce GTX 770",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780,     "NVIDIA GeForce GTX 780",           DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780TI,   "NVIDIA GeForce GTX 780 Ti",        DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX970,     "NVIDIA GeForce GTX 970",           DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX970M,    "NVIDIA GeForce GTX 970M",          DRIVER_NVIDIA_GEFORCE8,  3072},

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
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD3850,         "ATI Radeon HD 3850 AGP",           DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4200M,        "ATI Mobility Radeon HD 4200",      DRIVER_AMD_R600,         256 },
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
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6700,         "AMD Radeon HD 6700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6800,         "AMD Radeon HD 6800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6900,         "AMD Radeon HD 6900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7660D,        "AMD Radeon HD 7660D",              DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7700,         "AMD Radeon HD 7700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7800,         "AMD Radeon HD 7800 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7900,         "AMD Radeon HD 7900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8600M,        "AMD Radeon HD 8600M Series",       DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8670,         "AMD Radeon HD 8670",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8770,         "AMD Radeon HD 8770",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R3,             "AMD Radeon HD 8400 / R3 Series",   DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R7,             "AMD Radeon(TM) R7 Graphics",       DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R9,             "AMD Radeon R9 290",                DRIVER_AMD_R600,         4096},

    /* VMware */
    {HW_VENDOR_VMWARE,     CARD_VMWARE_SVGA3D,             "VMware SVGA 3D (Microsoft Corporation - WDDM)",             DRIVER_VMWARE,        1024},

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
    {HW_VENDOR_INTEL,      CARD_INTEL_HWM,                 "Intel(R) Haswell Mobile",                                   DRIVER_INTEL_GMA3000, 1024},
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
            TRACE("Found driver \"%s\", version %u, subversion %u, build %u.\n",
                    entry->driver_name, entry->version, entry->subversion, entry->build);
            return entry;
        }
    }
    return NULL;
}

static const struct gpu_description *get_gpu_description(enum wined3d_pci_vendor vendor,
        enum wined3d_pci_device device)
{
    unsigned int i;

    for (i = 0; i < (sizeof(gpu_description_table) / sizeof(*gpu_description_table)); ++i)
    {
        if (vendor == gpu_description_table[i].vendor && device == gpu_description_table[i].card)
            return &gpu_description_table[i];
    }

    return NULL;
}

/* Context activation is done by the caller. */
static void init_driver_info(struct wined3d_gl_info *gl_info, struct wined3d_driver_info *driver_info,
        enum wined3d_pci_vendor vendor, enum wined3d_pci_device device)
{
    OSVERSIONINFOW os_version;
    WORD driver_os_version;
    enum wined3d_display_driver driver;
    enum wined3d_driver_model driver_model;
    const struct driver_version_information *version_info;
    const struct gpu_description *gpu_desc;

    if (driver_info->vendor != PCI_VENDOR_NONE || driver_info->device != PCI_DEVICE_NONE)
    {
        static unsigned int once;
        unsigned int real_vendor, real_device;

        TRACE("GPU override %04x:%04x.\n", wined3d_settings.pci_vendor_id, wined3d_settings.pci_device_id);

        if (gl_info->supported[WGL_WINE_GPU_INFO] &&
            gl_info->gl_ops.ext.p_wglGetPCIInfoWINE(&real_vendor, &real_device))
        {
            if (get_gpu_description(real_vendor, real_device))
            {
                vendor = real_vendor;
                device = real_device;
            }
            else if (!once++)
                ERR_(winediag)("Could not find GPU info for %04x:%04x.\n", real_vendor, real_device);
        }

        driver_info->vendor = wined3d_settings.pci_vendor_id;
        if (driver_info->vendor == PCI_VENDOR_NONE)
            driver_info->vendor = vendor;

        driver_info->device = wined3d_settings.pci_device_id;
        if (driver_info->device == PCI_DEVICE_NONE)
            driver_info->device = device;

        if (get_gpu_description(driver_info->vendor, driver_info->device))
        {
            vendor = driver_info->vendor;
            device = driver_info->device;
        }
        else if (!once++)
            ERR_(winediag)("Invalid GPU override %04x:%04x specified, ignoring.\n",
                    driver_info->vendor, driver_info->device);
    }

    driver_info->vendor = vendor;
    driver_info->device = device;

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
                else if (os_version.dwMinorVersion == 1)
                {
                    driver_os_version = 8;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                else
                {
                    if (os_version.dwMinorVersion > 2)
                    {
                        FIXME("Unhandled OS version %u.%u, reporting Win 8.\n",
                                os_version.dwMajorVersion, os_version.dwMinorVersion);
                    }
                    driver_os_version = 9;
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

    if ((gpu_desc = get_gpu_description(driver_info->vendor, driver_info->device)))
    {
        driver_info->description = gpu_desc->description;
        driver_info->vram_bytes = (UINT64)gpu_desc->vidmem * 1024 * 1024;
        driver = gpu_desc->driver;
    }
    else
    {
        ERR("Card %04x:%04x not found in driver DB.\n", vendor, device);
        driver_info->description = "Direct3D HAL";
        driver_info->vram_bytes = WINE_DEFAULT_VIDMEM;
        driver = DRIVER_UNKNOWN;
    }

    if (gl_info->supported[NVX_GPU_MEMORY_INFO])
    {
        GLint vram_kb;
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &vram_kb);

        driver_info->vram_bytes = (UINT64)vram_kb * 1024;
        TRACE("Got 0x%s as video memory from NVX_GPU_MEMORY_INFO extension.\n",
                wine_dbgstr_longlong(driver_info->vram_bytes));
    }

    if (gl_info->supported[WGL_WINE_GPU_INFO])
    {
        unsigned int vram_mb;
        if (gl_info->gl_ops.ext.p_wglGetMemoryInfoWINE(&vram_mb))
        {
            driver_info->vram_bytes = (UINT64)vram_mb * 1024 * 1024;
            TRACE("Got 0x%s as video memory from wglGetGPUInfoWINE.\n",
                    wine_dbgstr_longlong(driver_info->vram_bytes));
        }
    }

    if (wined3d_settings.emulated_textureram)
    {
        TRACE("Overriding amount of video memory with 0x%s bytes.\n",
                wine_dbgstr_longlong(wined3d_settings.emulated_textureram));
        driver_info->vram_bytes = wined3d_settings.emulated_textureram;
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
    if ((version_info = get_driver_version_info(driver, driver_model))
            || (version_info = get_driver_version_info(driver, DRIVER_MODEL_NT5X)))
    {
        driver_info->name = version_info->driver_name;
        driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, version_info->version);
        driver_info->version_low = MAKEDWORD_VERSION(version_info->subversion, version_info->build);
    }
    else
    {
        ERR("No driver version info found for device %04x:%04x, driver model %#x.\n",
                vendor, device, driver_model);
        driver_info->name = "Display";
        driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, 15);
        driver_info->version_low = MAKEDWORD_VERSION(8, 6); /* Nvidia RIVA TNT, arbitrary */
    }

    TRACE("Reporting (fake) driver version 0x%08x-0x%08x.\n",
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
        TRACE("Applying driver quirk \"%s\".\n", quirk_table[i].description);
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
    if (major <= 0)
        ERR("Invalid OpenGL major version %d.\n", major);

    while (isdigit(*ptr)) ++ptr;
    if (*ptr++ != '.')
        ERR("Invalid OpenGL version string %s.\n", debugstr_a(gl_version));

    minor = atoi(ptr);

    TRACE("Found OpenGL version %d.%d.\n", major, minor);

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
            && gl_info->supported[APPLE_YCBCR_422])
        return GL_VENDOR_APPLE;

    if (strstr(gl_vendor_string, "NVIDIA"))
        return GL_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI"))
        return GL_VENDOR_FGLRX;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "X.Org")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "DRI R300 Project")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc.")
            || strstr(gl_vendor_string, "Intel")
            || strstr(gl_renderer, "Mesa")
            || strstr(gl_renderer, "Gallium")
            || strstr(gl_renderer, "Intel"))
        return GL_VENDOR_MESA;

    FIXME("Received unrecognized GL_VENDOR %s. Returning GL_VENDOR_UNKNOWN.\n",
            debugstr_a(gl_vendor_string));

    return GL_VENDOR_UNKNOWN;
}

static enum wined3d_pci_vendor wined3d_guess_card_vendor(const char *gl_vendor_string, const char *gl_renderer)
{
    if (strstr(gl_vendor_string, "NVIDIA")
            || strstr(gl_vendor_string, "Nouveau")
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

    if (strstr(gl_renderer, "SVGA3D"))
        return HW_VENDOR_VMWARE;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Brian Paul")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc."))
        return HW_VENDOR_SOFTWARE;

    FIXME("Received unrecognized GL_VENDOR %s. Returning HW_VENDOR_NVIDIA.\n", debugstr_a(gl_vendor_string));

    return HW_VENDOR_NVIDIA;
}

static enum wined3d_d3d_level d3d_level_from_caps(const struct shader_caps *shader_caps, const struct fragment_caps *fragment_caps, DWORD glsl_version)
{
    if (shader_caps->vs_version >= 5)
        return WINED3D_D3D_LEVEL_11;
    if (shader_caps->vs_version == 4)
    {
        /* No backed supports SM 5 at the moment */
        if (glsl_version >= MAKEDWORD_VERSION(4, 00))
            return WINED3D_D3D_LEVEL_11;
        return WINED3D_D3D_LEVEL_10;
    }
    if (shader_caps->vs_version == 3)
    {
        /* Wine cannot use SM 4 on mesa drivers as the necessary functionality
         * is not exposed on compatibility contexts */
        if (glsl_version >= MAKEDWORD_VERSION(1, 30))
            return WINED3D_D3D_LEVEL_10;
        return WINED3D_D3D_LEVEL_9_SM3;
    }
    if (shader_caps->vs_version == 2)
        return WINED3D_D3D_LEVEL_9_SM2;
    if (shader_caps->vs_version == 1)
        return WINED3D_D3D_LEVEL_8;

    if (fragment_caps->TextureOpCaps & WINED3DTEXOPCAPS_DOTPRODUCT3)
        return WINED3D_D3D_LEVEL_7;
    if (fragment_caps->MaxSimultaneousTextures > 1)
        return WINED3D_D3D_LEVEL_6;

    return WINED3D_D3D_LEVEL_5;
}

static const struct wined3d_renderer_table
{
    const char *renderer;
    enum wined3d_pci_device id;
}
cards_nvidia_binary[] =
{
    /* Direct 3D 11 */
    {"GTX 970M",                    CARD_NVIDIA_GEFORCE_GTX970M},   /* GeForce 900 - highend mobile*/
    {"GTX 970",                     CARD_NVIDIA_GEFORCE_GTX970},    /* GeForce 900 - highend */
    {"GTX 780 Ti",                  CARD_NVIDIA_GEFORCE_GTX780TI},  /* Geforce 700 - highend */
    {"GTX 780",                     CARD_NVIDIA_GEFORCE_GTX780},    /* Geforce 700 - highend */
    {"GTX 770M",                    CARD_NVIDIA_GEFORCE_GTX770M},   /* Geforce 700 - midend high mobile */
    {"GTX 770",                     CARD_NVIDIA_GEFORCE_GTX770},    /* Geforce 700 - highend */
    {"GTX 765M",                    CARD_NVIDIA_GEFORCE_GTX765M},   /* Geforce 700 - midend high mobile */
    {"GTX 760",                     CARD_NVIDIA_GEFORCE_GTX760},    /* Geforce 700 - midend high  */
    {"GTX 750 Ti",                  CARD_NVIDIA_GEFORCE_GTX750TI},  /* Geforce 700 - midend */
    {"GTX 750",                     CARD_NVIDIA_GEFORCE_GTX750},    /* Geforce 700 - midend */
    {"GT 750M",                     CARD_NVIDIA_GEFORCE_GT750M},    /* Geforce 700 - midend mobile */
    {"GTX 680",                     CARD_NVIDIA_GEFORCE_GTX680},    /* Geforce 600 - highend */
    {"GTX 670MX",                   CARD_NVIDIA_GEFORCE_GTX670MX},  /* Geforce 600 - highend */
    {"GTX 670",                     CARD_NVIDIA_GEFORCE_GTX670},    /* Geforce 600 - midend high */
    {"GTX 660 Ti",                  CARD_NVIDIA_GEFORCE_GTX660TI},  /* Geforce 600 - midend high */
    {"GTX 660M",                    CARD_NVIDIA_GEFORCE_GTX660M},   /* Geforce 600 - midend high mobile */
    {"GTX 660",                     CARD_NVIDIA_GEFORCE_GTX660},    /* Geforce 600 - midend high */
    {"GTX 650 Ti",                  CARD_NVIDIA_GEFORCE_GTX650TI},  /* Geforce 600 - lowend */
    {"GTX 650",                     CARD_NVIDIA_GEFORCE_GTX650},    /* Geforce 600 - lowend */
    {"GT 650M",                     CARD_NVIDIA_GEFORCE_GT650M},    /* Geforce 600 - midend mobile */
    {"GT 640M",                     CARD_NVIDIA_GEFORCE_GT640M},    /* Geforce 600 - midend mobile */
    {"GT 630M",                     CARD_NVIDIA_GEFORCE_GT630M},    /* Geforce 600 - midend mobile */
    {"GT 630",                      CARD_NVIDIA_GEFORCE_GT630},     /* Geforce 600 - lowend */
    {"GT 610",                      CARD_NVIDIA_GEFORCE_GT610},     /* Geforce 600 - lowend */
    {"GTX 580",                     CARD_NVIDIA_GEFORCE_GTX580},    /* Geforce 500 - highend */
    {"GTX 570",                     CARD_NVIDIA_GEFORCE_GTX570},    /* Geforce 500 - midend high */
    {"GTX 560 Ti",                  CARD_NVIDIA_GEFORCE_GTX560TI},  /* Geforce 500 - midend */
    {"GTX 560",                     CARD_NVIDIA_GEFORCE_GTX560},    /* Geforce 500 - midend */
    {"GT 555M",                     CARD_NVIDIA_GEFORCE_GT555M},    /* Geforce 500 - midend mobile */
    {"GTX 550 Ti",                  CARD_NVIDIA_GEFORCE_GTX550},    /* Geforce 500 - midend */
    {"GT 540M",                     CARD_NVIDIA_GEFORCE_GT540M},    /* Geforce 500 - midend mobile */
    {"GT 520",                      CARD_NVIDIA_GEFORCE_GT520},     /* Geforce 500 - lowend */
    {"GTX 480",                     CARD_NVIDIA_GEFORCE_GTX480},    /* Geforce 400 - highend */
    {"GTX 470",                     CARD_NVIDIA_GEFORCE_GTX470},    /* Geforce 400 - midend high */
    /* Direct 3D 10 */
    {"GTX 465",                     CARD_NVIDIA_GEFORCE_GTX465},    /* Geforce 400 - midend */
    {"GTX 460M",                    CARD_NVIDIA_GEFORCE_GTX460M},   /* Geforce 400 - highend mobile */
    {"GTX 460",                     CARD_NVIDIA_GEFORCE_GTX460},    /* Geforce 400 - midend */
    {"GTS 450",                     CARD_NVIDIA_GEFORCE_GTS450},    /* Geforce 400 - midend low */
    {"GT 440",                      CARD_NVIDIA_GEFORCE_GT440},     /* Geforce 400 - lowend */
    {"GT 430",                      CARD_NVIDIA_GEFORCE_GT430},     /* Geforce 400 - lowend */
    {"GT 425M",                     CARD_NVIDIA_GEFORCE_GT425M},    /* Geforce 400 - lowend mobile */
    {"GT 420",                      CARD_NVIDIA_GEFORCE_GT420},     /* Geforce 400 - lowend */
    {"410M",                        CARD_NVIDIA_GEFORCE_410M},      /* Geforce 400 - lowend mobile */
    {"GT 330",                      CARD_NVIDIA_GEFORCE_GT330},     /* Geforce 300 - highend */
    {"GTS 360M",                    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
    {"GTS 350M",                    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
    {"GT 330M",                     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
    {"GT 325M",                     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
    {"GT 320M",                     CARD_NVIDIA_GEFORCE_GT320M},    /* Geforce 300 - midend mobile */
    {"320M",                        CARD_NVIDIA_GEFORCE_320M},      /* Geforce 300 - midend mobile */
    {"315M",                        CARD_NVIDIA_GEFORCE_315M},      /* Geforce 300 - midend mobile */
    {"GTX 295",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 285",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 280",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 275",                     CARD_NVIDIA_GEFORCE_GTX275},    /* Geforce 200 - midend high */
    {"GTX 260",                     CARD_NVIDIA_GEFORCE_GTX260},    /* Geforce 200 - midend */
    {"GT 240",                      CARD_NVIDIA_GEFORCE_GT240},     /* Geforce 200 - midend */
    {"GT 220",                      CARD_NVIDIA_GEFORCE_GT220},     /* Geforce 200 - lowend */
    {"GeForce 310",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 305",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 210",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"G 210",                       CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GTS 250",                     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"GTS 150",                     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"9800",                        CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"GT 140",                      CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
    {"9600",                        CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
    {"GT 130",                      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"GT 120",                      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"9500",                        CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"9400M",                       CARD_NVIDIA_GEFORCE_9400M},     /* Geforce 9 - lowend */
    {"9400",                        CARD_NVIDIA_GEFORCE_9400GT},    /* Geforce 9 - lowend */
    {"9300",                        CARD_NVIDIA_GEFORCE_9300},      /* Geforce 9 - lowend low */
    {"9200",                        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"9100",                        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"G 100",                       CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"8800 GTX",                    CARD_NVIDIA_GEFORCE_8800GTX},   /* Geforce 8 - highend high */
    {"8800",                        CARD_NVIDIA_GEFORCE_8800GTS},   /* Geforce 8 - highend */
    {"8600M",                       CARD_NVIDIA_GEFORCE_8600MGT},   /* Geforce 8 - midend mobile */
    {"8600 M",                      CARD_NVIDIA_GEFORCE_8600MGT},   /* Geforce 8 - midend mobile */
    {"8700",                        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
    {"8600",                        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
    {"8500",                        CARD_NVIDIA_GEFORCE_8500GT},    /* Geforce 8 - mid-lowend */
    {"8400",                        CARD_NVIDIA_GEFORCE_8400GS},    /* Geforce 8 - mid-lowend */
    {"8300",                        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
    {"8200",                        CARD_NVIDIA_GEFORCE_8200},      /* Geforce 8 - lowend */
    {"8100",                        CARD_NVIDIA_GEFORCE_8200},      /* Geforce 8 - lowend */
    /* Direct 3D 9 SM3 */
    {"Quadro FX 5",                 CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
    {"Quadro FX 4",                 CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
    {"7950",                        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
    {"7900",                        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
    {"7800",                        CARD_NVIDIA_GEFORCE_7800GT},    /* Geforce 7 - highend */
    {"7700",                        CARD_NVIDIA_GEFORCE_7600},      /* Geforce 7 - midend */
    {"7600",                        CARD_NVIDIA_GEFORCE_7600},      /* Geforce 7 - midend */
    {"7400",                        CARD_NVIDIA_GEFORCE_7400},      /* Geforce 7 - lower medium */
    {"7300",                        CARD_NVIDIA_GEFORCE_7300},      /* Geforce 7 - lowend */
    {"6800",                        CARD_NVIDIA_GEFORCE_6800},      /* Geforce 6 - highend */
    {"6700",                        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
    {"6610",                        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
    {"6600",                        CARD_NVIDIA_GEFORCE_6600GT},    /* Geforce 6 - midend */
    /* Direct 3D 9 SM2 */
    {"Quadro FX",                   CARD_NVIDIA_GEFORCEFX_5800},    /* GeforceFX - highend */
    {"5950",                        CARD_NVIDIA_GEFORCEFX_5800},    /* GeforceFX - highend */
    {"5900",                        CARD_NVIDIA_GEFORCEFX_5800},    /* GeforceFX - highend */
    {"5800",                        CARD_NVIDIA_GEFORCEFX_5800},    /* GeforceFX - highend */
    {"5750",                        CARD_NVIDIA_GEFORCEFX_5600},    /* GeforceFX - midend */
    {"5700",                        CARD_NVIDIA_GEFORCEFX_5600},    /* GeforceFX - midend */
    {"5650",                        CARD_NVIDIA_GEFORCEFX_5600},    /* GeforceFX - midend */
    {"5600",                        CARD_NVIDIA_GEFORCEFX_5600},    /* GeforceFX - midend */
    {"5500",                        CARD_NVIDIA_GEFORCEFX_5200},    /* GeforceFX - lowend */
    {"5300",                        CARD_NVIDIA_GEFORCEFX_5200},    /* GeforceFX - lowend */
    {"5250",                        CARD_NVIDIA_GEFORCEFX_5200},    /* GeforceFX - lowend */
    {"5200",                        CARD_NVIDIA_GEFORCEFX_5200},    /* GeforceFX - lowend */
    {"5100",                        CARD_NVIDIA_GEFORCEFX_5200},    /* GeforceFX - lowend */
    /* Direct 3D 8 */
    {"Quadro4",                     CARD_NVIDIA_GEFORCE4_TI4200},
    {"GeForce4 Ti",                 CARD_NVIDIA_GEFORCE4_TI4200},   /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800 */
    /* Direct 3D 7 */
    {"GeForce4 MX",                 CARD_NVIDIA_GEFORCE4_MX},       /* MX420/MX440/MX460/MX4000 */
    {"Quadro2 MXR",                 CARD_NVIDIA_GEFORCE2_MX},
    {"GeForce2 MX",                 CARD_NVIDIA_GEFORCE2_MX},       /* Geforce2 standard/MX100/MX200/MX400 */
    {"Quadro2",                     CARD_NVIDIA_GEFORCE2},
    {"GeForce2",                    CARD_NVIDIA_GEFORCE2},          /* Geforce2 GTS/Pro/Ti/Ultra */
    /* Direct 3D 6 */
    {"TNT2",                        CARD_NVIDIA_RIVA_TNT2},         /* Riva TNT2 standard/M64/Pro/Ultra */
},
/* See http://developer.amd.com/resources/hardware-drivers/ati-catalyst-pc-vendor-id-1002-li/
 *
 * Beware: renderer string do not match exact card model,
 * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
cards_amd_binary[] =
{
    /* Southern Islands */
    {"HD 7900",                     CARD_AMD_RADEON_HD7900},
    {"HD 7800",                     CARD_AMD_RADEON_HD7800},
    {"HD 7700",                     CARD_AMD_RADEON_HD7700},
    /* Northern Islands */
    {"HD 6970",                     CARD_AMD_RADEON_HD6900},
    {"HD 6900",                     CARD_AMD_RADEON_HD6900},
    {"HD 6800",                     CARD_AMD_RADEON_HD6800},
    {"HD 6770M",                    CARD_AMD_RADEON_HD6600M},
    {"HD 6750M",                    CARD_AMD_RADEON_HD6600M},
    {"HD 6700",                     CARD_AMD_RADEON_HD6700},
    {"HD 6670",                     CARD_AMD_RADEON_HD6600},
    {"HD 6630M",                    CARD_AMD_RADEON_HD6600M},
    {"HD 6600M",                    CARD_AMD_RADEON_HD6600M},
    {"HD 6600",                     CARD_AMD_RADEON_HD6600},
    {"HD 6570",                     CARD_AMD_RADEON_HD6600},
    {"HD 6500M",                    CARD_AMD_RADEON_HD6600M},
    {"HD 6500",                     CARD_AMD_RADEON_HD6600},
    {"HD 6400",                     CARD_AMD_RADEON_HD6400},
    {"HD 6300",                     CARD_AMD_RADEON_HD6300},
    {"HD 6200",                     CARD_AMD_RADEON_HD6300},
    /* Evergreen */
    {"HD 5870",                     CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS PRO */
    {"HD 5850",                     CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS XT */
    {"HD 5800",                     CARD_AMD_RADEON_HD5800},    /* Radeon EG CYPRESS HD58xx generic renderer string */
    {"HD 5770",                     CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER XT */
    {"HD 5750",                     CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER LE */
    {"HD 5700",                     CARD_AMD_RADEON_HD5700},    /* Radeon EG JUNIPER HD57xx generic renderer string */
    {"HD 5670",                     CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD XT */
    {"HD 5570",                     CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD PRO mapped to HD5600 series */
    {"HD 5550",                     CARD_AMD_RADEON_HD5600},    /* Radeon EG REDWOOD LE mapped to HD5600 series */
    {"HD 5450",                     CARD_AMD_RADEON_HD5400},    /* Radeon EG CEDAR PRO */
    {"HD 5000",                     CARD_AMD_RADEON_HD5600},    /* Defaulting to HD 5600 */
    /* R700 */
    {"HD 4890",                     CARD_AMD_RADEON_HD4800},    /* Radeon RV790 */
    {"HD 4870",                     CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
    {"HD 4850",                     CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
    {"HD 4830",                     CARD_AMD_RADEON_HD4800},    /* Radeon RV770 */
    {"HD 4800",                     CARD_AMD_RADEON_HD4800},    /* Radeon RV7xx HD48xx generic renderer string */
    {"HD 4770",                     CARD_AMD_RADEON_HD4700},    /* Radeon RV740 */
    {"HD 4700",                     CARD_AMD_RADEON_HD4700},    /* Radeon RV7xx HD47xx generic renderer string */
    {"HD 4670",                     CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
    {"HD 4650",                     CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
    {"HD 4600",                     CARD_AMD_RADEON_HD4600},    /* Radeon RV730 */
    {"HD 4550",                     CARD_AMD_RADEON_HD4350},    /* Radeon RV710 */
    {"HD 4350",                     CARD_AMD_RADEON_HD4350},    /* Radeon RV710 */
    /* R600/R700 integrated */
    {"HD 4200M",                    CARD_AMD_RADEON_HD4200M},
    {"HD 3300",                     CARD_AMD_RADEON_HD3200},
    {"HD 3200",                     CARD_AMD_RADEON_HD3200},
    {"HD 3100",                     CARD_AMD_RADEON_HD3200},
    /* R600 */
    {"HD 3870",                     CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
    {"HD 3850",                     CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
    {"HD 2900",                     CARD_AMD_RADEON_HD2900},    /* HD2900/HD3800 - highend */
    {"HD 3830",                     CARD_AMD_RADEON_HD2600},    /* China-only midend */
    {"HD 3690",                     CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
    {"HD 3650",                     CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
    {"HD 2600",                     CARD_AMD_RADEON_HD2600},    /* HD2600/HD3600 - midend */
    {"HD 3470",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    {"HD 3450",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    {"HD 3430",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    {"HD 3400",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    {"HD 2400",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    {"HD 2350",                     CARD_AMD_RADEON_HD2350},    /* HD2350/HD2400/HD3400 - lowend */
    /* Radeon R5xx */
    {"X1950",                       CARD_AMD_RADEON_X1600},
    {"X1900",                       CARD_AMD_RADEON_X1600},
    {"X1800",                       CARD_AMD_RADEON_X1600},
    {"X1650",                       CARD_AMD_RADEON_X1600},
    {"X1600",                       CARD_AMD_RADEON_X1600},
    /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300/X2500/HD2300 (lowend R5xx)
     * Note X2300/X2500/HD2300 are R5xx GPUs with a 2xxx naming but they are still DX9-only */
    {"HD 2300",                     CARD_AMD_RADEON_X700},
    {"X2500",                       CARD_AMD_RADEON_X700},
    {"X2300",                       CARD_AMD_RADEON_X700},
    {"X1550",                       CARD_AMD_RADEON_X700},
    {"X1450",                       CARD_AMD_RADEON_X700},
    {"X1400",                       CARD_AMD_RADEON_X700},
    {"X1300",                       CARD_AMD_RADEON_X700},
    {"X850",                        CARD_AMD_RADEON_X700},
    {"X800",                        CARD_AMD_RADEON_X700},
    {"X700",                        CARD_AMD_RADEON_X700},
    /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400 MHz */
    {"Radeon Xpress",               CARD_AMD_RADEON_XPRESS_200M},
},
cards_intel[] =
{
    /* Haswell */
    {"Haswell Mobile",              CARD_INTEL_HWM},
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
},
/* 20101109 - These are never returned by current Gallium radeon
 * drivers: R700, RV790, R680, RV535, RV516, R410, RS485, RV360, RV351.
 *
 * These are returned but not handled: RC410, RV380. */
cards_amd_mesa[] =
{
    /* Sea Islands */
    {"HAWAII",                      CARD_AMD_RADEON_R9    },
    {"KAVERI",                      CARD_AMD_RADEON_R7    },
    {"KABINI",                      CARD_AMD_RADEON_R3    },
    {"BONAIRE",                     CARD_AMD_RADEON_HD8770},
    /* Southern Islands */
    {"OLAND",                       CARD_AMD_RADEON_HD8670},
    {"HAINAN",                      CARD_AMD_RADEON_HD8600M},
    {"TAHITI",                      CARD_AMD_RADEON_HD7900},
    {"PITCAIRN",                    CARD_AMD_RADEON_HD7800},
    {"CAPE VERDE",                  CARD_AMD_RADEON_HD7700},
    /* Northern Islands */
    {"ARUBA",                       CARD_AMD_RADEON_HD7660D},
    {"CAYMAN",                      CARD_AMD_RADEON_HD6900},
    {"BARTS",                       CARD_AMD_RADEON_HD6800},
    {"TURKS",                       CARD_AMD_RADEON_HD6600},
    {"SUMO2",                       CARD_AMD_RADEON_HD6410D},   /* SUMO2 first, because we do a strstr(). */
    {"SUMO",                        CARD_AMD_RADEON_HD6550D},
    {"CAICOS",                      CARD_AMD_RADEON_HD6400},
    {"PALM",                        CARD_AMD_RADEON_HD6300},
    /* Evergreen */
    {"HEMLOCK",                     CARD_AMD_RADEON_HD5900},
    {"CYPRESS",                     CARD_AMD_RADEON_HD5800},
    {"JUNIPER",                     CARD_AMD_RADEON_HD5700},
    {"REDWOOD",                     CARD_AMD_RADEON_HD5600},
    {"CEDAR",                       CARD_AMD_RADEON_HD5400},
    /* R700 */
    {"R700",                        CARD_AMD_RADEON_HD4800},
    {"RV790",                       CARD_AMD_RADEON_HD4800},
    {"RV770",                       CARD_AMD_RADEON_HD4800},
    {"RV740",                       CARD_AMD_RADEON_HD4700},
    {"RV730",                       CARD_AMD_RADEON_HD4600},
    {"RV710",                       CARD_AMD_RADEON_HD4350},
    /* R600/R700 integrated */
    {"RS880",                       CARD_AMD_RADEON_HD4200M},
    {"RS780",                       CARD_AMD_RADEON_HD3200},
    /* R600 */
    {"R680",                        CARD_AMD_RADEON_HD2900},
    {"R600",                        CARD_AMD_RADEON_HD2900},
    {"RV670",                       CARD_AMD_RADEON_HD3850},
    {"RV635",                       CARD_AMD_RADEON_HD2600},
    {"RV630",                       CARD_AMD_RADEON_HD2600},
    {"RV620",                       CARD_AMD_RADEON_HD2350},
    {"RV610",                       CARD_AMD_RADEON_HD2350},
    /* R500 */
    {"R580",                        CARD_AMD_RADEON_X1600},
    {"R520",                        CARD_AMD_RADEON_X1600},
    {"RV570",                       CARD_AMD_RADEON_X1600},
    {"RV560",                       CARD_AMD_RADEON_X1600},
    {"RV535",                       CARD_AMD_RADEON_X1600},
    {"RV530",                       CARD_AMD_RADEON_X1600},
    {"RV516",                       CARD_AMD_RADEON_X700},
    {"RV515",                       CARD_AMD_RADEON_X700},
    /* R400 */
    {"R481",                        CARD_AMD_RADEON_X700},
    {"R480",                        CARD_AMD_RADEON_X700},
    {"R430",                        CARD_AMD_RADEON_X700},
    {"R423",                        CARD_AMD_RADEON_X700},
    {"R420",                        CARD_AMD_RADEON_X700},
    {"R410",                        CARD_AMD_RADEON_X700},
    {"RV410",                       CARD_AMD_RADEON_X700},
    /* Radeon Xpress - onboard, DX9b, Shader 2.0, 300-400 MHz */
    {"RS740",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS690",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS600",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS485",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS482",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS480",                       CARD_AMD_RADEON_XPRESS_200M},
    {"RS400",                       CARD_AMD_RADEON_XPRESS_200M},
    /* R300 */
    {"R360",                        CARD_AMD_RADEON_9500},
    {"R350",                        CARD_AMD_RADEON_9500},
    {"R300",                        CARD_AMD_RADEON_9500},
    {"RV370",                       CARD_AMD_RADEON_9500},
    {"RV360",                       CARD_AMD_RADEON_9500},
    {"RV351",                       CARD_AMD_RADEON_9500},
    {"RV350",                       CARD_AMD_RADEON_9500},
},
cards_nvidia_mesa[] =
{
    /* Maxwell */
    {"NV124",                       CARD_NVIDIA_GEFORCE_GTX970},
    {"NV117",                       CARD_NVIDIA_GEFORCE_GTX750},
    /* Kepler */
    {"NVF1",                        CARD_NVIDIA_GEFORCE_GTX780TI},
    {"NVF0",                        CARD_NVIDIA_GEFORCE_GTX780},
    {"NVE6",                        CARD_NVIDIA_GEFORCE_GTX770M},
    {"NVE4",                        CARD_NVIDIA_GEFORCE_GTX680},
    /* Fermi */
    {"NVD9",                        CARD_NVIDIA_GEFORCE_GT520},
    {"NVCF",                        CARD_NVIDIA_GEFORCE_GTX550},
    {"NVCE",                        CARD_NVIDIA_GEFORCE_GTX560},
    {"NVC8",                        CARD_NVIDIA_GEFORCE_GTX570},
    {"NVC4",                        CARD_NVIDIA_GEFORCE_GTX460},
    {"NVC3",                        CARD_NVIDIA_GEFORCE_GT440},
    {"NVC1",                        CARD_NVIDIA_GEFORCE_GT420},
    {"NVC0",                        CARD_NVIDIA_GEFORCE_GTX480},
    /* Tesla */
    {"NVAF",                        CARD_NVIDIA_GEFORCE_GT320M},
    {"NVAC",                        CARD_NVIDIA_GEFORCE_8200},
    {"NVAA",                        CARD_NVIDIA_GEFORCE_8200},      /* 8100 */
    {"NVA8",                        CARD_NVIDIA_GEFORCE_210},
    {"NVA5",                        CARD_NVIDIA_GEFORCE_GT220},
    {"NVA3",                        CARD_NVIDIA_GEFORCE_GT240},
    {"NVA0",                        CARD_NVIDIA_GEFORCE_GTX280},
    {"NV98",                        CARD_NVIDIA_GEFORCE_9200},
    {"NV96",                        CARD_NVIDIA_GEFORCE_9400GT},
    {"NV94",                        CARD_NVIDIA_GEFORCE_9600GT},
    {"NV92",                        CARD_NVIDIA_GEFORCE_9800GT},
    {"NV86",                        CARD_NVIDIA_GEFORCE_8500GT},
    {"NV84",                        CARD_NVIDIA_GEFORCE_8600GT},
    {"NV50",                        CARD_NVIDIA_GEFORCE_8800GTX},
    /* Curie */
    {"NV68",                        CARD_NVIDIA_GEFORCE_6200},      /* 7050 */
    {"NV67",                        CARD_NVIDIA_GEFORCE_6200},      /* 7000M */
    {"NV63",                        CARD_NVIDIA_GEFORCE_6200},      /* 7100 */
    {"NV4E",                        CARD_NVIDIA_GEFORCE_6200},      /* 6100 Go / 6150 Go */
    {"NV4C",                        CARD_NVIDIA_GEFORCE_6200},      /* 6150SE */
    {"NV4B",                        CARD_NVIDIA_GEFORCE_7600},
    {"NV4A",                        CARD_NVIDIA_GEFORCE_6200},
    {"NV49",                        CARD_NVIDIA_GEFORCE_7800GT},    /* 7900 */
    {"NV47",                        CARD_NVIDIA_GEFORCE_7800GT},
    {"NV46",                        CARD_NVIDIA_GEFORCE_7400},
    {"NV45",                        CARD_NVIDIA_GEFORCE_6800},
    {"NV44",                        CARD_NVIDIA_GEFORCE_6200},
    {"NV43",                        CARD_NVIDIA_GEFORCE_6600GT},
    {"NV42",                        CARD_NVIDIA_GEFORCE_6800},
    {"NV41",                        CARD_NVIDIA_GEFORCE_6800},
    {"NV40",                        CARD_NVIDIA_GEFORCE_6800},
    /* Rankine */
    {"NV38",                        CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5950 Ultra */
    {"NV36",                        CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5700/5750 */
    {"NV35",                        CARD_NVIDIA_GEFORCEFX_5800},    /* FX 5900 */
    {"NV34",                        CARD_NVIDIA_GEFORCEFX_5200},
    {"NV31",                        CARD_NVIDIA_GEFORCEFX_5600},
    {"NV30",                        CARD_NVIDIA_GEFORCEFX_5800},
    /* Kelvin */
    {"nv28",                        CARD_NVIDIA_GEFORCE4_TI4200},
    {"nv25",                        CARD_NVIDIA_GEFORCE4_TI4200},
    {"nv20",                        CARD_NVIDIA_GEFORCE3},
    /* Celsius */
    {"nv1F",                        CARD_NVIDIA_GEFORCE4_MX},       /* GF4 MX IGP */
    {"nv1A",                        CARD_NVIDIA_GEFORCE2},          /* GF2 IGP */
    {"nv18",                        CARD_NVIDIA_GEFORCE4_MX},
    {"nv17",                        CARD_NVIDIA_GEFORCE4_MX},
    {"nv16",                        CARD_NVIDIA_GEFORCE2},
    {"nv15",                        CARD_NVIDIA_GEFORCE2},
    {"nv11",                        CARD_NVIDIA_GEFORCE2_MX},
    {"nv10",                        CARD_NVIDIA_GEFORCE},
    /* Fahrenheit */
    {"nv05",                        CARD_NVIDIA_RIVA_TNT2},
    {"nv04",                        CARD_NVIDIA_RIVA_TNT},
    {"nv03",                        CARD_NVIDIA_RIVA_128},
},
cards_vmware[] =
{
    {"SVGA3D",                      CARD_VMWARE_SVGA3D},
};

static const struct gl_vendor_selection
{
    enum wined3d_gl_vendor gl_vendor;
    const char *description;        /* Description of the card selector i.e. Apple OS/X Intel */
    const struct wined3d_renderer_table *cards; /* To be used as cards[], pointer to the first member in an array */
    size_t cards_size;              /* Number of entries in the array above */
}
amd_gl_vendor_table[] =
{
    {GL_VENDOR_APPLE,   "Apple OSX AMD/ATI binary driver",  cards_amd_binary,       ARRAY_SIZE(cards_amd_binary)},
    {GL_VENDOR_FGLRX,   "AMD/ATI binary driver",            cards_amd_binary,       ARRAY_SIZE(cards_amd_binary)},
    {GL_VENDOR_MESA,    "Mesa AMD/ATI driver",              cards_amd_mesa,         ARRAY_SIZE(cards_amd_mesa)},
},
nvidia_gl_vendor_table[] =
{
    {GL_VENDOR_APPLE,   "Apple OSX NVidia binary driver",   cards_nvidia_binary,    ARRAY_SIZE(cards_nvidia_binary)},
    {GL_VENDOR_MESA,    "Mesa Nouveau driver",              cards_nvidia_mesa,      ARRAY_SIZE(cards_nvidia_mesa)},
    {GL_VENDOR_NVIDIA,  "Nvidia binary driver",             cards_nvidia_binary,    ARRAY_SIZE(cards_nvidia_binary)},
},
vmware_gl_vendor_table[] =
{
    {GL_VENDOR_MESA,    "VMware driver",                    cards_vmware,           ARRAY_SIZE(cards_vmware)},
},
intel_gl_vendor_table[] =
{
    {GL_VENDOR_APPLE,   "Apple OSX Intel binary driver",    cards_intel,            ARRAY_SIZE(cards_intel)},
    {GL_VENDOR_MESA,    "Mesa Intel driver",                cards_intel,            ARRAY_SIZE(cards_intel)},
};

static const enum wined3d_pci_device
card_fallback_nvidia[] =
{
    CARD_NVIDIA_RIVA_128,           /* D3D5 */
    CARD_NVIDIA_RIVA_TNT,           /* D3D6 */
    CARD_NVIDIA_GEFORCE,            /* D3D7 */
    CARD_NVIDIA_GEFORCE3,           /* D3D8 */
    CARD_NVIDIA_GEFORCEFX_5800,     /* D3D9_SM2 */
    CARD_NVIDIA_GEFORCE_6800,       /* D3D9_SM3 */
    CARD_NVIDIA_GEFORCE_8800GTX,    /* D3D10 */
    CARD_NVIDIA_GEFORCE_GTX470,     /* D3D11 */
},
card_fallback_amd[] =
{
    CARD_AMD_RAGE_128PRO,           /* D3D5 */
    CARD_AMD_RAGE_128PRO,           /* D3D6 */
    CARD_AMD_RADEON_7200,           /* D3D7 */
    CARD_AMD_RADEON_8500,           /* D3D8 */
    CARD_AMD_RADEON_9500,           /* D3D9_SM2 */
    CARD_AMD_RADEON_X1600,          /* D3D9_SM3 */
    CARD_AMD_RADEON_HD2900,         /* D3D10 */
    CARD_AMD_RADEON_HD5600,         /* D3D11 */
},
card_fallback_intel[] =
{
    CARD_INTEL_845G,                /* D3D5 */
    CARD_INTEL_845G,                /* D3D6 */
    CARD_INTEL_845G,                /* D3D7 */
    CARD_INTEL_915G,                /* D3D8 */
    CARD_INTEL_915G,                /* D3D9_SM2 */
    CARD_INTEL_945G,                /* D3D9_SM3 */
    CARD_INTEL_G45,                 /* D3D10 */
    CARD_INTEL_IVBD,                /* D3D11 */
};
C_ASSERT(ARRAY_SIZE(card_fallback_nvidia)  == WINED3D_D3D_LEVEL_COUNT);
C_ASSERT(ARRAY_SIZE(card_fallback_amd)     == WINED3D_D3D_LEVEL_COUNT);
C_ASSERT(ARRAY_SIZE(card_fallback_intel)   == WINED3D_D3D_LEVEL_COUNT);

static enum wined3d_pci_device select_card_handler(const struct gl_vendor_selection *table,
        unsigned int table_size, enum wined3d_gl_vendor gl_vendor, const char *gl_renderer)
{
    unsigned int i, j;

    for (i = 0; i < table_size; ++i)
    {
        if (table[i].gl_vendor != gl_vendor)
            continue;

        TRACE("Applying card selector \"%s\".\n", table[i].description);

        for (j = 0; j < table[i].cards_size; ++j)
        {
            if (strstr(gl_renderer, table[i].cards[j].renderer))
                return table[i].cards[j].id;
        }
        return PCI_DEVICE_NONE;
    }
    FIXME("Couldn't find a suitable card selector for GL vendor %04x (using GL_RENDERER %s)\n",
            gl_vendor, debugstr_a(gl_renderer));

    return PCI_DEVICE_NONE;
}

static const struct
{
    enum wined3d_pci_vendor card_vendor;
    const char *description;        /* Description of the card selector i.e. Apple OS/X Intel */
    const struct gl_vendor_selection *gl_vendor_selection;
    unsigned int gl_vendor_count;
    const enum wined3d_pci_device *card_fallback; /* An array with D3D_LEVEL_COUNT elements */
}
card_vendor_table[] =
{
    {HW_VENDOR_AMD,         "AMD",      amd_gl_vendor_table,
            sizeof(amd_gl_vendor_table) / sizeof(*amd_gl_vendor_table),
            card_fallback_amd},
    {HW_VENDOR_NVIDIA,      "Nvidia",   nvidia_gl_vendor_table,
            sizeof(nvidia_gl_vendor_table) / sizeof(*nvidia_gl_vendor_table),
            card_fallback_nvidia},
    {HW_VENDOR_VMWARE,      "VMware",   vmware_gl_vendor_table,
            sizeof(vmware_gl_vendor_table) / sizeof(*vmware_gl_vendor_table),
            card_fallback_amd},
    {HW_VENDOR_INTEL,       "Intel",    intel_gl_vendor_table,
            sizeof(intel_gl_vendor_table) / sizeof(*intel_gl_vendor_table),
            card_fallback_intel},
};


static enum wined3d_pci_device wined3d_guess_card(const struct shader_caps *shader_caps, const struct fragment_caps *fragment_caps,
        DWORD glsl_version, const char *gl_renderer, enum wined3d_gl_vendor *gl_vendor, enum wined3d_pci_vendor *card_vendor)
{
    /* A Direct3D device object contains the PCI id (vendor + device) of the
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
     * the renderer string to distinguish between different models from that
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

    unsigned int i;
    enum wined3d_d3d_level d3d_level = d3d_level_from_caps(shader_caps, fragment_caps, glsl_version);
    enum wined3d_pci_device device;

    for (i = 0; i < (sizeof(card_vendor_table) / sizeof(*card_vendor_table)); ++i)
    {
        if (card_vendor_table[i].card_vendor != *card_vendor)
            continue;

        TRACE("Applying card selector \"%s\".\n", card_vendor_table[i].description);
        device = select_card_handler(card_vendor_table[i].gl_vendor_selection,
                card_vendor_table[i].gl_vendor_count, *gl_vendor, gl_renderer);
        if (device != PCI_DEVICE_NONE)
            return device;

        TRACE("Unrecognized renderer %s, falling back to default.\n", debugstr_a(gl_renderer));
        return card_vendor_table[i].card_fallback[d3d_level];
    }

    FIXME("No card selector available for card vendor %04x (using GL_RENDERER %s).\n",
            *card_vendor, debugstr_a(gl_renderer));

    /* Default to generic Nvidia hardware based on the supported OpenGL extensions. */
    *card_vendor = HW_VENDOR_NVIDIA;
    return card_fallback_nvidia[d3d_level];
}

static const struct wined3d_vertex_pipe_ops *select_vertex_implementation(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_backend_ops *shader_backend_ops)
{
    if (shader_backend_ops == &glsl_shader_backend)
        return &glsl_vertex_pipe;
    return &ffp_vertex_pipe;
}

static const struct fragment_pipeline *select_fragment_implementation(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_backend_ops *shader_backend_ops)
{
    if (shader_backend_ops == &glsl_shader_backend)
        return &glsl_fragment_pipe;
    if (shader_backend_ops == &arb_program_shader_backend && gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return &arbfp_fragment_pipeline;
    if (gl_info->supported[ATI_FRAGMENT_SHADER])
        return &atifs_fragment_pipeline;
    if (gl_info->supported[NV_REGISTER_COMBINERS] && gl_info->supported[NV_TEXTURE_SHADER2])
        return &nvts_fragment_pipeline;
    if (gl_info->supported[NV_REGISTER_COMBINERS])
        return &nvrc_fragment_pipeline;
    return &ffp_fragment_pipeline;
}

static const struct wined3d_shader_backend_ops *select_shader_backend(const struct wined3d_gl_info *gl_info)
{
    BOOL glsl = wined3d_settings.glslRequested && gl_info->glsl_version >= MAKEDWORD_VERSION(1, 20);

    if (glsl && gl_info->supported[ARB_FRAGMENT_SHADER])
        return &glsl_shader_backend;
    if (glsl && gl_info->supported[ARB_VERTEX_SHADER])
    {
        /* Geforce4 cards support GLSL but for vertex shaders only. Further
         * its reported GLSL caps are wrong. This combined with the fact that
         * GLSL won't offer more features or performance, use ARB shaders only
         * on this card. */
        if (gl_info->supported[NV_VERTEX_PROGRAM] && !gl_info->supported[NV_VERTEX_PROGRAM2])
            return &arb_program_shader_backend;
        return &glsl_shader_backend;
    }
    if (gl_info->supported[ARB_VERTEX_PROGRAM] || gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return &arb_program_shader_backend;
    return &none_shader_backend;
}

static const struct blit_shader *select_blit_implementation(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_backend_ops *shader_backend_ops)
{
    if ((shader_backend_ops == &glsl_shader_backend
            || shader_backend_ops == &arb_program_shader_backend)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return &arbfp_blit;
    return &ffp_blit;
}

static void parse_extension_string(struct wined3d_gl_info *gl_info, const char *extensions,
        const struct wined3d_extension_map *map, UINT entry_count)
{
    while (*extensions)
    {
        const char *start;
        size_t len;
        UINT i;

        while (isspace(*extensions))
            ++extensions;
        start = extensions;
        while (!isspace(*extensions) && *extensions)
            ++extensions;

        len = extensions - start;
        if (!len)
            continue;

        TRACE("- %s.\n", debugstr_an(start, len));

        for (i = 0; i < entry_count; ++i)
        {
            if (len == strlen(map[i].extension_string)
                    && !memcmp(start, map[i].extension_string, len))
            {
                TRACE(" FOUND: %s support.\n", map[i].extension_string);
                gl_info->supported[map[i].extension] = TRUE;
                break;
            }
        }
    }
}

static void enumerate_gl_extensions(struct wined3d_gl_info *gl_info,
        const struct wined3d_extension_map *map, unsigned int map_entries_count)
{
    const char *gl_extension_name;
    unsigned int i, j;
    GLint extensions_count;

    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
    for (i = 0; i < extensions_count; ++i)
    {
        gl_extension_name = (const char *)GL_EXTCALL(glGetStringi(GL_EXTENSIONS, i));
        TRACE("- %s.\n", debugstr_a(gl_extension_name));
        for (j = 0; j < map_entries_count; ++j)
        {
            if (!strcmp(gl_extension_name, map[j].extension_string))
            {
                TRACE("FOUND: %s support.\n", map[j].extension_string);
                gl_info->supported[map[j].extension] = TRUE;
                break;
            }
        }
    }
}

static void load_gl_funcs(struct wined3d_gl_info *gl_info)
{
#define USE_GL_FUNC(pfn) gl_info->gl_ops.ext.p_##pfn = (void *)wglGetProcAddress(#pfn);
    /* GL_APPLE_fence */
    USE_GL_FUNC(glDeleteFencesAPPLE)
    USE_GL_FUNC(glFinishFenceAPPLE)
    USE_GL_FUNC(glFinishObjectAPPLE)
    USE_GL_FUNC(glGenFencesAPPLE)
    USE_GL_FUNC(glIsFenceAPPLE)
    USE_GL_FUNC(glSetFenceAPPLE)
    USE_GL_FUNC(glTestFenceAPPLE)
    USE_GL_FUNC(glTestObjectAPPLE)
    /* GL_APPLE_flush_buffer_range */
    USE_GL_FUNC(glBufferParameteriAPPLE)
    USE_GL_FUNC(glFlushMappedBufferRangeAPPLE)
    /* GL_ARB_blend_func_extended */
    USE_GL_FUNC(glBindFragDataLocationIndexed)
    USE_GL_FUNC(glGetFragDataIndex)
    /* GL_ARB_color_buffer_float */
    USE_GL_FUNC(glClampColorARB)
    /* GL_ARB_debug_output */
    USE_GL_FUNC(glDebugMessageCallbackARB)
    USE_GL_FUNC(glDebugMessageControlARB)
    USE_GL_FUNC(glDebugMessageInsertARB)
    USE_GL_FUNC(glGetDebugMessageLogARB)
    /* GL_ARB_draw_buffers */
    USE_GL_FUNC(glDrawBuffersARB)
    /* GL_ARB_draw_elements_base_vertex */
    USE_GL_FUNC(glDrawElementsBaseVertex)
    USE_GL_FUNC(glDrawElementsInstancedBaseVertex)
    USE_GL_FUNC(glDrawRangeElementsBaseVertex)
    USE_GL_FUNC(glMultiDrawElementsBaseVertex)
    /* GL_ARB_draw_instanced */
    USE_GL_FUNC(glDrawArraysInstancedARB)
    USE_GL_FUNC(glDrawElementsInstancedARB)
    /* GL_ARB_ES2_compatibility */
    USE_GL_FUNC(glReleaseShaderCompiler)
    USE_GL_FUNC(glShaderBinary)
    USE_GL_FUNC(glGetShaderPrecisionFormat)
    USE_GL_FUNC(glDepthRangef)
    USE_GL_FUNC(glClearDepthf)
    /* GL_ARB_framebuffer_object */
    USE_GL_FUNC(glBindFramebuffer)
    USE_GL_FUNC(glBindRenderbuffer)
    USE_GL_FUNC(glBlitFramebuffer)
    USE_GL_FUNC(glCheckFramebufferStatus)
    USE_GL_FUNC(glDeleteFramebuffers)
    USE_GL_FUNC(glDeleteRenderbuffers)
    USE_GL_FUNC(glFramebufferRenderbuffer)
    USE_GL_FUNC(glFramebufferTexture1D)
    USE_GL_FUNC(glFramebufferTexture2D)
    USE_GL_FUNC(glFramebufferTexture3D)
    USE_GL_FUNC(glFramebufferTextureLayer)
    USE_GL_FUNC(glGenFramebuffers)
    USE_GL_FUNC(glGenRenderbuffers)
    USE_GL_FUNC(glGenerateMipmap)
    USE_GL_FUNC(glGetFramebufferAttachmentParameteriv)
    USE_GL_FUNC(glGetRenderbufferParameteriv)
    USE_GL_FUNC(glIsFramebuffer)
    USE_GL_FUNC(glIsRenderbuffer)
    USE_GL_FUNC(glRenderbufferStorage)
    USE_GL_FUNC(glRenderbufferStorageMultisample)
    /* GL_ARB_geometry_shader4 */
    USE_GL_FUNC(glFramebufferTextureARB)
    USE_GL_FUNC(glFramebufferTextureFaceARB)
    USE_GL_FUNC(glFramebufferTextureLayerARB)
    USE_GL_FUNC(glProgramParameteriARB)
    /* GL_ARB_instanced_arrays */
    USE_GL_FUNC(glVertexAttribDivisorARB)
    /* GL_ARB_internalformat_query */
    USE_GL_FUNC(glGetInternalformativ)
    /* GL_ARB_internalformat_query2 */
    USE_GL_FUNC(glGetInternalformati64v)
    /* GL_ARB_map_buffer_range */
    USE_GL_FUNC(glFlushMappedBufferRange)
    USE_GL_FUNC(glMapBufferRange)
    /* GL_ARB_multisample */
    USE_GL_FUNC(glSampleCoverageARB)
    /* GL_ARB_multitexture */
    USE_GL_FUNC(glActiveTextureARB)
    USE_GL_FUNC(glClientActiveTextureARB)
    USE_GL_FUNC(glMultiTexCoord1fARB)
    USE_GL_FUNC(glMultiTexCoord1fvARB)
    USE_GL_FUNC(glMultiTexCoord2fARB)
    USE_GL_FUNC(glMultiTexCoord2fvARB)
    USE_GL_FUNC(glMultiTexCoord2svARB)
    USE_GL_FUNC(glMultiTexCoord3fARB)
    USE_GL_FUNC(glMultiTexCoord3fvARB)
    USE_GL_FUNC(glMultiTexCoord4fARB)
    USE_GL_FUNC(glMultiTexCoord4fvARB)
    USE_GL_FUNC(glMultiTexCoord4svARB)
    /* GL_ARB_occlusion_query */
    USE_GL_FUNC(glBeginQueryARB)
    USE_GL_FUNC(glDeleteQueriesARB)
    USE_GL_FUNC(glEndQueryARB)
    USE_GL_FUNC(glGenQueriesARB)
    USE_GL_FUNC(glGetQueryivARB)
    USE_GL_FUNC(glGetQueryObjectivARB)
    USE_GL_FUNC(glGetQueryObjectuivARB)
    USE_GL_FUNC(glIsQueryARB)
    /* GL_ARB_point_parameters */
    USE_GL_FUNC(glPointParameterfARB)
    USE_GL_FUNC(glPointParameterfvARB)
    /* GL_ARB_provoking_vertex */
    USE_GL_FUNC(glProvokingVertex)
    /* GL_ARB_sampler_objects */
    USE_GL_FUNC(glGenSamplers)
    USE_GL_FUNC(glDeleteSamplers)
    USE_GL_FUNC(glIsSampler)
    USE_GL_FUNC(glBindSampler)
    USE_GL_FUNC(glSamplerParameteri)
    USE_GL_FUNC(glSamplerParameterf)
    USE_GL_FUNC(glSamplerParameteriv)
    USE_GL_FUNC(glSamplerParameterfv)
    USE_GL_FUNC(glSamplerParameterIiv)
    USE_GL_FUNC(glSamplerParameterIuiv)
    USE_GL_FUNC(glGetSamplerParameteriv)
    USE_GL_FUNC(glGetSamplerParameterfv)
    USE_GL_FUNC(glGetSamplerParameterIiv)
    USE_GL_FUNC(glGetSamplerParameterIuiv)
    /* GL_ARB_shader_objects */
    USE_GL_FUNC(glAttachObjectARB)
    USE_GL_FUNC(glBindAttribLocationARB)
    USE_GL_FUNC(glCompileShaderARB)
    USE_GL_FUNC(glCreateProgramObjectARB)
    USE_GL_FUNC(glCreateShaderObjectARB)
    USE_GL_FUNC(glDeleteObjectARB)
    USE_GL_FUNC(glDetachObjectARB)
    USE_GL_FUNC(glGetActiveUniformARB)
    USE_GL_FUNC(glGetAttachedObjectsARB)
    USE_GL_FUNC(glGetAttribLocationARB)
    USE_GL_FUNC(glGetHandleARB)
    USE_GL_FUNC(glGetInfoLogARB)
    USE_GL_FUNC(glGetObjectParameterfvARB)
    USE_GL_FUNC(glGetObjectParameterivARB)
    USE_GL_FUNC(glGetShaderSourceARB)
    USE_GL_FUNC(glGetUniformLocationARB)
    USE_GL_FUNC(glGetUniformfvARB)
    USE_GL_FUNC(glGetUniformivARB)
    USE_GL_FUNC(glLinkProgramARB)
    USE_GL_FUNC(glShaderSourceARB)
    USE_GL_FUNC(glUniform1fARB)
    USE_GL_FUNC(glUniform1fvARB)
    USE_GL_FUNC(glUniform1iARB)
    USE_GL_FUNC(glUniform1ivARB)
    USE_GL_FUNC(glUniform2fARB)
    USE_GL_FUNC(glUniform2fvARB)
    USE_GL_FUNC(glUniform2iARB)
    USE_GL_FUNC(glUniform2ivARB)
    USE_GL_FUNC(glUniform3fARB)
    USE_GL_FUNC(glUniform3fvARB)
    USE_GL_FUNC(glUniform3iARB)
    USE_GL_FUNC(glUniform3ivARB)
    USE_GL_FUNC(glUniform4fARB)
    USE_GL_FUNC(glUniform4fvARB)
    USE_GL_FUNC(glUniform4iARB)
    USE_GL_FUNC(glUniform4ivARB)
    USE_GL_FUNC(glUniformMatrix2fvARB)
    USE_GL_FUNC(glUniformMatrix3fvARB)
    USE_GL_FUNC(glUniformMatrix4fvARB)
    USE_GL_FUNC(glUseProgramObjectARB)
    USE_GL_FUNC(glValidateProgramARB)
    /* GL_ARB_sync */
    USE_GL_FUNC(glClientWaitSync)
    USE_GL_FUNC(glDeleteSync)
    USE_GL_FUNC(glFenceSync)
    USE_GL_FUNC(glGetInteger64v)
    USE_GL_FUNC(glGetSynciv)
    USE_GL_FUNC(glIsSync)
    USE_GL_FUNC(glWaitSync)
    /* GL_ARB_texture_compression */
    USE_GL_FUNC(glCompressedTexImage2DARB)
    USE_GL_FUNC(glCompressedTexImage3DARB)
    USE_GL_FUNC(glCompressedTexSubImage2DARB)
    USE_GL_FUNC(glCompressedTexSubImage3DARB)
    USE_GL_FUNC(glGetCompressedTexImageARB)
    /* GL_ARB_timer_query */
    USE_GL_FUNC(glQueryCounter)
    USE_GL_FUNC(glGetQueryObjectui64v)
    /* GL_ARB_uniform_buffer_object */
    USE_GL_FUNC(glBindBufferBase)
    USE_GL_FUNC(glBindBufferRange)
    USE_GL_FUNC(glGetActiveUniformBlockName)
    USE_GL_FUNC(glGetActiveUniformBlockiv)
    USE_GL_FUNC(glGetActiveUniformName)
    USE_GL_FUNC(glGetActiveUniformsiv)
    USE_GL_FUNC(glGetIntegeri_v)
    USE_GL_FUNC(glGetUniformBlockIndex)
    USE_GL_FUNC(glGetUniformIndices)
    USE_GL_FUNC(glUniformBlockBinding)
    /* GL_ARB_vertex_blend */
    USE_GL_FUNC(glVertexBlendARB)
    USE_GL_FUNC(glWeightPointerARB)
    USE_GL_FUNC(glWeightbvARB)
    USE_GL_FUNC(glWeightdvARB)
    USE_GL_FUNC(glWeightfvARB)
    USE_GL_FUNC(glWeightivARB)
    USE_GL_FUNC(glWeightsvARB)
    USE_GL_FUNC(glWeightubvARB)
    USE_GL_FUNC(glWeightuivARB)
    USE_GL_FUNC(glWeightusvARB)
    /* GL_ARB_vertex_buffer_object */
    USE_GL_FUNC(glBindBufferARB)
    USE_GL_FUNC(glBufferDataARB)
    USE_GL_FUNC(glBufferSubDataARB)
    USE_GL_FUNC(glDeleteBuffersARB)
    USE_GL_FUNC(glGenBuffersARB)
    USE_GL_FUNC(glGetBufferParameterivARB)
    USE_GL_FUNC(glGetBufferPointervARB)
    USE_GL_FUNC(glGetBufferSubDataARB)
    USE_GL_FUNC(glIsBufferARB)
    USE_GL_FUNC(glMapBufferARB)
    USE_GL_FUNC(glUnmapBufferARB)
    /* GL_ARB_vertex_program */
    USE_GL_FUNC(glBindProgramARB)
    USE_GL_FUNC(glDeleteProgramsARB)
    USE_GL_FUNC(glDisableVertexAttribArrayARB)
    USE_GL_FUNC(glEnableVertexAttribArrayARB)
    USE_GL_FUNC(glGenProgramsARB)
    USE_GL_FUNC(glGetProgramivARB)
    USE_GL_FUNC(glProgramEnvParameter4fvARB)
    USE_GL_FUNC(glProgramLocalParameter4fvARB)
    USE_GL_FUNC(glProgramStringARB)
    USE_GL_FUNC(glVertexAttrib1dARB)
    USE_GL_FUNC(glVertexAttrib1dvARB)
    USE_GL_FUNC(glVertexAttrib1fARB)
    USE_GL_FUNC(glVertexAttrib1fvARB)
    USE_GL_FUNC(glVertexAttrib1sARB)
    USE_GL_FUNC(glVertexAttrib1svARB)
    USE_GL_FUNC(glVertexAttrib2dARB)
    USE_GL_FUNC(glVertexAttrib2dvARB)
    USE_GL_FUNC(glVertexAttrib2fARB)
    USE_GL_FUNC(glVertexAttrib2fvARB)
    USE_GL_FUNC(glVertexAttrib2sARB)
    USE_GL_FUNC(glVertexAttrib2svARB)
    USE_GL_FUNC(glVertexAttrib3dARB)
    USE_GL_FUNC(glVertexAttrib3dvARB)
    USE_GL_FUNC(glVertexAttrib3fARB)
    USE_GL_FUNC(glVertexAttrib3fvARB)
    USE_GL_FUNC(glVertexAttrib3sARB)
    USE_GL_FUNC(glVertexAttrib3svARB)
    USE_GL_FUNC(glVertexAttrib4NbvARB)
    USE_GL_FUNC(glVertexAttrib4NivARB)
    USE_GL_FUNC(glVertexAttrib4NsvARB)
    USE_GL_FUNC(glVertexAttrib4NubARB)
    USE_GL_FUNC(glVertexAttrib4NubvARB)
    USE_GL_FUNC(glVertexAttrib4NuivARB)
    USE_GL_FUNC(glVertexAttrib4NusvARB)
    USE_GL_FUNC(glVertexAttrib4bvARB)
    USE_GL_FUNC(glVertexAttrib4dARB)
    USE_GL_FUNC(glVertexAttrib4dvARB)
    USE_GL_FUNC(glVertexAttrib4fARB)
    USE_GL_FUNC(glVertexAttrib4fvARB)
    USE_GL_FUNC(glVertexAttrib4ivARB)
    USE_GL_FUNC(glVertexAttrib4sARB)
    USE_GL_FUNC(glVertexAttrib4svARB)
    USE_GL_FUNC(glVertexAttrib4ubvARB)
    USE_GL_FUNC(glVertexAttrib4uivARB)
    USE_GL_FUNC(glVertexAttrib4usvARB)
    USE_GL_FUNC(glVertexAttribPointerARB)
    /* GL_ATI_fragment_shader */
    USE_GL_FUNC(glAlphaFragmentOp1ATI)
    USE_GL_FUNC(glAlphaFragmentOp2ATI)
    USE_GL_FUNC(glAlphaFragmentOp3ATI)
    USE_GL_FUNC(glBeginFragmentShaderATI)
    USE_GL_FUNC(glBindFragmentShaderATI)
    USE_GL_FUNC(glColorFragmentOp1ATI)
    USE_GL_FUNC(glColorFragmentOp2ATI)
    USE_GL_FUNC(glColorFragmentOp3ATI)
    USE_GL_FUNC(glDeleteFragmentShaderATI)
    USE_GL_FUNC(glEndFragmentShaderATI)
    USE_GL_FUNC(glGenFragmentShadersATI)
    USE_GL_FUNC(glPassTexCoordATI)
    USE_GL_FUNC(glSampleMapATI)
    USE_GL_FUNC(glSetFragmentShaderConstantATI)
    /* GL_ATI_separate_stencil */
    USE_GL_FUNC(glStencilOpSeparateATI)
    USE_GL_FUNC(glStencilFuncSeparateATI)
    /* GL_EXT_blend_color */
    USE_GL_FUNC(glBlendColorEXT)
    /* GL_EXT_blend_equation_separate */
    USE_GL_FUNC(glBlendFuncSeparateEXT)
    /* GL_EXT_blend_func_separate */
    USE_GL_FUNC(glBlendEquationSeparateEXT)
    /* GL_EXT_blend_minmax */
    USE_GL_FUNC(glBlendEquationEXT)
    /* GL_EXT_depth_bounds_test */
    USE_GL_FUNC(glDepthBoundsEXT)
    /* GL_EXT_draw_buffers2 */
    USE_GL_FUNC(glColorMaskIndexedEXT)
    USE_GL_FUNC(glDisableIndexedEXT)
    USE_GL_FUNC(glEnableIndexedEXT)
    USE_GL_FUNC(glGetBooleanIndexedvEXT)
    USE_GL_FUNC(glGetIntegerIndexedvEXT)
    USE_GL_FUNC(glIsEnabledIndexedEXT)
    /* GL_EXT_fog_coord */
    USE_GL_FUNC(glFogCoordPointerEXT)
    USE_GL_FUNC(glFogCoorddEXT)
    USE_GL_FUNC(glFogCoorddvEXT)
    USE_GL_FUNC(glFogCoordfEXT)
    USE_GL_FUNC(glFogCoordfvEXT)
    /* GL_EXT_framebuffer_blit */
    USE_GL_FUNC(glBlitFramebufferEXT)
    /* GL_EXT_framebuffer_multisample */
    USE_GL_FUNC(glRenderbufferStorageMultisampleEXT)
    /* GL_EXT_framebuffer_object */
    USE_GL_FUNC(glBindFramebufferEXT)
    USE_GL_FUNC(glBindRenderbufferEXT)
    USE_GL_FUNC(glCheckFramebufferStatusEXT)
    USE_GL_FUNC(glDeleteFramebuffersEXT)
    USE_GL_FUNC(glDeleteRenderbuffersEXT)
    USE_GL_FUNC(glFramebufferRenderbufferEXT)
    USE_GL_FUNC(glFramebufferTexture1DEXT)
    USE_GL_FUNC(glFramebufferTexture2DEXT)
    USE_GL_FUNC(glFramebufferTexture3DEXT)
    USE_GL_FUNC(glGenFramebuffersEXT)
    USE_GL_FUNC(glGenRenderbuffersEXT)
    USE_GL_FUNC(glGenerateMipmapEXT)
    USE_GL_FUNC(glGetFramebufferAttachmentParameterivEXT)
    USE_GL_FUNC(glGetRenderbufferParameterivEXT)
    USE_GL_FUNC(glIsFramebufferEXT)
    USE_GL_FUNC(glIsRenderbufferEXT)
    USE_GL_FUNC(glRenderbufferStorageEXT)
    /* GL_EXT_gpu_program_parameters */
    USE_GL_FUNC(glProgramEnvParameters4fvEXT)
    USE_GL_FUNC(glProgramLocalParameters4fvEXT)
    /* GL_EXT_gpu_shader4 */
    USE_GL_FUNC(glBindFragDataLocationEXT)
    USE_GL_FUNC(glGetFragDataLocationEXT)
    USE_GL_FUNC(glGetUniformuivEXT)
    USE_GL_FUNC(glGetVertexAttribIivEXT)
    USE_GL_FUNC(glGetVertexAttribIuivEXT)
    USE_GL_FUNC(glUniform1uiEXT)
    USE_GL_FUNC(glUniform1uivEXT)
    USE_GL_FUNC(glUniform2uiEXT)
    USE_GL_FUNC(glUniform2uivEXT)
    USE_GL_FUNC(glUniform3uiEXT)
    USE_GL_FUNC(glUniform3uivEXT)
    USE_GL_FUNC(glUniform4uiEXT)
    USE_GL_FUNC(glUniform4uivEXT)
    USE_GL_FUNC(glVertexAttribI1iEXT)
    USE_GL_FUNC(glVertexAttribI1ivEXT)
    USE_GL_FUNC(glVertexAttribI1uiEXT)
    USE_GL_FUNC(glVertexAttribI1uivEXT)
    USE_GL_FUNC(glVertexAttribI2iEXT)
    USE_GL_FUNC(glVertexAttribI2ivEXT)
    USE_GL_FUNC(glVertexAttribI2uiEXT)
    USE_GL_FUNC(glVertexAttribI2uivEXT)
    USE_GL_FUNC(glVertexAttribI3iEXT)
    USE_GL_FUNC(glVertexAttribI3ivEXT)
    USE_GL_FUNC(glVertexAttribI3uiEXT)
    USE_GL_FUNC(glVertexAttribI3uivEXT)
    USE_GL_FUNC(glVertexAttribI4bvEXT)
    USE_GL_FUNC(glVertexAttribI4iEXT)
    USE_GL_FUNC(glVertexAttribI4ivEXT)
    USE_GL_FUNC(glVertexAttribI4svEXT)
    USE_GL_FUNC(glVertexAttribI4ubvEXT)
    USE_GL_FUNC(glVertexAttribI4uiEXT)
    USE_GL_FUNC(glVertexAttribI4uivEXT)
    USE_GL_FUNC(glVertexAttribI4usvEXT)
    USE_GL_FUNC(glVertexAttribIPointerEXT)
    /* GL_EXT_point_parameters */
    USE_GL_FUNC(glPointParameterfEXT)
    USE_GL_FUNC(glPointParameterfvEXT)
    /* GL_EXT_provoking_vertex */
    USE_GL_FUNC(glProvokingVertexEXT)
    /* GL_EXT_secondary_color */
    USE_GL_FUNC(glSecondaryColor3fEXT)
    USE_GL_FUNC(glSecondaryColor3fvEXT)
    USE_GL_FUNC(glSecondaryColor3ubEXT)
    USE_GL_FUNC(glSecondaryColor3ubvEXT)
    USE_GL_FUNC(glSecondaryColorPointerEXT)
    /* GL_EXT_stencil_two_side */
    USE_GL_FUNC(glActiveStencilFaceEXT)
    /* GL_EXT_texture3D */
    USE_GL_FUNC(glTexImage3D)
    USE_GL_FUNC(glTexImage3DEXT)
    USE_GL_FUNC(glTexSubImage3D)
    USE_GL_FUNC(glTexSubImage3DEXT)
    /* GL_NV_fence */
    USE_GL_FUNC(glDeleteFencesNV)
    USE_GL_FUNC(glFinishFenceNV)
    USE_GL_FUNC(glGenFencesNV)
    USE_GL_FUNC(glGetFenceivNV)
    USE_GL_FUNC(glIsFenceNV)
    USE_GL_FUNC(glSetFenceNV)
    USE_GL_FUNC(glTestFenceNV)
    /* GL_NV_half_float */
    USE_GL_FUNC(glColor3hNV)
    USE_GL_FUNC(glColor3hvNV)
    USE_GL_FUNC(glColor4hNV)
    USE_GL_FUNC(glColor4hvNV)
    USE_GL_FUNC(glFogCoordhNV)
    USE_GL_FUNC(glFogCoordhvNV)
    USE_GL_FUNC(glMultiTexCoord1hNV)
    USE_GL_FUNC(glMultiTexCoord1hvNV)
    USE_GL_FUNC(glMultiTexCoord2hNV)
    USE_GL_FUNC(glMultiTexCoord2hvNV)
    USE_GL_FUNC(glMultiTexCoord3hNV)
    USE_GL_FUNC(glMultiTexCoord3hvNV)
    USE_GL_FUNC(glMultiTexCoord4hNV)
    USE_GL_FUNC(glMultiTexCoord4hvNV)
    USE_GL_FUNC(glNormal3hNV)
    USE_GL_FUNC(glNormal3hvNV)
    USE_GL_FUNC(glSecondaryColor3hNV)
    USE_GL_FUNC(glSecondaryColor3hvNV)
    USE_GL_FUNC(glTexCoord1hNV)
    USE_GL_FUNC(glTexCoord1hvNV)
    USE_GL_FUNC(glTexCoord2hNV)
    USE_GL_FUNC(glTexCoord2hvNV)
    USE_GL_FUNC(glTexCoord3hNV)
    USE_GL_FUNC(glTexCoord3hvNV)
    USE_GL_FUNC(glTexCoord4hNV)
    USE_GL_FUNC(glTexCoord4hvNV)
    USE_GL_FUNC(glVertex2hNV)
    USE_GL_FUNC(glVertex2hvNV)
    USE_GL_FUNC(glVertex3hNV)
    USE_GL_FUNC(glVertex3hvNV)
    USE_GL_FUNC(glVertex4hNV)
    USE_GL_FUNC(glVertex4hvNV)
    USE_GL_FUNC(glVertexAttrib1hNV)
    USE_GL_FUNC(glVertexAttrib1hvNV)
    USE_GL_FUNC(glVertexAttrib2hNV)
    USE_GL_FUNC(glVertexAttrib2hvNV)
    USE_GL_FUNC(glVertexAttrib3hNV)
    USE_GL_FUNC(glVertexAttrib3hvNV)
    USE_GL_FUNC(glVertexAttrib4hNV)
    USE_GL_FUNC(glVertexAttrib4hvNV)
    USE_GL_FUNC(glVertexAttribs1hvNV)
    USE_GL_FUNC(glVertexAttribs2hvNV)
    USE_GL_FUNC(glVertexAttribs3hvNV)
    USE_GL_FUNC(glVertexAttribs4hvNV)
    USE_GL_FUNC(glVertexWeighthNV)
    USE_GL_FUNC(glVertexWeighthvNV)
    /* GL_NV_point_sprite */
    USE_GL_FUNC(glPointParameteriNV)
    USE_GL_FUNC(glPointParameterivNV)
    /* GL_NV_register_combiners */
    USE_GL_FUNC(glCombinerInputNV)
    USE_GL_FUNC(glCombinerOutputNV)
    USE_GL_FUNC(glCombinerParameterfNV)
    USE_GL_FUNC(glCombinerParameterfvNV)
    USE_GL_FUNC(glCombinerParameteriNV)
    USE_GL_FUNC(glCombinerParameterivNV)
    USE_GL_FUNC(glFinalCombinerInputNV)
    /* WGL extensions */
    USE_GL_FUNC(wglChoosePixelFormatARB)
    USE_GL_FUNC(wglGetExtensionsStringARB)
    USE_GL_FUNC(wglGetPixelFormatAttribfvARB)
    USE_GL_FUNC(wglGetPixelFormatAttribivARB)
    USE_GL_FUNC(wglSetPixelFormatWINE)
    USE_GL_FUNC(wglSwapIntervalEXT)

    /* Newer core functions */
    USE_GL_FUNC(glActiveTexture)            /* OpenGL 1.3 */
    USE_GL_FUNC(glAttachShader)             /* OpenGL 2.0 */
    USE_GL_FUNC(glBeginQuery)               /* OpenGL 1.5 */
    USE_GL_FUNC(glBindAttribLocation)       /* OpenGL 2.0 */
    USE_GL_FUNC(glBindBuffer)               /* OpenGL 1.5 */
    USE_GL_FUNC(glBindVertexArray)          /* OpenGL 3.0 */
    USE_GL_FUNC(glBlendColor)               /* OpenGL 1.4 */
    USE_GL_FUNC(glBlendEquation)            /* OpenGL 1.4 */
    USE_GL_FUNC(glBlendEquationSeparate)    /* OpenGL 2.0 */
    USE_GL_FUNC(glBlendFuncSeparate)        /* OpenGL 1.4 */
    USE_GL_FUNC(glBufferData)               /* OpenGL 1.5 */
    USE_GL_FUNC(glBufferSubData)            /* OpenGL 1.5 */
    USE_GL_FUNC(glColorMaski)               /* OpenGL 3.0 */
    USE_GL_FUNC(glCompileShader)            /* OpenGL 2.0 */
    USE_GL_FUNC(glCompressedTexImage2D)     /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexImage3D)     /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexSubImage2D)  /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexSubImage3D)  /* OpenGL 1.3 */
    USE_GL_FUNC(glCreateProgram)            /* OpenGL 2.0 */
    USE_GL_FUNC(glCreateShader)             /* OpenGL 2.0 */
    USE_GL_FUNC(glDebugMessageCallback)     /* OpenGL 4.3 */
    USE_GL_FUNC(glDebugMessageControl)      /* OpenGL 4.3 */
    USE_GL_FUNC(glDebugMessageInsert)       /* OpenGL 4.3 */
    USE_GL_FUNC(glDeleteBuffers)            /* OpenGL 1.5 */
    USE_GL_FUNC(glDeleteProgram)            /* OpenGL 2.0 */
    USE_GL_FUNC(glDeleteQueries)            /* OpenGL 1.5 */
    USE_GL_FUNC(glDeleteShader)             /* OpenGL 2.0 */
    USE_GL_FUNC(glDeleteVertexArrays)       /* OpenGL 3.0 */
    USE_GL_FUNC(glDetachShader)             /* OpenGL 2.0 */
    USE_GL_FUNC(glDisableVertexAttribArray) /* OpenGL 2.0 */
    USE_GL_FUNC(glDrawArraysInstanced)      /* OpenGL 3.1 */
    USE_GL_FUNC(glDrawBuffers)              /* OpenGL 2.0 */
    USE_GL_FUNC(glDrawElementsInstanced)    /* OpenGL 3.1 */
    USE_GL_FUNC(glEnableVertexAttribArray)  /* OpenGL 2.0 */
    USE_GL_FUNC(glEndQuery)                 /* OpenGL 1.5 */
    USE_GL_FUNC(glGenBuffers)               /* OpenGL 1.5 */
    USE_GL_FUNC(glGenQueries)               /* OpenGL 1.5 */
    USE_GL_FUNC(glGenVertexArrays)          /* OpenGL 3.0 */
    USE_GL_FUNC(glGetActiveUniform)         /* OpenGL 2.0 */
    USE_GL_FUNC(glGetAttachedShaders)       /* OpenGL 2.0 */
    USE_GL_FUNC(glGetAttribLocation)        /* OpenGL 2.0 */
    USE_GL_FUNC(glGetBufferSubData)         /* OpenGL 1.5 */
    USE_GL_FUNC(glGetCompressedTexImage)    /* OpenGL 1.3 */
    USE_GL_FUNC(glGetDebugMessageLog)       /* OpenGL 4.3 */
    USE_GL_FUNC(glGetProgramInfoLog)        /* OpenGL 2.0 */
    USE_GL_FUNC(glGetProgramiv)             /* OpenGL 2.0 */
    USE_GL_FUNC(glGetQueryiv)               /* OpenGL 1.5 */
    USE_GL_FUNC(glGetQueryObjectuiv)        /* OpenGL 1.5 */
    USE_GL_FUNC(glGetShaderInfoLog)         /* OpenGL 2.0 */
    USE_GL_FUNC(glGetShaderiv)              /* OpenGL 2.0 */
    USE_GL_FUNC(glGetShaderSource)          /* OpenGL 2.0 */
    USE_GL_FUNC(glGetStringi)               /* OpenGL 3.0 */
    USE_GL_FUNC(glGetUniformfv)             /* OpenGL 2.0 */
    USE_GL_FUNC(glGetUniformiv)             /* OpenGL 2.0 */
    USE_GL_FUNC(glGetUniformLocation)       /* OpenGL 2.0 */
    USE_GL_FUNC(glLinkProgram)              /* OpenGL 2.0 */
    USE_GL_FUNC(glMapBuffer)                /* OpenGL 1.5 */
    USE_GL_FUNC(glPointParameteri)          /* OpenGL 1.4 */
    USE_GL_FUNC(glPointParameteriv)         /* OpenGL 1.4 */
    USE_GL_FUNC(glShaderSource)             /* OpenGL 2.0 */
    USE_GL_FUNC(glStencilFuncSeparate)      /* OpenGL 2.0 */
    USE_GL_FUNC(glStencilOpSeparate)        /* OpenGL 2.0 */
    USE_GL_FUNC(glTexImage3D)               /* OpenGL 1.2 */
    USE_GL_FUNC(glTexSubImage3D)            /* OpenGL 1.2 */
    USE_GL_FUNC(glUniform1f)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1fv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1i)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1iv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2f)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2fv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2i)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2iv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3f)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3fv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3i)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3iv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4f)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4fv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4i)                /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4iv)               /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix2fv)         /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix3fv)         /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix4fv)         /* OpenGL 2.0 */
    USE_GL_FUNC(glUnmapBuffer)              /* OpenGL 1.5 */
    USE_GL_FUNC(glUseProgram)               /* OpenGL 2.0 */
    USE_GL_FUNC(glValidateProgram)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib1f)           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib1fv)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib2f)           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib2fv)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib3f)           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib3fv)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4f)           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4fv)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nsv)         /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nubv)        /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nusv)        /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4sv)          /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4ubv)         /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttribDivisor)      /* OpenGL 3.3 */
    USE_GL_FUNC(glVertexAttribPointer)      /* OpenGL 2.0 */
#undef USE_GL_FUNC

#ifndef USE_WIN32_OPENGL
    /* hack: use the functions directly from the TEB table to bypass the thunks */
    /* note that we still need the above wglGetProcAddress calls to initialize the table */
    gl_info->gl_ops.ext = ((struct opengl_funcs *)NtCurrentTeb()->glTable)->ext;
#endif

#define MAP_GL_FUNCTION(core_func, ext_func)                                          \
        do                                                                            \
        {                                                                             \
            if (!gl_info->gl_ops.ext.p_##core_func)                                   \
                gl_info->gl_ops.ext.p_##core_func = gl_info->gl_ops.ext.p_##ext_func; \
        } while (0)
#define MAP_GL_FUNCTION_CAST(core_func, ext_func)                                             \
        do                                                                                    \
        {                                                                                     \
            if (!gl_info->gl_ops.ext.p_##core_func)                                           \
                gl_info->gl_ops.ext.p_##core_func = (void *)gl_info->gl_ops.ext.p_##ext_func; \
        } while (0)

    MAP_GL_FUNCTION(glActiveTexture, glActiveTextureARB);
    MAP_GL_FUNCTION(glAttachShader, glAttachObjectARB);
    MAP_GL_FUNCTION(glBeginQuery, glBeginQueryARB);
    MAP_GL_FUNCTION(glBindAttribLocation, glBindAttribLocationARB);
    MAP_GL_FUNCTION(glBindBuffer, glBindBufferARB);
    MAP_GL_FUNCTION(glBlendColor, glBlendColorEXT);
    MAP_GL_FUNCTION(glBlendEquation, glBlendEquationEXT);
    MAP_GL_FUNCTION(glBlendEquationSeparate, glBlendEquationSeparateEXT);
    MAP_GL_FUNCTION(glBlendFuncSeparate, glBlendFuncSeparateEXT);
    MAP_GL_FUNCTION(glBufferData, glBufferDataARB);
    MAP_GL_FUNCTION(glBufferSubData, glBufferSubDataARB);
    MAP_GL_FUNCTION(glColorMaski, glColorMaskIndexedEXT);
    MAP_GL_FUNCTION(glCompileShader, glCompileShaderARB);
    MAP_GL_FUNCTION(glCompressedTexImage2D, glCompressedTexImage2DARB);
    MAP_GL_FUNCTION(glCompressedTexImage3D, glCompressedTexImage3DARB);
    MAP_GL_FUNCTION(glCompressedTexSubImage2D, glCompressedTexSubImage2DARB);
    MAP_GL_FUNCTION(glCompressedTexSubImage3D, glCompressedTexSubImage3DARB);
    MAP_GL_FUNCTION(glCreateProgram, glCreateProgramObjectARB);
    MAP_GL_FUNCTION(glCreateShader, glCreateShaderObjectARB);
    MAP_GL_FUNCTION(glDebugMessageCallback, glDebugMessageCallbackARB);
    MAP_GL_FUNCTION(glDebugMessageControl, glDebugMessageControlARB);
    MAP_GL_FUNCTION(glDebugMessageInsert, glDebugMessageInsertARB);
    MAP_GL_FUNCTION(glDeleteBuffers, glDeleteBuffersARB);
    MAP_GL_FUNCTION(glDeleteProgram, glDeleteObjectARB);
    MAP_GL_FUNCTION(glDeleteQueries, glDeleteQueriesARB);
    MAP_GL_FUNCTION(glDeleteShader, glDeleteObjectARB);
    MAP_GL_FUNCTION(glDetachShader, glDetachObjectARB);
    MAP_GL_FUNCTION(glDisableVertexAttribArray, glDisableVertexAttribArrayARB);
    MAP_GL_FUNCTION(glDrawArraysInstanced, glDrawArraysInstancedARB);
    MAP_GL_FUNCTION(glDrawBuffers, glDrawBuffersARB);
    MAP_GL_FUNCTION(glDrawElementsInstanced, glDrawElementsInstancedARB);
    MAP_GL_FUNCTION(glEnableVertexAttribArray, glEnableVertexAttribArrayARB);
    MAP_GL_FUNCTION(glEndQuery, glEndQueryARB);
    MAP_GL_FUNCTION(glGenBuffers, glGenBuffersARB);
    MAP_GL_FUNCTION(glGenQueries, glGenQueriesARB);
    MAP_GL_FUNCTION(glGetActiveUniform, glGetActiveUniformARB);
    MAP_GL_FUNCTION(glGetAttachedShaders, glGetAttachedObjectsARB);
    MAP_GL_FUNCTION(glGetAttribLocation, glGetAttribLocationARB);
    MAP_GL_FUNCTION(glGetBufferSubData, glGetBufferSubDataARB);
    MAP_GL_FUNCTION(glGetCompressedTexImage, glGetCompressedTexImageARB);
    MAP_GL_FUNCTION(glGetDebugMessageLog, glGetDebugMessageLogARB);
    MAP_GL_FUNCTION(glGetProgramInfoLog, glGetInfoLogARB);
    MAP_GL_FUNCTION(glGetProgramiv, glGetObjectParameterivARB);
    MAP_GL_FUNCTION(glGetQueryiv, glGetQueryivARB);
    MAP_GL_FUNCTION(glGetQueryObjectuiv, glGetQueryObjectuivARB);
    MAP_GL_FUNCTION(glGetShaderInfoLog, glGetInfoLogARB);
    MAP_GL_FUNCTION(glGetShaderiv, glGetObjectParameterivARB);
    MAP_GL_FUNCTION(glGetShaderSource, glGetShaderSourceARB);
    MAP_GL_FUNCTION(glGetUniformfv, glGetUniformfvARB);
    MAP_GL_FUNCTION(glGetUniformiv, glGetUniformivARB);
    MAP_GL_FUNCTION(glGetUniformLocation, glGetUniformLocationARB);
    MAP_GL_FUNCTION(glLinkProgram, glLinkProgramARB);
    MAP_GL_FUNCTION(glMapBuffer, glMapBufferARB);
    MAP_GL_FUNCTION_CAST(glShaderSource, glShaderSourceARB);
    MAP_GL_FUNCTION_CAST(glTexImage3D, glTexImage3DEXT);
    MAP_GL_FUNCTION(glTexSubImage3D, glTexSubImage3DEXT);
    MAP_GL_FUNCTION(glUniform1f, glUniform1fARB);
    MAP_GL_FUNCTION(glUniform1fv, glUniform1fvARB);
    MAP_GL_FUNCTION(glUniform1i, glUniform1iARB);
    MAP_GL_FUNCTION(glUniform1iv, glUniform1ivARB);
    MAP_GL_FUNCTION(glUniform2f, glUniform2fARB);
    MAP_GL_FUNCTION(glUniform2fv, glUniform2fvARB);
    MAP_GL_FUNCTION(glUniform2i, glUniform2iARB);
    MAP_GL_FUNCTION(glUniform2iv, glUniform2ivARB);
    MAP_GL_FUNCTION(glUniform3f, glUniform3fARB);
    MAP_GL_FUNCTION(glUniform3fv, glUniform3fvARB);
    MAP_GL_FUNCTION(glUniform3i, glUniform3iARB);
    MAP_GL_FUNCTION(glUniform3iv, glUniform3ivARB);
    MAP_GL_FUNCTION(glUniform4f, glUniform4fARB);
    MAP_GL_FUNCTION(glUniform4fv, glUniform4fvARB);
    MAP_GL_FUNCTION(glUniform4i, glUniform4iARB);
    MAP_GL_FUNCTION(glUniform4iv, glUniform4ivARB);
    MAP_GL_FUNCTION(glUniformMatrix2fv, glUniformMatrix2fvARB);
    MAP_GL_FUNCTION(glUniformMatrix3fv, glUniformMatrix3fvARB);
    MAP_GL_FUNCTION(glUniformMatrix4fv, glUniformMatrix4fvARB);
    MAP_GL_FUNCTION(glUnmapBuffer, glUnmapBufferARB);
    MAP_GL_FUNCTION(glUseProgram, glUseProgramObjectARB);
    MAP_GL_FUNCTION(glValidateProgram, glValidateProgramARB);
    MAP_GL_FUNCTION(glVertexAttrib1f, glVertexAttrib1fARB);
    MAP_GL_FUNCTION(glVertexAttrib1fv, glVertexAttrib1fvARB);
    MAP_GL_FUNCTION(glVertexAttrib2f, glVertexAttrib2fARB);
    MAP_GL_FUNCTION(glVertexAttrib2fv, glVertexAttrib2fvARB);
    MAP_GL_FUNCTION(glVertexAttrib3f, glVertexAttrib3fARB);
    MAP_GL_FUNCTION(glVertexAttrib3fv, glVertexAttrib3fvARB);
    MAP_GL_FUNCTION(glVertexAttrib4f, glVertexAttrib4fARB);
    MAP_GL_FUNCTION(glVertexAttrib4fv, glVertexAttrib4fvARB);
    MAP_GL_FUNCTION(glVertexAttrib4Nsv, glVertexAttrib4NsvARB);
    MAP_GL_FUNCTION(glVertexAttrib4Nubv, glVertexAttrib4NubvARB);
    MAP_GL_FUNCTION(glVertexAttrib4Nusv, glVertexAttrib4NusvARB);
    MAP_GL_FUNCTION(glVertexAttrib4sv, glVertexAttrib4svARB);
    MAP_GL_FUNCTION(glVertexAttrib4ubv, glVertexAttrib4ubvARB);
    MAP_GL_FUNCTION(glVertexAttribDivisor, glVertexAttribDivisorARB);
    MAP_GL_FUNCTION(glVertexAttribPointer, glVertexAttribPointerARB);
#undef MAP_GL_FUNCTION
#undef MAP_GL_FUNCTION_CAST
}

static void wined3d_adapter_init_limits(struct wined3d_gl_info *gl_info)
{
    GLfloat gl_floatv[2];
    GLint gl_max;

    gl_info->limits.blends = 1;
    gl_info->limits.buffers = 1;
    gl_info->limits.textures = 1;
    gl_info->limits.texture_coords = 1;
    gl_info->limits.vertex_uniform_blocks = 0;
    gl_info->limits.geometry_uniform_blocks = 0;
    gl_info->limits.fragment_uniform_blocks = 0;
    gl_info->limits.fragment_samplers = 1;
    gl_info->limits.vertex_samplers = 0;
    gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers + gl_info->limits.vertex_samplers;
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

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    gl_info->limits.clipplanes = min(WINED3DMAXUSERCLIPPLANES, gl_max);
    TRACE("Clip plane support - max planes %d.\n", gl_max);

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    gl_info->limits.lights = gl_max;
    TRACE("Light support - max lights %d.\n", gl_max);

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    gl_info->limits.texture_size = gl_max;
    TRACE("Maximum texture size support - max texture size %d.\n", gl_max);

    gl_info->gl_ops.gl.p_glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv);
    gl_info->limits.pointsize_min = gl_floatv[0];
    gl_info->limits.pointsize_max = gl_floatv[1];
    TRACE("Maximum point size support - max point size %f.\n", gl_floatv[1]);

    if (gl_info->supported[ARB_MAP_BUFFER_ALIGNMENT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &gl_max);
        TRACE("Minimum buffer map alignment: %d.\n", gl_max);
    }
    else
    {
        WARN_(d3d_perf)("Driver doesn't guarantee a minimum buffer map alignment.\n");
    }
    if (gl_info->supported[NV_REGISTER_COMBINERS])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &gl_max);
        gl_info->limits.general_combiners = gl_max;
        TRACE("Max general combiners: %d.\n", gl_max);
    }
    if (gl_info->supported[ARB_DRAW_BUFFERS] && wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &gl_max);
        gl_info->limits.buffers = gl_max;
        TRACE("Max draw buffers: %u.\n", gl_max);
    }
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
        gl_info->limits.textures = min(MAX_TEXTURES, gl_max);
        TRACE("Max textures: %d.\n", gl_info->limits.textures);

        if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
        {
            GLint tmp;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &gl_max);
            gl_info->limits.texture_coords = min(MAX_TEXTURES, gl_max);
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.fragment_samplers = min(MAX_FRAGMENT_SAMPLERS, tmp);
        }
        else
        {
            gl_info->limits.texture_coords = max(gl_info->limits.texture_coords, gl_max);
            gl_info->limits.fragment_samplers = max(gl_info->limits.fragment_samplers, gl_max);
        }
        TRACE("Max texture coords: %d.\n", gl_info->limits.texture_coords);
        TRACE("Max fragment samplers: %d.\n", gl_info->limits.fragment_samplers);

        if (gl_info->supported[ARB_VERTEX_SHADER])
        {
            GLint tmp;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.vertex_samplers = tmp;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &tmp);
            gl_info->limits.combined_samplers = tmp;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &tmp);
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
        TRACE("Max vertex samplers: %u.\n", gl_info->limits.vertex_samplers);
        TRACE("Max combined samplers: %u.\n", gl_info->limits.combined_samplers);
    }
    if (gl_info->supported[ARB_VERTEX_BLEND])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
        gl_info->limits.blends = gl_max;
        TRACE("Max blends: %u.\n", gl_info->limits.blends);
    }
    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &gl_max);
        gl_info->limits.texture3d_size = gl_max;
        TRACE("Max texture3D size: %d.\n", gl_info->limits.texture3d_size);
    }
    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
        gl_info->limits.anisotropy = gl_max;
        TRACE("Max anisotropy: %d.\n", gl_info->limits.anisotropy);
    }
    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_float_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM float constants: %d.\n", gl_info->limits.arb_ps_float_constants);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_native_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_ps_native_constants);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
        gl_info->limits.arb_ps_temps = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_ps_temps);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
        gl_info->limits.arb_ps_instructions = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native instructions: %d.\n", gl_info->limits.arb_ps_instructions);
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_ps_local_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM local parameters: %d.\n", gl_info->limits.arb_ps_instructions);
    }
    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_vs_float_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM float constants: %d.\n", gl_info->limits.arb_vs_float_constants);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max));
        gl_info->limits.arb_vs_native_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_vs_native_constants);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max));
        gl_info->limits.arb_vs_temps = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_vs_temps);
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max));
        gl_info->limits.arb_vs_instructions = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native instructions: %d.\n", gl_info->limits.arb_vs_instructions);
    }
    if (gl_info->supported[ARB_VERTEX_SHADER])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &gl_max);
        gl_info->limits.glsl_vs_float_constants = gl_max / 4;
        TRACE("Max ARB_VERTEX_SHADER float constants: %u.\n", gl_info->limits.glsl_vs_float_constants);

        if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &gl_max);
            gl_info->limits.vertex_uniform_blocks = min(gl_max, WINED3D_MAX_CBS);
            TRACE("Max vertex uniform blocks: %u (%d).\n", gl_info->limits.vertex_uniform_blocks, gl_max);
        }
    }
    if (gl_info->supported[ARB_GEOMETRY_SHADER4] && gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &gl_max);
        gl_info->limits.geometry_uniform_blocks = min(gl_max, WINED3D_MAX_CBS);
        TRACE("Max geometry uniform blocks: %u (%d).\n", gl_info->limits.geometry_uniform_blocks, gl_max);
    }
    if (gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &gl_max);
        gl_info->limits.glsl_ps_float_constants = gl_max / 4;
        TRACE("Max ARB_FRAGMENT_SHADER float constants: %u.\n", gl_info->limits.glsl_ps_float_constants);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max);
        gl_info->limits.glsl_varyings = gl_max;
        TRACE("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_max, gl_max / 4);

        if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &gl_max);
            gl_info->limits.fragment_uniform_blocks = min(gl_max, WINED3D_MAX_CBS);
            TRACE("Max fragment uniform blocks: %u (%d).\n", gl_info->limits.fragment_uniform_blocks, gl_max);
        }
    }
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &gl_max);
        TRACE("Max combined uniform blocks: %d.\n", gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &gl_max);
        TRACE("Max uniform buffer bindings: %d.\n", gl_max);
    }

    if (gl_info->supported[NV_LIGHT_MAX_EXPONENT])
        gl_info->gl_ops.gl.p_glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess);
    else
        gl_info->limits.shininess = 128.0f;

    if ((gl_info->supported[ARB_FRAMEBUFFER_OBJECT] || gl_info->supported[EXT_FRAMEBUFFER_MULTISAMPLE])
            && wined3d_settings.allow_multisampling)
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
        gl_info->limits.samples = gl_max;
    }
}

/* Context activation is done by the caller. */
static BOOL wined3d_adapter_init_gl_caps(struct wined3d_adapter *adapter)
{
    static const struct
    {
        enum wined3d_gl_extension extension;
        DWORD min_gl_version;
    }
    core_extensions[] =
    {
        {EXT_TEXTURE3D,                    MAKEDWORD_VERSION(1, 2)},
        {ARB_MULTISAMPLE,                  MAKEDWORD_VERSION(1, 3)},
        {ARB_MULTITEXTURE,                 MAKEDWORD_VERSION(1, 3)},
        {ARB_TEXTURE_BORDER_CLAMP,         MAKEDWORD_VERSION(1, 3)},
        {ARB_TEXTURE_COMPRESSION,          MAKEDWORD_VERSION(1, 3)},
        {ARB_TEXTURE_CUBE_MAP,             MAKEDWORD_VERSION(1, 3)},
        {ARB_DEPTH_TEXTURE,                MAKEDWORD_VERSION(1, 4)},
        {ARB_POINT_PARAMETERS,             MAKEDWORD_VERSION(1, 4)},
        {ARB_SHADOW,                       MAKEDWORD_VERSION(1, 4)},
        {ARB_TEXTURE_MIRRORED_REPEAT,      MAKEDWORD_VERSION(1, 4)},
        {EXT_BLEND_COLOR,                  MAKEDWORD_VERSION(1, 4)},
        {EXT_BLEND_FUNC_SEPARATE,          MAKEDWORD_VERSION(1, 4)},
        {EXT_BLEND_MINMAX,                 MAKEDWORD_VERSION(1, 4)},
        {EXT_BLEND_SUBTRACT,               MAKEDWORD_VERSION(1, 4)},
        {EXT_STENCIL_WRAP,                 MAKEDWORD_VERSION(1, 4)},
        {NV_POINT_SPRITE,                  MAKEDWORD_VERSION(1, 4)},
        {ARB_OCCLUSION_QUERY,              MAKEDWORD_VERSION(1, 5)},
        {ARB_VERTEX_BUFFER_OBJECT,         MAKEDWORD_VERSION(1, 5)},
        {ARB_DRAW_BUFFERS,                 MAKEDWORD_VERSION(2, 0)},
        {ARB_FRAGMENT_SHADER,              MAKEDWORD_VERSION(2, 0)},
        {ARB_SHADING_LANGUAGE_100,         MAKEDWORD_VERSION(2, 0)},
        {ARB_TEXTURE_NON_POWER_OF_TWO,     MAKEDWORD_VERSION(2, 0)},
        {ARB_VERTEX_SHADER,                MAKEDWORD_VERSION(2, 0)},
        {EXT_BLEND_EQUATION_SEPARATE,      MAKEDWORD_VERSION(2, 0)},
        {ARB_PIXEL_BUFFER_OBJECT,          MAKEDWORD_VERSION(2, 1)},
        {EXT_TEXTURE_SRGB,                 MAKEDWORD_VERSION(2, 1)},
        {ARB_COLOR_BUFFER_FLOAT,           MAKEDWORD_VERSION(3, 0)},
        {ARB_DEPTH_BUFFER_FLOAT,           MAKEDWORD_VERSION(3, 0)},
        {ARB_FRAMEBUFFER_OBJECT,           MAKEDWORD_VERSION(3, 0)},
        {ARB_FRAMEBUFFER_SRGB,             MAKEDWORD_VERSION(3, 0)},
        {ARB_HALF_FLOAT_PIXEL,             MAKEDWORD_VERSION(3, 0)},
        {ARB_HALF_FLOAT_VERTEX,            MAKEDWORD_VERSION(3, 0)},
        {ARB_MAP_BUFFER_RANGE,             MAKEDWORD_VERSION(3, 0)},
        {ARB_TEXTURE_COMPRESSION_RGTC,     MAKEDWORD_VERSION(3, 0)},
        {ARB_TEXTURE_FLOAT,                MAKEDWORD_VERSION(3, 0)},
        {ARB_TEXTURE_RG,                   MAKEDWORD_VERSION(3, 0)},
        {EXT_DRAW_BUFFERS2,                MAKEDWORD_VERSION(3, 0)},
        /* We don't want to enable EXT_GPU_SHADER4: even though similar
         * functionality is available in core GL 3.0 / GLSL 1.30, it's different
         * enough that reusing the same flag for the new features hurts more
         * than it helps. */
        /* EXT_framebuffer_object, EXT_framebuffer_blit,
         * EXT_framebuffer_multisample and EXT_packed_depth_stencil
         * are integrated into ARB_framebuffer_object. */

        {ARB_DRAW_INSTANCED,               MAKEDWORD_VERSION(3, 1)},
        {ARB_UNIFORM_BUFFER_OBJECT,        MAKEDWORD_VERSION(3, 1)},
        {EXT_TEXTURE_SNORM,                MAKEDWORD_VERSION(3, 1)},
        /* We don't need or want GL_ARB_texture_rectangle (core in 3.1). */

        {ARB_DRAW_ELEMENTS_BASE_VERTEX,    MAKEDWORD_VERSION(3, 2)},
        /* ARB_geometry_shader4 exposes a somewhat different API compared to 3.2
         * core geometry shaders so it's not really correct to expose the
         * extension for core-only support. */
        {ARB_PROVOKING_VERTEX,             MAKEDWORD_VERSION(3, 2)},
        {ARB_SYNC,                         MAKEDWORD_VERSION(3, 2)},
        {ARB_VERTEX_ARRAY_BGRA,            MAKEDWORD_VERSION(3, 2)},

        {ARB_BLEND_FUNC_EXTENDED,          MAKEDWORD_VERSION(3, 3)},
        {ARB_INSTANCED_ARRAYS,             MAKEDWORD_VERSION(3, 3)},
        {ARB_SAMPLER_OBJECTS,              MAKEDWORD_VERSION(3, 3)},
        {ARB_SHADER_BIT_ENCODING,          MAKEDWORD_VERSION(3, 3)},
        {ARB_TIMER_QUERY,                  MAKEDWORD_VERSION(3, 3)},

        {ARB_MAP_BUFFER_ALIGNMENT,         MAKEDWORD_VERSION(4, 2)},

        {ARB_DEBUG_OUTPUT,                 MAKEDWORD_VERSION(4, 3)},
        {ARB_INTERNALFORMAT_QUERY2,        MAKEDWORD_VERSION(4, 3)},
    };
    struct wined3d_driver_info *driver_info = &adapter->driver_info;
    const char *gl_vendor_str, *gl_renderer_str, *gl_version_str;
    struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct wined3d_vertex_caps vertex_caps;
    enum wined3d_pci_vendor card_vendor;
    struct fragment_caps fragment_caps;
    struct shader_caps shader_caps;
    const char *WGL_Extensions = NULL;
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_device device;
    DWORD gl_version, gl_ext_emul_mask;
    HDC hdc;
    unsigned int i, j;
    GLint context_profile = 0;

    TRACE("adapter %p.\n", adapter);

    gl_renderer_str = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_RENDERER);
    TRACE("GL_RENDERER: %s.\n", debugstr_a(gl_renderer_str));
    if (!gl_renderer_str)
    {
        ERR("Received a NULL GL_RENDERER.\n");
        return FALSE;
    }

    gl_vendor_str = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_VENDOR);
    TRACE("GL_VENDOR: %s.\n", debugstr_a(gl_vendor_str));
    if (!gl_vendor_str)
    {
        ERR("Received a NULL GL_VENDOR.\n");
        return FALSE;
    }

    /* Parse the GL_VERSION field into major and minor information */
    gl_version_str = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_VERSION);
    TRACE("GL_VERSION: %s.\n", debugstr_a(gl_version_str));
    if (!gl_version_str)
    {
        ERR("Received a NULL GL_VERSION.\n");
        return FALSE;
    }
    gl_version = wined3d_parse_gl_version(gl_version_str);

    load_gl_funcs(gl_info);

    memset(gl_info->supported, 0, sizeof(gl_info->supported));
    gl_info->supported[WINED3D_GL_EXT_NONE] = TRUE;

    if (gl_version >= MAKEDWORD_VERSION(3, 2))
    {
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &context_profile);
        checkGLcall("Querying context profile");
    }
    if (context_profile & GL_CONTEXT_CORE_PROFILE_BIT)
        TRACE("Got a core profile context.\n");
    else
        gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] = TRUE;

    TRACE("GL extensions reported:\n");
    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        const char *gl_extensions = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_EXTENSIONS);

        if (!gl_extensions)
        {
            ERR("Received a NULL GL_EXTENSIONS.\n");
            return FALSE;
        }
        parse_extension_string(gl_info, gl_extensions, gl_extension_map,
                sizeof(gl_extension_map) / sizeof(*gl_extension_map));
    }
    else
    {
        enumerate_gl_extensions(gl_info, gl_extension_map,
                sizeof(gl_extension_map) / sizeof(*gl_extension_map));
    }

    hdc = wglGetCurrentDC();
    /* Not all GL drivers might offer WGL extensions e.g. VirtualBox. */
    if (GL_EXTCALL(wglGetExtensionsStringARB))
        WGL_Extensions = (const char *)GL_EXTCALL(wglGetExtensionsStringARB(hdc));
    if (!WGL_Extensions)
        WARN("WGL extensions not supported.\n");
    else
        parse_extension_string(gl_info, WGL_Extensions, wgl_extension_map,
                sizeof(wgl_extension_map) / sizeof(*wgl_extension_map));

    for (i = 0; i < ARRAY_SIZE(core_extensions); ++i)
    {
        if (!gl_info->supported[core_extensions[i].extension]
                && gl_version >= core_extensions[i].min_gl_version)
        {
            for (j = 0; j < ARRAY_SIZE(gl_extension_map); ++j)
                if (gl_extension_map[j].extension == core_extensions[i].extension)
                    break;

            if (j < ARRAY_SIZE(gl_extension_map))
            {
                TRACE("GL CORE: %s support.\n", gl_extension_map[j].extension_string);
                gl_info->supported[core_extensions[i].extension] = TRUE;
            }
            else
            {
                FIXME("GL extension %u not in the GL extensions map.\n", core_extensions[i].extension);
            }
        }
    }

    if (gl_info->supported[EXT_BLEND_MINMAX] || gl_info->supported[EXT_BLEND_SUBTRACT])
        gl_info->supported[WINED3D_GL_BLEND_EQUATION] = TRUE;

    if (gl_version >= MAKEDWORD_VERSION(2, 0))
        gl_info->supported[WINED3D_GL_VERSION_2_0] = TRUE;
    if (gl_version >= MAKEDWORD_VERSION(3, 2))
        gl_info->supported[WINED3D_GL_VERSION_3_2] = TRUE;

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
         * GL_RGBA16F_ARB     = GL_RGBA_FLOAT16_APPLE = 0x881a
         * GL_RGB16F_ARB      = GL_RGB_FLOAT16_APPLE  = 0x881b
         * GL_RGBA32F_ARB     = GL_RGBA_FLOAT32_APPLE = 0x8814
         * GL_RGB32F_ARB      = GL_RGB_FLOAT32_APPLE  = 0x8815
         * GL_HALF_FLOAT_ARB  = GL_HALF_APPLE         = 0x140b
         */
        if (!gl_info->supported[ARB_TEXTURE_FLOAT])
        {
            TRACE(" IMPLIED: GL_ARB_texture_float support (by GL_APPLE_float_pixels).\n");
            gl_info->supported[ARB_TEXTURE_FLOAT] = TRUE;
        }
        if (!gl_info->supported[ARB_HALF_FLOAT_PIXEL])
        {
            TRACE(" IMPLIED: GL_ARB_half_float_pixel support (by GL_APPLE_float_pixels).\n");
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
        TRACE(" IMPLIED: NVIDIA (NV) Texture Gen Reflection support.\n");
        gl_info->supported[NV_TEXGEN_REFLECTION] = TRUE;
    }
    if (!gl_info->supported[ARB_VERTEX_ARRAY_BGRA] && gl_info->supported[EXT_VERTEX_ARRAY_BGRA])
    {
        TRACE(" IMPLIED: ARB_vertex_array_bgra support (by EXT_vertex_array_bgra).\n");
        gl_info->supported[ARB_VERTEX_ARRAY_BGRA] = TRUE;
    }
    if (!gl_info->supported[EXT_TEXTURE_COMPRESSION_RGTC] && gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC])
    {
        TRACE(" IMPLIED: EXT_texture_compression_rgtc support (by ARB_texture_compression_rgtc).\n");
        gl_info->supported[EXT_TEXTURE_COMPRESSION_RGTC] = TRUE;
    }
    if (!gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] && gl_info->supported[EXT_TEXTURE_COMPRESSION_RGTC])
    {
        TRACE(" IMPLIED: ARB_texture_compression_rgtc support (by EXT_texture_compression_rgtc).\n");
        gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] = TRUE;
    }
    if (gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] && !gl_info->supported[ARB_TEXTURE_RG])
    {
        TRACE("ARB_texture_rg not supported, disabling ARB_texture_compression_rgtc.\n");
        gl_info->supported[ARB_TEXTURE_COMPRESSION_RGTC] = FALSE;
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
    if (gl_info->supported[ARB_FRAMEBUFFER_SRGB] && !gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        /* Current wined3d sRGB infrastructure requires EXT_texture_sRGB_decode
         * for GL_ARB_framebuffer_sRGB support (without EXT_texture_sRGB_decode
         * we never render to sRGB surfaces). */
        gl_info->supported[ARB_FRAMEBUFFER_SRGB] = FALSE;
    }
    if (gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        GLint counter_bits;

        GL_EXTCALL(glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &counter_bits));
        TRACE("Occlusion query counter has %d bits.\n", counter_bits);
        if (!counter_bits)
            gl_info->supported[ARB_OCCLUSION_QUERY] = FALSE;
    }
    if (gl_info->supported[ARB_TIMER_QUERY])
    {
        GLint counter_bits;

        GL_EXTCALL(glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &counter_bits));
        TRACE("Timestamp query counter has %d bits.\n", counter_bits);
        if (!counter_bits)
            gl_info->supported[ARB_TIMER_QUERY] = FALSE;
    }
    if (!gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] && gl_info->supported[EXT_TEXTURE_MIRROR_CLAMP])
    {
        TRACE(" IMPLIED: ATI_texture_mirror_once support (by EXT_texture_mirror_clamp).\n");
        gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] = TRUE;
    }
    if (!gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE] && gl_info->supported[ATI_TEXTURE_MIRROR_ONCE])
    {
        TRACE(" IMPLIED: ARB_texture_mirror_clamp_to_edge support (by ATI_texture_mirror_once).\n");
        gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE] = TRUE;
    }

    wined3d_adapter_init_limits(gl_info);

    if (gl_info->supported[ARB_VERTEX_PROGRAM] && test_arb_vs_offset_limit(gl_info))
        gl_info->quirks |= WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT;

    if (gl_info->supported[ARB_SHADING_LANGUAGE_100])
    {
        const char *str = (const char *)gl_info->gl_ops.gl.p_glGetString(GL_SHADING_LANGUAGE_VERSION_ARB);
        unsigned int major, minor;

        TRACE("GLSL version string: %s.\n", debugstr_a(str));

        /* The format of the GLSL version string is "major.minor[.release] [vendor info]". */
        sscanf(str, "%u.%u", &major, &minor);
        gl_info->glsl_version = MAKEDWORD_VERSION(major, minor);
    }

    checkGLcall("extension detection");

    adapter->shader_backend = select_shader_backend(gl_info);
    adapter->vertex_pipe = select_vertex_implementation(gl_info, adapter->shader_backend);
    adapter->fragment_pipe = select_fragment_implementation(gl_info, adapter->shader_backend);
    adapter->blitter = select_blit_implementation(gl_info, adapter->shader_backend);

    adapter->shader_backend->shader_get_caps(gl_info, &shader_caps);
    adapter->d3d_info.vs_clipping = shader_caps.wined3d_caps & WINED3D_SHADER_CAP_VS_CLIPPING;
    adapter->d3d_info.limits.vs_version = shader_caps.vs_version;
    adapter->d3d_info.limits.gs_version = shader_caps.gs_version;
    adapter->d3d_info.limits.ps_version = shader_caps.ps_version;
    adapter->d3d_info.limits.vs_uniform_count = shader_caps.vs_uniform_count;
    adapter->d3d_info.limits.ps_uniform_count = shader_caps.ps_uniform_count;
    adapter->d3d_info.limits.varying_count = shader_caps.varying_count;

    adapter->vertex_pipe->vp_get_caps(gl_info, &vertex_caps);
    adapter->d3d_info.xyzrhw = vertex_caps.xyzrhw;
    adapter->d3d_info.ffp_generic_attributes = vertex_caps.ffp_generic_attributes;
    adapter->d3d_info.limits.ffp_vertex_blend_matrices = vertex_caps.max_vertex_blend_matrices;
    adapter->d3d_info.emulated_flatshading = vertex_caps.emulated_flatshading;

    adapter->fragment_pipe->get_caps(gl_info, &fragment_caps);
    adapter->d3d_info.limits.ffp_blend_stages = fragment_caps.MaxTextureBlendStages;
    adapter->d3d_info.limits.ffp_textures = fragment_caps.MaxSimultaneousTextures;
    adapter->d3d_info.shader_color_key = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_COLOR_KEY;
    TRACE("Max texture stages: %u.\n", adapter->d3d_info.limits.ffp_blend_stages);

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
    {
        gl_info->fbo_ops.glIsRenderbuffer = gl_info->gl_ops.ext.p_glIsRenderbuffer;
        gl_info->fbo_ops.glBindRenderbuffer = gl_info->gl_ops.ext.p_glBindRenderbuffer;
        gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->gl_ops.ext.p_glDeleteRenderbuffers;
        gl_info->fbo_ops.glGenRenderbuffers = gl_info->gl_ops.ext.p_glGenRenderbuffers;
        gl_info->fbo_ops.glRenderbufferStorage = gl_info->gl_ops.ext.p_glRenderbufferStorage;
        gl_info->fbo_ops.glRenderbufferStorageMultisample = gl_info->gl_ops.ext.p_glRenderbufferStorageMultisample;
        gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->gl_ops.ext.p_glGetRenderbufferParameteriv;
        gl_info->fbo_ops.glIsFramebuffer = gl_info->gl_ops.ext.p_glIsFramebuffer;
        gl_info->fbo_ops.glBindFramebuffer = gl_info->gl_ops.ext.p_glBindFramebuffer;
        gl_info->fbo_ops.glDeleteFramebuffers = gl_info->gl_ops.ext.p_glDeleteFramebuffers;
        gl_info->fbo_ops.glGenFramebuffers = gl_info->gl_ops.ext.p_glGenFramebuffers;
        gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->gl_ops.ext.p_glCheckFramebufferStatus;
        gl_info->fbo_ops.glFramebufferTexture1D = gl_info->gl_ops.ext.p_glFramebufferTexture1D;
        gl_info->fbo_ops.glFramebufferTexture2D = gl_info->gl_ops.ext.p_glFramebufferTexture2D;
        gl_info->fbo_ops.glFramebufferTexture3D = gl_info->gl_ops.ext.p_glFramebufferTexture3D;
        gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->gl_ops.ext.p_glFramebufferRenderbuffer;
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv
                = gl_info->gl_ops.ext.p_glGetFramebufferAttachmentParameteriv;
        gl_info->fbo_ops.glBlitFramebuffer = gl_info->gl_ops.ext.p_glBlitFramebuffer;
        gl_info->fbo_ops.glGenerateMipmap = gl_info->gl_ops.ext.p_glGenerateMipmap;
    }
    else
    {
        if (gl_info->supported[EXT_FRAMEBUFFER_OBJECT])
        {
            gl_info->fbo_ops.glIsRenderbuffer = gl_info->gl_ops.ext.p_glIsRenderbufferEXT;
            gl_info->fbo_ops.glBindRenderbuffer = gl_info->gl_ops.ext.p_glBindRenderbufferEXT;
            gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->gl_ops.ext.p_glDeleteRenderbuffersEXT;
            gl_info->fbo_ops.glGenRenderbuffers = gl_info->gl_ops.ext.p_glGenRenderbuffersEXT;
            gl_info->fbo_ops.glRenderbufferStorage = gl_info->gl_ops.ext.p_glRenderbufferStorageEXT;
            gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->gl_ops.ext.p_glGetRenderbufferParameterivEXT;
            gl_info->fbo_ops.glIsFramebuffer = gl_info->gl_ops.ext.p_glIsFramebufferEXT;
            gl_info->fbo_ops.glBindFramebuffer = gl_info->gl_ops.ext.p_glBindFramebufferEXT;
            gl_info->fbo_ops.glDeleteFramebuffers = gl_info->gl_ops.ext.p_glDeleteFramebuffersEXT;
            gl_info->fbo_ops.glGenFramebuffers = gl_info->gl_ops.ext.p_glGenFramebuffersEXT;
            gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->gl_ops.ext.p_glCheckFramebufferStatusEXT;
            gl_info->fbo_ops.glFramebufferTexture1D = gl_info->gl_ops.ext.p_glFramebufferTexture1DEXT;
            gl_info->fbo_ops.glFramebufferTexture2D = gl_info->gl_ops.ext.p_glFramebufferTexture2DEXT;
            gl_info->fbo_ops.glFramebufferTexture3D = gl_info->gl_ops.ext.p_glFramebufferTexture3DEXT;
            gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->gl_ops.ext.p_glFramebufferRenderbufferEXT;
            gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv
                    = gl_info->gl_ops.ext.p_glGetFramebufferAttachmentParameterivEXT;
            gl_info->fbo_ops.glGenerateMipmap = gl_info->gl_ops.ext.p_glGenerateMipmapEXT;
        }
        else if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            WARN_(d3d_perf)("Framebuffer objects not supported, falling back to backbuffer offscreen rendering mode.\n");
            wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
        }
        if (gl_info->supported[EXT_FRAMEBUFFER_BLIT])
        {
            gl_info->fbo_ops.glBlitFramebuffer = gl_info->gl_ops.ext.p_glBlitFramebufferEXT;
        }
        if (gl_info->supported[EXT_FRAMEBUFFER_MULTISAMPLE])
        {
            gl_info->fbo_ops.glRenderbufferStorageMultisample
                    = gl_info->gl_ops.ext.p_glRenderbufferStorageMultisampleEXT;
        }
    }

    gl_vendor = wined3d_guess_gl_vendor(gl_info, gl_vendor_str, gl_renderer_str);
    card_vendor = wined3d_guess_card_vendor(gl_vendor_str, gl_renderer_str);
    TRACE("Found GL_VENDOR (%s)->(0x%04x/0x%04x).\n", debugstr_a(gl_vendor_str), gl_vendor, card_vendor);

    device = wined3d_guess_card(&shader_caps, &fragment_caps, gl_info->glsl_version, gl_renderer_str, &gl_vendor, &card_vendor);
    TRACE("Found (fake) card: 0x%x (vendor id), 0x%x (device id).\n", card_vendor, device);

    gl_info->wrap_lookup[WINED3D_TADDRESS_WRAP - WINED3D_TADDRESS_WRAP] = GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_CLAMP - WINED3D_TADDRESS_WRAP] = GL_CLAMP_TO_EDGE;
    gl_info->wrap_lookup[WINED3D_TADDRESS_BORDER - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE] ? GL_MIRROR_CLAMP_TO_EDGE : GL_REPEAT;

    adapter->d3d_info.valid_rt_mask = 0;
    for (i = 0; i < gl_info->limits.buffers; ++i)
        adapter->d3d_info.valid_rt_mask |= (1u << i);

    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        GLuint vao;

        GL_EXTCALL(glGenVertexArrays(1, &vao));
        GL_EXTCALL(glBindVertexArray(vao));
        checkGLcall("creating VAO");
    }

    fixup_extensions(gl_info, gl_renderer_str, gl_vendor, card_vendor, device);
    init_driver_info(gl_info, driver_info, card_vendor, device);
    gl_ext_emul_mask = adapter->vertex_pipe->vp_get_emul_mask(gl_info)
            | adapter->fragment_pipe->get_emul_mask(gl_info);
    if (gl_ext_emul_mask & GL_EXT_EMUL_ARB_MULTITEXTURE)
        install_gl_compat_wrapper(gl_info, ARB_MULTITEXTURE);
    if (gl_ext_emul_mask & GL_EXT_EMUL_EXT_FOG_COORD)
        install_gl_compat_wrapper(gl_info, EXT_FOG_COORD);

    return TRUE;
}

UINT CDECL wined3d_get_adapter_count(const struct wined3d *wined3d)
{
    TRACE("wined3d %p, reporting %u adapters.\n",
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
    TRACE("wined3d %p, adapter_idx %u.\n", wined3d, adapter_idx);

    if (adapter_idx >= wined3d->adapter_count)
        return NULL;

    return MonitorFromPoint(wined3d->adapters[adapter_idx].monitorPoint, MONITOR_DEFAULTTOPRIMARY);
}

/* FIXME: GetAdapterModeCount and EnumAdapterModes currently only returns modes
     of the same bpp but different resolutions                                  */

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
UINT CDECL wined3d_get_adapter_mode_count(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering)
{
    const struct wined3d_adapter *adapter;
    const struct wined3d_format *format;
    unsigned int i = 0;
    unsigned int j = 0;
    UINT format_bits;
    DEVMODEW mode;

    TRACE("wined3d %p, adapter_idx %u, format %s, scanline_ordering %#x.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), scanline_ordering);

    if (adapter_idx >= wined3d->adapter_count)
        return 0;

    adapter = &wined3d->adapters[adapter_idx];
    format = wined3d_get_format(&adapter->gl_info, format_id);
    format_bits = format->byte_count * CHAR_BIT;

    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);

    while (EnumDisplaySettingsExW(adapter->DeviceName, j++, &mode, 0))
    {
        if (mode.dmFields & DM_DISPLAYFLAGS)
        {
            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_PROGRESSIVE
                    && (mode.u2.dmDisplayFlags & DM_INTERLACED))
                continue;

            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED
                    && !(mode.u2.dmDisplayFlags & DM_INTERLACED))
                continue;
        }

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            /* This is for d3d8, do not enumerate P8 here. */
            if (mode.dmBitsPerPel == 32 || mode.dmBitsPerPel == 16) ++i;
        }
        else if (mode.dmBitsPerPel == format_bits)
        {
            ++i;
        }
    }

    TRACE("Returning %u matching modes (out of %u total) for adapter %u.\n", i, j, adapter_idx);

    return i;
}

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
HRESULT CDECL wined3d_enum_adapter_modes(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering,
        UINT mode_idx, struct wined3d_display_mode *mode)
{
    const struct wined3d_adapter *adapter;
    const struct wined3d_format *format;
    UINT format_bits;
    DEVMODEW m;
    UINT i = 0;
    int j = 0;

    TRACE("wined3d %p, adapter_idx %u, format %s, scanline_ordering %#x, mode_idx %u, mode %p.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), scanline_ordering, mode_idx, mode);

    if (!mode || adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    format = wined3d_get_format(&adapter->gl_info, format_id);
    format_bits = format->byte_count * CHAR_BIT;

    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    while (i <= mode_idx)
    {
        if (!EnumDisplaySettingsExW(adapter->DeviceName, j++, &m, 0))
        {
            WARN("Invalid mode_idx %u.\n", mode_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (m.dmFields & DM_DISPLAYFLAGS)
        {
            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_PROGRESSIVE
                    && (m.u2.dmDisplayFlags & DM_INTERLACED))
                continue;

            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED
                    && !(m.u2.dmDisplayFlags & DM_INTERLACED))
                continue;
        }

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            /* This is for d3d8, do not enumerate P8 here. */
            if (m.dmBitsPerPel == 32 || m.dmBitsPerPel == 16) ++i;
        }
        else if (m.dmBitsPerPel == format_bits)
        {
            ++i;
        }
    }

    mode->width = m.dmPelsWidth;
    mode->height = m.dmPelsHeight;
    mode->refresh_rate = DEFAULT_REFRESH_RATE;
    if (m.dmFields & DM_DISPLAYFREQUENCY)
        mode->refresh_rate = m.dmDisplayFrequency;

    if (format_id == WINED3DFMT_UNKNOWN)
        mode->format_id = pixelformat_for_depth(m.dmBitsPerPel);
    else
        mode->format_id = format_id;

    if (!(m.dmFields & DM_DISPLAYFLAGS))
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    else if (m.u2.dmDisplayFlags & DM_INTERLACED)
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_INTERLACED;
    else
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_PROGRESSIVE;

    TRACE("%ux%u@%u %u bpp, %s %#x.\n", mode->width, mode->height, mode->refresh_rate,
            m.dmBitsPerPel, debug_d3dformat(mode->format_id), mode->scanline_ordering);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_adapter_display_mode(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    const struct wined3d_adapter *adapter;
    DEVMODEW m;

    TRACE("wined3d %p, adapter_idx %u, display_mode %p, rotation %p.\n",
            wined3d, adapter_idx, mode, rotation);

    if (!mode || adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];

    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    EnumDisplaySettingsExW(adapter->DeviceName, ENUM_CURRENT_SETTINGS, &m, 0);
    mode->width = m.dmPelsWidth;
    mode->height = m.dmPelsHeight;
    mode->refresh_rate = DEFAULT_REFRESH_RATE;
    if (m.dmFields & DM_DISPLAYFREQUENCY)
        mode->refresh_rate = m.dmDisplayFrequency;
    mode->format_id = pixelformat_for_depth(m.dmBitsPerPel);

    /* Lie about the format. X11 can't change the color depth, and some apps
     * are pretty angry if they SetDisplayMode from 24 to 16 bpp and find out
     * that GetDisplayMode still returns 24 bpp. This should probably be
     * handled in winex11 instead. */
    if (adapter->screen_format && adapter->screen_format != mode->format_id)
    {
        WARN("Overriding format %s with stored format %s.\n",
                debug_d3dformat(mode->format_id),
                debug_d3dformat(adapter->screen_format));
        mode->format_id = adapter->screen_format;
    }

    if (!(m.dmFields & DM_DISPLAYFLAGS))
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    else if (m.u2.dmDisplayFlags & DM_INTERLACED)
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_INTERLACED;
    else
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_PROGRESSIVE;

    if (rotation)
    {
        switch (m.u1.s2.dmDisplayOrientation)
        {
            case DMDO_DEFAULT:
                *rotation = WINED3D_DISPLAY_ROTATION_0;
                break;
            case DMDO_90:
                *rotation = WINED3D_DISPLAY_ROTATION_90;
                break;
            case DMDO_180:
                *rotation = WINED3D_DISPLAY_ROTATION_180;
                break;
            case DMDO_270:
                *rotation = WINED3D_DISPLAY_ROTATION_270;
                break;
            default:
                FIXME("Unhandled display rotation %#x.\n", m.u1.s2.dmDisplayOrientation);
                *rotation = WINED3D_DISPLAY_ROTATION_UNSPECIFIED;
                break;
        }
    }

    TRACE("Returning %ux%u@%u %s %#x.\n", mode->width, mode->height,
            mode->refresh_rate, debug_d3dformat(mode->format_id),
            mode->scanline_ordering);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_set_adapter_display_mode(struct wined3d *wined3d,
        UINT adapter_idx, const struct wined3d_display_mode *mode)
{
    struct wined3d_adapter *adapter;
    DEVMODEW new_mode, current_mode;
    RECT clip_rc;
    LONG ret;
    enum wined3d_format_id new_format_id;

    TRACE("wined3d %p, adapter_idx %u, mode %p.\n", wined3d, adapter_idx, mode);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;
    adapter = &wined3d->adapters[adapter_idx];

    memset(&new_mode, 0, sizeof(new_mode));
    new_mode.dmSize = sizeof(new_mode);
    memset(&current_mode, 0, sizeof(current_mode));
    current_mode.dmSize = sizeof(current_mode);
    if (mode)
    {
        const struct wined3d_format *format;

        TRACE("mode %ux%u@%u %s %#x.\n", mode->width, mode->height, mode->refresh_rate,
                debug_d3dformat(mode->format_id), mode->scanline_ordering);

        format = wined3d_get_format(&adapter->gl_info, mode->format_id);

        new_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        new_mode.dmBitsPerPel = format->byte_count * CHAR_BIT;
        new_mode.dmPelsWidth = mode->width;
        new_mode.dmPelsHeight = mode->height;

        new_mode.dmDisplayFrequency = mode->refresh_rate;
        if (mode->refresh_rate)
            new_mode.dmFields |= DM_DISPLAYFREQUENCY;

        if (mode->scanline_ordering != WINED3D_SCANLINE_ORDERING_UNKNOWN)
        {
            new_mode.dmFields |= DM_DISPLAYFLAGS;
            if (mode->scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED)
                new_mode.u2.dmDisplayFlags |= DM_INTERLACED;
        }
        new_format_id = mode->format_id;
    }
    else
    {
        if (!EnumDisplaySettingsW(adapter->DeviceName, ENUM_REGISTRY_SETTINGS, &new_mode))
        {
            ERR("Failed to read mode from registry.\n");
            return WINED3DERR_NOTAVAILABLE;
        }
        new_format_id = pixelformat_for_depth(new_mode.dmBitsPerPel);
    }

    /* Only change the mode if necessary. */
    if (!EnumDisplaySettingsW(adapter->DeviceName, ENUM_CURRENT_SETTINGS, &current_mode))
    {
        ERR("Failed to get current display mode.\n");
    }
    else if (current_mode.dmPelsWidth == new_mode.dmPelsWidth
            && current_mode.dmPelsHeight == new_mode.dmPelsHeight
            && current_mode.dmBitsPerPel == new_mode.dmBitsPerPel
            && (current_mode.dmDisplayFrequency == new_mode.dmDisplayFrequency
            || !(new_mode.dmFields & DM_DISPLAYFREQUENCY))
            && (current_mode.u2.dmDisplayFlags == new_mode.u2.dmDisplayFlags
            || !(new_mode.dmFields & DM_DISPLAYFLAGS)))
    {
        TRACE("Skipping redundant mode setting call.\n");
        return WINED3D_OK;
    }

    ret = ChangeDisplaySettingsExW(adapter->DeviceName, &new_mode, NULL, CDS_FULLSCREEN, NULL);
    if (ret != DISP_CHANGE_SUCCESSFUL)
    {
        if (new_mode.dmFields & DM_DISPLAYFREQUENCY)
        {
            WARN("ChangeDisplaySettingsExW failed, trying without the refresh rate.\n");
            new_mode.dmFields &= ~DM_DISPLAYFREQUENCY;
            new_mode.dmDisplayFrequency = 0;
            ret = ChangeDisplaySettingsExW(adapter->DeviceName, &new_mode, NULL, CDS_FULLSCREEN, NULL);
        }
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return WINED3DERR_NOTAVAILABLE;
    }

    /* Store the new values. */
    adapter->screen_format = new_format_id;

    /* And finally clip mouse to our screen. */
    SetRect(&clip_rc, 0, 0, new_mode.dmPelsWidth, new_mode.dmPelsHeight);
    ClipCursor(&clip_rc);

    return WINED3D_OK;
}

/* NOTE: due to structure differences between dx8 and dx9 D3DADAPTER_IDENTIFIER,
   and fields being inserted in the middle, a new structure is used in place    */
HRESULT CDECL wined3d_get_adapter_identifier(const struct wined3d *wined3d,
        UINT adapter_idx, DWORD flags, struct wined3d_adapter_identifier *identifier)
{
    const struct wined3d_adapter *adapter;
    size_t len;

    TRACE("wined3d %p, adapter_idx %u, flags %#x, identifier %p.\n",
            wined3d, adapter_idx, flags, identifier);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];

    if (identifier->driver_size)
    {
        const char *name = adapter->driver_info.name;
        len = min(strlen(name), identifier->driver_size - 1);
        memcpy(identifier->driver, name, len);
        memset(&identifier->driver[len], 0, identifier->driver_size - len);
    }

    if (identifier->description_size)
    {
        const char *description = adapter->driver_info.description;
        len = min(strlen(description), identifier->description_size - 1);
        memcpy(identifier->description, description, len);
        memset(&identifier->description[len], 0, identifier->description_size - len);
    }

    /* Note that d3d8 doesn't supply a device name. */
    if (identifier->device_name_size)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, adapter->DeviceName, -1, identifier->device_name,
                identifier->device_name_size, NULL, NULL))
        {
            ERR("Failed to convert device name, last error %#x.\n", GetLastError());
            return WINED3DERR_INVALIDCALL;
        }
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
    identifier->video_memory = min(~(SIZE_T)0, adapter->vram_bytes);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_adapter_raster_status(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_raster_status *raster_status)
{
    LONGLONG freq_per_frame, freq_per_line;
    LARGE_INTEGER counter, freq_per_sec;
    struct wined3d_display_mode mode;
    static UINT once;

    if (!once++)
        FIXME("wined3d %p, adapter_idx %u, raster_status %p semi-stub!\n",
                wined3d, adapter_idx, raster_status);
    else
        WARN("wined3d %p, adapter_idx %u, raster_status %p semi-stub!\n",
                wined3d, adapter_idx, raster_status);

    /* Obtaining the raster status is a widely implemented but optional
     * feature. When this method returns OK StarCraft 2 expects the
     * raster_status->InVBlank value to actually change over time.
     * And Endless Alice Crysis doesn't care even if this method fails.
     * Thus this method returns OK and fakes raster_status by
     * QueryPerformanceCounter. */

    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&freq_per_sec))
        return WINED3DERR_INVALIDCALL;
    if (FAILED(wined3d_get_adapter_display_mode(wined3d, adapter_idx, &mode, NULL)))
        return WINED3DERR_INVALIDCALL;
    if (mode.refresh_rate == DEFAULT_REFRESH_RATE)
        mode.refresh_rate = 60;

    freq_per_frame = freq_per_sec.QuadPart / mode.refresh_rate;
    /* Assume 20 scan lines in the vertical blank. */
    freq_per_line = freq_per_frame / (mode.height + 20);
    raster_status->scan_line = (counter.QuadPart % freq_per_frame) / freq_per_line;
    if (raster_status->scan_line < mode.height)
        raster_status->in_vblank = FALSE;
    else
    {
        raster_status->scan_line = 0;
        raster_status->in_vblank = TRUE;
    }

    TRACE("Returning fake value, in_vblank %u, scan_line %u.\n",
            raster_status->in_vblank, raster_status->scan_line);

    return WINED3D_OK;
}

static BOOL wined3d_check_pixel_format_color(const struct wined3d_gl_info *gl_info,
        const struct wined3d_pixel_format *cfg, const struct wined3d_format *format)
{
    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
        return FALSE;

    /* Probably a RGBA_float or color index mode. */
    if (cfg->iPixelType != WGL_TYPE_RGBA_ARB)
        return FALSE;

    if (cfg->redSize < format->red_size
            || cfg->greenSize < format->green_size
            || cfg->blueSize < format->blue_size
            || cfg->alphaSize < format->alpha_size)
        return FALSE;

    return TRUE;
}

static BOOL wined3d_check_pixel_format_depth(const struct wined3d_gl_info *gl_info,
        const struct wined3d_pixel_format *cfg, const struct wined3d_format *format)
{
    BOOL lockable = FALSE;

    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
        return FALSE;

    if ((format->id == WINED3DFMT_D16_LOCKABLE) || (format->id == WINED3DFMT_D32_FLOAT))
        lockable = TRUE;

    /* On some modern cards like the Geforce8/9, GLX doesn't offer some
     * dephth/stencil formats which D3D9 reports. We can safely report
     * "compatible" formats (e.g. D24 can be used for D16) as long as we
     * aren't dealing with a lockable format. This also helps D3D <= 7 as they
     * expect D16 which isn't offered without this on Geforce8 cards. */
    if (!(cfg->depthSize == format->depth_size || (!lockable && cfg->depthSize > format->depth_size)))
        return FALSE;

    /* Some cards like Intel i915 ones only offer D24S8 but lots of games also
     * need a format without stencil. We can allow a mismatch if the format
     * doesn't have any stencil bits. If it does have stencil bits the size
     * must match, or stencil wrapping would break. */
    if (format->stencil_size && cfg->stencilSize != format->stencil_size)
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

    TRACE("wined3d %p, adapter_idx %u, device_type %s,\n"
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
        if ((rt_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_RENDERTARGET)
                && (ds_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        {
            TRACE("Formats match.\n");
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
                TRACE("Formats match.\n");
                return WINED3D_OK;
            }
        }
    }

    TRACE("Unsupported format pair: %s and %s.\n",
            debug_d3dformat(render_target_format_id),
            debug_d3dformat(depth_stencil_format_id));

    return WINED3DERR_NOTAVAILABLE;
}

HRESULT CDECL wined3d_check_device_multisample_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id surface_format_id, BOOL windowed,
        enum wined3d_multisample_type multisample_type, DWORD *quality_levels)
{
    const struct wined3d_gl_info *gl_info;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, surface_format %s,\n"
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
        if (wined3d_settings.msaa_quality_levels)
        {
            *quality_levels = wined3d_settings.msaa_quality_levels;
            TRACE("Overriding MSAA quality levels to %i\n", *quality_levels);
        }
        else if (multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE)
            *quality_levels = gl_info->limits.samples;
        else
            *quality_levels = 1;
    }

    return WINED3D_OK;
}

/* Check if the given DisplayFormat + DepthStencilFormat combination is valid for the Adapter */
static BOOL CheckDepthStencilCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *display_format, const struct wined3d_format *ds_format,
        enum wined3d_gl_resource_type gl_type)
{
    /* Only allow depth/stencil formats */
    if (!(ds_format->depth_size || ds_format->stencil_size)) return FALSE;

    /* Blacklist formats not supported on Windows */
    switch (ds_format->id)
    {
        case WINED3DFMT_S1_UINT_D15_UNORM: /* Breaks the shadowvol2 dx7 sdk sample */
        case WINED3DFMT_S4X4_UINT_D24_UNORM:
            TRACE("[FAILED] - not supported on windows.\n");
            return FALSE;

        default:
            break;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        /* With FBOs WGL limitations do not apply, but the format needs to be FBO attachable */
        if (ds_format->flags[gl_type] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            return TRUE;
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

/* Check the render target capabilities of a format */
static BOOL CheckRenderTargetCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format, const struct wined3d_format *check_format,
        enum wined3d_gl_resource_type gl_type)
{
    /* Filter out non-RT formats */
    if (!(check_format->flags[gl_type] & WINED3DFMT_FLAG_RENDERTARGET))
        return FALSE;
    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_pixel_format *cfgs = adapter->cfgs;
        unsigned int i;

        /* In backbuffer mode the front and backbuffer share the same WGL
         * pixelformat. The format must match in RGB, alpha is allowed to be
         * different. (Only the backbuffer can have alpha.) */
        if (adapter_format->red_size != check_format->red_size
                || adapter_format->green_size != check_format->green_size
                || adapter_format->blue_size != check_format->blue_size)
        {
            TRACE("[FAILED]\n");
            return FALSE;
        }

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        for (i = 0; i < adapter->cfg_count; ++i)
        {
            if (cfgs[i].windowDrawable
                    && wined3d_check_pixel_format_color(&adapter->gl_info, &cfgs[i], check_format))
            {
                TRACE("Pixel format %d is compatible with format %s.\n",
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

static BOOL CheckSurfaceCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format,
        const struct wined3d_format *check_format, BOOL no3d)
{
    if (no3d)
    {
        switch (check_format->id)
        {
            case WINED3DFMT_B8G8R8_UNORM:
                TRACE("[FAILED] - Not enumerated on Windows.\n");
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
                TRACE("[OK]\n");
                return TRUE;
            default:
                TRACE("[FAILED] - Not available on GDI surfaces.\n");
                return FALSE;
        }
    }

    /* All formats that are supported for textures are supported for surfaces
     * as well. */
    if (check_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_TEXTURE)
        return TRUE;
    /* All depth stencil formats are supported on surfaces */
    if (CheckDepthStencilCapability(adapter, adapter_format, check_format, WINED3D_GL_RES_TYPE_TEX_2D))
        return TRUE;
    if (CheckDepthStencilCapability(adapter, adapter_format, check_format, WINED3D_GL_RES_TYPE_RB))
        return TRUE;

    /* If opengl can't process the format natively, the blitter may be able to convert it */
    if (adapter->blitter->blit_supported(&adapter->gl_info, &adapter->d3d_info,
            WINED3D_BLIT_OP_COLOR_BLIT,
            NULL, WINED3D_POOL_DEFAULT, 0, check_format,
            NULL, WINED3D_POOL_DEFAULT, 0, adapter_format))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    /* Reject other formats */
    TRACE("[FAILED]\n");
    return FALSE;
}

/* OpenGL supports mipmapping on all formats. Wrapping is unsupported, but we
 * have to report mipmapping so we cannot reject WRAPANDMIP. Tests show that
 * Windows reports WRAPANDMIP on unfilterable surfaces as well, apparently to
 * show that wrapping is supported. The lack of filtering will sort out the
 * mipmapping capability anyway.
 *
 * For now lets report this on all formats, but in the future we may want to
 * restrict it to some should applications need that. */
HRESULT CDECL wined3d_check_device_format(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id, DWORD usage,
        enum wined3d_resource_type resource_type, enum wined3d_format_id check_format_id)
{
    const struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    const struct wined3d_format *adapter_format = wined3d_get_format(gl_info, adapter_format_id);
    const struct wined3d_format *format = wined3d_get_format(gl_info, check_format_id);
    DWORD format_flags = 0;
    DWORD allowed_usage;
    enum wined3d_gl_resource_type gl_type;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, adapter_format %s, usage %s, %s,\n"
            "resource_type %s, check_format %s.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(adapter_format_id),
            debug_d3dusage(usage), debug_d3dusagequery(usage), debug_d3dresourcetype(resource_type),
            debug_d3dformat(check_format_id));

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    switch (resource_type)
    {
        case WINED3D_RTYPE_CUBE_TEXTURE:
            format_flags |= WINED3DFMT_FLAG_TEXTURE;
            allowed_usage = WINED3DUSAGE_AUTOGENMIPMAP
                    | WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_RENDERTARGET
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            gl_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            break;

        case WINED3D_RTYPE_SURFACE:
            if (!CheckSurfaceCapability(adapter, adapter_format, format, wined3d->flags & WINED3D_NO3D))
            {
                TRACE("[FAILED] - Not supported for plain surfaces.\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            allowed_usage = WINED3DUSAGE_DEPTHSTENCIL
                    | WINED3DUSAGE_RENDERTARGET
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            gl_type = WINED3D_GL_RES_TYPE_RB;
            break;

        case WINED3D_RTYPE_TEXTURE:
            if ((usage & WINED3DUSAGE_DEPTHSTENCIL)
                    && (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_SHADOW)
                    && !gl_info->supported[ARB_SHADOW])
            {
                TRACE("[FAILED] - No shadow sampler support.\n");
                return WINED3DERR_NOTAVAILABLE;
            }

            format_flags |= WINED3DFMT_FLAG_TEXTURE;
            allowed_usage = WINED3DUSAGE_AUTOGENMIPMAP
                    | WINED3DUSAGE_DEPTHSTENCIL
                    | WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_RENDERTARGET
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_LEGACYBUMPMAP
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            gl_type = WINED3D_GL_RES_TYPE_TEX_2D;
            break;

        case WINED3D_RTYPE_VOLUME_TEXTURE:
        case WINED3D_RTYPE_VOLUME:
            format_flags |= WINED3DFMT_FLAG_TEXTURE;
            allowed_usage = WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            gl_type = WINED3D_GL_RES_TYPE_TEX_3D;
            break;

        default:
            FIXME("Unhandled resource type %s.\n", debug_d3dresourcetype(resource_type));
            return WINED3DERR_NOTAVAILABLE;
    }

    if ((usage & allowed_usage) != usage)
    {
        TRACE("Requested usage %#x, but resource type %s only allows %#x.\n",
                usage, debug_d3dresourcetype(resource_type), allowed_usage);
        return WINED3DERR_NOTAVAILABLE;
    }

    if (usage & WINED3DUSAGE_QUERY_FILTER)
        format_flags |= WINED3DFMT_FLAG_FILTERING;
    if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
        format_flags |= WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
    if (usage & WINED3DUSAGE_QUERY_SRGBREAD)
        format_flags |= WINED3DFMT_FLAG_SRGB_READ;
    if (usage & WINED3DUSAGE_QUERY_SRGBWRITE)
        format_flags |= WINED3DFMT_FLAG_SRGB_WRITE;
    if (usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE)
        format_flags |= WINED3DFMT_FLAG_VTF;
    if (usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP)
        format_flags |= WINED3DFMT_FLAG_BUMPMAP;

    if ((format->flags[gl_type] & format_flags) != format_flags)
    {
        TRACE("Requested format flags %#x, but format %s only has %#x.\n",
                format_flags, debug_d3dformat(check_format_id), format->flags[gl_type]);
        return WINED3DERR_NOTAVAILABLE;
    }

    if ((format_flags & WINED3DFMT_FLAG_TEXTURE) && (wined3d->flags & WINED3D_NO3D))
    {
        TRACE("Requested texturing support, but wined3d was created with WINED3D_NO3D.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if ((usage & WINED3DUSAGE_DEPTHSTENCIL)
            && !CheckDepthStencilCapability(adapter, adapter_format, format, gl_type))
    {
        TRACE("Requested WINED3DUSAGE_DEPTHSTENCIL, but format %s is not supported for depth / stencil buffers.\n",
                debug_d3dformat(check_format_id));
        return WINED3DERR_NOTAVAILABLE;
    }

    if ((usage & WINED3DUSAGE_RENDERTARGET)
            && !CheckRenderTargetCapability(adapter, adapter_format, format, gl_type))
    {
        TRACE("Requested WINED3DUSAGE_RENDERTARGET, but format %s is not supported for render targets.\n",
                debug_d3dformat(check_format_id));
        return WINED3DERR_NOTAVAILABLE;
    }

    if ((usage & WINED3DUSAGE_AUTOGENMIPMAP) && !gl_info->supported[SGIS_GENERATE_MIPMAP])
    {
        TRACE("No WINED3DUSAGE_AUTOGENMIPMAP support, returning WINED3DOK_NOAUTOGEN.\n");
        return WINED3DOK_NOAUTOGEN;
    }

    return WINED3D_OK;
}

UINT CDECL wined3d_calculate_format_pitch(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, UINT width)
{
    const struct wined3d_gl_info *gl_info;

    TRACE("wined3d %p, adapter_idx %u, format_id %s, width %u.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), width);

    if (adapter_idx >= wined3d->adapter_count)
        return ~0u;

    gl_info = &wined3d->adapters[adapter_idx].gl_info;
    return wined3d_format_calculate_pitch(wined3d_get_format(gl_info, format_id), width);
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
    BOOL present_conversion = wined3d->flags & WINED3D_PRESENT_CONVERSION;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, display_format %s, backbuffer_format %s, windowed %#x.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(display_format),
            debug_d3dformat(backbuffer_format), windowed);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    /* The task of this function is to check whether a certain display / backbuffer format
     * combination is available on the given adapter. In fullscreen mode microsoft specified
     * that the display format shouldn't provide alpha and that ignoring alpha the backbuffer
     * and display format should match exactly.
     * In windowed mode format conversion can occur and this depends on the driver. */

    /* There are only 4 display formats. */
    if (!(display_format == WINED3DFMT_B5G6R5_UNORM
            || display_format == WINED3DFMT_B5G5R5X1_UNORM
            || display_format == WINED3DFMT_B8G8R8X8_UNORM
            || display_format == WINED3DFMT_B10G10R10A2_UNORM))
    {
        TRACE("Format %s is not supported as display format.\n", debug_d3dformat(display_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!windowed)
    {
        /* If the requested display format is not available, don't continue. */
        if (!wined3d_get_adapter_mode_count(wined3d, adapter_idx,
                display_format, WINED3D_SCANLINE_ORDERING_UNKNOWN))
        {
            TRACE("No available modes for display format %s.\n", debug_d3dformat(display_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        present_conversion = FALSE;
    }
    else if (display_format == WINED3DFMT_B10G10R10A2_UNORM)
    {
        /* WINED3DFMT_B10G10R10A2_UNORM is only allowed in fullscreen mode. */
        TRACE("Unsupported format combination %s / %s in windowed mode.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    if (present_conversion)
    {
        /* Use the display format as back buffer format if the latter is
         * WINED3DFMT_UNKNOWN. */
        if (backbuffer_format == WINED3DFMT_UNKNOWN)
            backbuffer_format = display_format;

        if (FAILED(wined3d_check_device_format_conversion(wined3d, adapter_idx,
                device_type, backbuffer_format, display_format)))
        {
            TRACE("Format conversion from %s to %s not supported.\n",
                    debug_d3dformat(backbuffer_format), debug_d3dformat(display_format));
            return WINED3DERR_NOTAVAILABLE;
        }
    }
    else
    {
        /* When format conversion from the back buffer format to the display
         * format is not allowed, only a limited number of combinations are
         * valid. */

        if (display_format == WINED3DFMT_B5G6R5_UNORM && backbuffer_format != WINED3DFMT_B5G6R5_UNORM)
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B5G5R5X1_UNORM
                && !(backbuffer_format == WINED3DFMT_B5G5R5X1_UNORM || backbuffer_format == WINED3DFMT_B5G5R5A1_UNORM))
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B8G8R8X8_UNORM
                && !(backbuffer_format == WINED3DFMT_B8G8R8X8_UNORM || backbuffer_format == WINED3DFMT_B8G8R8A8_UNORM))
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B10G10R10A2_UNORM
                && backbuffer_format != WINED3DFMT_B10G10R10A2_UNORM)
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    /* Validate that the back buffer format is usable for render targets. */
    if (FAILED(wined3d_check_device_format(wined3d, adapter_idx, device_type, display_format,
            WINED3DUSAGE_RENDERTARGET, WINED3D_RTYPE_SURFACE, backbuffer_format)))
    {
        TRACE("Format %s not allowed for render targets.\n", debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_device_caps(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, WINED3DCAPS *caps)
{
    const struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct shader_caps shader_caps;
    struct fragment_caps fragment_caps;
    struct wined3d_vertex_caps vertex_caps;
    DWORD ckey_caps, blit_caps, fx_caps;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, caps %p.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), caps);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

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
                                     WINED3DDEVCAPS_DRAWPRIMITIVES2EX;

    caps->PrimitiveMiscCaps        = WINED3DPMISCCAPS_CULLNONE              |
                                     WINED3DPMISCCAPS_CULLCCW               |
                                     WINED3DPMISCCAPS_CULLCW                |
                                     WINED3DPMISCCAPS_COLORWRITEENABLE      |
                                     WINED3DPMISCCAPS_CLIPTLVERTS           |
                                     WINED3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                                     WINED3DPMISCCAPS_MASKZ                 |
                                     WINED3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING;
                                    /* TODO:
                                        WINED3DPMISCCAPS_NULLREFERENCE
                                        WINED3DPMISCCAPS_FOGANDSPECULARALPHA
                                        WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                                        WINED3DPMISCCAPS_FOGVERTEXCLAMPED */

    if (gl_info->supported[WINED3D_GL_BLEND_EQUATION])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_BLENDOP;
    if (gl_info->supported[EXT_BLEND_EQUATION_SEPARATE] && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_SEPARATEALPHABLEND;
    if (gl_info->supported[EXT_DRAW_BUFFERS2])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS;
    if (gl_info->supported[ARB_FRAMEBUFFER_SRGB])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_POSTBLENDSRGBCONVERT;

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

    caps->ZCmpCaps =  WINED3DPCMPCAPS_ALWAYS       |
                      WINED3DPCMPCAPS_EQUAL        |
                      WINED3DPCMPCAPS_GREATER      |
                      WINED3DPCMPCAPS_GREATEREQUAL |
                      WINED3DPCMPCAPS_LESS         |
                      WINED3DPCMPCAPS_LESSEQUAL    |
                      WINED3DPCMPCAPS_NEVER        |
                      WINED3DPCMPCAPS_NOTEQUAL;

    /* WINED3DPBLENDCAPS_BOTHINVSRCALPHA and WINED3DPBLENDCAPS_BOTHSRCALPHA
     * are legacy settings for srcblend only. */
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

    if (gl_info->supported[ARB_BLEND_FUNC_EXTENDED])
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_SRCALPHASAT;

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
        caps->TextureCaps |= WINED3DPTEXTURECAPS_POW2;
        if (gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT] || gl_info->supported[ARB_TEXTURE_RECTANGLE])
            caps->TextureCaps |= WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;
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
    if (gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE])
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
        if (gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE])
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
    if (gl_info->supported[WINED3D_GL_VERSION_2_0] || gl_info->supported[EXT_STENCIL_TWO_SIDE]
            || gl_info->supported[ATI_SEPARATE_STENCIL])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    caps->MaxAnisotropy = gl_info->limits.anisotropy;
    caps->MaxPointSize = gl_info->limits.pointsize_max;

    caps->MaxPrimitiveCount   = 0x555555; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    caps->MaxVertexIndex      = 0xffffff; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
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
    adapter->vertex_pipe->vp_get_caps(&adapter->gl_info, &vertex_caps);

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    caps->PrimitiveMiscCaps |= fragment_caps.PrimitiveMiscCaps;

    caps->VertexShaderVersion = shader_caps.vs_version;
    caps->MaxVertexShaderConst = shader_caps.vs_uniform_count;

    caps->PixelShaderVersion = shader_caps.ps_version;
    caps->PixelShader1xMaxValue = shader_caps.ps_1x_max_value;

    caps->TextureOpCaps                    = fragment_caps.TextureOpCaps;
    caps->MaxTextureBlendStages            = fragment_caps.MaxTextureBlendStages;
    caps->MaxSimultaneousTextures          = fragment_caps.MaxSimultaneousTextures;

    caps->MaxUserClipPlanes                = vertex_caps.max_user_clip_planes;
    caps->MaxActiveLights                  = vertex_caps.max_active_lights;
    caps->MaxVertexBlendMatrices           = vertex_caps.max_vertex_blend_matrices;
    caps->MaxVertexBlendMatrixIndex        = vertex_caps.max_vertex_blend_matrix_index;
    caps->VertexProcessingCaps             = vertex_caps.vertex_processing_caps;
    caps->FVFCaps                          = vertex_caps.fvf_caps;
    caps->RasterCaps                      |= vertex_caps.raster_caps;

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
        caps->VertexTextureFilterCaps = WINED3DPTFILTERCAPS_MINFPOINT | WINED3DPTFILTERCAPS_MAGFPOINT;
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
                                        WINEDDFXCAPS_BLTSHRINKYN            |
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

    if (!(wined3d->flags & WINED3D_NO3D))
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

    TRACE("wined3d %p, adapter_idx %u, device_type %#x, focus_window %p, flags %#x, surface_alignment %u, device_parent %p, device %p.\n",
            wined3d, adapter_idx, device_type, focus_window, flags, surface_alignment, device_parent, device);

    /* Validate the adapter number. If no adapters are available(no GL), ignore the adapter
     * number and create a device without a 3D adapter for 2D only operation. */
    if (wined3d->adapter_count && adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

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

#if defined(STAGING_CSMT)
/* Helper functions for providing vertex data to opengl. The arrays are initialized based on
 * the extension detection and are used in draw_strided_slow
 */
#else  /* STAGING_CSMT */
/* Helper functions for providing vertex data to opengl. The arrays are initialized based on
 * the extension detection and are used in drawStridedSlow
 */
#endif /* STAGING_CSMT */
static void WINE_GLAPI position_d3dcolor(const void *data)
{
    DWORD pos = *((const DWORD *)data);

    FIXME("Add a test for fixed function position from d3dcolor type\n");
    context_get_current()->gl_info->gl_ops.gl.p_glVertex4s(D3DCOLOR_B_R(pos),
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

        context_get_current()->gl_info->gl_ops.gl.p_glVertex4f(pos[0] * w, pos[1] * w, pos[2] * w, w);
    }
    else
    {
        context_get_current()->gl_info->gl_ops.gl.p_glVertex3fv(pos);
    }
}

static void WINE_GLAPI diffuse_d3dcolor(const void *data)
{
    DWORD diffuseColor = *((const DWORD *)data);

    context_get_current()->gl_info->gl_ops.gl.p_glColor4ub(D3DCOLOR_B_R(diffuseColor),
            D3DCOLOR_B_G(diffuseColor),
            D3DCOLOR_B_B(diffuseColor),
            D3DCOLOR_B_A(diffuseColor));
}

static void WINE_GLAPI specular_d3dcolor(const void *data)
{
    DWORD specularColor = *((const DWORD *)data);
    GLubyte d[] =
    {
        D3DCOLOR_B_R(specularColor),
        D3DCOLOR_B_G(specularColor),
        D3DCOLOR_B_B(specularColor)
    };

    context_get_current()->gl_info->gl_ops.ext.p_glSecondaryColor3ubvEXT(d);
}

static void WINE_GLAPI warn_no_specular_func(const void *data)
{
    WARN("GL_EXT_secondary_color not supported\n");
}

static void wined3d_adapter_init_ffp_attrib_ops(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    struct wined3d_ffp_attrib_ops *ops = &d3d_info->ffp_attrib_ops;

    ops->position[WINED3D_FFP_EMIT_FLOAT1]    = invalid_func;
    ops->position[WINED3D_FFP_EMIT_FLOAT2]    = invalid_func;
    ops->position[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex3fv;
    if (!d3d_info->xyzrhw)
        ops->position[WINED3D_FFP_EMIT_FLOAT4]    = position_float4;
    else
        ops->position[WINED3D_FFP_EMIT_FLOAT4]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex4fv;
    ops->position[WINED3D_FFP_EMIT_D3DCOLOR]  = position_d3dcolor;
    ops->position[WINED3D_FFP_EMIT_UBYTE4]    = invalid_func;
    ops->position[WINED3D_FFP_EMIT_SHORT2]    = invalid_func;
    ops->position[WINED3D_FFP_EMIT_SHORT4]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex2sv;
    ops->position[WINED3D_FFP_EMIT_UBYTE4N]   = invalid_func;
    ops->position[WINED3D_FFP_EMIT_SHORT2N]   = invalid_func;
    ops->position[WINED3D_FFP_EMIT_SHORT4N]   = invalid_func;
    ops->position[WINED3D_FFP_EMIT_USHORT2N]  = invalid_func;
    ops->position[WINED3D_FFP_EMIT_USHORT4N]  = invalid_func;
    ops->position[WINED3D_FFP_EMIT_UDEC3]     = invalid_func;
    ops->position[WINED3D_FFP_EMIT_DEC3N]     = invalid_func;
    ops->position[WINED3D_FFP_EMIT_FLOAT16_2] = invalid_func;
    ops->position[WINED3D_FFP_EMIT_FLOAT16_4] = invalid_func;
    ops->position[WINED3D_FFP_EMIT_INVALID]   = invalid_func;

    ops->diffuse[WINED3D_FFP_EMIT_FLOAT1]     = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT2]     = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT3]     = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor3fv;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT4]     = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4fv;
    ops->diffuse[WINED3D_FFP_EMIT_D3DCOLOR]   = diffuse_d3dcolor;
    ops->diffuse[WINED3D_FFP_EMIT_UBYTE4]     = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_SHORT2]     = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_SHORT4]     = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_UBYTE4N]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4ubv;
    ops->diffuse[WINED3D_FFP_EMIT_SHORT2N]    = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_SHORT4N]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4sv;
    ops->diffuse[WINED3D_FFP_EMIT_USHORT2N]   = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_USHORT4N]   = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4usv;
    ops->diffuse[WINED3D_FFP_EMIT_UDEC3]      = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_DEC3N]      = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT16_2]  = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT16_4]  = invalid_func;
    ops->diffuse[WINED3D_FFP_EMIT_INVALID]    = invalid_func;

    /* No 4 component entry points here. */
    ops->specular[WINED3D_FFP_EMIT_FLOAT1]    = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_FLOAT2]    = invalid_func;
    if (gl_info->supported[EXT_SECONDARY_COLOR])
        ops->specular[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_attrib_func)GL_EXTCALL(glSecondaryColor3fvEXT);
    else
        ops->specular[WINED3D_FFP_EMIT_FLOAT3]    = warn_no_specular_func;
    ops->specular[WINED3D_FFP_EMIT_FLOAT4]    = invalid_func;
    if (gl_info->supported[EXT_SECONDARY_COLOR])
        ops->specular[WINED3D_FFP_EMIT_D3DCOLOR]  = specular_d3dcolor;
    else
        ops->specular[WINED3D_FFP_EMIT_D3DCOLOR]  = warn_no_specular_func;
    ops->specular[WINED3D_FFP_EMIT_UBYTE4]    = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_SHORT2]    = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_SHORT4]    = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_UBYTE4N]   = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_SHORT2N]   = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_SHORT4N]   = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_USHORT2N]  = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_USHORT4N]  = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_UDEC3]     = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_DEC3N]     = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_FLOAT16_2] = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_FLOAT16_4] = invalid_func;
    ops->specular[WINED3D_FFP_EMIT_INVALID]   = invalid_func;

    /* Only 3 component entry points here. Test how others behave. Float4
     * normals are used by one of our tests, trying to pass it to the pixel
     * shader, which fails on Windows. */
    ops->normal[WINED3D_FFP_EMIT_FLOAT1]      = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_FLOAT2]      = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_FLOAT3]      = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glNormal3fv;
    /* Just ignore the 4th value. */
    ops->normal[WINED3D_FFP_EMIT_FLOAT4]      = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glNormal3fv;
    ops->normal[WINED3D_FFP_EMIT_D3DCOLOR]    = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_UBYTE4]      = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_SHORT2]      = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_SHORT4]      = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_UBYTE4N]     = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_SHORT2N]     = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_SHORT4N]     = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_USHORT2N]    = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_USHORT4N]    = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_UDEC3]       = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_DEC3N]       = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_FLOAT16_2]   = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_FLOAT16_4]   = invalid_func;
    ops->normal[WINED3D_FFP_EMIT_INVALID]     = invalid_func;

    ops->texcoord[WINED3D_FFP_EMIT_FLOAT1]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord1fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT2]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord3fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT4]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_D3DCOLOR]  = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_UBYTE4]    = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT2]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2svARB;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT4]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4svARB;
    ops->texcoord[WINED3D_FFP_EMIT_UBYTE4N]   = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT2N]   = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT4N]   = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_USHORT2N]  = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_USHORT4N]  = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_UDEC3]     = invalid_texcoord_func;
    ops->texcoord[WINED3D_FFP_EMIT_DEC3N]     = invalid_texcoord_func;
    if (gl_info->supported[NV_HALF_FLOAT])
    {
        /* Not supported by ARB_HALF_FLOAT_VERTEX, so check for NV_HALF_FLOAT. */
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_2] =
                (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2hvNV;
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_4] =
                (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4hvNV;
    }
    else
    {
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_2] = invalid_texcoord_func;
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_4] = invalid_texcoord_func;
    }
    ops->texcoord[WINED3D_FFP_EMIT_INVALID]   = invalid_texcoord_func;
}

static void wined3d_adapter_init_fb_cfgs(struct wined3d_adapter *adapter, HDC dc)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int i;

    if (gl_info->supported[WGL_ARB_PIXEL_FORMAT])
    {
        UINT attrib_count = 0;
        GLint cfg_count;
        int attribs[11];
        int values[11];
        int attribute;

        attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
        GL_EXTCALL(wglGetPixelFormatAttribivARB(dc, 0, 0, 1, &attribute, &cfg_count));

        adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cfg_count * sizeof(*adapter->cfgs));
        attribs[attrib_count++] = WGL_RED_BITS_ARB;
        attribs[attrib_count++] = WGL_GREEN_BITS_ARB;
        attribs[attrib_count++] = WGL_BLUE_BITS_ARB;
        attribs[attrib_count++] = WGL_ALPHA_BITS_ARB;
        attribs[attrib_count++] = WGL_COLOR_BITS_ARB;
        attribs[attrib_count++] = WGL_DEPTH_BITS_ARB;
        attribs[attrib_count++] = WGL_STENCIL_BITS_ARB;
        attribs[attrib_count++] = WGL_DRAW_TO_WINDOW_ARB;
        attribs[attrib_count++] = WGL_PIXEL_TYPE_ARB;
        attribs[attrib_count++] = WGL_DOUBLE_BUFFER_ARB;
        attribs[attrib_count++] = WGL_AUX_BUFFERS_ARB;

        for (i = 0, adapter->cfg_count = 0; i < cfg_count; ++i)
        {
            struct wined3d_pixel_format *cfg = &adapter->cfgs[adapter->cfg_count];
            int format_id = i + 1;

            if (!GL_EXTCALL(wglGetPixelFormatAttribivARB(dc, format_id, 0, attrib_count, attribs, values)))
                continue;

            cfg->iPixelFormat = format_id;
            cfg->redSize = values[0];
            cfg->greenSize = values[1];
            cfg->blueSize = values[2];
            cfg->alphaSize = values[3];
            cfg->colorSize = values[4];
            cfg->depthSize = values[5];
            cfg->stencilSize = values[6];
            cfg->windowDrawable = values[7];
            cfg->iPixelType = values[8];
            cfg->doubleBuffer = values[9];
            cfg->auxBuffers = values[10];

            cfg->numSamples = 0;
            /* Check multisample support. */
            if (gl_info->supported[ARB_MULTISAMPLE])
            {
                int attribs[2] = {WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB};
                int values[2];

                if (GL_EXTCALL(wglGetPixelFormatAttribivARB(dc, format_id, 0, 2, attribs, values)))
                {
                    /* values[0] = WGL_SAMPLE_BUFFERS_ARB which tells whether
                     * multisampling is supported. values[1] = number of
                     * multisample buffers. */
                    if (values[0])
                        cfg->numSamples = values[1];
                }
            }

            TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, "
                    "depth=%d, stencil=%d, samples=%d, windowDrawable=%d\n",
                    cfg->iPixelFormat, cfg->iPixelType, cfg->doubleBuffer,
                    cfg->redSize, cfg->greenSize, cfg->blueSize, cfg->alphaSize,
                    cfg->depthSize, cfg->stencilSize, cfg->numSamples, cfg->windowDrawable);

            ++adapter->cfg_count;
        }
    }
    else
    {
        int cfg_count;

        cfg_count = DescribePixelFormat(dc, 0, 0, 0);
        adapter->cfgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cfg_count * sizeof(*adapter->cfgs));

        for (i = 0, adapter->cfg_count = 0; i < cfg_count; ++i)
        {
            struct wined3d_pixel_format *cfg = &adapter->cfgs[adapter->cfg_count];
            PIXELFORMATDESCRIPTOR pfd;
            int format_id = i + 1;

            if (!DescribePixelFormat(dc, format_id, sizeof(pfd), &pfd))
                continue;

            /* We only want HW acceleration using an OpenGL ICD driver.
             * PFD_GENERIC_FORMAT = slow opengl 1.1 gdi software rendering.
             * PFD_GENERIC_ACCELERATED = partial hw acceleration using a MCD
             * driver (e.g. 3dfx minigl). */
            if (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED))
            {
                TRACE("Skipping format %d because it isn't ICD accelerated.\n", format_id);
                continue;
            }

            cfg->iPixelFormat = format_id;
            cfg->redSize = pfd.cRedBits;
            cfg->greenSize = pfd.cGreenBits;
            cfg->blueSize = pfd.cBlueBits;
            cfg->alphaSize = pfd.cAlphaBits;
            cfg->colorSize = pfd.cColorBits;
            cfg->depthSize = pfd.cDepthBits;
            cfg->stencilSize = pfd.cStencilBits;
            cfg->windowDrawable = (pfd.dwFlags & PFD_DRAW_TO_WINDOW) ? 1 : 0;
            cfg->iPixelType = (pfd.iPixelType == PFD_TYPE_RGBA) ? WGL_TYPE_RGBA_ARB : WGL_TYPE_COLORINDEX_ARB;
            cfg->doubleBuffer = (pfd.dwFlags & PFD_DOUBLEBUFFER) ? 1 : 0;
            cfg->auxBuffers = pfd.cAuxBuffers;
            cfg->numSamples = 0;

            TRACE("iPixelFormat=%d, iPixelType=%#x, doubleBuffer=%d, RGBA=%d/%d/%d/%d, "
                    "depth=%d, stencil=%d, windowDrawable=%d\n",
                    cfg->iPixelFormat, cfg->iPixelType, cfg->doubleBuffer,
                    cfg->redSize, cfg->greenSize, cfg->blueSize, cfg->alphaSize,
                    cfg->depthSize, cfg->stencilSize, cfg->windowDrawable);

            ++adapter->cfg_count;
        }
    }
}

static BOOL wined3d_adapter_init(struct wined3d_adapter *adapter, UINT ordinal)
{
    static const DWORD supported_gl_versions[] =
    {
        MAKEDWORD_VERSION(3, 2),
        MAKEDWORD_VERSION(1, 0),
    };
    struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct wined3d_caps_gl_ctx caps_gl_ctx = {0};
    unsigned int i;
    DISPLAY_DEVICEW display_device;

    TRACE("adapter %p, ordinal %u.\n", adapter, ordinal);

    adapter->ordinal = ordinal;
    adapter->monitorPoint.x = -1;
    adapter->monitorPoint.y = -1;

/* Dynamically load all GL core functions */
#ifdef USE_WIN32_OPENGL
    {
        HMODULE mod_gl = GetModuleHandleA("opengl32.dll");
#define USE_GL_FUNC(f) gl_info->gl_ops.gl.p_##f = (void *)GetProcAddress(mod_gl, #f);
        ALL_WGL_FUNCS
#undef USE_GL_FUNC
        gl_info->gl_ops.wgl.p_wglSwapBuffers = (void *)GetProcAddress(mod_gl, "wglSwapBuffers");
    }
#else
    /* To bypass the opengl32 thunks retrieve functions from the WGL driver instead of opengl32 */
    {
        HDC hdc = GetDC( 0 );
        const struct opengl_funcs *wgl_driver = __wine_get_wgl_driver( hdc, WINE_WGL_DRIVER_VERSION );
        ReleaseDC( 0, hdc );
        if (!wgl_driver || wgl_driver == (void *)-1) return FALSE;
        gl_info->gl_ops.wgl = wgl_driver->wgl;
        gl_info->gl_ops.gl = wgl_driver->gl;
    }
#endif

    glEnableWINE = gl_info->gl_ops.gl.p_glEnable;
    glDisableWINE = gl_info->gl_ops.gl.p_glDisable;

    if (!AllocateLocallyUniqueId(&adapter->luid))
    {
        ERR("Failed to set adapter LUID (%#x).\n", GetLastError());
        return FALSE;
    }
    TRACE("Allocated LUID %08x:%08x for adapter %p.\n",
            adapter->luid.HighPart, adapter->luid.LowPart, adapter);

    if (!wined3d_caps_gl_ctx_create(adapter, &caps_gl_ctx))
    {
        ERR("Failed to get a GL context for adapter %p.\n", adapter);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(supported_gl_versions); ++i)
    {
        if (supported_gl_versions[i] <= wined3d_settings.max_gl_version)
            break;
    }
    if (i == ARRAY_SIZE(supported_gl_versions))
    {
        ERR_(winediag)("Requested invalid GL version %u.%u.\n",
                wined3d_settings.max_gl_version >> 16, wined3d_settings.max_gl_version & 0xffff);
        i = ARRAY_SIZE(supported_gl_versions) - 1;
    }

    for (; i < ARRAY_SIZE(supported_gl_versions); ++i)
    {
        gl_info->selected_gl_version = supported_gl_versions[i];

        if (wined3d_caps_gl_ctx_create_attribs(&caps_gl_ctx, gl_info))
            break;

        WARN("Couldn't create an OpenGL %u.%u context, trying fallback to a lower version.\n",
                supported_gl_versions[i] >> 16, supported_gl_versions[i] & 0xffff);
    }

    if (!wined3d_adapter_init_gl_caps(adapter))
    {
        ERR("Failed to initialize GL caps for adapter %p.\n", adapter);
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        return FALSE;
    }

    wined3d_adapter_init_fb_cfgs(adapter, caps_gl_ctx.dc);
    /* We haven't found any suitable formats. This should only happen in
     * case of GDI software rendering, which is pretty useless anyway. */
    if (!adapter->cfg_count)
    {
        WARN("No suitable pixel formats found.\n");
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        HeapFree(GetProcessHeap(), 0, adapter->cfgs);
        return FALSE;
    }

    if (!wined3d_adapter_init_format_info(adapter, &caps_gl_ctx))
    {
        ERR("Failed to initialize GL format info.\n");
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        HeapFree(GetProcessHeap(), 0, adapter->cfgs);
        return FALSE;
    }

    gl_info->fixed_polyoffset_scale = wined3d_adapter_find_polyoffset_scale(&caps_gl_ctx, GL_DEPTH_COMPONENT);
    if (gl_info->supported[ARB_DEPTH_BUFFER_FLOAT])
        gl_info->float_polyoffset_scale = wined3d_adapter_find_polyoffset_scale(&caps_gl_ctx, GL_DEPTH32F_STENCIL8);

    adapter->vram_bytes = adapter->driver_info.vram_bytes;
    adapter->vram_bytes_used = 0;
    TRACE("Emulating 0x%s bytes of video ram.\n", wine_dbgstr_longlong(adapter->vram_bytes));

    display_device.cb = sizeof(display_device);
    EnumDisplayDevicesW(NULL, ordinal, &display_device, 0);
    TRACE("DeviceName: %s\n", debugstr_w(display_device.DeviceName));
    strcpyW(adapter->DeviceName, display_device.DeviceName);

    wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);

    wined3d_adapter_init_ffp_attrib_ops(adapter);

    return TRUE;
}

static void wined3d_adapter_init_nogl(struct wined3d_adapter *adapter, UINT ordinal)
{
    DISPLAY_DEVICEW display_device;

    memset(adapter, 0, sizeof(*adapter));
    adapter->ordinal = ordinal;
    adapter->monitorPoint.x = -1;
    adapter->monitorPoint.y = -1;

    adapter->driver_info.name = "Display";
    adapter->driver_info.description = "WineD3D DirectDraw Emulation";
    if (wined3d_settings.emulated_textureram)
        adapter->vram_bytes = wined3d_settings.emulated_textureram;
    else
        adapter->vram_bytes = 128 * 1024 * 1024;

    initPixelFormatsNoGL(&adapter->gl_info);

    adapter->vertex_pipe = &none_vertex_pipe;
    adapter->fragment_pipe = &none_fragment_pipe;
    adapter->shader_backend = &none_shader_backend;
    adapter->blitter = &cpu_blit;

    display_device.cb = sizeof(display_device);
    EnumDisplayDevicesW(NULL, ordinal, &display_device, 0);
    TRACE("DeviceName: %s\n", debugstr_w(display_device.DeviceName));
    strcpyW(adapter->DeviceName, display_device.DeviceName);
}

static void STDMETHODCALLTYPE wined3d_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops wined3d_null_parent_ops =
{
    wined3d_null_wined3d_object_destroyed,
};

HRESULT wined3d_init(struct wined3d *wined3d, DWORD flags)
{
    wined3d->ref = 1;
    wined3d->flags = flags;

    TRACE("Initializing adapters.\n");

    if (flags & WINED3D_NO3D)
    {
        wined3d_adapter_init_nogl(&wined3d->adapters[0], 0);
        wined3d->adapter_count = 1;
        return WINED3D_OK;
    }

    if (!wined3d_adapter_init(&wined3d->adapters[0], 0))
    {
        WARN("Failed to initialize adapter.\n");
        return E_FAIL;
    }
    wined3d->adapter_count = 1;

    return WINED3D_OK;
}
