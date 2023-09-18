/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011, 2018 Henri Verbeet for CodeWeavers
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
#include "wine/port.h"

#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

enum wined3d_gl_vendor
{
    GL_VENDOR_UNKNOWN,
    GL_VENDOR_APPLE,
    GL_VENDOR_FGLRX,
    GL_VENDOR_MESA,
    GL_VENDOR_NVIDIA,
};

struct wined3d_extension_map
{
    const char *extension_string;
    enum wined3d_gl_extension extension;
};

static const struct wined3d_extension_map gl_extension_map[] =
{
    /* APPLE */
    {"GL_APPLE_fence",                      APPLE_FENCE                   },
    {"GL_APPLE_float_pixels",               APPLE_FLOAT_PIXELS            },
    {"GL_APPLE_flush_buffer_range",         APPLE_FLUSH_BUFFER_RANGE      },
    {"GL_APPLE_ycbcr_422",                  APPLE_YCBCR_422               },

    /* ARB */
    {"GL_ARB_base_instance",                ARB_BASE_INSTANCE             },
    {"GL_ARB_blend_func_extended",          ARB_BLEND_FUNC_EXTENDED       },
    {"GL_ARB_buffer_storage",               ARB_BUFFER_STORAGE            },
    {"GL_ARB_clear_buffer_object",          ARB_CLEAR_BUFFER_OBJECT       },
    {"GL_ARB_clear_texture",                ARB_CLEAR_TEXTURE             },
    {"GL_ARB_clip_control",                 ARB_CLIP_CONTROL              },
    {"GL_ARB_color_buffer_float",           ARB_COLOR_BUFFER_FLOAT        },
    {"GL_ARB_compute_shader",               ARB_COMPUTE_SHADER            },
    {"GL_ARB_conservative_depth",           ARB_CONSERVATIVE_DEPTH        },
    {"GL_ARB_copy_buffer",                  ARB_COPY_BUFFER               },
    {"GL_ARB_copy_image",                   ARB_COPY_IMAGE                },
    {"GL_ARB_cull_distance",                ARB_CULL_DISTANCE             },
    {"GL_ARB_debug_output",                 ARB_DEBUG_OUTPUT              },
    {"GL_ARB_depth_buffer_float",           ARB_DEPTH_BUFFER_FLOAT        },
    {"GL_ARB_depth_clamp",                  ARB_DEPTH_CLAMP               },
    {"GL_ARB_depth_texture",                ARB_DEPTH_TEXTURE             },
    {"GL_ARB_derivative_control",           ARB_DERIVATIVE_CONTROL        },
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS              },
    {"GL_ARB_draw_elements_base_vertex",    ARB_DRAW_ELEMENTS_BASE_VERTEX },
    {"GL_ARB_draw_indirect",                ARB_DRAW_INDIRECT             },
    {"GL_ARB_draw_instanced",               ARB_DRAW_INSTANCED            },
    {"GL_ARB_ES2_compatibility",            ARB_ES2_COMPATIBILITY         },
    {"GL_ARB_ES3_compatibility",            ARB_ES3_COMPATIBILITY         },
    {"GL_ARB_explicit_attrib_location",     ARB_EXPLICIT_ATTRIB_LOCATION  },
    {"GL_ARB_fragment_coord_conventions",   ARB_FRAGMENT_COORD_CONVENTIONS},
    {"GL_ARB_fragment_layer_viewport",      ARB_FRAGMENT_LAYER_VIEWPORT   },
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM          },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER           },
    {"GL_ARB_framebuffer_no_attachments",   ARB_FRAMEBUFFER_NO_ATTACHMENTS},
    {"GL_ARB_framebuffer_object",           ARB_FRAMEBUFFER_OBJECT        },
    {"GL_ARB_framebuffer_sRGB",             ARB_FRAMEBUFFER_SRGB          },
    {"GL_ARB_geometry_shader4",             ARB_GEOMETRY_SHADER4          },
    {"GL_ARB_gpu_shader5",                  ARB_GPU_SHADER5               },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL          },
    {"GL_ARB_half_float_vertex",            ARB_HALF_FLOAT_VERTEX         },
    {"GL_ARB_instanced_arrays",             ARB_INSTANCED_ARRAYS          },
    {"GL_ARB_internalformat_query",         ARB_INTERNALFORMAT_QUERY      },
    {"GL_ARB_internalformat_query2",        ARB_INTERNALFORMAT_QUERY2     },
    {"GL_ARB_map_buffer_alignment",         ARB_MAP_BUFFER_ALIGNMENT      },
    {"GL_ARB_map_buffer_range",             ARB_MAP_BUFFER_RANGE          },
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE               },
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE              },
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY           },
    {"GL_ARB_pipeline_statistics_query",    ARB_PIPELINE_STATISTICS_QUERY },
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT       },
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS          },
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE              },
    {"GL_ARB_polygon_offset_clamp",         ARB_POLYGON_OFFSET_CLAMP      },
    {"GL_ARB_provoking_vertex",             ARB_PROVOKING_VERTEX          },
    {"GL_ARB_query_buffer_object",          ARB_QUERY_BUFFER_OBJECT       },
    {"GL_ARB_sample_shading",               ARB_SAMPLE_SHADING            },
    {"GL_ARB_sampler_objects",              ARB_SAMPLER_OBJECTS           },
    {"GL_ARB_seamless_cube_map",            ARB_SEAMLESS_CUBE_MAP         },
    {"GL_ARB_shader_atomic_counters",       ARB_SHADER_ATOMIC_COUNTERS    },
    {"GL_ARB_shader_bit_encoding",          ARB_SHADER_BIT_ENCODING       },
    {"GL_ARB_shader_image_load_store",      ARB_SHADER_IMAGE_LOAD_STORE   },
    {"GL_ARB_shader_image_size",            ARB_SHADER_IMAGE_SIZE         },
    {"GL_ARB_shader_storage_buffer_object", ARB_SHADER_STORAGE_BUFFER_OBJECT},
    {"GL_ARB_shader_texture_image_samples", ARB_SHADER_TEXTURE_IMAGE_SAMPLES},
    {"GL_ARB_shader_texture_lod",           ARB_SHADER_TEXTURE_LOD        },
    {"GL_ARB_shader_viewport_layer_array",  ARB_SHADER_VIEWPORT_LAYER_ARRAY},
    {"GL_ARB_shading_language_100",         ARB_SHADING_LANGUAGE_100      },
    {"GL_ARB_shading_language_420pack",     ARB_SHADING_LANGUAGE_420PACK  },
    {"GL_ARB_shading_language_packing",     ARB_SHADING_LANGUAGE_PACKING  },
    {"GL_ARB_shadow",                       ARB_SHADOW                    },
    {"GL_ARB_stencil_texturing",            ARB_STENCIL_TEXTURING         },
    {"GL_ARB_sync",                         ARB_SYNC                      },
    {"GL_ARB_tessellation_shader",          ARB_TESSELLATION_SHADER       },
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP      },
    {"GL_ARB_texture_buffer_object",        ARB_TEXTURE_BUFFER_OBJECT     },
    {"GL_ARB_texture_buffer_range",         ARB_TEXTURE_BUFFER_RANGE      },
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION       },
    {"GL_ARB_texture_compression_bptc",     ARB_TEXTURE_COMPRESSION_BPTC  },
    {"GL_ARB_texture_compression_rgtc",     ARB_TEXTURE_COMPRESSION_RGTC  },
    {"GL_ARB_texture_cube_map",             ARB_TEXTURE_CUBE_MAP          },
    {"GL_ARB_texture_cube_map_array",       ARB_TEXTURE_CUBE_MAP_ARRAY    },
    {"GL_ARB_texture_env_combine",          ARB_TEXTURE_ENV_COMBINE       },
    {"GL_ARB_texture_env_dot3",             ARB_TEXTURE_ENV_DOT3          },
    {"GL_ARB_texture_filter_anisotropic",   ARB_TEXTURE_FILTER_ANISOTROPIC},
    {"GL_ARB_texture_float",                ARB_TEXTURE_FLOAT             },
    {"GL_ARB_texture_gather",               ARB_TEXTURE_GATHER            },
    {"GL_ARB_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT   },
    {"GL_ARB_texture_mirror_clamp_to_edge", ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE},
    {"GL_ARB_texture_multisample",          ARB_TEXTURE_MULTISAMPLE       },
    {"GL_ARB_texture_non_power_of_two",     ARB_TEXTURE_NON_POWER_OF_TWO  },
    {"GL_ARB_texture_query_levels",         ARB_TEXTURE_QUERY_LEVELS      },
    {"GL_ARB_texture_rectangle",            ARB_TEXTURE_RECTANGLE         },
    {"GL_ARB_texture_rg",                   ARB_TEXTURE_RG                },
    {"GL_ARB_texture_rgb10_a2ui",           ARB_TEXTURE_RGB10_A2UI        },
    {"GL_ARB_texture_storage",              ARB_TEXTURE_STORAGE           },
    {"GL_ARB_texture_storage_multisample",  ARB_TEXTURE_STORAGE_MULTISAMPLE},
    {"GL_ARB_texture_swizzle",              ARB_TEXTURE_SWIZZLE           },
    {"GL_ARB_texture_view",                 ARB_TEXTURE_VIEW              },
    {"GL_ARB_timer_query",                  ARB_TIMER_QUERY               },
    {"GL_ARB_transform_feedback2",          ARB_TRANSFORM_FEEDBACK2       },
    {"GL_ARB_transform_feedback3",          ARB_TRANSFORM_FEEDBACK3       },
    {"GL_ARB_uniform_buffer_object",        ARB_UNIFORM_BUFFER_OBJECT     },
    {"GL_ARB_vertex_array_bgra",            ARB_VERTEX_ARRAY_BGRA         },
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT      },
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM            },
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER             },
    {"GL_ARB_vertex_type_2_10_10_10_rev",   ARB_VERTEX_TYPE_2_10_10_10_REV},
    {"GL_ARB_viewport_array",               ARB_VIEWPORT_ARRAY            },

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
    {"GL_EXT_memory_object",                EXT_MEMORY_OBJECT             },
    {"GL_EXT_gpu_program_parameters",       EXT_GPU_PROGRAM_PARAMETERS    },
    {"GL_EXT_gpu_shader4",                  EXT_GPU_SHADER4               },
    {"GL_EXT_packed_depth_stencil",         EXT_PACKED_DEPTH_STENCIL      },
    {"GL_EXT_packed_float",                 EXT_PACKED_FLOAT              },
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS          },
    {"GL_EXT_polygon_offset_clamp",         ARB_POLYGON_OFFSET_CLAMP      },
    {"GL_EXT_provoking_vertex",             EXT_PROVOKING_VERTEX          },
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR           },
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE          },
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP              },
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D                 },
    {"GL_EXT_texture_array",                EXT_TEXTURE_ARRAY             },
    {"GL_EXT_texture_compression_rgtc",     EXT_TEXTURE_COMPRESSION_RGTC  },
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC  },
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE       },
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3          },
    {"GL_EXT_texture_filter_anisotropic",   ARB_TEXTURE_FILTER_ANISOTROPIC},
    {"GL_EXT_texture_integer",              EXT_TEXTURE_INTEGER           },
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS          },
    {"GL_EXT_texture_mirror_clamp",         EXT_TEXTURE_MIRROR_CLAMP      },
    {"GL_EXT_texture_shadow_lod",           EXT_TEXTURE_SHADOW_LOD        },
    {"GL_EXT_texture_shared_exponent",      EXT_TEXTURE_SHARED_EXPONENT   },
    {"GL_EXT_texture_snorm",                EXT_TEXTURE_SNORM             },
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB              },
    {"GL_EXT_texture_sRGB_decode",          EXT_TEXTURE_SRGB_DECODE       },
    {"GL_EXT_texture_swizzle",              ARB_TEXTURE_SWIZZLE           },
    {"GL_EXT_vertex_array_bgra",            ARB_VERTEX_ARRAY_BGRA         },

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
};

static const struct wined3d_extension_map wgl_extension_map[] =
{
    {"WGL_ARB_pixel_format",                WGL_ARB_PIXEL_FORMAT             },
    {"WGL_EXT_swap_control",                WGL_EXT_SWAP_CONTROL             },
    {"WGL_WINE_pixel_format_passthrough",   WGL_WINE_PIXEL_FORMAT_PASSTHROUGH},
    {"WGL_WINE_query_renderer",             WGL_WINE_QUERY_RENDERER          },
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

static BOOL match_amd_r300_to_500(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return card_vendor == HW_VENDOR_AMD
            && (device == CARD_AMD_RADEON_9500
            || device == CARD_AMD_RADEON_X700
            || device == CARD_AMD_RADEON_X1600);
}

static BOOL match_geforce5(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return card_vendor == HW_VENDOR_NVIDIA
            && (device == CARD_NVIDIA_GEFORCEFX_5200
            || device == CARD_NVIDIA_GEFORCEFX_5600
            || device == CARD_NVIDIA_GEFORCEFX_5800);
}

static BOOL match_apple(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* MacOS has various specialities in the extensions it advertises. Some
     * have to be loaded from the OpenGL 1.2+ core, while other extensions are
     * advertised, but software emulated. So try to detect the Apple OpenGL
     * implementation to apply some extension fixups afterwards.
     *
     * Detecting this isn't really easy. The vendor string doesn't mention
     * Apple. Compile-time checks aren't sufficient either because a Linux
     * binary may display on a macOS X server via remote X11. So try to detect
     * the OpenGL implementation by looking at certain Apple extensions. Some
     * extensions like client storage might be supported on other
     * implementations too, but GL_APPLE_flush_render is specific to the
     * macOS X window management, and GL_APPLE_ycbcr_422 is QuickTime
     * specific. So the chance that other implementations support them is
     * rather small since Win32 QuickTime uses DirectDraw, not OpenGL.
     *
     * This test has been moved into wined3d_guess_gl_vendor(). */
    return gl_vendor == GL_VENDOR_APPLE;
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
    unsigned int check[ARRAY_SIZE(pattern)];

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

static BOOL match_apple_intel(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return (card_vendor == HW_VENDOR_INTEL) && (gl_vendor == GL_VENDOR_APPLE);
}

static BOOL match_apple_nonr500ati(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (gl_vendor != GL_VENDOR_APPLE) return FALSE;
    if (card_vendor != HW_VENDOR_AMD) return FALSE;
    if (device == CARD_AMD_RADEON_X1600) return FALSE;
    return TRUE;
}

static BOOL match_dx10_capable(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* Direct3D 9 cards support 40 single float varyings in hardware, most
     * drivers report 32. ATI misreports 44 varyings. So assume that if we
     * have more than 44 varyings we have a Direct3D 10+ card. This detection
     * is for the gl_ClipPos varying quirk. If a Direct3D 9 card really
     * supports more than 44 varyings and we subtract one in Direct3D 9
     * shaders it's not going to hurt us because the Direct3D 9 limit is
     * hardcoded.
     *
     * Direct3D 10 cards usually have 64 varyings. */
    return gl_info->limits.glsl_varyings > 44;
}

static BOOL match_not_dx10_capable(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return !match_dx10_capable(gl_info, ctx, gl_renderer, gl_vendor, card_vendor, device);
}

/* A GL context is provided by the caller */
static BOOL match_allows_spec_alpha(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLenum error;
    DWORD data[16];

    if (!gl_info->supported[EXT_SECONDARY_COLOR] || !gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
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
static BOOL match_broken_nv_clip(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
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
static BOOL match_fbo_tex_update(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
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
static BOOL match_broken_rgba16(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
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

static BOOL match_fglrx(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return gl_vendor == GL_VENDOR_FGLRX;
}

static BOOL match_r200(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_AMD) return FALSE;
    if (device == CARD_AMD_RADEON_8500) return TRUE;
    return FALSE;
}

static BOOL match_broken_arb_fog(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
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
    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
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

static BOOL match_broken_viewport_subpixel_bits(const struct wined3d_gl_info *gl_info,
        struct wined3d_caps_gl_ctx *ctx, const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (!gl_info->supported[ARB_VIEWPORT_ARRAY])
        return FALSE;
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return FALSE;
    return !wined3d_caps_gl_ctx_test_viewport_subpixel_bits(ctx);
}

static BOOL match_no_independent_bit_depths(const struct wined3d_gl_info *gl_info,
        struct wined3d_caps_gl_ctx *ctx, const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLuint tex[2], fbo;
    GLenum status;

    /* ARB_framebuffer_object allows implementation-dependent internal format
     * restrictions. The EXT extension explicitly calls out an error in the
     * relevant case. */
    if (!gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
        return TRUE;
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return TRUE;

    gl_info->gl_ops.gl.p_glGenTextures(2, tex);

    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex[0]);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);

    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex[1]);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 4, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, 0);

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[0], 0);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex[1], 0);

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    gl_info->fbo_ops.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->gl_ops.gl.p_glDeleteTextures(2, tex);
    checkGLcall("testing multiple framebuffer attachments with different bit depths");

    return status != GL_FRAMEBUFFER_COMPLETE;
}

static void quirk_apple_glsl_constants(struct wined3d_gl_info *gl_info)
{
    /* MacOS needs uniforms for relative addressing offsets. This can
     * accumulate to quite a few uniforms. Beyond that the general uniform
     * isn't optimal, so reserve a number of uniforms. 12 vec4's should allow
     * 48 different offsets or other helper immediate values. */
    TRACE("Reserving 12 GLSL constants for compiler private use.\n");
    gl_info->reserved_glsl_constants = max(gl_info->reserved_glsl_constants, 12);
}

static void quirk_amd_dx9(struct wined3d_gl_info *gl_info)
{
    /* MacOS advertises GL_ARB_texture_non_power_of_two on ATI r500 and
     * earlier cards, although these cards only support
     * GL_ARB_texture_rectangle (D3DPTEXTURECAPS_NONPOW2CONDITIONAL).
     *
     * If real NP2 textures are used, the driver falls back to software. We
     * could just disable the extension and use GL_ARB_texture_rectangle
     * instead, but texture_rectangle is inconvenient due to the
     * non-normalised texture coordinates. Thus set an internal extension
     * flag, GL_WINE_normalized_texrect, which signals the code that it can
     * use non-power-of-two textures as per GL_ARB_texture_non_power_of_two,
     * but has to stick to the texture_rectangle limits.
     *
     * Fglrx doesn't advertise GL_ARB_texture_non_power_of_two, but it
     * advertises OpenGL 2.0, which has this extension promoted to core. The
     * extension loading code sets this extension supported due to that, so
     * this code works on fglrx as well. */
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        TRACE("GL_ARB_texture_non_power_of_two advertised on R500 or earlier card, removing.\n");
        gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
        gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT] = TRUE;
    }
}

static void quirk_no_np2(struct wined3d_gl_info *gl_info)
{
    /*  The NVIDIA GeForce FX series reports OpenGL 2.0 capabilities with the
     *  latest drivers versions, but doesn't explicitly advertise the
     *  ARB_tex_npot extension in the OpenGL extension string. This usually
     *  means that ARB_tex_npot is supported in hardware as long as the
     *  application is staying within the limits enforced by the
     *  ARB_texture_rectangle extension. This however is not true for the
     *  FX series, which instantly falls back to a slower software path as
     *  soon as ARB_tex_npot is used. We therefore completely remove
     *  ARB_tex_npot from the list of supported extensions.
     *
     *  Note that WINE_normalized_texrect can't be used in this case because
     *  internally it uses ARB_tex_npot, triggering the software fallback.
     *  There is not much we can do here apart from disabling the
     *  software-emulated extension and re-enable ARB_tex_rect (which was
     *  previously disabled in wined3d_adapter_init_gl_caps).
     *
     *  This fixup removes performance problems on both the FX 5900 and
     *  FX 5700 (e.g. for framebuffer post-processing effects in the game
     *  "Max Payne 2"). The behaviour can be verified through a simple test
     *  app attached in bugreport #14724. */
    TRACE("GL_ARB_texture_non_power_of_two advertised through OpenGL 2.0 on NV FX card, removing.\n");
    gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
    gl_info->supported[ARB_TEXTURE_RECTANGLE] = TRUE;
}

static void quirk_texcoord_w(struct wined3d_gl_info *gl_info)
{
    /* The Intel GPUs on macOS set the .w register of texcoords to 0.0 by
     * default, which causes problems with fixed-function fragment processing.
     * Ideally this flag should be detected with a test shader and OpenGL
     * feedback mode, but some OpenGL implementations (macOS ATI at least,
     * probably all macOS ones) do not like vertex shaders in feedback mode
     * and return an error, even though it should be valid according to the
     * spec.
     *
     * We don't want to enable this on all cards, as it adds an extra
     * instruction per texcoord used. This makes the shader slower and eats
     * instruction slots which should be available to the Direct3D
     * application.
     *
     * ATI Radeon HD 2xxx cards on macOS have the issue. Instead of checking
     * for the buggy cards, blacklist all Radeon cards on macOS and whitelist
     * the good ones. That way we're prepared for the future. If this
     * workaround is activated on cards that do not need it, it won't break
     * things, just affect performance negatively. */
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
    /* NVIDIA GeForce 6xxx and 7xxx support accelerated VTF only on a few
     * selected texture formats. They are apparently the only Direct3D 9 class
     * GPUs supporting VTF. Also, Direct3D 9-era GPUs are somewhat limited
     * with float texture filtering and blending. */
    gl_info->quirks |= WINED3D_QUIRK_LIMITED_TEX_FILTERING;
}

static void quirk_r200_constants(struct wined3d_gl_info *gl_info)
{
    /* The Mesa r200 driver (and there is no other driver for this GPU Wine
     * would run on) loads some fog parameters (start, end, exponent, but not
     * the colour) into the program.
     *
     * Apparently the fog hardware is only able to handle linear fog with a
     * range of 0.0;1.0, and it is the responsibility of the vertex pipeline
     * to handle non-linear fog and linear fog with start and end other than
     * 0.0 and 1.0. */
    TRACE("Reserving 1 ARB constant for compiler private use.\n");
    gl_info->reserved_arb_constants = max(gl_info->reserved_arb_constants, 1);
}

static void quirk_broken_arb_fog(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_BROKEN_ARB_FOG;
}

static void quirk_broken_viewport_subpixel_bits(struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[ARB_CLIP_CONTROL])
    {
        TRACE("Disabling ARB_clip_control.\n");
        gl_info->supported[ARB_CLIP_CONTROL] = FALSE;
    }
}

static void quirk_no_independent_bit_depths(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_NO_INDEPENDENT_BIT_DEPTHS;
}

static const struct wined3d_gpu_description *query_gpu_description(const struct wined3d_gl_info *gl_info,
        UINT64 *vram_bytes)
{
    const struct wined3d_gpu_description *gpu_description = NULL, *gpu_description_override;
    enum wined3d_pci_vendor vendor = PCI_VENDOR_NONE;
    enum wined3d_pci_device device = PCI_DEVICE_NONE;
    GLuint value;

    if (gl_info->supported[WGL_WINE_QUERY_RENDERER])
    {
        if (GL_EXTCALL(wglQueryCurrentRendererIntegerWINE(WGL_RENDERER_VENDOR_ID_WINE, &value)))
            vendor = value;
        if (GL_EXTCALL(wglQueryCurrentRendererIntegerWINE(WGL_RENDERER_DEVICE_ID_WINE, &value)))
            device = value;
        if (GL_EXTCALL(wglQueryCurrentRendererIntegerWINE(WGL_RENDERER_VIDEO_MEMORY_WINE, &value)))
            *vram_bytes = (UINT64)value * 1024 * 1024;

        TRACE("Card reports vendor PCI ID 0x%04x, device PCI ID 0x%04x, 0x%s bytes of video memory.\n",
                vendor, device, wine_dbgstr_longlong(*vram_bytes));

        gpu_description = wined3d_get_gpu_description(vendor, device);
    }
    else if (gl_info->supported[NVX_GPU_MEMORY_INFO])
    {
        GLint vram_kb;
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &vram_kb);

        *vram_bytes = (UINT64)vram_kb * 1024;
        TRACE("Got 0x%s as video memory from NVX_GPU_MEMORY_INFO extension.\n",
                wine_dbgstr_longlong(*vram_bytes));

        gpu_description = wined3d_get_gpu_description(vendor, device);
    }

    if ((gpu_description_override = wined3d_get_user_override_gpu_description(vendor, device)))
        gpu_description = gpu_description_override;

    return gpu_description;
}

/* Context activation is done by the caller. */
static void fixup_extensions(struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
        const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
        enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    unsigned int i;

    static const struct driver_quirk
    {
        BOOL (*match)(const struct wined3d_gl_info *gl_info, struct wined3d_caps_gl_ctx *ctx,
                const char *gl_renderer, enum wined3d_gl_vendor gl_vendor,
                enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device);
        void (*apply)(struct wined3d_gl_info *gl_info);
        const char *description;
    }
    quirk_table[] =
    {
        {
            match_amd_r300_to_500,
            quirk_amd_dx9,
            "AMD normalised texrect quirk"
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
            /* GL_EXT_secondary_color does not allow 4 component secondary
             * colours, but most OpenGL implementations accept it. Apple's
             * is the only OpenGL implementation known to reject it.
             *
             * If we can pass 4-component specular colours, do it, because
             * (a) we don't have to screw around with the data, and (b) the
             * Direct3D fixed-function vertex pipeline passes specular alpha
             * to the pixel shader if any is used. Otherwise the specular
             * alpha is used to pass the fog coordinate, which we pass to
             * OpenGL via GL_EXT_fog_coord. */
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
        {
            match_broken_viewport_subpixel_bits,
            quirk_broken_viewport_subpixel_bits,
            "NVIDIA viewport subpixel bits bug"
        },
        {
            match_no_independent_bit_depths,
            quirk_no_independent_bit_depths,
            "No support for MRT with independent bit depths"
        },
    };

    for (i = 0; i < ARRAY_SIZE(quirk_table); ++i)
    {
        if (!quirk_table[i].match(gl_info, ctx, gl_renderer, gl_vendor, card_vendor, device)) continue;
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
        const char *gl_vendor_string, const char *gl_renderer, const char *gl_version)
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
    if (gl_info->supported[APPLE_FENCE] && gl_info->supported[APPLE_YCBCR_422])
        return GL_VENDOR_APPLE;

    if (strstr(gl_vendor_string, "NVIDIA"))
        return GL_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI"))
        return GL_VENDOR_FGLRX;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Brian Paul")
            || strstr(gl_vendor_string, "X.Org")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "DRI R300 Project")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc.")
            || strstr(gl_vendor_string, "Red Hat")
            || strstr(gl_vendor_string, "Intel")
            || strstr(gl_renderer, "Mesa")
            || strstr(gl_renderer, "Gallium")
            || strstr(gl_renderer, "Intel")
            || strstr(gl_version, "Mesa"))
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
            || strstr(gl_renderer, "FirePro")
            || strstr(gl_renderer, "Radeon")
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

    if (strstr(gl_vendor_string, "Red Hat")
            || strstr(gl_renderer, "virgl"))
        return HW_VENDOR_REDHAT;

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

static enum wined3d_feature_level feature_level_from_caps(const struct wined3d_gl_info *gl_info,
        const struct shader_caps *shader_caps, const struct fragment_caps *fragment_caps)
{
    unsigned int shader_model;

    shader_model = min(shader_caps->vs_version, shader_caps->ps_version);
    shader_model = min(shader_model, max(shader_caps->gs_version, 3));
    shader_model = min(shader_model, max(shader_caps->hs_version, 4));
    shader_model = min(shader_model, max(shader_caps->ds_version, 4));

    if (gl_info->supported[WINED3D_GL_VERSION_3_2]
            && gl_info->supported[ARB_POLYGON_OFFSET_CLAMP]
            && gl_info->supported[ARB_SAMPLER_OBJECTS])
    {
        if (shader_model >= 5
                && gl_info->supported[ARB_DRAW_INDIRECT]
                && gl_info->supported[ARB_TEXTURE_COMPRESSION_BPTC])
            return WINED3D_FEATURE_LEVEL_11_1;

        if (shader_model >= 4)
        {
            if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
                return WINED3D_FEATURE_LEVEL_10_1;
            return WINED3D_FEATURE_LEVEL_10;
        }
    }

    if (shader_model >= 3 && gl_info->limits.texture_size >= 4096 && gl_info->limits.buffers >= 4)
        return WINED3D_FEATURE_LEVEL_9_3;
    if (shader_model >= 2)
    {
        if (gl_info->supported[ARB_OCCLUSION_QUERY]
                && gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE]
                && gl_info->supported[EXT_BLEND_EQUATION_SEPARATE]
                && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
            return WINED3D_FEATURE_LEVEL_9_2;

        return WINED3D_FEATURE_LEVEL_9_1;
    }
    if (shader_model >= 1)
        return WINED3D_FEATURE_LEVEL_8;

    if (fragment_caps->TextureOpCaps & WINED3DTEXOPCAPS_DOTPRODUCT3)
        return WINED3D_FEATURE_LEVEL_7;
    if (fragment_caps->MaxSimultaneousTextures > 1)
        return WINED3D_FEATURE_LEVEL_6;

    return WINED3D_FEATURE_LEVEL_5;
}

static const struct wined3d_renderer_table
{
    const char *renderer;
    enum wined3d_pci_device id;
}
cards_nvidia_binary[] =
{
    /* Direct 3D 11 */
    {"RTX 2080 Ti",                 CARD_NVIDIA_GEFORCE_RTX2080TI}, /* GeForce 2000 - highend */
    {"RTX 2080",                    CARD_NVIDIA_GEFORCE_RTX2080},   /* GeForce 2000 - highend */
    {"RTX 2070",                    CARD_NVIDIA_GEFORCE_RTX2070},   /* GeForce 2000 - highend */
    {"RTX 2060",                    CARD_NVIDIA_GEFORCE_RTX2060},   /* GeForce 2000 - highend */
    {"GTX 1660 Ti",                 CARD_NVIDIA_GEFORCE_GTX1660TI}, /* GeForce 1600 - highend */
    {"TITAN V",                     CARD_NVIDIA_TITANV},            /* GeForce 1000 - highend */
    {"TITAN X (Pascal)",            CARD_NVIDIA_TITANX_PASCAL},     /* GeForce 1000 - highend */
    {"GTX 1080 Ti",                 CARD_NVIDIA_GEFORCE_GTX1080TI}, /* GeForce 1000 - highend */
    {"GTX 1080",                    CARD_NVIDIA_GEFORCE_GTX1080},   /* GeForce 1000 - highend */
    {"GTX 1070",                    CARD_NVIDIA_GEFORCE_GTX1070},   /* GeForce 1000 - highend */
    {"GTX 1060",                    CARD_NVIDIA_GEFORCE_GTX1060},   /* GeForce 1000 - midend high */
    {"GTX 1050 Ti",                 CARD_NVIDIA_GEFORCE_GTX1050TI}, /* GeForce 1000 - midend */
    {"GTX 1050",                    CARD_NVIDIA_GEFORCE_GTX1050},   /* GeForce 1000 - midend */
    {"GTX 980 Ti",                  CARD_NVIDIA_GEFORCE_GTX980TI},  /* GeForce 900 - highend */
    {"GTX 980",                     CARD_NVIDIA_GEFORCE_GTX980},    /* GeForce 900 - highend */
    {"GTX 970M",                    CARD_NVIDIA_GEFORCE_GTX970M},   /* GeForce 900 - highend mobile*/
    {"GTX 970",                     CARD_NVIDIA_GEFORCE_GTX970},    /* GeForce 900 - highend */
    {"GTX TITAN X",                 CARD_NVIDIA_GEFORCE_GTXTITANX}, /* Geforce 900 - highend */
    {"GTX 960M",                    CARD_NVIDIA_GEFORCE_GTX960M},   /* GeForce 900 - midend high mobile */
    {"GTX 960",                     CARD_NVIDIA_GEFORCE_GTX960},    /* GeForce 900 - midend high */
    {"GTX 950M",                    CARD_NVIDIA_GEFORCE_GTX950M},   /* GeForce 900 - midend mobile */
    {"GTX 950",                     CARD_NVIDIA_GEFORCE_GTX950},    /* GeForce 900 - midend */
    {"GeForce 940M",                CARD_NVIDIA_GEFORCE_940M},      /* GeForce 900 - midend mobile */
    {"GTX 880M",                    CARD_NVIDIA_GEFORCE_GTX880M},   /* GeForce 800 - mobile */
    {"GTX 870M",                    CARD_NVIDIA_GEFORCE_GTX870M},   /* GeForce 800 - mobile */
    {"GTX 860M",                    CARD_NVIDIA_GEFORCE_GTX860M},   /* GeForce 800 - mobile */
    {"GTX 850M",                    CARD_NVIDIA_GEFORCE_GTX850M},   /* GeForce 800 - mobile */
    {"GeForce 845M",                CARD_NVIDIA_GEFORCE_845M},      /* GeForce 800 - mobile */
    {"GeForce 840M",                CARD_NVIDIA_GEFORCE_840M},      /* GeForce 800 - mobile */
    {"GeForce 830M",                CARD_NVIDIA_GEFORCE_830M},      /* GeForce 800 - mobile */
    {"GeForce 820M",                CARD_NVIDIA_GEFORCE_820M},      /* GeForce 800 - mobile */
    {"GTX 780 Ti",                  CARD_NVIDIA_GEFORCE_GTX780TI},  /* Geforce 700 - highend */
    {"GTX TITAN Black",             CARD_NVIDIA_GEFORCE_GTXTITANB}, /* Geforce 700 - highend */
    {"GTX TITAN Z",                 CARD_NVIDIA_GEFORCE_GTXTITANZ}, /* Geforce 700 - highend */
    {"GTX TITAN",                   CARD_NVIDIA_GEFORCE_GTXTITAN},  /* Geforce 700 - highend */
    {"GTX 780",                     CARD_NVIDIA_GEFORCE_GTX780},    /* Geforce 700 - highend */
    {"GTX 770M",                    CARD_NVIDIA_GEFORCE_GTX770M},   /* Geforce 700 - midend high mobile */
    {"GTX 770",                     CARD_NVIDIA_GEFORCE_GTX770},    /* Geforce 700 - highend */
    {"GTX 765M",                    CARD_NVIDIA_GEFORCE_GTX765M},   /* Geforce 700 - midend high mobile */
    {"GTX 760 Ti",                  CARD_NVIDIA_GEFORCE_GTX760TI},  /* Geforce 700 - midend high */
    {"GTX 760",                     CARD_NVIDIA_GEFORCE_GTX760},    /* Geforce 700 - midend high  */
    {"GTX 750 Ti",                  CARD_NVIDIA_GEFORCE_GTX750TI},  /* Geforce 700 - midend */
    {"GTX 750",                     CARD_NVIDIA_GEFORCE_GTX750},    /* Geforce 700 - midend */
    {"GT 750M",                     CARD_NVIDIA_GEFORCE_GT750M},    /* Geforce 700 - midend mobile */
    {"GT 740M",                     CARD_NVIDIA_GEFORCE_GT740M},    /* Geforce 700 - midend mobile */
    {"GT 730M",                     CARD_NVIDIA_GEFORCE_GT730M},    /* Geforce 700 - midend mobile */
    {"GT 730",                      CARD_NVIDIA_GEFORCE_GT730},     /* Geforce 700 - lowend */
    {"GTX 690",                     CARD_NVIDIA_GEFORCE_GTX690},    /* Geforce 600 - highend */
    {"GTX 680",                     CARD_NVIDIA_GEFORCE_GTX680},    /* Geforce 600 - highend */
    {"GTX 675MX",                   CARD_NVIDIA_GEFORCE_GTX675MX_1},/* Geforce 600 - highend */
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
    {"GTX 560M",                    CARD_NVIDIA_GEFORCE_GTX560M},   /* Geforce 500 - midend mobile */
    {"GTX 560",                     CARD_NVIDIA_GEFORCE_GTX560},    /* Geforce 500 - midend */
    {"GT 555M",                     CARD_NVIDIA_GEFORCE_GT555M},    /* Geforce 500 - midend mobile */
    {"GTX 550 Ti",                  CARD_NVIDIA_GEFORCE_GTX550},    /* Geforce 500 - midend */
    {"GT 540M",                     CARD_NVIDIA_GEFORCE_GT540M},    /* Geforce 500 - midend mobile */
    {"GT 525M",                     CARD_NVIDIA_GEFORCE_GT525M},    /* Geforce 500 - lowend mobile */
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
    {"GTS 250",                     CARD_NVIDIA_GEFORCE_GTS250},    /* Geforce 200 - midend */
    {"GT 240",                      CARD_NVIDIA_GEFORCE_GT240},     /* Geforce 200 - midend */
    {"GT 220",                      CARD_NVIDIA_GEFORCE_GT220},     /* Geforce 200 - lowend */
    {"GeForce 310",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 305",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 210",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"G 210",                       CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GTS 150",                     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"9800",                        CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"9700M GT",                    CARD_NVIDIA_GEFORCE_9700MGT},   /* Geforce 9 - midend */
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
 * e.g. HD 4800 is returned for multiple cards, even for RV790 based ones. */
cards_amd_binary[] =
{
    {"RX 480",                      CARD_AMD_RADEON_RX_480},
    {"RX 460",                      CARD_AMD_RADEON_RX_460},
    {"R9 Fury Series",              CARD_AMD_RADEON_R9_FURY},
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
    {"HD 6480G",                    CARD_AMD_RADEON_HD6480G},
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
    /* Skylake */
    {"Iris Pro Graphics P580",      CARD_INTEL_IPP580_1},
    {"Skylake",                     CARD_INTEL_HD520_1},
    /* Broadwell */
    {"Iris Pro P6300",              CARD_INTEL_IPP6300},
    {"Iris Pro 6200",               CARD_INTEL_IP6200},
    {"Iris 6100",                   CARD_INTEL_I6100},
    {"Iris(TM) Graphics 6100",      CARD_INTEL_I6100},  /* MacOS */
    /* Haswell */
    {"Iris Pro 5200",               CARD_INTEL_IP5200_1},
    {"Iris 5100",                   CARD_INTEL_I5100_1},
    {"HD Graphics 5000",            CARD_INTEL_HD5000_1}, /* MacOS */
    {"Haswell Mobile",              CARD_INTEL_HWM},
    {"Iris OpenGL Engine",          CARD_INTEL_HWM},    /* MacOS */
    /* Ivybridge */
    {"Ivybridge Server",            CARD_INTEL_IVBS},
    {"Ivybridge Mobile",            CARD_INTEL_IVBM},
    {"Ivybridge Desktop",           CARD_INTEL_IVBD},
    {"HD Graphics 4000",            CARD_INTEL_IVBD},   /* MacOS */
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
 * drivers: R700, RV790, R680, RV535, RV516, R410, RS485, RV360, RV351. */
cards_amd_mesa[] =
{
    /* Polaris 10/11 */
    {"POLARIS10",                   CARD_AMD_RADEON_RX_480},
    {"POLARIS11",                   CARD_AMD_RADEON_RX_460},
    /* Volcanic Islands */
    {"FIJI",                        CARD_AMD_RADEON_R9_FURY},
    {"TONGA",                       CARD_AMD_RADEON_R9_285},
    /* Sea Islands */
    {"HAWAII",                      CARD_AMD_RADEON_R9_290},
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
    {"RC410",                       CARD_AMD_RADEON_XPRESS_200M},
    /* R300 */
    {"R360",                        CARD_AMD_RADEON_9500},
    {"R350",                        CARD_AMD_RADEON_9500},
    {"R300",                        CARD_AMD_RADEON_9500},
    {"RV380",                       CARD_AMD_RADEON_9500},
    {"RV370",                       CARD_AMD_RADEON_9500},
    {"RV360",                       CARD_AMD_RADEON_9500},
    {"RV351",                       CARD_AMD_RADEON_9500},
    {"RV350",                       CARD_AMD_RADEON_9500},
},
cards_nvidia_mesa[] =
{
    /* Maxwell */
    {"NV124",                       CARD_NVIDIA_GEFORCE_GTX970},
    {"NV120",                       CARD_NVIDIA_GEFORCE_GTX980TI},
    {"NV118",                       CARD_NVIDIA_GEFORCE_840M},
    {"NV117",                       CARD_NVIDIA_GEFORCE_GTX750},
    /* Kepler */
    {"NV108",                       CARD_NVIDIA_GEFORCE_GT740M},
    {"NV106",                       CARD_NVIDIA_GEFORCE_GT720},
    {"NVF1",                        CARD_NVIDIA_GEFORCE_GTX780TI},
    {"NVF0",                        CARD_NVIDIA_GEFORCE_GTX780},
    {"NVE6",                        CARD_NVIDIA_GEFORCE_GTX770M},
    {"NVE4",                        CARD_NVIDIA_GEFORCE_GTX680},    /* 690 / 675MX / 760TI */
    /* Fermi */
    {"NVD9",                        CARD_NVIDIA_GEFORCE_GT520},
    {"NVD7",                        CARD_NVIDIA_GEFORCE_820M},
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
cards_redhat[] =
{
    {"virgl",                       CARD_REDHAT_VIRGL},
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
    {GL_VENDOR_NVIDIA,  "NVIDIA binary driver",             cards_nvidia_binary,    ARRAY_SIZE(cards_nvidia_binary)},
},
redhat_gl_vendor_table[] =
{
    {GL_VENDOR_MESA,    "Red Hat driver",                   cards_redhat,           ARRAY_SIZE(cards_redhat)},
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
}
card_vendor_table[] =
{
    {HW_VENDOR_AMD,    "AMD",    amd_gl_vendor_table,    ARRAY_SIZE(amd_gl_vendor_table)},
    {HW_VENDOR_NVIDIA, "NVIDIA", nvidia_gl_vendor_table, ARRAY_SIZE(nvidia_gl_vendor_table)},
    {HW_VENDOR_REDHAT, "Red Hat",redhat_gl_vendor_table, ARRAY_SIZE(redhat_gl_vendor_table)},
    {HW_VENDOR_VMWARE, "VMware", vmware_gl_vendor_table, ARRAY_SIZE(vmware_gl_vendor_table)},
    {HW_VENDOR_INTEL,  "Intel",  intel_gl_vendor_table,  ARRAY_SIZE(intel_gl_vendor_table)},
};

static enum wined3d_pci_device wined3d_guess_card(enum wined3d_feature_level feature_level,
        const char *gl_renderer, enum wined3d_gl_vendor *gl_vendor,
        enum wined3d_pci_vendor *card_vendor)
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
     * GPUs just for NVIDIA. Various cards share the same renderer string, so
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
     * the Direct3D9 check, we know that the card (in case of NVIDIA) is at
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

    enum wined3d_pci_device device;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(card_vendor_table); ++i)
    {
        if (card_vendor_table[i].card_vendor != *card_vendor)
            continue;

        TRACE("Applying card selector \"%s\".\n", card_vendor_table[i].description);
        device = select_card_handler(card_vendor_table[i].gl_vendor_selection,
                card_vendor_table[i].gl_vendor_count, *gl_vendor, gl_renderer);
        if (device != PCI_DEVICE_NONE)
            return device;

        TRACE("Unrecognized renderer %s, falling back to default.\n", debugstr_a(gl_renderer));

        return wined3d_gpu_from_feature_level(card_vendor, feature_level);
    }

    FIXME("No card selector available for card vendor %04x (using GL_RENDERER %s).\n",
            *card_vendor, debugstr_a(gl_renderer));

    return wined3d_gpu_from_feature_level(card_vendor, feature_level);
}

static const struct wined3d_vertex_pipe_ops *select_vertex_implementation(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_backend_ops *shader_backend_ops)
{
    if (shader_backend_ops == &glsl_shader_backend && gl_info->supported[ARB_VERTEX_SHADER])
        return &glsl_vertex_pipe;
    return &ffp_vertex_pipe;
}

static const struct wined3d_fragment_pipe_ops *select_fragment_implementation(const struct wined3d_gl_info *gl_info,
        const struct wined3d_shader_backend_ops *shader_backend_ops)
{
    if (shader_backend_ops == &glsl_shader_backend && gl_info->supported[ARB_FRAGMENT_SHADER])
        return &glsl_fragment_pipe;
    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
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
    BOOL glsl = wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_AUTO
            || wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_GLSL;
    BOOL arb = wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_AUTO
            || wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_ARB;

    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] && !glsl)
    {
        ERR_(winediag)("Ignoring the shader backend registry key. "
                "GLSL is the only shader backend available on core profile contexts. "
                "You need to explicitly set GL version to use legacy contexts.\n");
        glsl = TRUE;
    }

    glsl = glsl && gl_info->glsl_version >= MAKEDWORD_VERSION(1, 20);

    if (glsl && gl_info->supported[ARB_VERTEX_SHADER] && gl_info->supported[ARB_FRAGMENT_SHADER])
        return &glsl_shader_backend;
    if (arb && gl_info->supported[ARB_VERTEX_PROGRAM] && gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return &arb_program_shader_backend;
    if (glsl && (gl_info->supported[ARB_VERTEX_SHADER] || gl_info->supported[ARB_FRAGMENT_SHADER]))
        return &glsl_shader_backend;
    if (arb && (gl_info->supported[ARB_VERTEX_PROGRAM] || gl_info->supported[ARB_FRAGMENT_PROGRAM]))
        return &arb_program_shader_backend;
    return &none_shader_backend;
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

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
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
    /* GL_ARB_base_instance */
    USE_GL_FUNC(glDrawArraysInstancedBaseInstance)
    USE_GL_FUNC(glDrawElementsInstancedBaseVertexBaseInstance)
    /* GL_ARB_blend_func_extended */
    USE_GL_FUNC(glBindFragDataLocationIndexed)
    USE_GL_FUNC(glGetFragDataIndex)
    /* GL_ARB_buffer_storage */
    USE_GL_FUNC(glBufferStorage)
    /* GL_ARB_clear_buffer_object */
    USE_GL_FUNC(glClearBufferData)
    USE_GL_FUNC(glClearBufferSubData)
    /* GL_ARB_clear_texture */
    USE_GL_FUNC(glClearTexImage)
    USE_GL_FUNC(glClearTexSubImage)
    /* GL_ARB_clip_control */
    USE_GL_FUNC(glClipControl)
    /* GL_ARB_color_buffer_float */
    USE_GL_FUNC(glClampColorARB)
    /* GL_ARB_compute_shader */
    USE_GL_FUNC(glDispatchCompute)
    USE_GL_FUNC(glDispatchComputeIndirect)
    /* GL_ARB_copy_buffer */
    USE_GL_FUNC(glCopyBufferSubData)
    /* GL_ARB_copy_image */
    USE_GL_FUNC(glCopyImageSubData)
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
    /* GL_ARB_draw_indirect */
    USE_GL_FUNC(glDrawArraysIndirect)
    USE_GL_FUNC(glDrawElementsIndirect)
    /* GL_ARB_draw_instanced */
    USE_GL_FUNC(glDrawArraysInstancedARB)
    USE_GL_FUNC(glDrawElementsInstancedARB)
    /* GL_ARB_ES2_compatibility */
    USE_GL_FUNC(glReleaseShaderCompiler)
    USE_GL_FUNC(glShaderBinary)
    USE_GL_FUNC(glGetShaderPrecisionFormat)
    USE_GL_FUNC(glDepthRangef)
    USE_GL_FUNC(glClearDepthf)
    /* GL_ARB_framebuffer_no_attachments */
    USE_GL_FUNC(glFramebufferParameteri)
    /* GL_ARB_framebuffer_object */
    USE_GL_FUNC(glBindFramebuffer)
    USE_GL_FUNC(glBindRenderbuffer)
    USE_GL_FUNC(glBlitFramebuffer)
    USE_GL_FUNC(glCheckFramebufferStatus)
    USE_GL_FUNC(glDeleteFramebuffers)
    USE_GL_FUNC(glDeleteRenderbuffers)
    USE_GL_FUNC(glFramebufferRenderbuffer)
    USE_GL_FUNC(glFramebufferTexture)
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
    /* GL_ARB_polgyon_offset_clamp */
    USE_GL_FUNC(glPolygonOffsetClamp)
    /* GL_ARB_provoking_vertex */
    USE_GL_FUNC(glProvokingVertex)
    /* GL_ARB_sample_shading */
    USE_GL_FUNC(glMinSampleShadingARB)
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
    /* GL_ARB_shader_atomic_counters */
    USE_GL_FUNC(glGetActiveAtomicCounterBufferiv)
    /* GL_ARB_shader_image_load_store */
    USE_GL_FUNC(glBindImageTexture)
    USE_GL_FUNC(glMemoryBarrier)
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
    /* GL_ARB_shader_storage_buffer_object */
    USE_GL_FUNC(glShaderStorageBlockBinding)
    /* GL_ARB_sync */
    USE_GL_FUNC(glClientWaitSync)
    USE_GL_FUNC(glDeleteSync)
    USE_GL_FUNC(glFenceSync)
    USE_GL_FUNC(glGetInteger64v)
    USE_GL_FUNC(glGetSynciv)
    USE_GL_FUNC(glIsSync)
    USE_GL_FUNC(glWaitSync)
    /* GL_ARB_tessellation_shader */
    USE_GL_FUNC(glPatchParameteri)
    USE_GL_FUNC(glPatchParameterfv)
    /* GL_ARB_texture_buffer_object */
    USE_GL_FUNC(glTexBufferARB)
    /* GL_ARB_texture_buffer_range */
    USE_GL_FUNC(glTexBufferRange)
    /* GL_ARB_texture_compression */
    USE_GL_FUNC(glCompressedTexImage2DARB)
    USE_GL_FUNC(glCompressedTexImage3DARB)
    USE_GL_FUNC(glCompressedTexSubImage2DARB)
    USE_GL_FUNC(glCompressedTexSubImage3DARB)
    USE_GL_FUNC(glGetCompressedTexImageARB)
    /* GL_ARB_texture_multisample */
    USE_GL_FUNC(glGetMultisamplefv);
    USE_GL_FUNC(glSampleMaski);
    USE_GL_FUNC(glTexImage2DMultisample);
    USE_GL_FUNC(glTexImage3DMultisample);
    /* GL_ARB_texture_storage */
    USE_GL_FUNC(glTexStorage1D)
    USE_GL_FUNC(glTexStorage2D)
    USE_GL_FUNC(glTexStorage3D)
    /* GL_ARB_texture_storage_multisample */
    USE_GL_FUNC(glTexStorage2DMultisample);
    USE_GL_FUNC(glTexStorage3DMultisample);
    /* GL_ARB_texture_view */
    USE_GL_FUNC(glTextureView)
    /* GL_ARB_timer_query */
    USE_GL_FUNC(glQueryCounter)
    USE_GL_FUNC(glGetQueryObjectui64v)
    /* GL_ARB_transform_feedback2 */
    USE_GL_FUNC(glBindTransformFeedback);
    USE_GL_FUNC(glDeleteTransformFeedbacks);
    USE_GL_FUNC(glDrawTransformFeedback);
    USE_GL_FUNC(glGenTransformFeedbacks);
    USE_GL_FUNC(glIsTransformFeedback);
    USE_GL_FUNC(glPauseTransformFeedback);
    USE_GL_FUNC(glResumeTransformFeedback);
    /* GL_ARB_transform_feedback3 */
    USE_GL_FUNC(glBeginQueryIndexed);
    USE_GL_FUNC(glDrawTransformFeedbackStream);
    USE_GL_FUNC(glEndQueryIndexed);
    USE_GL_FUNC(glGetQueryIndexediv);
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
    /* GL_ARB_viewport_array */
    USE_GL_FUNC(glDepthRangeArrayv)
    USE_GL_FUNC(glDepthRangeIndexed)
    USE_GL_FUNC(glGetDoublei_v)
    USE_GL_FUNC(glGetFloati_v)
    USE_GL_FUNC(glScissorArrayv)
    USE_GL_FUNC(glScissorIndexed)
    USE_GL_FUNC(glScissorIndexedv)
    USE_GL_FUNC(glViewportArrayv)
    USE_GL_FUNC(glViewportIndexedf)
    USE_GL_FUNC(glViewportIndexedfv)
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
    /* GL_EXT_memory_object */
    USE_GL_FUNC(glGetUnsignedBytei_vEXT)
    USE_GL_FUNC(glGetUnsignedBytevEXT)
    /* GL_EXT_point_parameters */
    USE_GL_FUNC(glPointParameterfEXT)
    USE_GL_FUNC(glPointParameterfvEXT)
    /* GL_EXT_polgyon_offset_clamp */
    USE_GL_FUNC(glPolygonOffsetClampEXT)
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
    USE_GL_FUNC(wglQueryCurrentRendererIntegerWINE)
    USE_GL_FUNC(wglQueryCurrentRendererStringWINE)
    USE_GL_FUNC(wglQueryRendererIntegerWINE)
    USE_GL_FUNC(wglQueryRendererStringWINE)
    USE_GL_FUNC(wglSetPixelFormatWINE)
    USE_GL_FUNC(wglSwapIntervalEXT)

    /* Newer core functions */
    USE_GL_FUNC(glActiveTexture)                               /* OpenGL 1.3 */
    USE_GL_FUNC(glAttachShader)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glBeginQuery)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glBeginTransformFeedback)                      /* OpenGL 3.0 */
    USE_GL_FUNC(glBindAttribLocation)                          /* OpenGL 2.0 */
    USE_GL_FUNC(glBindBuffer)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glBindFragDataLocation)                        /* OpenGL 3.0 */
    USE_GL_FUNC(glBindVertexArray)                             /* OpenGL 3.0 */
    USE_GL_FUNC(glBlendColor)                                  /* OpenGL 1.4 */
    USE_GL_FUNC(glBlendEquation)                               /* OpenGL 1.4 */
    USE_GL_FUNC(glBlendEquationSeparate)                       /* OpenGL 2.0 */
    USE_GL_FUNC(glBlendFuncSeparate)                           /* OpenGL 1.4 */
    USE_GL_FUNC(glBufferData)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glBufferSubData)                               /* OpenGL 1.5 */
    USE_GL_FUNC(glColorMaski)                                  /* OpenGL 3.0 */
    USE_GL_FUNC(glCompileShader)                               /* OpenGL 2.0 */
    USE_GL_FUNC(glCompressedTexImage2D)                        /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexImage3D)                        /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexSubImage2D)                     /* OpenGL 1.3 */
    USE_GL_FUNC(glCompressedTexSubImage3D)                     /* OpenGL 1.3 */
    USE_GL_FUNC(glCreateProgram)                               /* OpenGL 2.0 */
    USE_GL_FUNC(glCreateShader)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glDebugMessageCallback)                        /* OpenGL 4.3 */
    USE_GL_FUNC(glDebugMessageControl)                         /* OpenGL 4.3 */
    USE_GL_FUNC(glDebugMessageInsert)                          /* OpenGL 4.3 */
    USE_GL_FUNC(glDeleteBuffers)                               /* OpenGL 1.5 */
    USE_GL_FUNC(glDeleteProgram)                               /* OpenGL 2.0 */
    USE_GL_FUNC(glDeleteQueries)                               /* OpenGL 1.5 */
    USE_GL_FUNC(glDeleteShader)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glDeleteVertexArrays)                          /* OpenGL 3.0 */
    USE_GL_FUNC(glDetachShader)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glDisablei)                                    /* OpenGL 3.0 */
    USE_GL_FUNC(glDisableVertexAttribArray)                    /* OpenGL 2.0 */
    USE_GL_FUNC(glDrawArraysInstanced)                         /* OpenGL 3.1 */
    USE_GL_FUNC(glDrawBuffers)                                 /* OpenGL 2.0 */
    USE_GL_FUNC(glDrawElementsInstanced)                       /* OpenGL 3.1 */
    USE_GL_FUNC(glEnablei)                                     /* OpenGL 3.0 */
    USE_GL_FUNC(glEnableVertexAttribArray)                     /* OpenGL 2.0 */
    USE_GL_FUNC(glEndQuery)                                    /* OpenGL 1.5 */
    USE_GL_FUNC(glEndTransformFeedback)                        /* OpenGL 3.0 */
    USE_GL_FUNC(glFramebufferTexture)                          /* OpenGL 3.2 */
    USE_GL_FUNC(glGenBuffers)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glGenQueries)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glGenVertexArrays)                             /* OpenGL 3.0 */
    USE_GL_FUNC(glGetActiveUniform)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glGetAttachedShaders)                          /* OpenGL 2.0 */
    USE_GL_FUNC(glGetAttribLocation)                           /* OpenGL 2.0 */
    USE_GL_FUNC(glGetBooleani_v)                               /* OpenGL 3.0 */
    USE_GL_FUNC(glGetBufferSubData)                            /* OpenGL 1.5 */
    USE_GL_FUNC(glGetCompressedTexImage)                       /* OpenGL 1.3 */
    USE_GL_FUNC(glGetDebugMessageLog)                          /* OpenGL 4.3 */
    USE_GL_FUNC(glGetIntegeri_v)                               /* OpenGL 3.0 */
    USE_GL_FUNC(glGetProgramInfoLog)                           /* OpenGL 2.0 */
    USE_GL_FUNC(glGetProgramiv)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glGetQueryiv)                                  /* OpenGL 1.5 */
    USE_GL_FUNC(glGetQueryObjectuiv)                           /* OpenGL 1.5 */
    USE_GL_FUNC(glGetShaderInfoLog)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glGetShaderiv)                                 /* OpenGL 2.0 */
    USE_GL_FUNC(glGetShaderSource)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glGetStringi)                                  /* OpenGL 3.0 */
    USE_GL_FUNC(glGetTextureLevelParameteriv)                  /* OpenGL 4.5 */
    USE_GL_FUNC(glGetTextureParameteriv)                       /* OpenGL 4.5 */
    USE_GL_FUNC(glGetUniformfv)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glGetUniformiv)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glGetUniformLocation)                          /* OpenGL 2.0 */
    USE_GL_FUNC(glIsEnabledi)                                  /* OpenGL 3.0 */
    USE_GL_FUNC(glLinkProgram)                                 /* OpenGL 2.0 */
    USE_GL_FUNC(glMapBuffer)                                   /* OpenGL 1.5 */
    USE_GL_FUNC(glMinSampleShading)                            /* OpenGL 4.0 */
    USE_GL_FUNC(glPointParameteri)                             /* OpenGL 1.4 */
    USE_GL_FUNC(glPointParameteriv)                            /* OpenGL 1.4 */
    USE_GL_FUNC(glShaderSource)                                /* OpenGL 2.0 */
    USE_GL_FUNC(glStencilFuncSeparate)                         /* OpenGL 2.0 */
    USE_GL_FUNC(glStencilOpSeparate)                           /* OpenGL 2.0 */
    USE_GL_FUNC(glTexBuffer)                                   /* OpenGL 3.1 */
    USE_GL_FUNC(glTexImage3D)                                  /* OpenGL 1.2 */
    USE_GL_FUNC(glTexSubImage3D)                               /* OpenGL 1.2 */
    USE_GL_FUNC(glTransformFeedbackVaryings)                   /* OpenGL 3.0 */
    USE_GL_FUNC(glUniform1f)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1fv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1i)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform1iv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2f)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2fv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2i)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform2iv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3f)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3fv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3i)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform3iv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4f)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4fv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4i)                                   /* OpenGL 2.0 */
    USE_GL_FUNC(glUniform4iv)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix2fv)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix3fv)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glUniformMatrix4fv)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glUnmapBuffer)                                 /* OpenGL 1.5 */
    USE_GL_FUNC(glUseProgram)                                  /* OpenGL 2.0 */
    USE_GL_FUNC(glValidateProgram)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib1f)                              /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib1fv)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib2f)                              /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib2fv)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib3f)                              /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib3fv)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4f)                              /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4fv)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nsv)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nub)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nubv)                           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4Nusv)                           /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4sv)                             /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttrib4ubv)                            /* OpenGL 2.0 */
    USE_GL_FUNC(glVertexAttribDivisor)                         /* OpenGL 3.3 */
    USE_GL_FUNC(glVertexAttribIPointer)                        /* OpenGL 3.0 */
    USE_GL_FUNC(glVertexAttribPointer)                         /* OpenGL 2.0 */
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
    MAP_GL_FUNCTION(glBindFragDataLocation, glBindFragDataLocationEXT);
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
    MAP_GL_FUNCTION(glDisablei, glDisableIndexedEXT);
    MAP_GL_FUNCTION(glDisableVertexAttribArray, glDisableVertexAttribArrayARB);
    MAP_GL_FUNCTION(glDrawArraysInstanced, glDrawArraysInstancedARB);
    MAP_GL_FUNCTION(glDrawBuffers, glDrawBuffersARB);
    MAP_GL_FUNCTION(glDrawElementsInstanced, glDrawElementsInstancedARB);
    MAP_GL_FUNCTION(glEnablei, glEnableIndexedEXT);
    MAP_GL_FUNCTION(glEnableVertexAttribArray, glEnableVertexAttribArrayARB);
    MAP_GL_FUNCTION(glEndQuery, glEndQueryARB);
    MAP_GL_FUNCTION(glFramebufferTexture, glFramebufferTextureARB);
    MAP_GL_FUNCTION(glGenBuffers, glGenBuffersARB);
    MAP_GL_FUNCTION(glGenQueries, glGenQueriesARB);
    MAP_GL_FUNCTION(glGetActiveUniform, glGetActiveUniformARB);
    MAP_GL_FUNCTION(glGetAttachedShaders, glGetAttachedObjectsARB);
    MAP_GL_FUNCTION(glGetAttribLocation, glGetAttribLocationARB);
    MAP_GL_FUNCTION(glGetBooleani_v, glGetBooleanIndexedvEXT);
    MAP_GL_FUNCTION(glGetBufferSubData, glGetBufferSubDataARB);
    MAP_GL_FUNCTION(glGetCompressedTexImage, glGetCompressedTexImageARB);
    MAP_GL_FUNCTION(glGetDebugMessageLog, glGetDebugMessageLogARB);
    MAP_GL_FUNCTION(glGetIntegeri_v, glGetIntegerIndexedvEXT);
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
    MAP_GL_FUNCTION(glIsEnabledi, glIsEnabledIndexedEXT);
    MAP_GL_FUNCTION(glLinkProgram, glLinkProgramARB);
    MAP_GL_FUNCTION(glMapBuffer, glMapBufferARB);
    MAP_GL_FUNCTION(glMinSampleShading, glMinSampleShadingARB);
    MAP_GL_FUNCTION(glPolygonOffsetClamp, glPolygonOffsetClampEXT);
    MAP_GL_FUNCTION_CAST(glShaderSource, glShaderSourceARB);
    MAP_GL_FUNCTION(glTexBuffer, glTexBufferARB);
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
    MAP_GL_FUNCTION(glVertexAttrib4Nub, glVertexAttrib4NubARB);
    MAP_GL_FUNCTION(glVertexAttrib4Nubv, glVertexAttrib4NubvARB);
    MAP_GL_FUNCTION(glVertexAttrib4Nusv, glVertexAttrib4NusvARB);
    MAP_GL_FUNCTION(glVertexAttrib4sv, glVertexAttrib4svARB);
    MAP_GL_FUNCTION(glVertexAttrib4ubv, glVertexAttrib4ubvARB);
    MAP_GL_FUNCTION(glVertexAttribDivisor, glVertexAttribDivisorARB);
    MAP_GL_FUNCTION(glVertexAttribIPointer, glVertexAttribIPointerEXT);
    MAP_GL_FUNCTION(glVertexAttribPointer, glVertexAttribPointerARB);
#undef MAP_GL_FUNCTION
#undef MAP_GL_FUNCTION_CAST
}

static void wined3d_adapter_init_limits(struct wined3d_gl_info *gl_info)
{
    unsigned int i, sampler_count;
    GLint gl_max;

    gl_info->limits.buffers = 1;
    gl_info->limits.textures = 0;
    gl_info->limits.texture_coords = 0;
    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        gl_info->limits.uniform_blocks[i] = 0;
        gl_info->limits.samplers[i] = 0;
    }
    gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL] = 1;
    gl_info->limits.combined_samplers = gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL];
    gl_info->limits.graphics_samplers = gl_info->limits.combined_samplers;
    gl_info->limits.vertex_attribs = 16;
    gl_info->limits.texture_buffer_offset_alignment = 1;
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

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_CLIP_DISTANCES, &gl_max);
    gl_info->limits.user_clip_distances = min(WINED3D_MAX_CLIP_DISTANCES, gl_max);
    TRACE("Clip plane support - max planes %d.\n", gl_max);

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
        gl_info->limits.lights = gl_max;
        TRACE("Light support - max lights %d.\n", gl_max);
    }

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    gl_info->limits.texture_size = gl_max;
    TRACE("Maximum texture size support - max texture size %d.\n", gl_max);

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
        gl_info->limits.buffers = min(MAX_RENDER_TARGET_VIEWS, gl_max);
        TRACE("Max draw buffers: %u.\n", gl_max);
    }
    if (gl_info->supported[ARB_BLEND_FUNC_EXTENDED])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &gl_max);
        gl_info->limits.dual_buffers = gl_max;
        TRACE("Max dual source draw buffers: %u.\n", gl_max);
    }
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
        if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
            gl_info->limits.textures = min(WINED3D_MAX_TEXTURES, gl_max);
            TRACE("Max textures: %d.\n", gl_info->limits.textures);

            if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
            {
                gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &gl_max);
                gl_info->limits.texture_coords = min(WINED3D_MAX_TEXTURES, gl_max);
            }
            else
            {
                gl_info->limits.texture_coords = gl_info->limits.textures;
            }
            TRACE("Max texture coords: %d.\n", gl_info->limits.texture_coords);
        }

        if (gl_info->supported[ARB_FRAGMENT_PROGRAM] || gl_info->supported[ARB_FRAGMENT_SHADER])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gl_max);
            gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL] = gl_max;
        }
        else
        {
            gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL] = gl_info->limits.textures;
        }
        TRACE("Max fragment samplers: %d.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL]);

        if (gl_info->supported[ARB_VERTEX_SHADER])
        {
            unsigned int vertex_sampler_count;

            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &gl_max);
            vertex_sampler_count = gl_info->limits.samplers[WINED3D_SHADER_TYPE_VERTEX] = gl_max;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &gl_max);
            gl_info->limits.combined_samplers = gl_max;
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &gl_max);
            gl_info->limits.vertex_attribs = gl_max;

            /* Loading GLSL sampler uniforms is much simpler if we can assume
             * that the sampler setup is known at shader link time. In a
             * vertex shader + pixel shader combination this isn't an issue
             * because then the sampler setup only depends on the two shaders.
             * If a pixel shader is used with fixed-function vertex processing
             * we're fine too because fixed-function vertex processing doesn't
             * use any samplers. If fixed-function fragment processing is used
             * we have to make sure that all vertex sampler setups are valid
             * together with all possible fixed-function fragment processing
             * setups. This is true if vsamplers + WINED3D_MAX_TEXTURES <= max_samplers.
             * This is true on all Direct3D 9 cards that support vertex
             * texture fetch (GeForce 6 and GeForce 7 cards). Direct3D 9
             * Radeon cards do not support vertex texture fetch. Direct3D 10
             * cards have 128 samplers, and Direct3D 9 is limited to 8
             * fixed-function texture stages and 4 vertex samplers.
             * Direct3D 10 does not have a fixed-function pipeline anymore.
             *
             * So this is just a check to check that our assumption holds
             * true. If not, write a warning and reduce the number of vertex
             * samplers or probably disable vertex texture fetch. */
            if (vertex_sampler_count && gl_info->limits.combined_samplers < 12
                    && WINED3D_MAX_TEXTURES + vertex_sampler_count > gl_info->limits.combined_samplers)
            {
                FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers.\n",
                        vertex_sampler_count, gl_info->limits.combined_samplers);
                FIXME("Expected vertex samplers + WINED3D_MAX_TEXTURES(=8) > combined_samplers.\n");
                if (gl_info->limits.combined_samplers > WINED3D_MAX_TEXTURES)
                    vertex_sampler_count = gl_info->limits.combined_samplers - WINED3D_MAX_TEXTURES;
                else
                    vertex_sampler_count = 0;
                gl_info->limits.samplers[WINED3D_SHADER_TYPE_VERTEX] = vertex_sampler_count;
            }
        }
        else
        {
            gl_info->limits.combined_samplers = gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL];
        }
        TRACE("Max vertex samplers: %u.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_VERTEX]);
        TRACE("Max combined samplers: %u.\n", gl_info->limits.combined_samplers);
        TRACE("Max vertex attributes: %u.\n", gl_info->limits.vertex_attribs);
    }
    else
    {
        gl_info->limits.textures = 1;
        gl_info->limits.texture_coords = 1;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &gl_max);
        gl_info->limits.texture3d_size = gl_max;
        TRACE("Max texture3D size: %d.\n", gl_info->limits.texture3d_size);
    }
    if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &gl_max);
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
            gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_VERTEX] = min(gl_max, WINED3D_MAX_CBS);
            TRACE("Max vertex uniform blocks: %u (%d).\n",
                    gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_VERTEX], gl_max);
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &gl_max);
            gl_info->limits.glsl_max_uniform_block_size = gl_max;
            TRACE("Max uniform block size %u.\n", gl_max);
        }
    }
    if (gl_info->supported[ARB_TESSELLATION_SHADER])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS, &gl_max);
        gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_HULL] = min(gl_max, WINED3D_MAX_CBS);
        TRACE("Max hull uniform blocks: %u (%d).\n",
                gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_HULL], gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, &gl_max);
        gl_info->limits.samplers[WINED3D_SHADER_TYPE_HULL] = gl_max;
        TRACE("Max hull samplers: %u.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_HULL]);

        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, &gl_max);
        gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_DOMAIN] = min(gl_max, WINED3D_MAX_CBS);
        TRACE("Max domain uniform blocks: %u (%d).\n",
                gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_DOMAIN], gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, &gl_max);
        gl_info->limits.samplers[WINED3D_SHADER_TYPE_DOMAIN] = gl_max;
        TRACE("Max domain samplers: %u.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_DOMAIN]);
    }
    if (gl_info->supported[WINED3D_GL_VERSION_3_2] && gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &gl_max);
        gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_GEOMETRY] = min(gl_max, WINED3D_MAX_CBS);
        TRACE("Max geometry uniform blocks: %u (%d).\n",
                gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_GEOMETRY], gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &gl_max);
        gl_info->limits.samplers[WINED3D_SHADER_TYPE_GEOMETRY] = gl_max;
        TRACE("Max geometry samplers: %u.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_GEOMETRY]);
    }
    if (gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &gl_max);
        gl_info->limits.glsl_ps_float_constants = gl_max / 4;
        TRACE("Max ARB_FRAGMENT_SHADER float constants: %u.\n", gl_info->limits.glsl_ps_float_constants);
        if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max);
            gl_info->limits.glsl_varyings = gl_max;
        }
        else
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &gl_max);
            gl_info->limits.glsl_varyings = gl_max - 4;
        }
        TRACE("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_info->limits.glsl_varyings,
                gl_info->limits.glsl_varyings / 4);

        if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &gl_max);
            gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_PIXEL] = min(gl_max, WINED3D_MAX_CBS);
            TRACE("Max fragment uniform blocks: %u (%d).\n",
                    gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_PIXEL], gl_max);
        }
    }
    if (gl_info->supported[ARB_COMPUTE_SHADER])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &gl_max);
        gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_COMPUTE] = min(gl_max, WINED3D_MAX_CBS);
        TRACE("Max compute uniform blocks: %u (%d).\n",
                gl_info->limits.uniform_blocks[WINED3D_SHADER_TYPE_COMPUTE], gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &gl_max);
        gl_info->limits.samplers[WINED3D_SHADER_TYPE_COMPUTE] = gl_max;
        TRACE("Max compute samplers: %u.\n", gl_info->limits.samplers[WINED3D_SHADER_TYPE_COMPUTE]);
    }
    if (gl_info->supported[ARB_UNIFORM_BUFFER_OBJECT])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &gl_max);
        TRACE("Max combined uniform blocks: %d.\n", gl_max);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &gl_max);
        TRACE("Max uniform buffer bindings: %d.\n", gl_max);
    }
    if (gl_info->supported[ARB_TEXTURE_BUFFER_RANGE])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &gl_max);
        gl_info->limits.texture_buffer_offset_alignment = gl_max;
        TRACE("Minimum required texture buffer offset alignment %d.\n", gl_max);
    }
    if (gl_info->supported[ARB_SHADER_ATOMIC_COUNTERS])
    {
        GLint max_fragment_buffers, max_combined_buffers, max_bindings;
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &max_fragment_buffers);
        TRACE("Max fragment atomic counter buffers: %d.\n", max_fragment_buffers);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, &max_combined_buffers);
        TRACE("Max combined atomic counter buffers: %d.\n", max_combined_buffers);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &max_bindings);
        TRACE("Max atomic counter buffer bindings: %d.\n", max_bindings);
        if (max_fragment_buffers < MAX_UNORDERED_ACCESS_VIEWS
                || max_combined_buffers < MAX_UNORDERED_ACCESS_VIEWS
                || max_bindings < MAX_UNORDERED_ACCESS_VIEWS)
        {
            WARN("Disabling ARB_shader_atomic_counters.\n");
            gl_info->supported[ARB_SHADER_ATOMIC_COUNTERS] = FALSE;
        }
    }
    if (gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_VERTEX_STREAMS, &gl_max);
        TRACE("Max vertex streams: %d.\n", gl_max);
    }

    if (gl_info->supported[NV_LIGHT_MAX_EXPONENT])
        gl_info->gl_ops.gl.p_glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess);
    else
        gl_info->limits.shininess = 128.0f;

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT] || gl_info->supported[EXT_FRAMEBUFFER_MULTISAMPLE])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
        gl_info->limits.samples = gl_max;
    }

    if (gl_info->supported[ARB_FRAMEBUFFER_NO_ATTACHMENTS])
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &gl_max);
        gl_info->limits.framebuffer_width = gl_max;
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &gl_max);
        gl_info->limits.framebuffer_height = gl_max;
    }
    else
    {
        gl_info->limits.framebuffer_width = gl_info->limits.texture_size;
        gl_info->limits.framebuffer_height = gl_info->limits.texture_size;
    }

    gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL] =
            min(gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL], MAX_GL_FRAGMENT_SAMPLERS);
    sampler_count = 0;
    for (i = 0; i < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++i)
        sampler_count += gl_info->limits.samplers[i];
    if (gl_info->supported[WINED3D_GL_VERSION_3_2] && gl_info->limits.combined_samplers < sampler_count)
    {
        /* The minimum value for GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS in OpenGL
         * 3.2 is 48 (16 per stage). When tessellation shaders are supported
         * the minimum value is increased to 80. */
        WARN("Graphics pipeline sampler count %u is greater than combined sampler count %u.\n",
                sampler_count, gl_info->limits.combined_samplers);
        for (i = 0; i < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++i)
            gl_info->limits.samplers[i] = min(gl_info->limits.samplers[i], 16);
    }

    /* A majority of OpenGL implementations allow us to statically partition
     * the set of texture bindings into six separate sets. */
    gl_info->limits.graphics_samplers = gl_info->limits.combined_samplers;
    sampler_count = 0;
    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
        sampler_count += gl_info->limits.samplers[i];
    if (gl_info->limits.combined_samplers >= sampler_count)
        gl_info->limits.graphics_samplers -= gl_info->limits.samplers[WINED3D_SHADER_TYPE_COMPUTE];
}

/* Context activation is done by the caller. */
static BOOL wined3d_adapter_init_gl_caps(struct wined3d_adapter *adapter,
        struct wined3d_caps_gl_ctx *caps_gl_ctx, unsigned int wined3d_creation_flags)
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
        {EXT_PACKED_FLOAT,                 MAKEDWORD_VERSION(3, 0)},
        {EXT_TEXTURE_ARRAY,                MAKEDWORD_VERSION(3, 0)},
        {EXT_TEXTURE_INTEGER,              MAKEDWORD_VERSION(3, 0)},
        {EXT_TEXTURE_SHARED_EXPONENT,      MAKEDWORD_VERSION(3, 0)},
        /* We don't want to enable EXT_GPU_SHADER4: even though similar
         * functionality is available in core GL 3.0 / GLSL 1.30, it's different
         * enough that reusing the same flag for the new features hurts more
         * than it helps. */
        /* EXT_framebuffer_object, EXT_framebuffer_blit,
         * EXT_framebuffer_multisample and EXT_packed_depth_stencil
         * are integrated into ARB_framebuffer_object. */

        {ARB_COPY_BUFFER,                  MAKEDWORD_VERSION(3, 1)},
        {ARB_DRAW_INSTANCED,               MAKEDWORD_VERSION(3, 1)},
        {ARB_TEXTURE_BUFFER_OBJECT,        MAKEDWORD_VERSION(3, 1)},
        {ARB_UNIFORM_BUFFER_OBJECT,        MAKEDWORD_VERSION(3, 1)},
        {EXT_TEXTURE_SNORM,                MAKEDWORD_VERSION(3, 1)},
        /* We don't need or want GL_ARB_texture_rectangle (core in 3.1). */

        {ARB_DEPTH_CLAMP,                  MAKEDWORD_VERSION(3, 2)},
        {ARB_DRAW_ELEMENTS_BASE_VERTEX,    MAKEDWORD_VERSION(3, 2)},
        /* ARB_geometry_shader4 exposes a somewhat different API compared to 3.2
         * core geometry shaders so it's not really correct to expose the
         * extension for core-only support. */
        {ARB_FRAGMENT_COORD_CONVENTIONS,   MAKEDWORD_VERSION(3, 2)},
        {ARB_PROVOKING_VERTEX,             MAKEDWORD_VERSION(3, 2)},
        {ARB_SEAMLESS_CUBE_MAP,            MAKEDWORD_VERSION(3, 2)},
        {ARB_SYNC,                         MAKEDWORD_VERSION(3, 2)},
        {ARB_TEXTURE_MULTISAMPLE,          MAKEDWORD_VERSION(3, 2)},
        {ARB_VERTEX_ARRAY_BGRA,            MAKEDWORD_VERSION(3, 2)},

        {ARB_BLEND_FUNC_EXTENDED,          MAKEDWORD_VERSION(3, 3)},
        {ARB_EXPLICIT_ATTRIB_LOCATION,     MAKEDWORD_VERSION(3, 3)},
        {ARB_INSTANCED_ARRAYS,             MAKEDWORD_VERSION(3, 3)},
        {ARB_SAMPLER_OBJECTS,              MAKEDWORD_VERSION(3, 3)},
        {ARB_SHADER_BIT_ENCODING,          MAKEDWORD_VERSION(3, 3)},
        {ARB_TEXTURE_RGB10_A2UI,           MAKEDWORD_VERSION(3, 3)},
        {ARB_TEXTURE_SWIZZLE,              MAKEDWORD_VERSION(3, 3)},
        {ARB_TIMER_QUERY,                  MAKEDWORD_VERSION(3, 3)},
        {ARB_VERTEX_TYPE_2_10_10_10_REV,   MAKEDWORD_VERSION(3, 3)},

        {ARB_DRAW_INDIRECT,                MAKEDWORD_VERSION(4, 0)},
        {ARB_GPU_SHADER5,                  MAKEDWORD_VERSION(4, 0)},
        {ARB_SAMPLE_SHADING,               MAKEDWORD_VERSION(4, 0)},
        {ARB_TESSELLATION_SHADER,          MAKEDWORD_VERSION(4, 0)},
        {ARB_TEXTURE_CUBE_MAP_ARRAY,       MAKEDWORD_VERSION(4, 0)},
        {ARB_TEXTURE_GATHER,               MAKEDWORD_VERSION(4, 0)},
        {ARB_TRANSFORM_FEEDBACK2,          MAKEDWORD_VERSION(4, 0)},
        {ARB_TRANSFORM_FEEDBACK3,          MAKEDWORD_VERSION(4, 0)},

        {ARB_ES2_COMPATIBILITY,            MAKEDWORD_VERSION(4, 1)},
        {ARB_VIEWPORT_ARRAY,               MAKEDWORD_VERSION(4, 1)},

        {ARB_BASE_INSTANCE,                MAKEDWORD_VERSION(4, 2)},
        {ARB_CONSERVATIVE_DEPTH,           MAKEDWORD_VERSION(4, 2)},
        {ARB_INTERNALFORMAT_QUERY,         MAKEDWORD_VERSION(4, 2)},
        {ARB_MAP_BUFFER_ALIGNMENT,         MAKEDWORD_VERSION(4, 2)},
        {ARB_SHADER_ATOMIC_COUNTERS,       MAKEDWORD_VERSION(4, 2)},
        {ARB_SHADER_IMAGE_LOAD_STORE,      MAKEDWORD_VERSION(4, 2)},
        {ARB_SHADING_LANGUAGE_420PACK,     MAKEDWORD_VERSION(4, 2)},
        {ARB_SHADING_LANGUAGE_PACKING,     MAKEDWORD_VERSION(4, 2)},
        {ARB_TEXTURE_COMPRESSION_BPTC,     MAKEDWORD_VERSION(4, 2)},
        {ARB_TEXTURE_STORAGE,              MAKEDWORD_VERSION(4, 2)},

        {ARB_CLEAR_BUFFER_OBJECT,          MAKEDWORD_VERSION(4, 3)},
        {ARB_COMPUTE_SHADER,               MAKEDWORD_VERSION(4, 3)},
        {ARB_COPY_IMAGE,                   MAKEDWORD_VERSION(4, 3)},
        {ARB_DEBUG_OUTPUT,                 MAKEDWORD_VERSION(4, 3)},
        {ARB_ES3_COMPATIBILITY,            MAKEDWORD_VERSION(4, 3)},
        {ARB_FRAGMENT_LAYER_VIEWPORT,      MAKEDWORD_VERSION(4, 3)},
        {ARB_FRAMEBUFFER_NO_ATTACHMENTS,   MAKEDWORD_VERSION(4, 3)},
        {ARB_INTERNALFORMAT_QUERY2,        MAKEDWORD_VERSION(4, 3)},
        {ARB_SHADER_IMAGE_SIZE,            MAKEDWORD_VERSION(4, 3)},
        {ARB_SHADER_STORAGE_BUFFER_OBJECT, MAKEDWORD_VERSION(4, 3)},
        {ARB_STENCIL_TEXTURING,            MAKEDWORD_VERSION(4, 3)},
        {ARB_TEXTURE_BUFFER_RANGE,         MAKEDWORD_VERSION(4, 3)},
        {ARB_TEXTURE_QUERY_LEVELS,         MAKEDWORD_VERSION(4, 3)},
        {ARB_TEXTURE_STORAGE_MULTISAMPLE,  MAKEDWORD_VERSION(4, 2)},
        {ARB_TEXTURE_VIEW,                 MAKEDWORD_VERSION(4, 3)},

        {ARB_BUFFER_STORAGE,               MAKEDWORD_VERSION(4, 4)},
        {ARB_CLEAR_TEXTURE,                MAKEDWORD_VERSION(4, 4)},
        {ARB_QUERY_BUFFER_OBJECT,          MAKEDWORD_VERSION(4, 4)},
        {ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE, MAKEDWORD_VERSION(4, 4)},

        {ARB_CLIP_CONTROL,                 MAKEDWORD_VERSION(4, 5)},
        {ARB_CULL_DISTANCE,                MAKEDWORD_VERSION(4, 5)},
        {ARB_DERIVATIVE_CONTROL,           MAKEDWORD_VERSION(4, 5)},
        {ARB_SHADER_TEXTURE_IMAGE_SAMPLES, MAKEDWORD_VERSION(4, 5)},

        {ARB_PIPELINE_STATISTICS_QUERY,    MAKEDWORD_VERSION(4, 6)},
        {ARB_POLYGON_OFFSET_CLAMP,         MAKEDWORD_VERSION(4, 6)},
        {ARB_TEXTURE_FILTER_ANISOTROPIC,   MAKEDWORD_VERSION(4, 6)},
    };
    struct wined3d_driver_info *driver_info = &adapter->driver_info;
    const char *gl_vendor_str, *gl_renderer_str, *gl_version_str;
    const struct wined3d_gpu_description *gpu_description;
    struct wined3d_gl_info *gl_info = &adapter->gl_info;
    const char *WGL_Extensions = NULL;
    enum wined3d_gl_vendor gl_vendor;
    DWORD gl_version, gl_ext_emul_mask;
    GLint context_profile = 0;
    UINT64 vram_bytes = 0;
    unsigned int i, j;
    HDC hdc;

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
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &context_profile);
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
        parse_extension_string(gl_info, gl_extensions, gl_extension_map, ARRAY_SIZE(gl_extension_map));
    }
    else
    {
        enumerate_gl_extensions(gl_info, gl_extension_map, ARRAY_SIZE(gl_extension_map));
    }

    hdc = wglGetCurrentDC();
    /* Not all GL drivers might offer WGL extensions e.g. VirtualBox. */
    if (GL_EXTCALL(wglGetExtensionsStringARB))
        WGL_Extensions = (const char *)GL_EXTCALL(wglGetExtensionsStringARB(hdc));
    if (!WGL_Extensions)
        WARN("WGL extensions not supported.\n");
    else
        parse_extension_string(gl_info, WGL_Extensions, wgl_extension_map, ARRAY_SIZE(wgl_extension_map));

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
    {
        gl_info->supported[WINED3D_GL_VERSION_2_0] = TRUE;
        /* We want to use the core APIs for two-sided stencil in GL 2.0. */
        gl_info->supported[EXT_STENCIL_TWO_SIDE] = FALSE;
    }
    if (gl_version >= MAKEDWORD_VERSION(3, 2))
        gl_info->supported[WINED3D_GL_VERSION_3_2] = TRUE;

    /* All the points are actually point sprites in core contexts, the APIs from
     * ARB_point_sprite are not supported anymore. */
    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        gl_info->supported[ARB_POINT_SPRITE] = FALSE;

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
        TRACE("EXT_texture_sRGB_decode is not supported, disabling ARB_framebuffer_sRGB.\n");
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
    if (gl_version >= MAKEDWORD_VERSION(3, 0))
    {
        GLint counter_bits;

        gl_info->supported[WINED3D_GL_PRIMITIVE_QUERY] = TRUE;

        GL_EXTCALL(glGetQueryiv(GL_PRIMITIVES_GENERATED, GL_QUERY_COUNTER_BITS, &counter_bits));
        TRACE("Primitives query counter has %d bits.\n", counter_bits);
        if (!counter_bits)
            gl_info->supported[WINED3D_GL_PRIMITIVE_QUERY] = FALSE;

        GL_EXTCALL(glGetQueryiv(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, GL_QUERY_COUNTER_BITS, &counter_bits));
        TRACE("Transform feedback primitives query counter has %d bits.\n", counter_bits);
        if (!counter_bits)
            gl_info->supported[WINED3D_GL_PRIMITIVE_QUERY] = FALSE;
    }
    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        GLint subpixel_bits;

        gl_info->gl_ops.gl.p_glGetIntegerv(GL_VIEWPORT_SUBPIXEL_BITS, &subpixel_bits);
        TRACE("Viewport supports %d subpixel bits.\n", subpixel_bits);
        if (subpixel_bits < 8 && gl_info->supported[ARB_CLIP_CONTROL])
        {
            TRACE("Disabling ARB_clip_control because viewport subpixel bits < 8.\n");
            gl_info->supported[ARB_CLIP_CONTROL] = FALSE;
        }
    }
    if (gl_info->supported[ARB_CLIP_CONTROL] && !gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        /* When using ARB_clip_control we need the float viewport parameters
         * introduced by ARB_viewport_array to take care of the shifted pixel
         * coordinates. */
        TRACE("Disabling ARB_clip_control because ARB_viewport_array is not supported.\n");
        gl_info->supported[ARB_CLIP_CONTROL] = FALSE;
    }
    if (gl_info->supported[ARB_STENCIL_TEXTURING] && !gl_info->supported[ARB_TEXTURE_SWIZZLE])
    {
        /* The stencil value needs to be placed in the green channel.  */
        TRACE("Disabling ARB_stencil_texturing because ARB_texture_swizzle is not supported.\n");
        gl_info->supported[ARB_STENCIL_TEXTURING] = FALSE;
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
    if (gl_info->supported[ARB_TEXTURE_STORAGE] && gl_info->supported[APPLE_YCBCR_422])
    {
        /* AFAIK APPLE_ycbcr_422 is only available in legacy contexts so we shouldn't ever hit this. */
        ERR("Disabling APPLE_ycbcr_422 because of ARB_texture_storage.\n");
        gl_info->supported[APPLE_YCBCR_422] = FALSE;
    }
    if (gl_info->supported[ARB_DRAW_INDIRECT] && !gl_info->supported[ARB_BASE_INSTANCE])
    {
        /* If ARB_base_instance is not supported the baseInstance field
         * in indirect draw parameters must be 0 or behavior is undefined.
         */
        WARN("Disabling ARB_draw_indirect because ARB_base_instance is not supported.\n");
        gl_info->supported[ARB_DRAW_INDIRECT] = FALSE;
    }
    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE] && !wined3d_settings.multisample_textures)
        gl_info->supported[ARB_TEXTURE_MULTISAMPLE] = FALSE;
    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE] && !gl_info->supported[ARB_TEXTURE_STORAGE_MULTISAMPLE])
    {
        WARN("Disabling ARB_texture_multisample because immutable storage is not supported.\n");
        gl_info->supported[ARB_TEXTURE_MULTISAMPLE] = FALSE;
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
        if (gl_info->glsl_version >= MAKEDWORD_VERSION(1, 30))
            gl_info->supported[WINED3D_GLSL_130] = TRUE;
    }

    checkGLcall("extension detection");

    adapter->shader_backend = select_shader_backend(gl_info);
    adapter->vertex_pipe = select_vertex_implementation(gl_info, adapter->shader_backend);
    adapter->fragment_pipe = select_fragment_implementation(gl_info, adapter->shader_backend);

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
        gl_info->fbo_ops.glFramebufferTextureLayer = gl_info->gl_ops.ext.p_glFramebufferTextureLayer;
        gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->gl_ops.ext.p_glFramebufferRenderbuffer;
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv
                = gl_info->gl_ops.ext.p_glGetFramebufferAttachmentParameteriv;
        gl_info->fbo_ops.glBlitFramebuffer = gl_info->gl_ops.ext.p_glBlitFramebuffer;
        gl_info->fbo_ops.glGenerateMipmap = gl_info->gl_ops.ext.p_glGenerateMipmap;
        gl_info->fbo_ops.glFramebufferTexture = gl_info->gl_ops.ext.p_glFramebufferTexture;
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

        if (gl_info->supported[ARB_GEOMETRY_SHADER4])
        {
            gl_info->fbo_ops.glFramebufferTexture = gl_info->gl_ops.ext.p_glFramebufferTextureARB;
            gl_info->fbo_ops.glFramebufferTextureLayer = gl_info->gl_ops.ext.p_glFramebufferTextureLayerARB;
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

    gl_info->wrap_lookup[WINED3D_TADDRESS_WRAP - WINED3D_TADDRESS_WRAP] = GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_CLAMP - WINED3D_TADDRESS_WRAP] = GL_CLAMP_TO_EDGE;
    gl_info->wrap_lookup[WINED3D_TADDRESS_BORDER - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE] ? GL_MIRROR_CLAMP_TO_EDGE : GL_REPEAT;

    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        GLuint vao;

        GL_EXTCALL(glGenVertexArrays(1, &vao));
        GL_EXTCALL(glBindVertexArray(vao));
        checkGLcall("creating VAO");
    }

    gl_vendor = wined3d_guess_gl_vendor(gl_info, gl_vendor_str, gl_renderer_str, gl_version_str);
    TRACE("Guessed GL vendor %#x.\n", gl_vendor);

    if (!(gpu_description = query_gpu_description(gl_info, &vram_bytes)))
    {
        enum wined3d_feature_level feature_level;
        struct fragment_caps fragment_caps;
        enum wined3d_pci_vendor vendor;
        enum wined3d_pci_device device;
        struct shader_caps shader_caps;

        adapter->shader_backend->shader_get_caps(adapter, &shader_caps);
        adapter->fragment_pipe->get_caps(adapter, &fragment_caps);
        feature_level = feature_level_from_caps(gl_info, &shader_caps, &fragment_caps);

        vendor = wined3d_guess_card_vendor(gl_vendor_str, gl_renderer_str);
        TRACE("Guessed vendor PCI ID 0x%04x.\n", vendor);

        device = wined3d_guess_card(feature_level, gl_renderer_str, &gl_vendor, &vendor);
        TRACE("Guessed device PCI ID 0x%04x.\n", device);

        if (!(gpu_description = wined3d_get_gpu_description(vendor, device)))
        {
            ERR("Card %04x:%04x not found in driver DB.\n", vendor, device);
            return FALSE;
        }
    }
    fixup_extensions(gl_info, caps_gl_ctx, gl_renderer_str, gl_vendor,
            gpu_description->vendor, gpu_description->device);
    wined3d_driver_info_init(driver_info, gpu_description, vram_bytes, 0);
    TRACE("Reporting (fake) driver version 0x%08x-0x%08x.\n",
            driver_info->version_high, driver_info->version_low);

    adapter->vram_bytes_used = 0;
    TRACE("Emulating 0x%s bytes of video ram.\n", wine_dbgstr_longlong(driver_info->vram_bytes));

    if (gl_info->supported[EXT_MEMORY_OBJECT])
    {
        GLint device_count = 0;

        gl_info->gl_ops.gl.p_glGetIntegerv(GL_NUM_DEVICE_UUIDS_EXT, &device_count);
        if (device_count > 0)
        {
            if (device_count > 1)
                FIXME("A set of %d devices is not supported.\n", device_count);

            GL_EXTCALL(glGetUnsignedBytevEXT(GL_DRIVER_UUID_EXT, (GLubyte *)&adapter->driver_uuid));
            GL_EXTCALL(glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, 0, (GLubyte *)&adapter->device_uuid));

            TRACE("Driver UUID: %s, device UUID %s.\n",
                    debugstr_guid(&adapter->driver_uuid), debugstr_guid(&adapter->device_uuid));
        }
        else
        {
            WARN("Unexpected device count %d.\n", device_count);
        }
    }

    gl_ext_emul_mask = adapter->vertex_pipe->vp_get_emul_mask(gl_info)
            | adapter->fragment_pipe->get_emul_mask(gl_info);
    if (gl_ext_emul_mask & GL_EXT_EMUL_ARB_MULTITEXTURE)
        install_gl_compat_wrapper(gl_info, ARB_MULTITEXTURE);
    if (gl_ext_emul_mask & GL_EXT_EMUL_EXT_FOG_COORD)
        install_gl_compat_wrapper(gl_info, EXT_FOG_COORD);

    return TRUE;
}

static void WINE_GLAPI invalid_func(const void *data)
{
    ERR("Invalid vertex attribute function called.\n");
    DebugBreak();
}

static void WINE_GLAPI invalid_texcoord_func(GLenum unit, const void *data)
{
    ERR("Invalid texcoord function called.\n");
    DebugBreak();
}

static void WINE_GLAPI invalid_generic_attrib_func(GLuint idx, const void *data)
{
    ERR("Invalid attribute function called.\n");
    DebugBreak();
}

/* Helper functions for providing vertex data to OpenGL. The arrays are
 * initialised based on the extension detection and are used in
 * draw_primitive_immediate_mode(). */
static void WINE_GLAPI position_d3dcolor(const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    DWORD pos = *((const DWORD *)data);

    FIXME("Add a test for fixed function position from d3dcolor type.\n");
    gl_info->gl_ops.gl.p_glVertex4s(D3DCOLOR_B_R(pos),
            D3DCOLOR_B_G(pos),
            D3DCOLOR_B_B(pos),
            D3DCOLOR_B_A(pos));
}

static void WINE_GLAPI position_float4(const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    const GLfloat *pos = data;

    if (pos[3] != 0.0f && pos[3] != 1.0f)
    {
        float w = 1.0f / pos[3];

        gl_info->gl_ops.gl.p_glVertex4f(pos[0] * w, pos[1] * w, pos[2] * w, w);
    }
    else
    {
        gl_info->gl_ops.gl.p_glVertex3fv(pos);
    }
}

static void WINE_GLAPI diffuse_d3dcolor(const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    DWORD diffuseColor = *((const DWORD *)data);

    gl_info->gl_ops.gl.p_glColor4ub(D3DCOLOR_B_R(diffuseColor),
            D3DCOLOR_B_G(diffuseColor),
            D3DCOLOR_B_B(diffuseColor),
            D3DCOLOR_B_A(diffuseColor));
}

static void WINE_GLAPI specular_d3dcolor(const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    DWORD specularColor = *((const DWORD *)data);
    GLubyte d[] =
    {
        D3DCOLOR_B_R(specularColor),
        D3DCOLOR_B_G(specularColor),
        D3DCOLOR_B_B(specularColor)
    };

    gl_info->gl_ops.ext.p_glSecondaryColor3ubvEXT(d);
}

static void WINE_GLAPI warn_no_specular_func(const void *data)
{
    WARN("GL_EXT_secondary_color not supported.\n");
}

static void WINE_GLAPI generic_d3dcolor(GLuint idx, const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    DWORD color = *((const DWORD *)data);

    gl_info->gl_ops.ext.p_glVertexAttrib4Nub(idx,
            D3DCOLOR_B_R(color), D3DCOLOR_B_G(color),
            D3DCOLOR_B_B(color), D3DCOLOR_B_A(color));
}

static void WINE_GLAPI generic_short2n(GLuint idx, const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    const GLshort s[] = {((const GLshort *)data)[0], ((const GLshort *)data)[1], 0, 1};

    gl_info->gl_ops.ext.p_glVertexAttrib4Nsv(idx, s);
}

static void WINE_GLAPI generic_ushort2n(GLuint idx, const void *data)
{
    const GLushort s[] = {((const GLushort *)data)[0], ((const GLushort *)data)[1], 0, 1};
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;

    gl_info->gl_ops.ext.p_glVertexAttrib4Nusv(idx, s);
}

static void WINE_GLAPI generic_float16_2(GLuint idx, const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    float x = float_16_to_32(((const unsigned short *)data) + 0);
    float y = float_16_to_32(((const unsigned short *)data) + 1);

    gl_info->gl_ops.ext.p_glVertexAttrib2f(idx, x, y);
}

static void WINE_GLAPI generic_float16_4(GLuint idx, const void *data)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_get_current()->gl_info;
    float x = float_16_to_32(((const unsigned short *)data) + 0);
    float y = float_16_to_32(((const unsigned short *)data) + 1);
    float z = float_16_to_32(((const unsigned short *)data) + 2);
    float w = float_16_to_32(((const unsigned short *)data) + 3);

    gl_info->gl_ops.ext.p_glVertexAttrib4f(idx, x, y, z, w);
}

static void wined3d_adapter_init_ffp_attrib_ops(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    struct wined3d_ffp_attrib_ops *ops = &d3d_info->ffp_attrib_ops;
    unsigned int i;

    for (i = 0; i < WINED3D_FFP_EMIT_COUNT; ++i)
    {
        ops->position[i] = invalid_func;
        ops->diffuse[i]  = invalid_func;
        ops->specular[i] = invalid_func;
        ops->normal[i]   = invalid_func;
        ops->texcoord[i] = invalid_texcoord_func;
        ops->generic[i]  = invalid_generic_attrib_func;
    }

    ops->position[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex3fv;
    if (!d3d_info->xyzrhw)
        ops->position[WINED3D_FFP_EMIT_FLOAT4]    = position_float4;
    else
        ops->position[WINED3D_FFP_EMIT_FLOAT4]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex4fv;
    ops->position[WINED3D_FFP_EMIT_D3DCOLOR]  = position_d3dcolor;
    ops->position[WINED3D_FFP_EMIT_SHORT4]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glVertex2sv;

    ops->diffuse[WINED3D_FFP_EMIT_FLOAT3]     = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor3fv;
    ops->diffuse[WINED3D_FFP_EMIT_FLOAT4]     = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4fv;
    ops->diffuse[WINED3D_FFP_EMIT_D3DCOLOR]   = diffuse_d3dcolor;
    ops->diffuse[WINED3D_FFP_EMIT_UBYTE4N]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4ubv;
    ops->diffuse[WINED3D_FFP_EMIT_SHORT4N]    = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4sv;
    ops->diffuse[WINED3D_FFP_EMIT_USHORT4N]   = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glColor4usv;

    /* No 4 component entry points here. */
    if (gl_info->supported[EXT_SECONDARY_COLOR])
        ops->specular[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_attrib_func)GL_EXTCALL(glSecondaryColor3fvEXT);
    else
        ops->specular[WINED3D_FFP_EMIT_FLOAT3]    = warn_no_specular_func;
    if (gl_info->supported[EXT_SECONDARY_COLOR])
        ops->specular[WINED3D_FFP_EMIT_D3DCOLOR]  = specular_d3dcolor;
    else
        ops->specular[WINED3D_FFP_EMIT_D3DCOLOR]  = warn_no_specular_func;

    /* Only 3 component entry points here. Test how others behave. Float4
     * normals are used by one of our tests, trying to pass it to the pixel
     * shader, which fails on Windows. */
    ops->normal[WINED3D_FFP_EMIT_FLOAT3]      = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glNormal3fv;
    /* Just ignore the 4th value. */
    ops->normal[WINED3D_FFP_EMIT_FLOAT4]      = (wined3d_ffp_attrib_func)gl_info->gl_ops.gl.p_glNormal3fv;

    ops->texcoord[WINED3D_FFP_EMIT_FLOAT1]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord1fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT2]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT3]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord3fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_FLOAT4]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4fvARB;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT2]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2svARB;
    ops->texcoord[WINED3D_FFP_EMIT_SHORT4]    = (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4svARB;
    if (gl_info->supported[NV_HALF_FLOAT])
    {
        /* Not supported by ARB_HALF_FLOAT_VERTEX, so check for NV_HALF_FLOAT. */
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_2] =
                (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord2hvNV;
        ops->texcoord[WINED3D_FFP_EMIT_FLOAT16_4] =
                (wined3d_ffp_texcoord_func)gl_info->gl_ops.ext.p_glMultiTexCoord4hvNV;
    }

    ops->generic[WINED3D_FFP_EMIT_FLOAT1]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib1fv;
    ops->generic[WINED3D_FFP_EMIT_FLOAT2]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib2fv;
    ops->generic[WINED3D_FFP_EMIT_FLOAT3]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib3fv;
    ops->generic[WINED3D_FFP_EMIT_FLOAT4]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4fv;
    if (gl_info->supported[ARB_VERTEX_ARRAY_BGRA])
        ops->generic[WINED3D_FFP_EMIT_D3DCOLOR] = generic_d3dcolor;
    else
        ops->generic[WINED3D_FFP_EMIT_D3DCOLOR] =
                (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4Nubv;
    ops->generic[WINED3D_FFP_EMIT_UBYTE4]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4ubv;
    ops->generic[WINED3D_FFP_EMIT_SHORT2]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib2sv;
    ops->generic[WINED3D_FFP_EMIT_SHORT4]     = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4sv;
    ops->generic[WINED3D_FFP_EMIT_UBYTE4N]    = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4Nubv;
    ops->generic[WINED3D_FFP_EMIT_SHORT2N]    = generic_short2n;
    ops->generic[WINED3D_FFP_EMIT_SHORT4N]    = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4Nsv;
    ops->generic[WINED3D_FFP_EMIT_USHORT2N]   = generic_ushort2n;
    ops->generic[WINED3D_FFP_EMIT_USHORT4N]   = (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4Nusv;
    if (gl_info->supported[NV_HALF_FLOAT] && gl_info->supported[NV_VERTEX_PROGRAM])
    {
        ops->generic[WINED3D_FFP_EMIT_FLOAT16_2] =
                (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib2hvNV;
        ops->generic[WINED3D_FFP_EMIT_FLOAT16_4] =
                (wined3d_generic_attrib_func)gl_info->gl_ops.ext.p_glVertexAttrib4hvNV;
    }
    else
    {
        ops->generic[WINED3D_FFP_EMIT_FLOAT16_2] = generic_float16_2;
        ops->generic[WINED3D_FFP_EMIT_FLOAT16_4] = generic_float16_4;
    }
}

static void wined3d_adapter_init_fb_cfgs(struct wined3d_adapter_gl *adapter_gl, HDC dc)
{
    const struct wined3d_gl_info *gl_info = &adapter_gl->a.gl_info;
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

        adapter_gl->pixel_formats = heap_calloc(cfg_count, sizeof(*adapter_gl->pixel_formats));
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

        for (i = 0, adapter_gl->pixel_format_count = 0; i < cfg_count; ++i)
        {
            struct wined3d_pixel_format *cfg = &adapter_gl->pixel_formats[adapter_gl->pixel_format_count];
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

            ++adapter_gl->pixel_format_count;
        }
    }
    else
    {
        int cfg_count;

        cfg_count = DescribePixelFormat(dc, 0, 0, 0);
        adapter_gl->pixel_formats = heap_calloc(cfg_count, sizeof(*adapter_gl->pixel_formats));

        for (i = 0, adapter_gl->pixel_format_count = 0; i < cfg_count; ++i)
        {
            struct wined3d_pixel_format *cfg = &adapter_gl->pixel_formats[adapter_gl->pixel_format_count];
            PIXELFORMATDESCRIPTOR pfd;
            int format_id = i + 1;

            if (!DescribePixelFormat(dc, format_id, sizeof(pfd), &pfd))
                continue;

            /* We only want HW acceleration using an OpenGL ICD driver.
             * PFD_GENERIC_FORMAT = slow OpenGL 1.1 GDI software rendering.
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

            ++adapter_gl->pixel_format_count;
        }
    }
}

static void adapter_gl_destroy(struct wined3d_adapter *adapter)
{
    struct wined3d_adapter_gl *adapter_gl = wined3d_adapter_gl(adapter);

    heap_free(adapter_gl->pixel_formats);
    wined3d_adapter_cleanup(adapter);
    heap_free(adapter_gl);
}

static HRESULT adapter_gl_create_device(struct wined3d *wined3d, const struct wined3d_adapter *adapter,
        enum wined3d_device_type device_type, HWND focus_window, unsigned int flags, BYTE surface_alignment,
        const enum wined3d_feature_level *levels, unsigned int level_count,
        struct wined3d_device_parent *device_parent, struct wined3d_device **device)
{
    struct wined3d_device_gl *device_gl;
    HRESULT hr;

    if (!(device_gl = heap_alloc_zero(sizeof(*device_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_device_init(&device_gl->d, wined3d, adapter->ordinal, device_type,
            focus_window, flags, surface_alignment, levels, level_count, device_parent)))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        heap_free(device_gl);
        return hr;
    }

    *device = &device_gl->d;
    return WINED3D_OK;
}

static void adapter_gl_destroy_device(struct wined3d_device *device)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(device);

    wined3d_device_cleanup(&device_gl->d);
    heap_free(device_gl);
}

struct wined3d_context *adapter_gl_acquire_context(struct wined3d_device *device,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    return wined3d_context_gl_acquire(device, texture, sub_resource_idx);
}

void adapter_gl_release_context(struct wined3d_context *context)
{
#if __REACTOS__
    wined3d_context_gl_release(wined3d_context_gl(context));
#else
    return wined3d_context_gl_release(wined3d_context_gl(context));
#endif
}

static void adapter_gl_get_wined3d_caps(const struct wined3d_adapter *adapter, struct wined3d_caps *caps)
{
    const struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->ddraw_caps.dds_caps |= WINEDDSCAPS_BACKBUFFER
            | WINEDDSCAPS_FLIP
            | WINEDDSCAPS_COMPLEX
            | WINEDDSCAPS_FRONTBUFFER
            | WINEDDSCAPS_3DDEVICE
            | WINEDDSCAPS_VIDEOMEMORY
            | WINEDDSCAPS_OWNDC
            | WINEDDSCAPS_LOCALVIDMEM
            | WINEDDSCAPS_NONLOCALVIDMEM;

    caps->ddraw_caps.caps |= WINEDDCAPS_3D;

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT] || gl_info->supported[EXT_FRAMEBUFFER_OBJECT])
        caps->Caps2 |= WINED3DCAPS2_CANGENMIPMAP;

    if (gl_info->supported[WINED3D_GL_BLEND_EQUATION])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_BLENDOP;
    if (gl_info->supported[EXT_BLEND_EQUATION_SEPARATE] && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_SEPARATEALPHABLEND;
    if (gl_info->supported[EXT_DRAW_BUFFERS2])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS;
    if (gl_info->supported[ARB_FRAMEBUFFER_SRGB])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_POSTBLENDSRGBCONVERT;
    if (~gl_info->quirks & WINED3D_QUIRK_NO_INDEPENDENT_BIT_DEPTHS)
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS;

    if (gl_info->supported[ARB_SAMPLER_OBJECTS] || gl_info->supported[EXT_TEXTURE_LOD_BIAS])
        caps->RasterCaps |= WINED3DPRASTERCAPS_MIPMAPLODBIAS;

    if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
    {
        caps->RasterCaps |= WINED3DPRASTERCAPS_ANISOTROPY;

        caps->TextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC
                | WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (gl_info->supported[ARB_BLEND_FUNC_EXTENDED])
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_SRCALPHASAT;

    if (gl_info->supported[EXT_BLEND_COLOR])
    {
        caps->SrcBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->TextureCaps |= WINED3DPTEXTURECAPS_VOLUMEMAP
                | WINED3DPTEXTURECAPS_MIPVOLUMEMAP;
        if (!d3d_info->texture_npot)
            caps->TextureCaps |= WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;

        caps->VolumeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFLINEAR
                | WINED3DPTFILTERCAPS_MAGFPOINT
                | WINED3DPTFILTERCAPS_MINFLINEAR
                | WINED3DPTFILTERCAPS_MINFPOINT
                | WINED3DPTFILTERCAPS_MIPFLINEAR
                | WINED3DPTFILTERCAPS_MIPFPOINT
                | WINED3DPTFILTERCAPS_LINEAR
                | WINED3DPTFILTERCAPS_LINEARMIPLINEAR
                | WINED3DPTFILTERCAPS_LINEARMIPNEAREST
                | WINED3DPTFILTERCAPS_MIPLINEAR
                | WINED3DPTFILTERCAPS_MIPNEAREST
                | WINED3DPTFILTERCAPS_NEAREST;

        caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_INDEPENDENTUV
                | WINED3DPTADDRESSCAPS_CLAMP
                | WINED3DPTADDRESSCAPS_WRAP;

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

        caps->MaxVolumeExtent = gl_info->limits.texture3d_size;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        caps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP
                | WINED3DPTEXTURECAPS_MIPCUBEMAP;
        if (!d3d_info->texture_npot)
            caps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP_POW2;

        caps->CubeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFLINEAR
                | WINED3DPTFILTERCAPS_MAGFPOINT
                | WINED3DPTFILTERCAPS_MINFLINEAR
                | WINED3DPTFILTERCAPS_MINFPOINT
                | WINED3DPTFILTERCAPS_MIPFLINEAR
                | WINED3DPTFILTERCAPS_MIPFPOINT
                | WINED3DPTFILTERCAPS_LINEAR
                | WINED3DPTFILTERCAPS_LINEARMIPLINEAR
                | WINED3DPTFILTERCAPS_LINEARMIPNEAREST
                | WINED3DPTFILTERCAPS_MIPLINEAR
                | WINED3DPTFILTERCAPS_MIPNEAREST
                | WINED3DPTFILTERCAPS_NEAREST;

        if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
        {
            caps->CubeTextureFilterCaps |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC
                    | WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    }

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

    if (gl_info->supported[EXT_STENCIL_WRAP])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_DECR
                | WINED3DSTENCILCAPS_INCR;
    }

    if (gl_info->supported[WINED3D_GL_VERSION_2_0]
            || gl_info->supported[EXT_STENCIL_TWO_SIDE]
            || gl_info->supported[ATI_SEPARATE_STENCIL])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    caps->MaxAnisotropy = gl_info->limits.anisotropy;

    if (caps->VertexShaderVersion >= 3)
    {
        caps->MaxVertexShader30InstructionSlots
                = max(caps->MaxVertexShader30InstructionSlots, gl_info->limits.arb_vs_instructions);
    }
    if (caps->VertexShaderVersion >= 2)
    {
        caps->VS20Caps.temp_count = max(caps->VS20Caps.temp_count, gl_info->limits.arb_vs_temps);

        if (gl_info->supported[ARB_HALF_FLOAT_VERTEX])
            caps->DeclTypes |= WINED3DDTCAPS_FLOAT16_2 | WINED3DDTCAPS_FLOAT16_4;
    }

    if (caps->PixelShaderVersion >= 3)
    {
        caps->MaxPixelShader30InstructionSlots
                = max(caps->MaxPixelShader30InstructionSlots, gl_info->limits.arb_ps_instructions);
    }
    if (caps->PixelShaderVersion >= 2)
    {
        caps->PS20Caps.temp_count = max(caps->PS20Caps.temp_count, gl_info->limits.arb_ps_temps);
    }
}

static BOOL wined3d_check_pixel_format_color(const struct wined3d_pixel_format *cfg,
        const struct wined3d_format *format)
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

static BOOL wined3d_check_pixel_format_depth(const struct wined3d_pixel_format *cfg,
        const struct wined3d_format *format)
{
    BOOL lockable = FALSE;

    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
        return FALSE;

    if ((format->id == WINED3DFMT_D16_LOCKABLE) || (format->id == WINED3DFMT_D32_FLOAT))
        lockable = TRUE;

    /* On some modern cards like the Geforce8/9, GLX doesn't offer some
     * depth/stencil formats which D3D9 reports. We can safely report
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

static BOOL adapter_gl_check_format(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format, const struct wined3d_format *rt_format,
        const struct wined3d_format *ds_format)
{
    const struct wined3d_adapter_gl *adapter_gl = wined3d_adapter_gl_const(adapter);
    unsigned int i;

    if (wined3d_settings.offscreen_rendering_mode != ORM_BACKBUFFER)
        return TRUE;

    if (adapter_format && rt_format)
    {
        /* In backbuffer mode the front and backbuffer share the same WGL
         * pixelformat. The format must match in RGB, alpha is allowed to be
         * different. (Only the backbuffer can have alpha.) */
        if (adapter_format->red_size != rt_format->red_size
                || adapter_format->green_size != rt_format->green_size
                || adapter_format->blue_size != rt_format->blue_size)
        {
            TRACE("Render target format %s doesn't match with adapter format %s.\n",
                    debug_d3dformat(rt_format->id), debug_d3dformat(adapter_format->id));
            return FALSE;
        }
    }

    for (i = 0; i < adapter_gl->pixel_format_count; ++i)
    {
        const struct wined3d_pixel_format *cfg = &adapter_gl->pixel_formats[i];

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        if (adapter_format && rt_format && !cfg->windowDrawable)
            continue;

        if ((!adapter_format || wined3d_check_pixel_format_color(cfg, adapter_format))
                && (!rt_format || wined3d_check_pixel_format_color(cfg, rt_format))
                && (!ds_format || wined3d_check_pixel_format_depth(cfg, ds_format)))
        {
            TRACE("Pixel format %d is compatible.\n", cfg->iPixelFormat);
            return TRUE;
        }
    }

    return FALSE;
}

static HRESULT adapter_gl_init_3d(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    wined3d_cs_init_object(device->cs, wined3d_device_create_primary_opengl_context_cs, device);
    wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    if (!wined3d_swapchain_gl(device->swapchains[0])->context_count)
        return E_FAIL;

    device->d3d_initialized = TRUE;

    return WINED3D_OK;
}

static void adapter_gl_uninit_3d(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    wined3d_cs_destroy_object(device->cs, wined3d_device_delete_opengl_contexts_cs, device);
    wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void *adapter_gl_map_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *data, size_t size, uint32_t bind_flags, uint32_t map_flags)
{
    struct wined3d_context_gl *context_gl;
    GLenum binding;

    context_gl = wined3d_context_gl(context);
    binding = wined3d_buffer_gl_binding_from_bind_flags(context_gl->gl_info, bind_flags);

    return wined3d_context_gl_map_bo_address(context_gl, data, size, binding, map_flags);
}

static void adapter_gl_unmap_bo_address(struct wined3d_context *context, const struct wined3d_bo_address *data,
        uint32_t bind_flags, unsigned int range_count, const struct wined3d_map_range *ranges)
{
    struct wined3d_context_gl *context_gl;
    GLenum binding;

    context_gl = wined3d_context_gl(context);
    binding = wined3d_buffer_gl_binding_from_bind_flags(context_gl->gl_info, bind_flags);

    wined3d_context_gl_unmap_bo_address(context_gl, data, binding, range_count, ranges);
}

static void adapter_gl_copy_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *dst, uint32_t dst_bind_flags,
        const struct wined3d_bo_address *src, uint32_t src_bind_flags, size_t size)
{
    struct wined3d_context_gl *context_gl;
    GLenum dst_binding, src_binding;

    context_gl = wined3d_context_gl(context);
    dst_binding = wined3d_buffer_gl_binding_from_bind_flags(context_gl->gl_info, dst_bind_flags);
    src_binding = wined3d_buffer_gl_binding_from_bind_flags(context_gl->gl_info, src_bind_flags);

    wined3d_context_gl_copy_bo_address(context_gl, dst, dst_binding, src, src_binding, size);
}

static HRESULT adapter_gl_create_swapchain(struct wined3d_device *device, struct wined3d_swapchain_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_swapchain **swapchain)
{
    struct wined3d_swapchain_gl *swapchain_gl;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, swapchain %p.\n",
            device, desc, parent, parent_ops, swapchain);

    if (!(swapchain_gl = heap_alloc_zero(sizeof(*swapchain_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_swapchain_gl_init(swapchain_gl, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialise swapchain, hr %#x.\n", hr);
        heap_free(swapchain_gl);
        return hr;
    }

    TRACE("Created swapchain %p.\n", swapchain_gl);
    *swapchain = &swapchain_gl->s;

    return hr;
}

static void adapter_gl_destroy_swapchain(struct wined3d_swapchain *swapchain)
{
    struct wined3d_swapchain_gl *swapchain_gl = wined3d_swapchain_gl(swapchain);

    wined3d_swapchain_gl_cleanup(swapchain_gl);
    heap_free(swapchain_gl);
}

static HRESULT adapter_gl_create_buffer(struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_buffer **buffer)
{
    struct wined3d_buffer_gl *buffer_gl;
    HRESULT hr;

    TRACE("device %p, desc %p, data %p, parent %p, parent_ops %p, buffer %p.\n",
            device, desc, data, parent, parent_ops, buffer);

    if (!(buffer_gl = heap_alloc_zero(sizeof(*buffer_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_buffer_gl_init(buffer_gl, device, desc, data, parent, parent_ops)))
    {
        WARN("Failed to initialise buffer, hr %#x.\n", hr);
        heap_free(buffer_gl);
        return hr;
    }

    TRACE("Created buffer %p.\n", buffer_gl);
    *buffer = &buffer_gl->b;

    return hr;
}

static void wined3d_buffer_gl_destroy_object(void *object)
{
    struct wined3d_buffer_gl *buffer_gl = object;
    struct wined3d_context *context;

    if (buffer_gl->b.buffer_object)
    {
        context = context_acquire(buffer_gl->b.resource.device, NULL, 0);
        wined3d_buffer_gl_destroy_buffer_object(buffer_gl, wined3d_context_gl(context));
        context_release(context);
    }

    heap_free(buffer_gl);
}

static void adapter_gl_destroy_buffer(struct wined3d_buffer *buffer)
{
    struct wined3d_buffer_gl *buffer_gl = wined3d_buffer_gl(buffer);
    struct wined3d_device *device = buffer_gl->b.resource.device;
    unsigned int swapchain_count = device->swapchain_count;

    TRACE("buffer_gl %p.\n", buffer_gl);

    /* Take a reference to the device, in case releasing the buffer would
     * cause the device to be destroyed. However, swapchain resources don't
     * take a reference to the device, and we wouldn't want to increment the
     * refcount on a device that's in the process of being destroyed. */
    if (swapchain_count)
        wined3d_device_incref(device);
    wined3d_buffer_cleanup(&buffer_gl->b);
    wined3d_cs_destroy_object(device->cs, wined3d_buffer_gl_destroy_object, buffer_gl);
    if (swapchain_count)
        wined3d_device_decref(device);
}

static HRESULT adapter_gl_create_texture(struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture)
{
    struct wined3d_texture_gl *texture_gl;
    HRESULT hr;

    TRACE("device %p, desc %p, layer_count %u, level_count %u, flags %#x, parent %p, parent_ops %p, texture %p.\n",
            device, desc, layer_count, level_count, flags, parent, parent_ops, texture);

    if (!(texture_gl = wined3d_texture_allocate_object_memory(sizeof(*texture_gl), level_count, layer_count)))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_texture_gl_init(texture_gl, device, desc,
            layer_count, level_count, flags, parent, parent_ops)))
    {
        WARN("Failed to initialise texture, hr %#x.\n", hr);
        heap_free(texture_gl);
        return hr;
    }

    TRACE("Created texture %p.\n", texture_gl);
    *texture = &texture_gl->t;

    return hr;
}

static void wined3d_texture_gl_destroy_object(void *object)
{
    struct wined3d_renderbuffer_entry *entry, *entry2;
    struct wined3d_texture_gl *texture_gl = object;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_device *device;

    TRACE("texture_gl %p.\n", texture_gl);

    if (!list_empty(&texture_gl->renderbuffers))
    {
        device = texture_gl->t.resource.device;
        context = context_acquire(device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &texture_gl->renderbuffers, struct wined3d_renderbuffer_entry, entry)
        {
            TRACE("Deleting renderbuffer %u.\n", entry->id);
            context_gl_resource_released(device, entry->id, TRUE);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
            heap_free(entry);
        }

        context_release(context);
    }

    wined3d_texture_gl_unload_texture(texture_gl);

    heap_free(texture_gl);
}

static void adapter_gl_destroy_texture(struct wined3d_texture *texture)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_device *device = texture_gl->t.resource.device;
    unsigned int swapchain_count = device->swapchain_count;

    TRACE("texture_gl %p.\n", texture_gl);

    /* Take a reference to the device, in case releasing the texture would
     * cause the device to be destroyed. However, swapchain resources don't
     * take a reference to the device, and we wouldn't want to increment the
     * refcount on a device that's in the process of being destroyed. */
    if (swapchain_count)
        wined3d_device_incref(device);

    wined3d_texture_sub_resources_destroyed(texture);
    texture->resource.parent_ops->wined3d_object_destroyed(texture->resource.parent);

    wined3d_texture_cleanup(&texture_gl->t);
    wined3d_cs_destroy_object(device->cs, wined3d_texture_gl_destroy_object, texture_gl);

    if (swapchain_count)
        wined3d_device_decref(device);
}

static HRESULT adapter_gl_create_rendertarget_view(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_rendertarget_view_gl *view_gl;
    HRESULT hr;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    if (!(view_gl = heap_alloc_zero(sizeof(*view_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_rendertarget_view_gl_init(view_gl, desc, resource, parent, parent_ops)))
    {
        WARN("Failed to initialise view, hr %#x.\n", hr);
        heap_free(view_gl);
        return hr;
    }

    TRACE("Created render target view %p.\n", view_gl);
    *view = &view_gl->v;

    return hr;
}

struct wined3d_view_gl_destroy_ctx
{
    struct wined3d_device *device;
    const struct wined3d_gl_view *gl_view;
    GLuint counter_bo;
    void *object;
    struct wined3d_view_gl_destroy_ctx *free;
};

static void wined3d_view_gl_destroy_object(void *object)
{
    struct wined3d_view_gl_destroy_ctx *ctx = object;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_device *device;

    device = ctx->device;

    if (ctx->gl_view->name || ctx->counter_bo)
    {
        context = context_acquire(device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
        if (ctx->gl_view->name)
        {
            context_gl_resource_released(device, ctx->gl_view->name, FALSE);
            gl_info->gl_ops.gl.p_glDeleteTextures(1, &ctx->gl_view->name);
        }
        if (ctx->counter_bo)
            GL_EXTCALL(glDeleteBuffers(1, &ctx->counter_bo));
        checkGLcall("delete resources");
        context_release(context);
    }

    heap_free(ctx->object);
    heap_free(ctx->free);
}

static void wined3d_view_gl_destroy(struct wined3d_device *device,
        const struct wined3d_gl_view *gl_view, GLuint counter_bo, void *object)
{
    struct wined3d_view_gl_destroy_ctx *ctx, c;

    if (!(ctx = heap_alloc(sizeof(*ctx))))
        ctx = &c;
    ctx->device = device;
    ctx->gl_view = gl_view;
    ctx->counter_bo = counter_bo;
    ctx->object = object;
    ctx->free = ctx != &c ? ctx : NULL;

    wined3d_cs_destroy_object(device->cs, wined3d_view_gl_destroy_object, ctx);
    if (!ctx->free)
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void adapter_gl_destroy_rendertarget_view(struct wined3d_rendertarget_view *view)
{
    struct wined3d_rendertarget_view_gl *view_gl = wined3d_rendertarget_view_gl(view);
    struct wined3d_device *device = view_gl->v.resource->device;
    unsigned int swapchain_count = device->swapchain_count;

    TRACE("view_gl %p.\n", view_gl);

    /* Take a reference to the device, in case releasing the view's resource
     * would cause the device to be destroyed. However, swapchain resources
     * don't take a reference to the device, and we wouldn't want to increment
     * the refcount on a device that's in the process of being destroyed. */
    if (swapchain_count)
        wined3d_device_incref(device);
    wined3d_rendertarget_view_cleanup(&view_gl->v);
    wined3d_view_gl_destroy(device, &view_gl->gl_view, 0, view_gl);
    if (swapchain_count)
        wined3d_device_decref(device);
}

static HRESULT adapter_gl_create_shader_resource_view(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_shader_resource_view **view)
{
    struct wined3d_shader_resource_view_gl *view_gl;
    HRESULT hr;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    if (!(view_gl = heap_alloc_zero(sizeof(*view_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_shader_resource_view_gl_init(view_gl, desc, resource, parent, parent_ops)))
    {
        WARN("Failed to initialise view, hr %#x.\n", hr);
        heap_free(view_gl);
        return hr;
    }

    TRACE("Created shader resource view %p.\n", view_gl);
    *view = &view_gl->v;

    return hr;
}

static void adapter_gl_destroy_shader_resource_view(struct wined3d_shader_resource_view *view)
{
    struct wined3d_shader_resource_view_gl *view_gl = wined3d_shader_resource_view_gl(view);
    struct wined3d_device *device = view_gl->v.resource->device;
    unsigned int swapchain_count = device->swapchain_count;

    TRACE("view_gl %p.\n", view_gl);

    /* Take a reference to the device, in case releasing the view's resource
     * would cause the device to be destroyed. However, swapchain resources
     * don't take a reference to the device, and we wouldn't want to increment
     * the refcount on a device that's in the process of being destroyed. */
    if (swapchain_count)
        wined3d_device_incref(device);
    wined3d_shader_resource_view_cleanup(&view_gl->v);
    wined3d_view_gl_destroy(device, &view_gl->gl_view, 0, view_gl);
    if (swapchain_count)
        wined3d_device_decref(device);
}

static HRESULT adapter_gl_create_unordered_access_view(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_unordered_access_view **view)
{
    struct wined3d_unordered_access_view_gl *view_gl;
    HRESULT hr;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    if (!(view_gl = heap_alloc_zero(sizeof(*view_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_unordered_access_view_gl_init(view_gl, desc, resource, parent, parent_ops)))
    {
        WARN("Failed to initialise view, hr %#x.\n", hr);
        heap_free(view_gl);
        return hr;
    }

    TRACE("Created unordered access view %p.\n", view_gl);
    *view = &view_gl->v;

    return hr;
}

static void adapter_gl_destroy_unordered_access_view(struct wined3d_unordered_access_view *view)
{
    struct wined3d_unordered_access_view_gl *view_gl = wined3d_unordered_access_view_gl(view);
    struct wined3d_device *device = view_gl->v.resource->device;
    unsigned int swapchain_count = device->swapchain_count;

    TRACE("view_gl %p.\n", view_gl);

    /* Take a reference to the device, in case releasing the view's resource
     * would cause the device to be destroyed. However, swapchain resources
     * don't take a reference to the device, and we wouldn't want to increment
     * the refcount on a device that's in the process of being destroyed. */
    if (swapchain_count)
        wined3d_device_incref(device);
    wined3d_unordered_access_view_cleanup(&view_gl->v);
    wined3d_view_gl_destroy(device, &view_gl->gl_view, view_gl->counter_bo, view_gl);
    if (swapchain_count)
        wined3d_device_decref(device);
}

static HRESULT adapter_gl_create_sampler(struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_sampler **sampler)
{
    struct wined3d_sampler_gl *sampler_gl;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, sampler %p.\n",
            device, desc, parent, parent_ops, sampler);

    if (!(sampler_gl = heap_alloc_zero(sizeof(*sampler_gl))))
        return E_OUTOFMEMORY;

    wined3d_sampler_gl_init(sampler_gl, device, desc, parent, parent_ops);

    TRACE("Created sampler %p.\n", sampler_gl);
    *sampler = &sampler_gl->s;

    return WINED3D_OK;
}

static void wined3d_sampler_gl_destroy_object(void *object)
{
    struct wined3d_sampler_gl *sampler_gl = object;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    if (sampler_gl->name)
    {
        context = context_acquire(sampler_gl->s.device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
        GL_EXTCALL(glDeleteSamplers(1, &sampler_gl->name));
        context_release(context);
    }

    heap_free(sampler_gl);
}

static void adapter_gl_destroy_sampler(struct wined3d_sampler *sampler)
{
    struct wined3d_sampler_gl *sampler_gl = wined3d_sampler_gl(sampler);

    TRACE("sampler_gl %p.\n", sampler_gl);

    wined3d_cs_destroy_object(sampler->device->cs, wined3d_sampler_gl_destroy_object, sampler_gl);
}

static HRESULT adapter_gl_create_query(struct wined3d_device *device, enum wined3d_query_type type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query)
{
    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    return wined3d_query_gl_create(device, type, parent, parent_ops, query);
}

static void wined3d_query_gl_destroy_object(void *object)
{
    struct wined3d_query *query = object;

    if (query->buffer_object)
    {
        struct wined3d_context *context;

        context = context_acquire(query->device, NULL, 0);
        wined3d_query_gl_destroy_buffer_object(wined3d_context_gl(context), query);
        context_release(context);
    }

    /* Queries are specific to the GL context that created them. Not
     * deleting the query will obviously leak it, but that's still better
     * than potentially deleting a different query with the same id in this
     * context, and (still) leaking the actual query. */
    query->query_ops->query_destroy(query);
}

static void adapter_gl_destroy_query(struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    wined3d_cs_destroy_object(query->device->cs, wined3d_query_gl_destroy_object, query);
}

static void adapter_gl_flush_context(struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);

    TRACE("context_gl %p.\n", context_gl);

    if (context_gl->valid)
        context_gl->gl_info->gl_ops.gl.p_glFlush();
}

void adapter_gl_clear_uav(struct wined3d_context *context,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value)
{
    TRACE("context %p, view %p, clear_value %s.\n", context, view, debug_uvec4(clear_value));

    wined3d_unordered_access_view_gl_clear_uint(wined3d_unordered_access_view_gl(view),
            clear_value, wined3d_context_gl(context));
}

static const struct wined3d_adapter_ops wined3d_adapter_gl_ops =
{
    adapter_gl_destroy,
    adapter_gl_create_device,
    adapter_gl_destroy_device,
    adapter_gl_acquire_context,
    adapter_gl_release_context,
    adapter_gl_get_wined3d_caps,
    adapter_gl_check_format,
    adapter_gl_init_3d,
    adapter_gl_uninit_3d,
    adapter_gl_map_bo_address,
    adapter_gl_unmap_bo_address,
    adapter_gl_copy_bo_address,
    adapter_gl_create_swapchain,
    adapter_gl_destroy_swapchain,
    adapter_gl_create_buffer,
    adapter_gl_destroy_buffer,
    adapter_gl_create_texture,
    adapter_gl_destroy_texture,
    adapter_gl_create_rendertarget_view,
    adapter_gl_destroy_rendertarget_view,
    adapter_gl_create_shader_resource_view,
    adapter_gl_destroy_shader_resource_view,
    adapter_gl_create_unordered_access_view,
    adapter_gl_destroy_unordered_access_view,
    adapter_gl_create_sampler,
    adapter_gl_destroy_sampler,
    adapter_gl_create_query,
    adapter_gl_destroy_query,
    adapter_gl_flush_context,
    adapter_gl_clear_uav,
};

static void wined3d_adapter_gl_init_d3d_info(struct wined3d_adapter_gl *adapter_gl, uint32_t wined3d_creation_flags)
{
    const struct wined3d_gl_info *gl_info = &adapter_gl->a.gl_info;
    struct wined3d_d3d_info *d3d_info = &adapter_gl->a.d3d_info;
    struct wined3d_vertex_caps vertex_caps;
    struct fragment_caps fragment_caps;
    struct shader_caps shader_caps;
    GLfloat f[2];
    int i;

    adapter_gl->a.shader_backend->shader_get_caps(&adapter_gl->a, &shader_caps);
    adapter_gl->a.vertex_pipe->vp_get_caps(&adapter_gl->a, &vertex_caps);
    adapter_gl->a.fragment_pipe->get_caps(&adapter_gl->a, &fragment_caps);

    d3d_info->limits.vs_version = shader_caps.vs_version;
    d3d_info->limits.hs_version = shader_caps.hs_version;
    d3d_info->limits.ds_version = shader_caps.ds_version;
    d3d_info->limits.gs_version = shader_caps.gs_version;
    d3d_info->limits.ps_version = shader_caps.ps_version;
    d3d_info->limits.cs_version = shader_caps.cs_version;
    d3d_info->limits.vs_uniform_count_swvp = shader_caps.vs_uniform_count;
    d3d_info->limits.vs_uniform_count = min(WINED3D_MAX_VS_CONSTS_F, shader_caps.vs_uniform_count);
    d3d_info->limits.ps_uniform_count = shader_caps.ps_uniform_count;
    d3d_info->limits.varying_count = shader_caps.varying_count;
    d3d_info->limits.ffp_textures = fragment_caps.MaxSimultaneousTextures;
    d3d_info->limits.ffp_blend_stages = fragment_caps.MaxTextureBlendStages;
    TRACE("Max texture stages: %u.\n", d3d_info->limits.ffp_blend_stages);
    d3d_info->limits.ffp_vertex_blend_matrices = vertex_caps.max_vertex_blend_matrices;
    d3d_info->limits.active_light_count = vertex_caps.max_active_lights;
    d3d_info->limits.ffp_max_vertex_blend_matrix_index = vertex_caps.max_vertex_blend_matrix_index;

    d3d_info->valid_dual_rt_mask = 0;
    for (i = 0; i < gl_info->limits.dual_buffers; ++i)
        d3d_info->valid_dual_rt_mask |= (1u << i);

    d3d_info->limits.max_rt_count = gl_info->limits.buffers;
    d3d_info->limits.max_clip_distances = gl_info->limits.user_clip_distances;
    d3d_info->limits.texture_size = gl_info->limits.texture_size;

    gl_info->gl_ops.gl.p_glGetFloatv(gl_info->supported[WINED3D_GL_LEGACY_CONTEXT]
            ? GL_ALIASED_POINT_SIZE_RANGE : GL_POINT_SIZE_RANGE, f);
    d3d_info->limits.pointsize_max = f[1];
    TRACE("Maximum point size support - max point size %.8e.\n", f[1]);

    d3d_info->wined3d_creation_flags = wined3d_creation_flags;
    d3d_info->xyzrhw = vertex_caps.xyzrhw;
    d3d_info->emulated_flatshading = vertex_caps.emulated_flatshading;
    d3d_info->ffp_generic_attributes = vertex_caps.ffp_generic_attributes;
    d3d_info->ffp_alpha_test = !!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT];
    d3d_info->vs_clipping = shader_caps.wined3d_caps & WINED3D_SHADER_CAP_VS_CLIPPING;
    d3d_info->shader_color_key = !!(fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_COLOR_KEY);
    d3d_info->shader_double_precision = !!(shader_caps.wined3d_caps & WINED3D_SHADER_CAP_DOUBLE_PRECISION);
    d3d_info->shader_output_interpolation = !!(shader_caps.wined3d_caps & WINED3D_SHADER_CAP_OUTPUT_INTERPOLATION);
    d3d_info->viewport_array_index_any_shader = !!gl_info->supported[ARB_SHADER_VIEWPORT_LAYER_ARRAY];
    d3d_info->texture_npot = !!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO];
    d3d_info->texture_npot_conditional = gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT]
            || gl_info->supported[ARB_TEXTURE_RECTANGLE];
    d3d_info->draw_base_vertex_offset = !!gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX];
    d3d_info->vertex_bgra = !!gl_info->supported[ARB_VERTEX_ARRAY_BGRA];
    d3d_info->texture_swizzle = !!gl_info->supported[ARB_TEXTURE_SWIZZLE];
    d3d_info->srgb_read_control = !!gl_info->supported[EXT_TEXTURE_SRGB_DECODE];
    d3d_info->srgb_write_control = !!gl_info->supported[ARB_FRAMEBUFFER_SRGB];
    d3d_info->clip_control = !!gl_info->supported[ARB_CLIP_CONTROL];
    d3d_info->full_ffp_varyings = !!(shader_caps.wined3d_caps & WINED3D_SHADER_CAP_FULL_FFP_VARYINGS);
    d3d_info->feature_level = feature_level_from_caps(gl_info, &shader_caps, &fragment_caps);

    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
        d3d_info->multisample_draw_location = WINED3D_LOCATION_TEXTURE_RGB;
    else
        d3d_info->multisample_draw_location = WINED3D_LOCATION_RB_MULTISAMPLE;
}

static BOOL wined3d_adapter_gl_init(struct wined3d_adapter_gl *adapter_gl,
        unsigned int ordinal, unsigned int wined3d_creation_flags)
{
    static const DWORD supported_gl_versions[] =
    {
        MAKEDWORD_VERSION(4, 4),
        MAKEDWORD_VERSION(3, 2),
        MAKEDWORD_VERSION(1, 0),
    };
    struct wined3d_gl_info *gl_info = &adapter_gl->a.gl_info;
    struct wined3d_caps_gl_ctx caps_gl_ctx = {0};
    unsigned int i;

    TRACE("adapter_gl %p, ordinal %u, wined3d_creation_flags %#x.\n",
            adapter_gl, ordinal, wined3d_creation_flags);

    if (!wined3d_adapter_init(&adapter_gl->a, ordinal, &wined3d_adapter_gl_ops))
        return FALSE;

    /* Dynamically load all GL core functions */
#ifdef USE_WIN32_OPENGL
    {
        HMODULE mod_gl = GetModuleHandleA("opengl32.dll");
#define USE_GL_FUNC(f) gl_info->gl_ops.gl.p_##f = (void *)GetProcAddress(mod_gl, #f);
        ALL_WGL_FUNCS
#undef USE_GL_FUNC
        gl_info->gl_ops.wgl.p_wglSwapBuffers = (void *)GetProcAddress(mod_gl, "wglSwapBuffers");
        gl_info->gl_ops.wgl.p_wglGetPixelFormat = (void *)GetProcAddress(mod_gl, "wglGetPixelFormat");
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

    gl_info->p_glEnableWINE = gl_info->gl_ops.gl.p_glEnable;
    gl_info->p_glDisableWINE = gl_info->gl_ops.gl.p_glDisable;

    if (!wined3d_caps_gl_ctx_create(&adapter_gl->a, &caps_gl_ctx))
    {
        ERR("Failed to get a GL context for adapter %p.\n", adapter_gl);
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

    if (!wined3d_adapter_init_gl_caps(&adapter_gl->a, &caps_gl_ctx, wined3d_creation_flags))
    {
        ERR("Failed to initialize GL caps for adapter %p.\n", adapter_gl);
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        return FALSE;
    }

    wined3d_adapter_gl_init_d3d_info(adapter_gl, wined3d_creation_flags);
    if (!adapter_gl->a.d3d_info.shader_color_key)
    {
        /* We do not want to deal with re-creating immutable texture storage
         * for colour-keying emulation. */
        WARN("Disabling ARB_texture_storage because fragment pipe doesn't support colour-keying.\n");
        gl_info->supported[ARB_TEXTURE_STORAGE] = FALSE;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
        ERR_(winediag)("You are using the backbuffer for offscreen rendering. "
                "This is unsupported, and will be removed in a future version.\n");

    wined3d_adapter_init_fb_cfgs(adapter_gl, caps_gl_ctx.dc);
    /* We haven't found any suitable formats. This should only happen in
     * case of GDI software rendering, which is pretty useless anyway. */
    if (!adapter_gl->pixel_format_count)
    {
        WARN("No suitable pixel formats found.\n");
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        heap_free(adapter_gl->pixel_formats);
        return FALSE;
    }

    if (!wined3d_adapter_gl_init_format_info(&adapter_gl->a, &caps_gl_ctx))
    {
        ERR("Failed to initialize GL format info.\n");
        wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);
        heap_free(adapter_gl->pixel_formats);
        return FALSE;
    }

    wined3d_caps_gl_ctx_destroy(&caps_gl_ctx);

    wined3d_adapter_init_ffp_attrib_ops(&adapter_gl->a);

    return TRUE;
}

struct wined3d_adapter *wined3d_adapter_gl_create(unsigned int ordinal, unsigned int wined3d_creation_flags)
{
    struct wined3d_adapter_gl *adapter;

    if (!(adapter = heap_alloc_zero(sizeof(*adapter))))
        return NULL;

    if (!wined3d_adapter_gl_init(adapter, ordinal, wined3d_creation_flags))
    {
        heap_free(adapter);
        return NULL;
    }

    TRACE("Created adapter %p.\n", adapter);

    return &adapter->a;
}
