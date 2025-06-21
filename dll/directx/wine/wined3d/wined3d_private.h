/*
 * Direct3D wine internal private include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003, 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2011, 2013 Stefan Dösinger for CodeWeavers
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

#ifndef __WINE_WINED3D_PRIVATE_H
#define __WINE_WINED3D_PRIVATE_H

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#define LIBVKD3D_SHADER_SOURCE
#include <vkd3d_shader.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"
#include "wine/debug.h"

#include "objbase.h"
#include "wine/wined3d.h"
#include "wine/list.h"
#include "wine/rbtree.h"

static inline size_t align(size_t addr, size_t alignment)
{
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

static inline uint32_t float_to_int(float f)
{
    union
    {
        uint32_t u;
        float f;
    } u;

    u.f = f;
    return u.u;
}

static inline float int_to_float(uint32_t i)
{
    union
    {
        uint32_t u;
        float f;
    } u;

    u.u = i;
    return u.f;
}

static inline const char *wined3d_get_line(const char **ptr, const char *end)
{
    const char *p, *q;

    if ((p = *ptr) >= end)
        return NULL;

    if (!(q = memchr(p, '\n', end - p)))
        *ptr = end;
    else
        *ptr = q + 1;
    return p;
}

#define MAKEDWORD_VERSION(maj, min) (((maj & 0xffffu) << 16) | (min & 0xffffu))

#define WINED3D_MAX_DIRTY_REGION_COUNT 7

#define WINED3D_ALPHA_TO_COVERAGE_ENABLE MAKEFOURCC('A','2','M','1')
#define WINED3D_ALPHA_TO_COVERAGE_DISABLE MAKEFOURCC('A','2','M','0')

#define WINED3D_BITMAP_SIZE(x) (((x) + 31) >> 5)

struct wined3d_fragment_pipe_ops;
struct wined3d_adapter;
struct wined3d_context;
struct wined3d_state;
struct wined3d_vertex_pipe_ops;

enum wined3d_ffp_idx
{
    WINED3D_FFP_POSITION = 0,
    WINED3D_FFP_BLENDWEIGHT = 1,
    WINED3D_FFP_BLENDINDICES = 2,
    WINED3D_FFP_NORMAL = 3,
    WINED3D_FFP_PSIZE = 4,
    WINED3D_FFP_DIFFUSE = 5,
    WINED3D_FFP_SPECULAR = 6,
    WINED3D_FFP_TEXCOORD0 = 7,
    WINED3D_FFP_TEXCOORD1 = 8,
    WINED3D_FFP_TEXCOORD2 = 9,
    WINED3D_FFP_TEXCOORD3 = 10,
    WINED3D_FFP_TEXCOORD4 = 11,
    WINED3D_FFP_TEXCOORD5 = 12,
    WINED3D_FFP_TEXCOORD6 = 13,
    WINED3D_FFP_TEXCOORD7 = 14,
    WINED3D_FFP_ATTRIBS_COUNT = 15,
};

enum wined3d_ffp_emit_idx
{
    WINED3D_FFP_EMIT_FLOAT1,
    WINED3D_FFP_EMIT_FLOAT2,
    WINED3D_FFP_EMIT_FLOAT3,
    WINED3D_FFP_EMIT_FLOAT4,
    WINED3D_FFP_EMIT_D3DCOLOR,
    WINED3D_FFP_EMIT_UBYTE4,
    WINED3D_FFP_EMIT_SHORT2,
    WINED3D_FFP_EMIT_SHORT4,
    WINED3D_FFP_EMIT_UBYTE4N,
    WINED3D_FFP_EMIT_SHORT2N,
    WINED3D_FFP_EMIT_SHORT4N,
    WINED3D_FFP_EMIT_USHORT2N,
    WINED3D_FFP_EMIT_USHORT4N,
    WINED3D_FFP_EMIT_UDEC3,
    WINED3D_FFP_EMIT_DEC3N,
    WINED3D_FFP_EMIT_FLOAT16_2,
    WINED3D_FFP_EMIT_FLOAT16_4,
    WINED3D_FFP_EMIT_INVALID,
    WINED3D_FFP_EMIT_COUNT,
};

/* Texture format fixups */

enum fixup_channel_source
{
    CHANNEL_SOURCE_ZERO = 0,
    CHANNEL_SOURCE_ONE = 1,
    CHANNEL_SOURCE_X = 2,
    CHANNEL_SOURCE_Y = 3,
    CHANNEL_SOURCE_Z = 4,
    CHANNEL_SOURCE_W = 5,
    CHANNEL_SOURCE_COMPLEX0 = 6,
    CHANNEL_SOURCE_COMPLEX1 = 7,
};

enum complex_fixup
{
    COMPLEX_FIXUP_NONE = 0,
    COMPLEX_FIXUP_YUY2 = 1,
    COMPLEX_FIXUP_UYVY = 2,
    COMPLEX_FIXUP_YV12 = 3,
    COMPLEX_FIXUP_P8   = 4,
    COMPLEX_FIXUP_NV12 = 5,
    COMPLEX_FIXUP_YUV  = 6,
};

#include <pshpack2.h>
struct color_fixup_desc
{
    unsigned short x_sign_fixup : 1;
    unsigned short x_source : 3;
    unsigned short y_sign_fixup : 1;
    unsigned short y_source : 3;
    unsigned short z_sign_fixup : 1;
    unsigned short z_source : 3;
    unsigned short w_sign_fixup : 1;
    unsigned short w_source : 3;
};
#include <poppack.h>

struct fragment_caps
{
    unsigned int PrimitiveMiscCaps;
    unsigned int TextureOpCaps;
    unsigned int max_blend_stages;
    unsigned int max_textures;
};

struct wined3d_d3d_limits
{
    unsigned int vs_version, hs_version, ds_version, gs_version, ps_version, cs_version;
    DWORD vs_uniform_count;
    DWORD ps_uniform_count;
    unsigned int varying_count;
    unsigned int ffp_vertex_blend_matrices;
    unsigned int active_light_count;

    unsigned int max_rt_count;
    unsigned int max_clip_distances;
    unsigned int texture_size;
    unsigned int sample_count;
    float pointsize_max;
};

struct wined3d_d3d_info
{
    struct fragment_caps ffp_fragment_caps;
    struct wined3d_d3d_limits limits;
    uint32_t wined3d_creation_flags;
    uint32_t emulated_flatshading : 1;
    uint32_t ffp_alpha_test : 1;
    uint32_t shader_double_precision : 1;
    uint32_t shader_output_interpolation : 1;
    uint32_t viewport_array_index_any_shader : 1;
    uint32_t stencil_export : 1;
    /* If zero, only conditional NPOT textures are supported, via
     * WINED3D_GL_NORMALIZED_TEXRECT. */
    uint32_t unconditional_npot : 1;
    uint32_t draw_base_vertex_offset : 1;
    uint32_t vertex_bgra : 1;
    uint32_t texture_swizzle : 1;
    uint32_t srgb_read_control : 1;
    uint32_t srgb_write_control : 1;
    uint32_t clip_control : 1;
    uint32_t full_ffp_varyings : 1;
    uint32_t scaled_resolve : 1;
    uint32_t pbo : 1;
    uint32_t subpixel_viewport : 1;
    uint32_t fences : 1;
    uint32_t persistent_map : 1;
    uint32_t gpu_push_constants : 1;
    uint32_t ffp_hlsl : 1;
    enum wined3d_feature_level feature_level;

    DWORD multisample_draw_location;

    float filling_convention_offset;
};

static const struct color_fixup_desc COLOR_FIXUP_IDENTITY =
        {0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_Z, 0, CHANNEL_SOURCE_W};

static inline struct color_fixup_desc create_complex_fixup_desc(enum complex_fixup complex_fixup)
{
    struct color_fixup_desc fixup =
    {
        0u, complex_fixup & (1u << 0) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0u, complex_fixup & (1u << 1) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0u, complex_fixup & (1u << 2) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0u, complex_fixup & (1u << 3) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
    };
    return fixup;
}

static inline BOOL is_identity_fixup(struct color_fixup_desc fixup)
{
    return !memcmp(&fixup, &COLOR_FIXUP_IDENTITY, sizeof(fixup));
}

static inline BOOL is_complex_fixup(struct color_fixup_desc fixup)
{
    return fixup.x_source == CHANNEL_SOURCE_COMPLEX0 || fixup.x_source == CHANNEL_SOURCE_COMPLEX1;
}

static inline BOOL is_scaling_fixup(struct color_fixup_desc fixup)
{
    return fixup.x_sign_fixup || fixup.y_sign_fixup || fixup.z_sign_fixup || fixup.w_sign_fixup;
}

static inline BOOL is_same_fixup(struct color_fixup_desc f1, struct color_fixup_desc f2)
{
    return f1.x_sign_fixup == f2.x_sign_fixup && f1.x_source == f2.x_source
            && f1.y_sign_fixup == f2.y_sign_fixup && f1.y_source == f2.y_source
            && f1.z_sign_fixup == f2.z_sign_fixup && f1.z_source == f2.z_source
            && f1.w_sign_fixup == f2.w_sign_fixup && f1.w_source == f2.w_source;
}

static inline enum complex_fixup get_complex_fixup(struct color_fixup_desc fixup)
{
    enum complex_fixup complex_fixup = 0;
    if (fixup.x_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1u << 0);
    if (fixup.y_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1u << 1);
    if (fixup.z_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1u << 2);
    if (fixup.w_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1u << 3);
    return complex_fixup;
}

/* Device caps */
#define WINED3D_MAX_ACTIVE_LIGHTS   8
#define WINED3D_MAX_SOFTWARE_ACTIVE_LIGHTS 32
#define MAX_CONSTANT_BUFFERS        15
#define MAX_SAMPLER_OBJECTS         16
#define MAX_SHADER_RESOURCE_VIEWS   128
#define MAX_UNORDERED_ACCESS_VIEWS  8
#define MAX_TGSM_REGISTERS          8192
#define MAX_VERTEX_BLENDS           4

static const uint32_t WINED3D_READ_ONLY_BIND_MASK = WINED3D_BIND_VERTEX_BUFFER | WINED3D_BIND_INDEX_BUFFER
        | WINED3D_BIND_CONSTANT_BUFFER | WINED3D_BIND_SHADER_RESOURCE | WINED3D_BIND_INDIRECT_BUFFER;

static inline enum wined3d_cmp_func wined3d_sanitize_cmp_func(enum wined3d_cmp_func func)
{
    if (func < WINED3D_CMP_NEVER || func > WINED3D_CMP_ALWAYS)
        return WINED3D_CMP_ALWAYS;
    return func;
}

/* float_16_to_32() and float_32_to_16() convert 16 bit floats in the
 * FLOAT16 data type to standard C floats and vice versa. They do not
 * depend on the encoding of the C float, so they are platform independent,
 * but slow. On x86 and other IEEE 754 compliant platforms the conversion
 * can be accelerated by bit shifting the exponent and mantissa. There are
 * also some SSE-based assembly routines out there.
 *
 * See GL_NV_half_float for a reference of the FLOAT16 / GL_HALF format
 */
static inline float float_16_to_32(const unsigned short *in)
{
    const unsigned short s = ((*in) & 0x8000u);
    const unsigned short e = ((*in) & 0x7c00u) >> 10;
    const unsigned short m = (*in) & 0x3ffu;
    const float sgn = (s ? -1.0f : 1.0f);

    if(e == 0) {
        if(m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -14.0f) * ((float)m / 1024.0f);
    } else if(e < 31) {
        return sgn * powf(2, (float)e - 15.0f) * (1.0f + ((float)m / 1024.0f));
    } else {
        if(m == 0) return sgn * INFINITY;
        else return NAN;
    }
}

static inline float float_24_to_32(DWORD in)
{
    const float sgn = in & 0x800000u ? -1.0f : 1.0f;
    const unsigned short e = (in & 0x780000u) >> 19;
    const unsigned int m = in & 0x7ffffu;

    if (e == 0)
    {
        if (m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -6.0f) * ((float)m / 524288.0f);
    }
    else if (e < 15)
    {
        return sgn * powf(2, (float)e - 7.0f) * (1.0f + ((float)m / 524288.0f));
    }
    else
    {
        if (m == 0) return sgn * INFINITY;
        else return NAN;
    }
}

static inline unsigned short float_32_to_16(const float *in)
{
    int exp = 0;
    float tmp = fabsf(*in);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (*in == 0.0f)
        return 0x0000;
    if (isnan(*in))
        return 0x7c01;
    if (isinf(*in))
        return (*in < 0.0f ? 0xfc00 : 0x7c00);

    if (tmp < (float)(1u << 10))
    {
        do
        {
            tmp = tmp * 2.0f;
            exp--;
        } while (tmp < (float)(1u << 10));
    }
    else if (tmp >= (float)(1u << 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= (float)(1u << 11));
    }

    mantissa = (unsigned int)tmp;
    if (tmp - mantissa >= 0.5f)
        ++mantissa; /* Round to nearest, away from zero. */

    exp += 10;  /* Normalize the mantissa. */
    exp += 15;  /* Exponent is encoded with excess 15. */

    if (exp > 30) /* too big */
    {
        ret = 0x7c00; /* INF */
    }
    else if (exp <= 0)
    {
        /* exp == 0: Non-normalized mantissa. Returns 0x0000 (=0.0) for too small numbers. */
        while (exp <= 0)
        {
            mantissa = mantissa >> 1;
            ++exp;
        }
        ret = mantissa & 0x3ff;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((*in < 0.0f ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

static inline unsigned int wined3d_popcount(unsigned int x)
{
#if defined(__MINGW32__)
    return __builtin_popcount(x);
#else
    x -= x >> 1 & 0x55555555;
    x = (x & 0x33333333) + (x >> 2 & 0x33333333);
    return ((x + (x >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static inline int wined3d_uint32_compare(uint32_t x, uint32_t y)
{
    return (x > y) - (x < y);
}

static inline int wined3d_uint64_compare(uint64_t x, uint64_t y)
{
    return (x > y) - (x < y);
}

#define PCI_VENDOR_NONE 0xffff /* e.g. 0x8086 for Intel and 0x10de for Nvidia */
#define PCI_DEVICE_NONE 0xffff /* e.g. 0x14f for a Geforce6200 */

enum wined3d_shader_backend
{
    WINED3D_SHADER_BACKEND_AUTO,
    WINED3D_SHADER_BACKEND_GLSL,
    WINED3D_SHADER_BACKEND_GLSL_VKD3D,
};

#define WINED3D_CSMT_ENABLE    0x00000001
#define WINED3D_CSMT_SERIALIZE 0x00000002

/* NOTE: When adding fields to this structure, make sure to update the default
 * values in wined3d_main.c as well. */
struct wined3d_settings
{
    unsigned int cs_multithreaded;
    unsigned int max_gl_version;
    unsigned short pci_vendor_id;
    unsigned short pci_device_id;
    /* Memory tracking and object counting. */
    UINT64 emulated_textureram;
    char *logo;
    unsigned int multisample_textures;
    unsigned int sample_count;
    unsigned int strict_shader_math;
    unsigned int max_sm_vs;
    unsigned int max_sm_hs;
    unsigned int max_sm_ds;
    unsigned int max_sm_gs;
    unsigned int max_sm_ps;
    unsigned int max_sm_cs;
    enum wined3d_renderer renderer;
    enum wined3d_shader_backend shader_backend;
    bool check_float_constants;
    bool cb_access_map_w;
    bool ffp_hlsl;
};

extern struct wined3d_settings wined3d_settings;

enum wined3d_shader_resource_type
{
    WINED3D_SHADER_RESOURCE_NONE,
    WINED3D_SHADER_RESOURCE_BUFFER,
    WINED3D_SHADER_RESOURCE_TEXTURE_1D,
    WINED3D_SHADER_RESOURCE_TEXTURE_2D,
    WINED3D_SHADER_RESOURCE_TEXTURE_2DMS,
    WINED3D_SHADER_RESOURCE_TEXTURE_3D,
    WINED3D_SHADER_RESOURCE_TEXTURE_CUBE,
    WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY,
    WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY,
    WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY,
    WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY,
};

#define WINED3D_SHADER_CONST_VS_F            0x00000001
#define WINED3D_SHADER_CONST_VS_I            0x00000002
#define WINED3D_SHADER_CONST_VS_B            0x00000004
#define WINED3D_SHADER_CONST_VS_CLIP_PLANES  0x00000008
#define WINED3D_SHADER_CONST_VS_POINTSIZE    0x00000010
#define WINED3D_SHADER_CONST_POS_FIXUP       0x00000020
#define WINED3D_SHADER_CONST_PS_F            0x00000040
#define WINED3D_SHADER_CONST_PS_I            0x00000080
#define WINED3D_SHADER_CONST_PS_B            0x00000100
#define WINED3D_SHADER_CONST_PS_BUMP_ENV     0x00000200
#define WINED3D_SHADER_CONST_PS_FOG          0x00000400
#define WINED3D_SHADER_CONST_PS_ALPHA_TEST   0x00000800
#define WINED3D_SHADER_CONST_FFP_MODELVIEW   0x00004000
#define WINED3D_SHADER_CONST_FFP_VERTEXBLEND 0x00008000
#define WINED3D_SHADER_CONST_FFP_PROJ        0x00010000
#define WINED3D_SHADER_CONST_FFP_TEXMATRIX   0x00020000
#define WINED3D_SHADER_CONST_FFP_MATERIAL    0x00040000
#define WINED3D_SHADER_CONST_FFP_LIGHTS      0x00080000
#define WINED3D_SHADER_CONST_FFP_PS          0x00100000
#define WINED3D_SHADER_CONST_FFP_COLOR_KEY   0x00200000
#define WINED3D_SHADER_CONST_BASE_VERTEX_ID  0x00400000

enum wined3d_shader_register_type
{
    WINED3DSPR_TEMP = 0,
    WINED3DSPR_INPUT = 1,
    WINED3DSPR_CONST = 2,
    WINED3DSPR_ADDR = 3,
    WINED3DSPR_TEXTURE = 3,
    WINED3DSPR_RASTOUT = 4,
    WINED3DSPR_ATTROUT = 5,
    WINED3DSPR_TEXCRDOUT = 6,
    WINED3DSPR_OUTPUT = 6,
    WINED3DSPR_CONSTINT = 7,
    WINED3DSPR_COLOROUT = 8,
    WINED3DSPR_DEPTHOUT = 9,
    WINED3DSPR_SAMPLER = 10,
    WINED3DSPR_CONST2 = 11,
    WINED3DSPR_CONST3 = 12,
    WINED3DSPR_CONST4 = 13,
    WINED3DSPR_CONSTBOOL = 14,
    WINED3DSPR_LOOP = 15,
    WINED3DSPR_TEMPFLOAT16 = 16,
    WINED3DSPR_MISCTYPE = 17,
    WINED3DSPR_LABEL = 18,
    WINED3DSPR_PREDICATE = 19,
    WINED3DSPR_IMMCONST,
    WINED3DSPR_CONSTBUFFER,
    WINED3DSPR_IMMCONSTBUFFER,
    WINED3DSPR_PRIMID,
    WINED3DSPR_NULL,
    WINED3DSPR_RESOURCE,
    WINED3DSPR_UAV,
    WINED3DSPR_OUTPOINTID,
    WINED3DSPR_FORKINSTID,
    WINED3DSPR_JOININSTID,
    WINED3DSPR_INCONTROLPOINT,
    WINED3DSPR_OUTCONTROLPOINT,
    WINED3DSPR_PATCHCONST,
    WINED3DSPR_TESSCOORD,
    WINED3DSPR_GROUPSHAREDMEM,
    WINED3DSPR_THREADID,
    WINED3DSPR_THREADGROUPID,
    WINED3DSPR_LOCALTHREADID,
    WINED3DSPR_LOCALTHREADINDEX,
    WINED3DSPR_IDXTEMP,
    WINED3DSPR_STREAM,
    WINED3DSPR_FUNCTIONBODY,
    WINED3DSPR_FUNCTIONPOINTER,
    WINED3DSPR_COVERAGE,
    WINED3DSPR_SAMPLEMASK,
    WINED3DSPR_GSINSTID,
    WINED3DSPR_DEPTHOUTGE,
    WINED3DSPR_DEPTHOUTLE,
    WINED3DSPR_RASTERIZER,
    WINED3DSPR_STENCILREF,
};

enum wined3d_data_type
{
    WINED3D_DATA_FLOAT,
    WINED3D_DATA_INT,
    WINED3D_DATA_RESOURCE,
    WINED3D_DATA_SAMPLER,
    WINED3D_DATA_UAV,
    WINED3D_DATA_UINT,
    WINED3D_DATA_UNORM,
    WINED3D_DATA_SNORM,
    WINED3D_DATA_OPAQUE,
};

enum wined3d_immconst_type
{
    WINED3D_IMMCONST_SCALAR,
    WINED3D_IMMCONST_VEC4,
};

#define WINED3DSP_NOSWIZZLE (0u | (1u << 2) | (2u << 4) | (3u << 6))

enum wined3d_shader_src_modifier
{
    WINED3DSPSM_NONE = 0,
    WINED3DSPSM_NEG = 1,
    WINED3DSPSM_BIAS = 2,
    WINED3DSPSM_BIASNEG = 3,
    WINED3DSPSM_SIGN = 4,
    WINED3DSPSM_SIGNNEG = 5,
    WINED3DSPSM_COMP = 6,
    WINED3DSPSM_X2 = 7,
    WINED3DSPSM_X2NEG = 8,
    WINED3DSPSM_DZ = 9,
    WINED3DSPSM_DW = 10,
    WINED3DSPSM_ABS = 11,
    WINED3DSPSM_ABSNEG = 12,
    WINED3DSPSM_NOT = 13,
};

#define WINED3DSP_WRITEMASK_0   0x1u /* .x r */
#define WINED3DSP_WRITEMASK_1   0x2u /* .y g */
#define WINED3DSP_WRITEMASK_2   0x4u /* .z b */
#define WINED3DSP_WRITEMASK_3   0x8u /* .w a */
#define WINED3DSP_WRITEMASK_ALL 0xfu /* all */

enum wined3d_shader_dst_modifier
{
    WINED3DSPDM_NONE = 0,
    WINED3DSPDM_SATURATE = 1,
    WINED3DSPDM_PARTIALPRECISION = 2,
    WINED3DSPDM_MSAMPCENTROID = 4,
};

enum wined3d_shader_interpolation_mode
{
    WINED3DSIM_NONE = 0,
    WINED3DSIM_CONSTANT = 1,
    WINED3DSIM_LINEAR = 2,
    WINED3DSIM_LINEAR_CENTROID = 3,
    WINED3DSIM_LINEAR_NOPERSPECTIVE = 4,
    WINED3DSIM_LINEAR_NOPERSPECTIVE_CENTROID = 5,
    WINED3DSIM_LINEAR_SAMPLE = 6,
    WINED3DSIM_LINEAR_NOPERSPECTIVE_SAMPLE = 7,
};

#define WINED3D_PACKED_INTERPOLATION_SIZE 3
#define WINED3D_PACKED_INTERPOLATION_BIT_COUNT 3

enum wined3d_shader_global_flags
{
    WINED3DSGF_REFACTORING_ALLOWED               = 0x1,
    WINED3DSGF_FORCE_EARLY_DEPTH_STENCIL         = 0x4,
    WINED3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS = 0x8,
};

enum wined3d_shader_sync_flags
{
    WINED3DSSF_THREAD_GROUP        = 0x1,
    WINED3DSSF_GROUP_SHARED_MEMORY = 0x2,
    WINED3DSSF_GLOBAL_UAV          = 0x8,
};

enum wined3d_shader_uav_flags
{
    WINED3DSUF_GLOBALLY_COHERENT = 0x2,
    WINED3DSUF_ORDER_PRESERVING_COUNTER = 0x100,
};

enum wined3d_tessellator_domain
{
    WINED3D_TESSELLATOR_DOMAIN_LINE      = 1,
    WINED3D_TESSELLATOR_DOMAIN_TRIANGLE  = 2,
    WINED3D_TESSELLATOR_DOMAIN_QUAD      = 3,
};

enum wined3d_tessellator_output_primitive
{
    WINED3D_TESSELLATOR_OUTPUT_POINT        = 1,
    WINED3D_TESSELLATOR_OUTPUT_LINE         = 2,
    WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CW  = 3,
    WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW = 4,
};

enum wined3d_tessellator_partitioning
{
    WINED3D_TESSELLATOR_PARTITIONING_INTEGER         = 1,
    WINED3D_TESSELLATOR_PARTITIONING_POW2            = 2,
    WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD  = 3,
    WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN = 4,
};

/* Undocumented opcode control to identify projective texture lookups in ps 2.0 and later */
#define WINED3DSI_TEXLD_PROJECT     0x1
#define WINED3DSI_TEXLD_BIAS        0x2
#define WINED3DSI_INDEXED_DYNAMIC   0x4
#define WINED3DSI_RESINFO_RCP_FLOAT 0x1
#define WINED3DSI_RESINFO_UINT      0x2
#define WINED3DSI_SAMPLE_INFO_UINT  0x1
#define WINED3DSI_SAMPLER_COMPARISON_MODE 0x1

#define WINED3DSI_PRECISE_X         0x100
#define WINED3DSI_PRECISE_Y         0x200
#define WINED3DSI_PRECISE_Z         0x400
#define WINED3DSI_PRECISE_W         0x800
#define WINED3DSI_PRECISE_XYZW      (WINED3DSI_PRECISE_X | WINED3DSI_PRECISE_Y \
                                    | WINED3DSI_PRECISE_Z | WINED3DSI_PRECISE_W)
#define WINED3DSI_PRECISE_SHIFT     8

enum wined3d_shader_rel_op
{
    WINED3D_SHADER_REL_OP_GT = 1,
    WINED3D_SHADER_REL_OP_EQ = 2,
    WINED3D_SHADER_REL_OP_GE = 3,
    WINED3D_SHADER_REL_OP_LT = 4,
    WINED3D_SHADER_REL_OP_NE = 5,
    WINED3D_SHADER_REL_OP_LE = 6,
};

enum wined3d_shader_conditional_op
{
    WINED3D_SHADER_CONDITIONAL_OP_NZ = 0,
    WINED3D_SHADER_CONDITIONAL_OP_Z  = 1
};

enum wined3d_sysval_semantic
{
    WINED3D_SV_POSITION                  = 0x01,
    WINED3D_SV_CLIP_DISTANCE             = 0x02,
    WINED3D_SV_CULL_DISTANCE             = 0x03,
    WINED3D_SV_RENDER_TARGET_ARRAY_INDEX = 0x04,
    WINED3D_SV_VIEWPORT_ARRAY_INDEX      = 0x05,
    WINED3D_SV_VERTEX_ID                 = 0x06,
    WINED3D_SV_PRIMITIVE_ID              = 0x07,
    WINED3D_SV_INSTANCE_ID               = 0x08,
    WINED3D_SV_IS_FRONT_FACE             = 0x09,
    WINED3D_SV_SAMPLE_INDEX              = 0x0a,
    WINED3D_SV_TESS_FACTOR_QUADEDGE      = 0x0b,
    WINED3D_SV_TESS_FACTOR_QUADINT       = 0x0c,
    WINED3D_SV_TESS_FACTOR_TRIEDGE       = 0x0d,
    WINED3D_SV_TESS_FACTOR_TRIINT        = 0x0e,
    WINED3D_SV_TESS_FACTOR_LINEDET       = 0x0f,
    WINED3D_SV_TESS_FACTOR_LINEDEN       = 0x10,
};

enum wined3d_component_type
{
    WINED3D_TYPE_UNKNOWN = 0x0,
    WINED3D_TYPE_UINT    = 0x1,
    WINED3D_TYPE_INT     = 0x2,
    WINED3D_TYPE_FLOAT   = 0x3,
};

struct wined3d_shader_signature_element
{
    const char *semantic_name;
    unsigned int semantic_idx;
    unsigned int stream_idx;
    enum wined3d_sysval_semantic sysval_semantic;
    enum wined3d_component_type component_type;
    unsigned int register_idx;
    uint32_t mask;
    unsigned int min_precision;
};

struct wined3d_shader_signature
{
    unsigned int element_count;
    struct wined3d_shader_signature_element *elements;
};

#define WINED3D_SM1_VS  0xfffeu
#define WINED3D_SM1_PS  0xffffu
#define WINED3D_SM4_PS  0x0000u
#define WINED3D_SM4_VS  0x0001u
#define WINED3D_SM4_GS  0x0002u
#define WINED3D_SM5_HS  0x0003u
#define WINED3D_SM5_DS  0x0004u
#define WINED3D_SM5_CS  0x0005u

/* Shader version tokens, and shader end tokens */
#define WINED3DPS_VERSION(major, minor) ((WINED3D_SM1_PS << 16) | ((major) << 8) | (minor))
#define WINED3DVS_VERSION(major, minor) ((WINED3D_SM1_VS << 16) | ((major) << 8) | (minor))

/* Shader backends */

#define MAX_ATTRIBS 32
#define MAX_REG_ADDR 1
#define MAX_REG_TEXCRD 8
#define MAX_REG_INPUT 32
#define MAX_REG_OUTPUT 32
#define WINED3D_MAX_CBS 15

/* FIXME: This needs to go up to 2048 for
 * Shader model 3 according to msdn (and for software shaders) */
#define MAX_LABELS 16

#define MAX_IMMEDIATE_CONSTANT_BUFFER_SIZE 4096

struct wined3d_string_buffer
{
    struct list entry;
    char *buffer;
    unsigned int buffer_size;
    unsigned int content_size;
};

enum WINED3D_SHADER_INSTRUCTION_HANDLER
{
    WINED3DSIH_ABS,
    WINED3DSIH_ADD,
    WINED3DSIH_AND,
    WINED3DSIH_ATOMIC_AND,
    WINED3DSIH_ATOMIC_CMP_STORE,
    WINED3DSIH_ATOMIC_IADD,
    WINED3DSIH_ATOMIC_IMAX,
    WINED3DSIH_ATOMIC_IMIN,
    WINED3DSIH_ATOMIC_OR,
    WINED3DSIH_ATOMIC_UMAX,
    WINED3DSIH_ATOMIC_UMIN,
    WINED3DSIH_ATOMIC_XOR,
    WINED3DSIH_BEM,
    WINED3DSIH_BFI,
    WINED3DSIH_BFREV,
    WINED3DSIH_BREAK,
    WINED3DSIH_BREAKC,
    WINED3DSIH_BREAKP,
    WINED3DSIH_BUFINFO,
    WINED3DSIH_CALL,
    WINED3DSIH_CALLNZ,
    WINED3DSIH_CASE,
    WINED3DSIH_CMP,
    WINED3DSIH_CND,
    WINED3DSIH_CONTINUE,
    WINED3DSIH_CONTINUEP,
    WINED3DSIH_COUNTBITS,
    WINED3DSIH_CRS,
    WINED3DSIH_CUT,
    WINED3DSIH_CUT_STREAM,
    WINED3DSIH_DCL,
    WINED3DSIH_DCL_CONSTANT_BUFFER,
    WINED3DSIH_DCL_FUNCTION_BODY,
    WINED3DSIH_DCL_FUNCTION_TABLE,
    WINED3DSIH_DCL_GLOBAL_FLAGS,
    WINED3DSIH_DCL_GS_INSTANCES,
    WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT,
    WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT,
    WINED3DSIH_DCL_HS_MAX_TESSFACTOR,
    WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER,
    WINED3DSIH_DCL_INDEX_RANGE,
    WINED3DSIH_DCL_INDEXABLE_TEMP,
    WINED3DSIH_DCL_INPUT,
    WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT,
    WINED3DSIH_DCL_INPUT_PRIMITIVE,
    WINED3DSIH_DCL_INPUT_PS,
    WINED3DSIH_DCL_INPUT_PS_SGV,
    WINED3DSIH_DCL_INPUT_PS_SIV,
    WINED3DSIH_DCL_INPUT_SGV,
    WINED3DSIH_DCL_INPUT_SIV,
    WINED3DSIH_DCL_INTERFACE,
    WINED3DSIH_DCL_OUTPUT,
    WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT,
    WINED3DSIH_DCL_OUTPUT_SIV,
    WINED3DSIH_DCL_OUTPUT_TOPOLOGY,
    WINED3DSIH_DCL_RESOURCE_RAW,
    WINED3DSIH_DCL_RESOURCE_STRUCTURED,
    WINED3DSIH_DCL_SAMPLER,
    WINED3DSIH_DCL_STREAM,
    WINED3DSIH_DCL_TEMPS,
    WINED3DSIH_DCL_TESSELLATOR_DOMAIN,
    WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE,
    WINED3DSIH_DCL_TESSELLATOR_PARTITIONING,
    WINED3DSIH_DCL_TGSM_RAW,
    WINED3DSIH_DCL_TGSM_STRUCTURED,
    WINED3DSIH_DCL_THREAD_GROUP,
    WINED3DSIH_DCL_UAV_RAW,
    WINED3DSIH_DCL_UAV_STRUCTURED,
    WINED3DSIH_DCL_UAV_TYPED,
    WINED3DSIH_DCL_VERTICES_OUT,
    WINED3DSIH_DEF,
    WINED3DSIH_DEFAULT,
    WINED3DSIH_DEFB,
    WINED3DSIH_DEFI,
    WINED3DSIH_DIV,
    WINED3DSIH_DP2,
    WINED3DSIH_DP2ADD,
    WINED3DSIH_DP3,
    WINED3DSIH_DP4,
    WINED3DSIH_DST,
    WINED3DSIH_DSX,
    WINED3DSIH_DSX_COARSE,
    WINED3DSIH_DSX_FINE,
    WINED3DSIH_DSY,
    WINED3DSIH_DSY_COARSE,
    WINED3DSIH_DSY_FINE,
    WINED3DSIH_ELSE,
    WINED3DSIH_EMIT,
    WINED3DSIH_EMIT_STREAM,
    WINED3DSIH_ENDIF,
    WINED3DSIH_ENDLOOP,
    WINED3DSIH_ENDREP,
    WINED3DSIH_ENDSWITCH,
    WINED3DSIH_EQ,
    WINED3DSIH_EVAL_CENTROID,
    WINED3DSIH_EVAL_SAMPLE_INDEX,
    WINED3DSIH_EXP,
    WINED3DSIH_EXPP,
    WINED3DSIH_F16TOF32,
    WINED3DSIH_F32TOF16,
    WINED3DSIH_FCALL,
    WINED3DSIH_FIRSTBIT_HI,
    WINED3DSIH_FIRSTBIT_LO,
    WINED3DSIH_FIRSTBIT_SHI,
    WINED3DSIH_FRC,
    WINED3DSIH_FTOI,
    WINED3DSIH_FTOU,
    WINED3DSIH_GATHER4,
    WINED3DSIH_GATHER4_C,
    WINED3DSIH_GATHER4_PO,
    WINED3DSIH_GATHER4_PO_C,
    WINED3DSIH_GE,
    WINED3DSIH_HS_CONTROL_POINT_PHASE,
    WINED3DSIH_HS_DECLS,
    WINED3DSIH_HS_FORK_PHASE,
    WINED3DSIH_HS_JOIN_PHASE,
    WINED3DSIH_IADD,
    WINED3DSIH_IBFE,
    WINED3DSIH_IEQ,
    WINED3DSIH_IF,
    WINED3DSIH_IFC,
    WINED3DSIH_IGE,
    WINED3DSIH_ILT,
    WINED3DSIH_IMAD,
    WINED3DSIH_IMAX,
    WINED3DSIH_IMIN,
    WINED3DSIH_IMM_ATOMIC_ALLOC,
    WINED3DSIH_IMM_ATOMIC_AND,
    WINED3DSIH_IMM_ATOMIC_CMP_EXCH,
    WINED3DSIH_IMM_ATOMIC_CONSUME,
    WINED3DSIH_IMM_ATOMIC_EXCH,
    WINED3DSIH_IMM_ATOMIC_IADD,
    WINED3DSIH_IMM_ATOMIC_IMAX,
    WINED3DSIH_IMM_ATOMIC_IMIN,
    WINED3DSIH_IMM_ATOMIC_OR,
    WINED3DSIH_IMM_ATOMIC_UMAX,
    WINED3DSIH_IMM_ATOMIC_UMIN,
    WINED3DSIH_IMM_ATOMIC_XOR,
    WINED3DSIH_IMUL,
    WINED3DSIH_INE,
    WINED3DSIH_INEG,
    WINED3DSIH_ISHL,
    WINED3DSIH_ISHR,
    WINED3DSIH_ITOF,
    WINED3DSIH_LABEL,
    WINED3DSIH_LD,
    WINED3DSIH_LD2DMS,
    WINED3DSIH_LD_RAW,
    WINED3DSIH_LD_STRUCTURED,
    WINED3DSIH_LD_UAV_TYPED,
    WINED3DSIH_LIT,
    WINED3DSIH_LOD,
    WINED3DSIH_LOG,
    WINED3DSIH_LOGP,
    WINED3DSIH_LOOP,
    WINED3DSIH_LRP,
    WINED3DSIH_LT,
    WINED3DSIH_M3x2,
    WINED3DSIH_M3x3,
    WINED3DSIH_M3x4,
    WINED3DSIH_M4x3,
    WINED3DSIH_M4x4,
    WINED3DSIH_MAD,
    WINED3DSIH_MAX,
    WINED3DSIH_MIN,
    WINED3DSIH_MOV,
    WINED3DSIH_MOVA,
    WINED3DSIH_MOVC,
    WINED3DSIH_MUL,
    WINED3DSIH_NE,
    WINED3DSIH_NOP,
    WINED3DSIH_NOT,
    WINED3DSIH_NRM,
    WINED3DSIH_OR,
    WINED3DSIH_PHASE,
    WINED3DSIH_POW,
    WINED3DSIH_RCP,
    WINED3DSIH_REP,
    WINED3DSIH_RESINFO,
    WINED3DSIH_RET,
    WINED3DSIH_RETP,
    WINED3DSIH_ROUND_NE,
    WINED3DSIH_ROUND_NI,
    WINED3DSIH_ROUND_PI,
    WINED3DSIH_ROUND_Z,
    WINED3DSIH_RSQ,
    WINED3DSIH_SAMPLE,
    WINED3DSIH_SAMPLE_B,
    WINED3DSIH_SAMPLE_C,
    WINED3DSIH_SAMPLE_C_LZ,
    WINED3DSIH_SAMPLE_GRAD,
    WINED3DSIH_SAMPLE_INFO,
    WINED3DSIH_SAMPLE_LOD,
    WINED3DSIH_SAMPLE_POS,
    WINED3DSIH_SETP,
    WINED3DSIH_SGE,
    WINED3DSIH_SGN,
    WINED3DSIH_SINCOS,
    WINED3DSIH_SLT,
    WINED3DSIH_SQRT,
    WINED3DSIH_STORE_RAW,
    WINED3DSIH_STORE_STRUCTURED,
    WINED3DSIH_STORE_UAV_TYPED,
    WINED3DSIH_SUB,
    WINED3DSIH_SWAPC,
    WINED3DSIH_SWITCH,
    WINED3DSIH_SYNC,
    WINED3DSIH_TEX,
    WINED3DSIH_TEXBEM,
    WINED3DSIH_TEXBEML,
    WINED3DSIH_TEXCOORD,
    WINED3DSIH_TEXDEPTH,
    WINED3DSIH_TEXDP3,
    WINED3DSIH_TEXDP3TEX,
    WINED3DSIH_TEXKILL,
    WINED3DSIH_TEXLDD,
    WINED3DSIH_TEXLDL,
    WINED3DSIH_TEXM3x2DEPTH,
    WINED3DSIH_TEXM3x2PAD,
    WINED3DSIH_TEXM3x2TEX,
    WINED3DSIH_TEXM3x3,
    WINED3DSIH_TEXM3x3DIFF,
    WINED3DSIH_TEXM3x3PAD,
    WINED3DSIH_TEXM3x3SPEC,
    WINED3DSIH_TEXM3x3TEX,
    WINED3DSIH_TEXM3x3VSPEC,
    WINED3DSIH_TEXREG2AR,
    WINED3DSIH_TEXREG2GB,
    WINED3DSIH_TEXREG2RGB,
    WINED3DSIH_UBFE,
    WINED3DSIH_UDIV,
    WINED3DSIH_UGE,
    WINED3DSIH_ULT,
    WINED3DSIH_UMAX,
    WINED3DSIH_UMIN,
    WINED3DSIH_UMUL,
    WINED3DSIH_USHR,
    WINED3DSIH_UTOF,
    WINED3DSIH_XOR,
    WINED3DSIH_TABLE_SIZE
};

struct wined3d_shader_version
{
    enum wined3d_shader_type type;
    BYTE major;
    BYTE minor;
};

struct wined3d_shader_resource_info
{
    enum wined3d_shader_resource_type type;
    enum wined3d_data_type data_type;
    unsigned int flags;
    unsigned int stride;
};

#define WINED3D_SAMPLER_DEFAULT ~0x0u

struct wined3d_shader_sampler_map_entry
{
    unsigned int resource_idx;
    unsigned int sampler_idx;
    unsigned int bind_idx;
};

struct wined3d_shader_sampler_map
{
    struct wined3d_shader_sampler_map_entry *entries;
    size_t size;
    size_t count;
};

struct wined3d_shader_immediate_constant_buffer
{
    unsigned int vec4_count;
    uint32_t data[MAX_IMMEDIATE_CONSTANT_BUFFER_SIZE];
};

struct wined3d_shader_indexable_temp
{
    struct list entry;
    unsigned int register_idx;
    unsigned int register_size;
    unsigned int component_count;
};

#define WINED3D_SHADER_VERSION(major, minor) (((major) << 8) | (minor))

struct wined3d_shader_reg_maps
{
    struct wined3d_shader_version shader_version;
    BYTE texcoord;                                  /* MAX_REG_TEXCRD, 8 */
    BYTE address;                                   /* MAX_REG_ADDR, 1 */
    WORD labels;                                    /* MAX_LABELS, 16 */
    DWORD temporary;                                /* 32 */
    unsigned int temporary_count;
    uint32_t *constf;                               /* pixel, vertex */
    struct list indexable_temps;
    const struct wined3d_shader_immediate_constant_buffer *icb;
    union
    {
        DWORD texcoord_mask[MAX_REG_TEXCRD];        /* vertex < 3.0 */
        BYTE output_registers_mask[MAX_REG_OUTPUT]; /* vertex >= 3.0 */
    } u;
    DWORD input_registers;                          /* max(MAX_REG_INPUT, MAX_ATTRIBS), 32 */
    DWORD output_registers;                         /* MAX_REG_OUTPUT, 32 */
    WORD integer_constants;                         /* WINED3D_MAX_CONSTS_I, 16 */
    WORD boolean_constants;                         /* WINED3D_MAX_CONSTS_B, 16 */
    WORD local_int_consts;                          /* WINED3D_MAX_CONSTS_I, 16 */
    WORD local_bool_consts;                         /* WINED3D_MAX_CONSTS_B, 16 */
    UINT cb_sizes[WINED3D_MAX_CBS];
    uint32_t cb_map;                                /* WINED3D_MAX_CBS, 15 */

    struct wined3d_shader_resource_info resource_info[MAX_SHADER_RESOURCE_VIEWS];
    uint32_t resource_map[WINED3D_BITMAP_SIZE(MAX_SHADER_RESOURCE_VIEWS)];
    struct wined3d_shader_sampler_map sampler_map;
    DWORD sampler_comparison_mode;
    BYTE bumpmat;                                   /* WINED3D_MAX_FFP_TEXTURES, 8 */
    BYTE luminanceparams;                           /* WINED3D_MAX_FFP_TEXTURES, 8 */
    struct wined3d_shader_resource_info uav_resource_info[MAX_UNORDERED_ACCESS_VIEWS];
    DWORD uav_read_mask : 8;                        /* MAX_UNORDERED_ACCESS_VIEWS, 8 */
    DWORD uav_counter_mask : 8;                     /* MAX_UNORDERED_ACCESS_VIEWS, 8 */

    DWORD clip_distance_mask : 8;                   /* WINED3D_MAX_CLIP_DISTANCES, 8 */
    DWORD cull_distance_mask : 8;                   /* WINED3D_MAX_CLIP_DISTANCES, 8 */
    DWORD usesnrm        : 1;
    DWORD vpos           : 1;
    DWORD usesdsx        : 1;
    DWORD usesdsy        : 1;
    DWORD usestexldd     : 1;
    DWORD usesmova       : 1;
    DWORD usesfacing     : 1;
    DWORD usesrelconstF  : 1;
    DWORD fog            : 1;
    DWORD usestexldl     : 1;
    DWORD usesifc        : 1;
    DWORD usescall       : 1;
    DWORD usespow        : 1;
    DWORD point_size     : 1;
    DWORD vocp           : 1;
    DWORD input_rel_addressing : 1;
    DWORD viewport_array : 1;
    DWORD sample_mask    : 1;
    DWORD stencil_ref    : 1;

    DWORD rt_mask; /* Used render targets, 32 max. */

    /* Whether or not loops are used in this shader, and nesting depth */
    unsigned int loop_depth;
    unsigned int min_rel_offset, max_rel_offset;

    struct wined3d_shader_tgsm *tgsm;
    SIZE_T tgsm_capacity;
    unsigned int tgsm_count;
};

/* Keeps track of details for TEX_M#x# instructions which need to maintain
 * state information between multiple instructions. */
struct wined3d_shader_tex_mx
{
    unsigned int current_row;
    unsigned int texcoord_w[2];
};

struct wined3d_shader_parser_state
{
    unsigned int current_loop_depth;
    unsigned int current_loop_reg;
    BOOL in_subroutine;
};

struct wined3d_shader_context
{
    const struct wined3d_shader *shader;
    const struct wined3d_shader_reg_maps *reg_maps;
    struct wined3d_string_buffer *buffer;
    struct wined3d_shader_tex_mx *tex_mx;
    struct wined3d_shader_parser_state *state;
    void *backend_data;
};

struct wined3d_shader_register_index
{
    const struct wined3d_shader_src_param *rel_addr;
    unsigned int offset;
};

struct wined3d_shader_register
{
    enum wined3d_shader_register_type type;
    enum wined3d_data_type data_type;
    struct wined3d_shader_register_index idx[2];
    enum wined3d_immconst_type immconst_type;
    union
    {
        unsigned int immconst_data[4];
        unsigned fp_body_idx;
    } u;
};

struct wined3d_shader_dst_param
{
    struct wined3d_shader_register reg;
    uint32_t write_mask;
    uint32_t modifiers;
    unsigned int shift;
};

struct wined3d_shader_src_param
{
    struct wined3d_shader_register reg;
    DWORD swizzle;
    enum wined3d_shader_src_modifier modifiers;
};

struct wined3d_shader_index_range
{
    struct wined3d_shader_dst_param first_register;
    unsigned int last_register;
};

struct wined3d_shader_semantic
{
    enum wined3d_decl_usage usage;
    UINT usage_idx;
    enum wined3d_shader_resource_type resource_type;
    unsigned int sample_count;
    enum wined3d_data_type resource_data_type;
    struct wined3d_shader_dst_param reg;
};

enum wined3d_shader_input_sysval_semantic
{
    WINED3D_SIV_POSITION                     = 1,
    WINED3D_SIV_CLIP_DISTANCE                = 2,
    WINED3D_SIV_CULL_DISTANCE                = 3,
    WINED3D_SIV_RENDER_TARGET_ARRAY_INDEX    = 4,
    WINED3D_SIV_VIEWPORT_ARRAY_INDEX         = 5,
    WINED3D_SIV_VERTEX_ID                    = 6,
    WINED3D_SIV_PRIMITIVE_ID                 = 7,
    WINED3D_SIV_INSTANCE_ID                  = 8,
    WINED3D_SIV_IS_FRONT_FACE                = 9,
    WINED3D_SIV_SAMPLE_INDEX                 = 10,
    WINED3D_SIV_QUAD_U0_TESS_FACTOR          = 11,
    WINED3D_SIV_QUAD_V0_TESS_FACTOR          = 12,
    WINED3D_SIV_QUAD_U1_TESS_FACTOR          = 13,
    WINED3D_SIV_QUAD_V1_TESS_FACTOR          = 14,
    WINED3D_SIV_QUAD_U_INNER_TESS_FACTOR     = 15,
    WINED3D_SIV_QUAD_V_INNER_TESS_FACTOR     = 16,
    WINED3D_SIV_TRIANGLE_U_TESS_FACTOR       = 17,
    WINED3D_SIV_TRIANGLE_V_TESS_FACTOR       = 18,
    WINED3D_SIV_TRIANGLE_W_TESS_FACTOR       = 19,
    WINED3D_SIV_TRIANGLE_INNER_TESS_FACTOR   = 20,
    WINED3D_SIV_LINE_DETAIL_TESS_FACTOR      = 21,
    WINED3D_SIV_LINE_DENSITY_TESS_FACTOR     = 22,
};

struct wined3d_shader_register_semantic
{
    struct wined3d_shader_dst_param reg;
    enum wined3d_shader_input_sysval_semantic sysval_semantic;
};

struct wined3d_shader_structured_resource
{
    struct wined3d_shader_dst_param reg;
    unsigned int byte_stride;
};

struct wined3d_shader_tgsm
{
    unsigned int size;
    unsigned int stride;
};

struct wined3d_shader_tgsm_raw
{
    struct wined3d_shader_dst_param reg;
    unsigned int byte_count;
};

struct wined3d_shader_tgsm_structured
{
    struct wined3d_shader_dst_param reg;
    unsigned int byte_stride;
    unsigned int structure_count;
};

struct wined3d_shader_thread_group_size
{
    unsigned int x, y, z;
};

struct wined3d_shader_function_table_pointer
{
    unsigned int index;
    unsigned int array_size;
    unsigned int body_count;
    unsigned int table_count;
};

struct wined3d_shader_texel_offset
{
    signed char u, v, w;
};

struct wined3d_shader_primitive_type
{
    enum wined3d_primitive_type type;
    unsigned int patch_vertex_count;
};

struct wined3d_shader_instruction
{
    const struct wined3d_shader_context *ctx;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    uint32_t flags;
    unsigned int dst_count;
    unsigned int src_count;
    const struct wined3d_shader_dst_param *dst;
    const struct wined3d_shader_src_param *src;
    struct wined3d_shader_texel_offset texel_offset;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_data_type resource_data_type;
    BOOL coissue;
    const struct wined3d_shader_src_param *predicate;
    union
    {
        struct wined3d_shader_semantic semantic;
        struct wined3d_shader_register_semantic register_semantic;
        struct wined3d_shader_primitive_type primitive_type;
        struct wined3d_shader_dst_param dst;
        struct wined3d_shader_src_param src;
        unsigned int count;
        unsigned int index;
        const struct wined3d_shader_immediate_constant_buffer *icb;
        struct wined3d_shader_structured_resource structured_resource;
        struct wined3d_shader_tgsm_raw tgsm_raw;
        struct wined3d_shader_tgsm_structured tgsm_structured;
        struct wined3d_shader_thread_group_size thread_group_size;
        enum wined3d_tessellator_domain tessellator_domain;
        enum wined3d_tessellator_output_primitive tessellator_output_primitive;
        enum wined3d_tessellator_partitioning tessellator_partitioning;
        float max_tessellation_factor;
        struct wined3d_shader_index_range index_range;
        struct wined3d_shader_indexable_temp indexable_temp;
        struct wined3d_shader_function_table_pointer fp;
    } declaration;
};

static inline BOOL wined3d_shader_instruction_has_texel_offset(const struct wined3d_shader_instruction *ins)
{
    return ins->texel_offset.u || ins->texel_offset.v || ins->texel_offset.w;
}

struct wined3d_shader_attribute
{
    enum wined3d_decl_usage usage;
    UINT usage_idx;
};

struct wined3d_shader_loop_control
{
    unsigned int count;
    unsigned int start;
    int step;
};

struct wined3d_shader_frontend
{
    void *(*shader_init)(const DWORD *byte_code, size_t byte_code_size,
            const struct wined3d_shader_signature *output_signature);
    void (*shader_free)(void *data);
    void (*shader_read_header)(void *data, const DWORD **ptr, struct wined3d_shader_version *shader_version);
    void (*shader_read_instruction)(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins);
    BOOL (*shader_is_end)(void *data, const DWORD **ptr);
};

extern const struct wined3d_shader_frontend sm1_shader_frontend;
extern const struct wined3d_shader_frontend sm4_shader_frontend;

HRESULT wined3d_shader_extract_from_dxbc(struct wined3d_shader *shader,
        unsigned int max_shader_version, enum vkd3d_shader_source_type *source_type);
BOOL shader_get_stream_output_register_info(const struct wined3d_shader *shader,
        const struct wined3d_stream_output_element *so_element, unsigned int *register_idx,
        unsigned int *component_idx);

typedef void (*SHADER_HANDLER)(const struct wined3d_shader_instruction *);

#define WINED3D_SHADER_CAP_DOUBLE_PRECISION         0x00000004u
#define WINED3D_SHADER_CAP_OUTPUT_INTERPOLATION     0x00000008u
#define WINED3D_SHADER_CAP_FULL_FFP_VARYINGS        0x00000010u

struct shader_caps
{
    unsigned int vs_version;
    unsigned int hs_version;
    unsigned int ds_version;
    unsigned int gs_version;
    unsigned int ps_version;
    unsigned int cs_version;

    unsigned int vs_uniform_count;
    unsigned int ps_uniform_count;
    float ps_1x_max_value;
    unsigned int varying_count;

    DWORD wined3d_caps;
};

enum wined3d_gl_resource_type
{
    WINED3D_GL_RES_TYPE_TEX_1D          = 0,
    WINED3D_GL_RES_TYPE_TEX_2D          = 1,
    WINED3D_GL_RES_TYPE_TEX_3D          = 2,
    WINED3D_GL_RES_TYPE_TEX_CUBE        = 3,
    WINED3D_GL_RES_TYPE_BUFFER          = 4,
    WINED3D_GL_RES_TYPE_RB              = 5,
    WINED3D_GL_RES_TYPE_COUNT           = 6,
};

enum wined3d_vertex_processing_mode
{
    WINED3D_VP_MODE_FF,
    WINED3D_VP_MODE_SHADER,
    WINED3D_VP_MODE_NONE,
};

#define WINED3D_CONST_NUM_UNUSED ~0U

enum wined3d_ffp_ps_fog_mode
{
    WINED3D_FFP_PS_FOG_OFF,
    WINED3D_FFP_PS_FOG_LINEAR,
    WINED3D_FFP_PS_FOG_EXP,
    WINED3D_FFP_PS_FOG_EXP2,
};

/* Stateblock dependent parameters which have to be hardcoded
 * into the shader code
 */

#define WINED3D_PSARGS_PROJECTED (1u << 3)
#define WINED3D_PSARGS_TEXTRANSFORM_SHIFT 4
#define WINED3D_PSARGS_TEXTRANSFORM_MASK 0xfu
#define WINED3D_PSARGS_TEXTYPE_SHIFT 2
#define WINED3D_PSARGS_TEXTYPE_MASK 0x3u

/* Used for Shader Model 1 pixel shaders to track the bound texture type. */
enum wined3d_shader_tex_types
{
    WINED3D_SHADER_TEX_2D   = 0,
    WINED3D_SHADER_TEX_3D   = 1,
    WINED3D_SHADER_TEX_CUBE = 2,
    WINED3D_SHADER_TEX_ERR  = 3,
};

struct ps_compile_args
{
    struct color_fixup_desc     color_fixup[WINED3D_MAX_FRAGMENT_SAMPLERS];
    enum wined3d_vertex_processing_mode vp_mode;
    enum wined3d_ffp_ps_fog_mode fog;
    DWORD                       tex_types; /* ps 1 - 3, 16 textures */
    WORD                        tex_transform; /* ps 1.0-1.3, 4 textures */
    WORD                        srgb_correction;
    WORD shadow; /* WINED3D_MAX_FRAGMENT_SAMPLERS, 16 */
    WORD texcoords_initialized; /* WINED3D_MAX_FFP_TEXTURES, 8 */
    DWORD pointsprite : 1;
    DWORD flatshading : 1;
    DWORD alpha_test_func : 3;
    DWORD rt_alpha_swizzle : 8; /* WINED3D_MAX_RENDER_TARGETS, 8 */
    DWORD dual_source_blend : 1;
    DWORD padding : 18;
};

enum fog_src_type
{
    VS_FOG_Z,
    VS_FOG_COORD,
    VS_FOG_W,
};

struct vs_compile_args
{
    DWORD swizzle_map;   /* MAX_ATTRIBS, 32 */
    unsigned int next_shader_input_count;
    uint32_t interpolation_mode[WINED3D_PACKED_INTERPOLATION_SIZE];
    BYTE fog_src;
    BYTE clip_enabled : 1;
    BYTE point_size : 1;
    BYTE flatshading : 1;
    BYTE next_shader_type : 3;
    BYTE padding : 2;
};

struct ds_compile_args
{
    enum wined3d_tessellator_output_primitive tessellator_output_primitive;
    enum wined3d_tessellator_partitioning tessellator_partitioning;
    unsigned int output_count : 16;
    unsigned int next_shader_type : 3;
    unsigned int padding : 13;
    uint32_t interpolation_mode[WINED3D_PACKED_INTERPOLATION_SIZE];
};

struct gs_compile_args
{
    unsigned int output_count;
    enum wined3d_primitive_type primitive_type;
    uint32_t interpolation_mode[WINED3D_PACKED_INTERPOLATION_SIZE];
};

struct wined3d_shader_backend_ops
{
    void (*shader_handle_instruction)(const struct wined3d_shader_instruction *);
    void (*shader_precompile)(void *shader_priv, struct wined3d_shader *shader);
    void (*shader_apply_draw_state)(void *shader_priv, struct wined3d_context *context,
            const struct wined3d_state *state);
    void (*shader_apply_compute_state)(void *shader_priv, struct wined3d_context *context,
            const struct wined3d_state *state);
    void (*shader_disable)(void *shader_priv, struct wined3d_context *context);
    void (*shader_update_float_vertex_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_update_float_pixel_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_destroy)(struct wined3d_shader *shader);
    HRESULT (*shader_alloc_private)(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
            const struct wined3d_fragment_pipe_ops *fragment_pipe);
    void (*shader_free_private)(struct wined3d_device *device, struct wined3d_context *context);
    BOOL (*shader_allocate_context_data)(struct wined3d_context *context);
    void (*shader_free_context_data)(struct wined3d_context *context);
    void (*shader_init_context_state)(struct wined3d_context *context);
    void (*shader_get_caps)(const struct wined3d_adapter *adapter, struct shader_caps *caps);
    BOOL (*shader_color_fixup_supported)(struct color_fixup_desc fixup);
    uint64_t (*shader_compile)(struct wined3d_context *context, const struct wined3d_shader_desc *shader_desc,
        enum wined3d_shader_type shader_type);
};

extern const struct wined3d_shader_backend_ops glsl_shader_backend;
extern const struct wined3d_shader_backend_ops none_shader_backend;

const struct wined3d_shader_backend_ops *wined3d_spirv_shader_backend_init_vk(void);

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xff)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xff)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xff)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xff)

static inline void wined3d_color_from_d3dcolor(struct wined3d_color *wined3d_color, DWORD d3d_color)
{
    wined3d_color->r = D3DCOLOR_B_R(d3d_color) / 255.0f;
    wined3d_color->g = D3DCOLOR_B_G(d3d_color) / 255.0f;
    wined3d_color->b = D3DCOLOR_B_B(d3d_color) / 255.0f;
    wined3d_color->a = D3DCOLOR_B_A(d3d_color) / 255.0f;
}

extern const struct wined3d_vec4 wined3d_srgb_const[];

static inline float wined3d_srgb_from_linear(float colour)
{
    if (colour < 0.0f)
        return 0.0f;
    if (colour < wined3d_srgb_const[1].x)
        return colour * wined3d_srgb_const[0].w;
    if (colour < 1.0f)
        return wined3d_srgb_const[0].y * powf(colour, wined3d_srgb_const[0].x) - wined3d_srgb_const[0].z;
    return 1.0f;
}

static inline void wined3d_colour_srgb_from_linear(struct wined3d_color *colour_srgb,
        const struct wined3d_color *colour)
{
    colour_srgb->r = wined3d_srgb_from_linear(colour->r);
    colour_srgb->g = wined3d_srgb_from_linear(colour->g);
    colour_srgb->b = wined3d_srgb_from_linear(colour->b);
    colour_srgb->a = colour->a;
}

struct wined3d_bo
{
    /* client_map_count and map_ptr are accessed from both the client and CS
     * threads, and protected by wined3d_device.bo_map_lock. */
    struct list users;
    void *map_ptr;
    size_t buffer_offset;
    size_t memory_offset;
    unsigned int client_map_count;
    bool coherent;
    /* Number of resources referencing this BO, used for COW tracking.
     * If a resource has this BO as a location and wants to write to it, it
     * needs to make a copy unless it's the only owner (refcount == 1).
     * Deferred contexts may also hold a reference. */
    uint8_t refcount;
};

struct wined3d_bo_user
{
    struct list entry;
    bool valid;
};

struct wined3d_bo_address
{
    struct wined3d_bo *buffer_object;
    BYTE *addr;
};

struct wined3d_const_bo_address
{
    struct wined3d_bo *buffer_object;
    const BYTE *addr;
};

static inline struct wined3d_const_bo_address *wined3d_const_bo_address(const struct wined3d_bo_address *data)
{
    return (struct wined3d_const_bo_address *)data;
}

struct wined3d_stream_info_element
{
    const struct wined3d_format *format;
    struct wined3d_bo_address data;
    unsigned int stride;
    unsigned int stream_idx;
    unsigned int divisor;
    bool instanced;
};

struct wined3d_stream_info
{
    struct wined3d_stream_info_element elements[MAX_ATTRIBS];
    DWORD position_transformed : 1;
    DWORD all_vbo : 1;
    DWORD swizzle_map; /* MAX_ATTRIBS, 32 */
    DWORD use_map; /* MAX_ATTRIBS, 32 */
};

void wined3d_stream_info_from_declaration(struct wined3d_stream_info *stream_info,
        const struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info);

struct wined3d_direct_dispatch_parameters
{
    unsigned int group_count_x;
    unsigned int group_count_y;
    unsigned int group_count_z;
};

struct wined3d_indirect_dispatch_parameters
{
    struct wined3d_buffer *buffer;
    unsigned int offset;
};

struct wined3d_dispatch_parameters
{
    BOOL indirect;
    union
    {
        struct wined3d_direct_dispatch_parameters direct;
        struct wined3d_indirect_dispatch_parameters indirect;
    } u;
};

struct wined3d_direct_draw_parameters
{
    int base_vertex_idx;
    unsigned int start_idx;
    unsigned int index_count;
    unsigned int start_instance;
    unsigned int instance_count;
};

struct wined3d_indirect_draw_parameters
{
    struct wined3d_buffer *buffer;
    unsigned int offset;
};

struct wined3d_draw_parameters
{
    BOOL indirect;
    union
    {
        struct wined3d_direct_draw_parameters direct;
        struct wined3d_indirect_draw_parameters indirect;
    } u;
    BOOL indexed;
};

void draw_primitive(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_draw_parameters *draw_parameters);
void dispatch_compute(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_dispatch_parameters *dispatch_parameters);

#define eps 1e-8f

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

/* Routines and structures related to state management */

#define STATE_RENDER(a) (a)
#define STATE_IS_RENDER(a) ((a) >= STATE_RENDER(1) && (a) <= STATE_RENDER(WINEHIGHEST_RENDER_STATE))

#define STATE_TEXTURESTAGE(stage, num) \
    (STATE_RENDER(WINEHIGHEST_RENDER_STATE) + 1 + (stage) * (WINED3D_HIGHEST_TEXTURE_STATE + 1) + (num))
#define STATE_IS_TEXTURESTAGE(a) \
    ((a) >= STATE_TEXTURESTAGE(0, 1) && (a) <= STATE_TEXTURESTAGE(WINED3D_MAX_FFP_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE))

#define STATE_GRAPHICS_SHADER(a) (STATE_TEXTURESTAGE(WINED3D_MAX_FFP_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE) + 1 + (a))
#define STATE_IS_GRAPHICS_SHADER(a) \
    ((a) >= STATE_GRAPHICS_SHADER(0) && (a) < STATE_GRAPHICS_SHADER(WINED3D_SHADER_TYPE_GRAPHICS_COUNT))

#define STATE_GRAPHICS_CONSTANT_BUFFER(a) (STATE_GRAPHICS_SHADER(WINED3D_SHADER_TYPE_GRAPHICS_COUNT) + (a))
#define STATE_IS_GRAPHICS_CONSTANT_BUFFER(a) \
    ((a) >= STATE_GRAPHICS_CONSTANT_BUFFER(0) \
    && (a) < STATE_GRAPHICS_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GRAPHICS_COUNT))

#define STATE_GRAPHICS_SHADER_RESOURCE_BINDING (STATE_GRAPHICS_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GRAPHICS_COUNT))
#define STATE_IS_GRAPHICS_SHADER_RESOURCE_BINDING(a) ((a) == STATE_GRAPHICS_SHADER_RESOURCE_BINDING)

#define STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING (STATE_GRAPHICS_SHADER_RESOURCE_BINDING + 1)
#define STATE_IS_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING(a) ((a) == STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING)

#define STATE_STREAMSRC (STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING + 1)
#define STATE_IS_STREAMSRC(a) ((a) == STATE_STREAMSRC)
#define STATE_INDEXBUFFER (STATE_STREAMSRC + 1)
#define STATE_IS_INDEXBUFFER(a) ((a) == STATE_INDEXBUFFER)

#define STATE_VDECL (STATE_INDEXBUFFER + 1)
#define STATE_IS_VDECL(a) ((a) == STATE_VDECL)

#define STATE_VIEWPORT (STATE_VDECL + 1)
#define STATE_IS_VIEWPORT(a) ((a) == STATE_VIEWPORT)

#define STATE_SCISSORRECT (STATE_VIEWPORT + 1)
#define STATE_IS_SCISSORRECT(a) ((a) == STATE_SCISSORRECT)

#define STATE_CLIPPLANE(a) (STATE_SCISSORRECT + 1 + (a))
#define STATE_IS_CLIPPLANE(a) ((a) >= STATE_CLIPPLANE(0) && (a) <= STATE_CLIPPLANE(WINED3D_MAX_CLIP_DISTANCES - 1))

#define STATE_RASTERIZER (STATE_CLIPPLANE(WINED3D_MAX_CLIP_DISTANCES))
#define STATE_IS_RASTERIZER(a) ((a) == STATE_RASTERIZER)

#define STATE_DEPTH_BOUNDS (STATE_RASTERIZER + 1)
#define STATE_IS_DEPTH_BOUNDS(a) ((a) == STATE_DEPTH_BOUNDS)

#define STATE_BASEVERTEXINDEX  (STATE_DEPTH_BOUNDS + 1)
#define STATE_IS_BASEVERTEXINDEX(a) ((a) == STATE_BASEVERTEXINDEX)

#define STATE_FRAMEBUFFER (STATE_BASEVERTEXINDEX + 1)
#define STATE_IS_FRAMEBUFFER(a) ((a) == STATE_FRAMEBUFFER)

#define STATE_STREAM_OUTPUT (STATE_FRAMEBUFFER + 1)
#define STATE_IS_STREAM_OUTPUT(a) ((a) == STATE_STREAM_OUTPUT)

#define STATE_BLEND (STATE_STREAM_OUTPUT + 1)
#define STATE_IS_BLEND(a) ((a) == STATE_BLEND)

#define STATE_BLEND_FACTOR (STATE_BLEND + 1)
#define STATE_IS_BLEND_FACTOR(a) ((a) == STATE_BLEND_FACTOR)

#define STATE_SAMPLE_MASK (STATE_BLEND_FACTOR + 1)
#define STATE_IS_SAMPLE_MASK(a) ((a) == STATE_SAMPLE_MASK)

#define STATE_DEPTH_STENCIL (STATE_SAMPLE_MASK + 1)
#define STATE_IS_DEPTH_STENCIL(a) ((a) == STATE_DEPTH_STENCIL)

#define STATE_STENCIL_REF (STATE_DEPTH_STENCIL + 1)
#define STATE_IS_STENCIL_REF(a) ((a) == STATE_STENCIL_REF)

#define STATE_COMPUTE_OFFSET (STATE_STENCIL_REF + 1)

#define STATE_COMPUTE_SHADER (STATE_COMPUTE_OFFSET)
#define STATE_IS_COMPUTE_SHADER(a) ((a) == STATE_COMPUTE_SHADER)

#define STATE_COMPUTE_CONSTANT_BUFFER (STATE_COMPUTE_SHADER + 1)
#define STATE_IS_COMPUTE_CONSTANT_BUFFER(a) ((a) == STATE_COMPUTE_CONSTANT_BUFFER)

#define STATE_COMPUTE_SHADER_RESOURCE_BINDING (STATE_COMPUTE_CONSTANT_BUFFER + 1)
#define STATE_IS_COMPUTE_SHADER_RESOURCE_BINDING(a) ((a) == STATE_COMPUTE_SHADER_RESOURCE_BINDING)

#define STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING (STATE_COMPUTE_SHADER_RESOURCE_BINDING + 1)
#define STATE_IS_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING(a) ((a) == STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING)

#define STATE_COMPUTE_HIGHEST (STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING)
#define STATE_HIGHEST (STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING)

#define STATE_IS_COMPUTE(a) ((a) >= STATE_COMPUTE_OFFSET && (a) <= STATE_COMPUTE_HIGHEST)
#define STATE_COMPUTE_COUNT (STATE_COMPUTE_HIGHEST - STATE_COMPUTE_OFFSET + 1)

#define STATE_SHADER(a) ((a) != WINED3D_SHADER_TYPE_COMPUTE ? STATE_GRAPHICS_SHADER(a) : STATE_COMPUTE_SHADER)
#define STATE_CONSTANT_BUFFER(a) \
    ((a) != WINED3D_SHADER_TYPE_COMPUTE ? STATE_GRAPHICS_CONSTANT_BUFFER(a) : STATE_COMPUTE_CONSTANT_BUFFER)
#define STATE_UNORDERED_ACCESS_VIEW_BINDING(a) ((a) == WINED3D_PIPELINE_GRAPHICS ? \
    STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING : STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING)

enum fogsource {
    FOGSOURCE_FFP,
    FOGSOURCE_VS,
};

/* Direct3D terminology with little modifications. We do not have an issued
 * state because only the driver knows about it, but we have a created state
 * because D3D allows GetData() on a created query, but OpenGL doesn't. */
enum wined3d_query_state
{
    QUERY_CREATED,
    QUERY_SIGNALLED,
    QUERY_BUILDING
};

struct wined3d_query_ops
{
    BOOL (*query_poll)(struct wined3d_query *query, uint32_t flags);
    BOOL (*query_issue)(struct wined3d_query *query, uint32_t flags);
    void (*query_destroy)(struct wined3d_query *query);
};

struct wined3d_query
{
    LONG ref;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    struct wined3d_device *device;
    enum wined3d_query_state state;
    enum wined3d_query_type type;
    const void *data;
    DWORD data_size;
    const struct wined3d_query_ops *query_ops;

    LONG counter_main, counter_retrieved;
    struct list poll_list_entry;

    /* FIXME: This is GL-specific. */
    unsigned int buffer_object;
    UINT64 *map_ptr;
    bool poll_in_cs;
};

#define WINED3D_QUERY_POOL_SIZE 256

struct wined3d_range
{
    unsigned int offset;
    unsigned int size;
};

struct wined3d_fb_state
{
    struct wined3d_rendertarget_view *render_targets[WINED3D_MAX_RENDER_TARGETS];
    struct wined3d_rendertarget_view *depth_stencil;
};

struct wined3d_context
{
    const struct wined3d_d3d_info *d3d_info;
    const struct wined3d_state_entry *state_table;
    uint32_t dirty_graphics_states[WINED3D_BITMAP_SIZE(STATE_HIGHEST)];
    uint32_t dirty_compute_states[WINED3D_BITMAP_SIZE(STATE_COMPUTE_COUNT)];

    struct wined3d_device *device;
    struct wined3d_swapchain *swapchain;
    struct
    {
        struct wined3d_texture *texture;
        unsigned int sub_resource_idx;
    } current_rt;

    /* Stores some information about the context state for optimization */
    DWORD last_swizzle_map; /* MAX_ATTRIBS, 32 */

    DWORD shader_update_mask : 6; /* WINED3D_SHADER_TYPE_COUNT, 6 */
    DWORD update_shader_resource_bindings : 1;
    DWORD update_compute_shader_resource_bindings : 1;
    DWORD last_was_rhw : 1; /* True iff last draw_primitive was in xyzrhw mode. */
    DWORD last_was_ffp_blit : 1;
    DWORD last_was_blit : 1;
    DWORD last_was_dual_source_blend : 1;
    DWORD lowest_disabled_stage : 4;    /* Max WINED3D_MAX_FFP_TEXTURES, 8 */

    DWORD fixed_function_usage_map : 8; /* WINED3D_MAX_FFP_TEXTURES, 8 */
    DWORD uses_uavs : 1;
    DWORD uses_fbo_attached_resources : 1;
    DWORD transform_feedback_active : 1;
    DWORD transform_feedback_paused : 1;
    DWORD current : 1;
    DWORD destroyed : 1;
    DWORD destroy_delayed : 1;
    DWORD update_unordered_access_view_bindings : 1;
    DWORD update_compute_unordered_access_view_bindings : 1;
    DWORD update_primitive_type : 1;
    DWORD update_multisample_state : 1;
    DWORD update_patch_vertex_count : 1;
    DWORD padding : 28;

    DWORD clip_distance_mask : 8; /* WINED3D_MAX_CLIP_DISTANCES, 8 */

    DWORD constant_update_mask;
    DWORD numbered_array_mask;
    enum fogsource fog_source;


    void *shader_backend_data;

    struct wined3d_stream_info stream_info;

    unsigned int viewport_count;
    unsigned int scissor_rect_count;
};

void wined3d_context_cleanup(struct wined3d_context *context);
void wined3d_context_init(struct wined3d_context *context, struct wined3d_swapchain *swapchain);
void context_update_stream_info(struct wined3d_context *context, const struct wined3d_state *state);

HRESULT wined3d_context_no3d_init(struct wined3d_context *context_no3d,
        struct wined3d_swapchain *swapchain);

typedef void (*APPLYSTATEFUNC)(struct wined3d_context *ctx, const struct wined3d_state *state, DWORD state_id);

struct wined3d_state_entry
{
    unsigned int representative;
    APPLYSTATEFUNC apply;
};

struct wined3d_state_entry_template
{
    unsigned int state;
    struct wined3d_state_entry content;
    unsigned int extension;
};

#define GL_EXT_EMUL_ARB_MULTITEXTURE 0x00000001
#define GL_EXT_EMUL_EXT_FOG_COORD    0x00000002

struct wined3d_fragment_pipe_ops
{
    void (*fp_apply_draw_state)(struct wined3d_context *context, const struct wined3d_state *state);
    void (*fp_disable)(const struct wined3d_context *context);
    void (*get_caps)(const struct wined3d_adapter *adapter, struct fragment_caps *caps);
    unsigned int (*get_emul_mask)(const struct wined3d_adapter *adapter);
    void *(*alloc_private)(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv);
    void (*free_private)(struct wined3d_device *device, struct wined3d_context *context);
    BOOL (*allocate_context_data)(struct wined3d_context *context);
    void (*free_context_data)(struct wined3d_context *context);
    BOOL (*color_fixup_supported)(struct color_fixup_desc fixup);
    const struct wined3d_state_entry_template *states;
};

struct wined3d_vertex_caps
{
    BOOL emulated_flatshading;
    DWORD max_active_lights;
    DWORD max_vertex_blend_matrices;
    DWORD max_vertex_blend_matrix_index;
    DWORD vertex_processing_caps;
    DWORD fvf_caps;
    DWORD max_user_clip_planes;
    DWORD raster_caps;
};

struct wined3d_vertex_pipe_ops
{
    void (*vp_apply_draw_state)(struct wined3d_context *context, const struct wined3d_state *state);
    void (*vp_disable)(const struct wined3d_context *context);
    void (*vp_get_caps)(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps);
    unsigned int (*vp_get_emul_mask)(const struct wined3d_adapter *adapter);
    void *(*vp_alloc)(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv);
    void (*vp_free)(struct wined3d_device *device, struct wined3d_context *context);
    const struct wined3d_state_entry_template *vp_states;
};

extern const struct wined3d_state_entry_template misc_state_template_gl[];
extern const struct wined3d_fragment_pipe_ops none_fragment_pipe;
extern const struct wined3d_fragment_pipe_ops glsl_fragment_pipe;

const struct wined3d_fragment_pipe_ops *wined3d_spirv_fragment_pipe_init_vk(void);

extern const struct wined3d_vertex_pipe_ops none_vertex_pipe;
extern const struct wined3d_vertex_pipe_ops glsl_vertex_pipe;

const struct wined3d_vertex_pipe_ops *wined3d_spirv_vertex_pipe_init_vk(void);

/* "Base" state table */
HRESULT compile_state_table(struct wined3d_state_entry *state_table, APPLYSTATEFUNC **dev_multistate_funcs,
        const struct wined3d_d3d_info *d3d_info, const BOOL *supported_extensions,
        const struct wined3d_vertex_pipe_ops *vertex, const struct wined3d_fragment_pipe_ops *fragment,
        const struct wined3d_state_entry_template *misc);

enum wined3d_blit_op
{
    WINED3D_BLIT_OP_COLOR_BLIT,
    WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST,
    WINED3D_BLIT_OP_COLOR_BLIT_CKEY,
    WINED3D_BLIT_OP_DEPTH_BLIT,
    WINED3D_BLIT_OP_RAW_BLIT,
};

struct wined3d_blitter
{
    const struct wined3d_blitter_ops *ops;
    struct wined3d_blitter *next;
};

struct wined3d_blitter_ops
{
    void (*blitter_destroy)(struct wined3d_blitter *blitter, struct wined3d_context *context);
    void (*blitter_clear)(struct wined3d_blitter *blitter, struct wined3d_device *device,
            unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
            const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil);
    DWORD (*blitter_blit)(struct wined3d_blitter *blitter, enum wined3d_blit_op op, struct wined3d_context *context,
            struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, DWORD src_location,
            const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
            DWORD dst_location, const RECT *dst_rect, const struct wined3d_color_key *colour_key,
            enum wined3d_texture_filter_type filter, const struct wined3d_format *resolve_format);
};

struct wined3d_blitter *wined3d_cpu_blitter_create(void);
void wined3d_vk_blitter_create(struct wined3d_blitter **next);

void cpu_blitter_clear_texture(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_box *box, const struct wined3d_color *colour);

BOOL wined3d_clip_blit(const RECT *clip_rect, RECT *clipped, RECT *other);

void context_invalidate_compute_state(struct wined3d_context *context, DWORD state_id);
void context_invalidate_state(struct wined3d_context *context, unsigned int state_id);
void context_resource_released(const struct wined3d_device *device, struct wined3d_resource *resource);
void context_restore(struct wined3d_context *context, struct wined3d_texture *texture,
        unsigned int sub_resource_idx);
void context_set_tls_idx(DWORD idx);
void context_state_drawbuf(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);
void context_state_fb(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);

struct wined3d_light_constants
{
    /* Padding is needed for the HLSL backend. */
    struct wined3d_color diffuse, specular, ambient;
    struct wined3d_vec4 position, direction;
    float range, falloff, cos_half_theta, cos_half_phi;
    float const_att, linear_att, quad_att, padding;
};

/*****************************************************************************
 * Internal representation of a light
 */
struct wined3d_light_info
{
    struct wined3d_light OriginalParms; /* Note D3D8LIGHT == D3D9LIGHT */
    unsigned int OriginalIndex;
    LONG         glIndex;
    BOOL         enabled;

    /* Computed constants used by the vertex pipe. */
    struct wined3d_light_constants constants;

    struct rb_entry entry;
    struct list changed_entry;
    bool changed;
};

/* The default light parameters */
extern const struct wined3d_light WINED3D_default_light;

struct wined3d_pixel_format
{
    int iPixelFormat; /* WGL pixel format */
    int iPixelType; /* WGL pixel type e.g. WGL_TYPE_RGBA_ARB, WGL_TYPE_RGBA_FLOAT_ARB or WGL_TYPE_COLORINDEX_ARB */
    int redSize, greenSize, blueSize, alphaSize, colorSize;
    int depthSize, stencilSize;
    BOOL windowDrawable;
    BOOL doubleBuffer;
    int auxBuffers;
    int numSamples;
    int swap_method;
};

enum wined3d_pci_vendor
{
    HW_VENDOR_SOFTWARE              = 0x0000,
    HW_VENDOR_AMD                   = 0x1002,
    HW_VENDOR_NVIDIA                = 0x10de,
    HW_VENDOR_VMWARE                = 0x15ad,
    HW_VENDOR_REDHAT                = 0x1af4,
    HW_VENDOR_INTEL                 = 0x8086,
};

enum wined3d_pci_device
{
    CARD_WINE                       = 0x0000,

    CARD_AMD_RAGE_128PRO            = 0x5246,
    CARD_AMD_RADEON_7200            = 0x5144,
    CARD_AMD_RADEON_8500            = 0x514c,
    CARD_AMD_RADEON_9500            = 0x4144,
    CARD_AMD_RADEON_XPRESS_200M     = 0x5955,
    CARD_AMD_RADEON_X700            = 0x5e4c,
    CARD_AMD_RADEON_X1600           = 0x71c2,
    CARD_AMD_RADEON_HD2350          = 0x94c7,
    CARD_AMD_RADEON_HD2600          = 0x9581,
    CARD_AMD_RADEON_HD2900          = 0x9400,
    CARD_AMD_RADEON_HD3200          = 0x9620,
    CARD_AMD_RADEON_HD3850          = 0x9515,
    CARD_AMD_RADEON_HD4200M         = 0x9712,
    CARD_AMD_RADEON_HD4350          = 0x954f,
    CARD_AMD_RADEON_HD4600          = 0x9495,
    CARD_AMD_RADEON_HD4700          = 0x944e,
    CARD_AMD_RADEON_HD4800          = 0x944c,
    CARD_AMD_RADEON_HD5400          = 0x68f9,
    CARD_AMD_RADEON_HD5600          = 0x68d8,
    CARD_AMD_RADEON_HD5700          = 0x68be,
    CARD_AMD_RADEON_HD5800          = 0x6898,
    CARD_AMD_RADEON_HD5900          = 0x689c,
    CARD_AMD_RADEON_HD6300          = 0x9803,
    CARD_AMD_RADEON_HD6400          = 0x6770,
    CARD_AMD_RADEON_HD6490M         = 0x6760,
    CARD_AMD_RADEON_HD6410D         = 0x9644,
    CARD_AMD_RADEON_HD6480G         = 0x9648,
    CARD_AMD_RADEON_HD6550D         = 0x9640,
    CARD_AMD_RADEON_HD6600          = 0x6758,
    CARD_AMD_RADEON_HD6600M         = 0x6741,
    CARD_AMD_RADEON_HD6700          = 0x68ba,
    CARD_AMD_RADEON_HD6800          = 0x6739,
    CARD_AMD_RADEON_HD6900          = 0x6719,
    CARD_AMD_RADEON_HD7660D         = 0x9901,
    CARD_AMD_RADEON_HD7700          = 0x683d,
    CARD_AMD_RADEON_HD7800          = 0x6819,
    CARD_AMD_RADEON_HD7870          = 0x6818,
    CARD_AMD_RADEON_HD7900          = 0x679a,
    CARD_AMD_RADEON_HD8600M         = 0x6660,
    CARD_AMD_RADEON_HD8670          = 0x6610,
    CARD_AMD_RADEON_HD8770          = 0x665c,
    CARD_AMD_RADEON_R3              = 0x9830,
    CARD_AMD_RADEON_R7              = 0x130f,
    CARD_AMD_RADEON_R9_285          = 0x6939,
    CARD_AMD_RADEON_R9_290          = 0x67b1,
    CARD_AMD_RADEON_R9_290X         = 0x67b0,
    CARD_AMD_RADEON_R9_FURY         = 0x7300,
    CARD_AMD_RADEON_R9_M370X        = 0x6821,
    CARD_AMD_RADEON_R9_M380         = 0x6647,
    CARD_AMD_RADEON_R9_M395X        = 0x6920,
    CARD_AMD_RADEON_RX_460          = 0x67ef,
    CARD_AMD_RADEON_RX_480          = 0x67df,
    CARD_AMD_RADEON_RX_VEGA_10      = 0x687f,
    CARD_AMD_RADEON_RX_VEGA_12      = 0x69af,
    CARD_AMD_RADEON_RAVEN           = 0x15dd,
    CARD_AMD_RADEON_RX_VEGA_20      = 0x66af,
    CARD_AMD_RADEON_RX_NAVI_10      = 0x731f,
    CARD_AMD_RADEON_RX_NAVI_14      = 0x7340,
    CARD_AMD_RADEON_RX_NAVI_21      = 0x73bf,
    CARD_AMD_RADEON_PRO_V620        = 0x73a1,
    CARD_AMD_RADEON_PRO_V620_VF     = 0x73ae,
    CARD_AMD_VANGOGH                = 0x163f,
    CARD_AMD_RAPHAEL                = 0x164e,

    CARD_NVIDIA_RIVA_128            = 0x0018,
    CARD_NVIDIA_RIVA_TNT            = 0x0020,
    CARD_NVIDIA_RIVA_TNT2           = 0x0028,
    CARD_NVIDIA_GEFORCE             = 0x0100,
    CARD_NVIDIA_GEFORCE2_MX         = 0x0110,
    CARD_NVIDIA_GEFORCE2            = 0x0150,
    CARD_NVIDIA_GEFORCE3            = 0x0200,
    CARD_NVIDIA_GEFORCE4_MX         = 0x0170,
    CARD_NVIDIA_GEFORCE4_TI4200     = 0x0253,
    CARD_NVIDIA_GEFORCEFX_5200      = 0x0320,
    CARD_NVIDIA_GEFORCEFX_5600      = 0x0312,
    CARD_NVIDIA_GEFORCEFX_5800      = 0x0302,
    CARD_NVIDIA_GEFORCE_6200        = 0x014f,
    CARD_NVIDIA_GEFORCE_6600GT      = 0x0140,
    CARD_NVIDIA_GEFORCE_6800        = 0x0041,
    CARD_NVIDIA_GEFORCE_7300        = 0x01d7, /* GeForce Go 7300 */
    CARD_NVIDIA_GEFORCE_7400        = 0x01d8,
    CARD_NVIDIA_GEFORCE_7600        = 0x0391,
    CARD_NVIDIA_GEFORCE_7800GT      = 0x0092,
    CARD_NVIDIA_GEFORCE_8200        = 0x0849, /* Other PCI ID 0x084b */
    CARD_NVIDIA_GEFORCE_8300GS      = 0x0423,
    CARD_NVIDIA_GEFORCE_8400GS      = 0x0404,
    CARD_NVIDIA_GEFORCE_8500GT      = 0x0421,
    CARD_NVIDIA_GEFORCE_8600GT      = 0x0402,
    CARD_NVIDIA_GEFORCE_8600MGT     = 0x0407,
    CARD_NVIDIA_GEFORCE_8800GTS     = 0x0193,
    CARD_NVIDIA_GEFORCE_8800GTX     = 0x0191,
    CARD_NVIDIA_GEFORCE_9200        = 0x086d,
    CARD_NVIDIA_GEFORCE_9300        = 0x086c,
    CARD_NVIDIA_GEFORCE_9400M       = 0x0863,
    CARD_NVIDIA_GEFORCE_9400GT      = 0x042c,
    CARD_NVIDIA_GEFORCE_9500GT      = 0x0640,
    CARD_NVIDIA_GEFORCE_9600GT      = 0x0622,
    CARD_NVIDIA_GEFORCE_9700MGT     = 0x064a,
    CARD_NVIDIA_GEFORCE_9800GT      = 0x0614,
    CARD_NVIDIA_GEFORCE_210         = 0x0a23,
    CARD_NVIDIA_GEFORCE_GT220       = 0x0a20,
    CARD_NVIDIA_GEFORCE_GT240       = 0x0ca3,
    CARD_NVIDIA_GEFORCE_GTS250      = 0x0615,
    CARD_NVIDIA_GEFORCE_GTX260      = 0x05e2,
    CARD_NVIDIA_GEFORCE_GTX275      = 0x05e6,
    CARD_NVIDIA_GEFORCE_GTX280      = 0x05e1,
    CARD_NVIDIA_GEFORCE_315M        = 0x0a7a,
    CARD_NVIDIA_GEFORCE_320M        = 0x08a3,
    CARD_NVIDIA_GEFORCE_GT320M      = 0x0a2d,
    CARD_NVIDIA_GEFORCE_GT325M      = 0x0a35,
    CARD_NVIDIA_GEFORCE_GT330       = 0x0ca0,
    CARD_NVIDIA_GEFORCE_GTS350M     = 0x0cb0,
    CARD_NVIDIA_GEFORCE_410M        = 0x1055,
    CARD_NVIDIA_GEFORCE_GT420       = 0x0de2,
    CARD_NVIDIA_GEFORCE_GT425M      = 0x0df0,
    CARD_NVIDIA_GEFORCE_GT430       = 0x0de1,
    CARD_NVIDIA_GEFORCE_GT440       = 0x0de0,
    CARD_NVIDIA_GEFORCE_GTS450      = 0x0dc4,
    CARD_NVIDIA_GEFORCE_GTX460      = 0x0e22,
    CARD_NVIDIA_GEFORCE_GTX460M     = 0x0dd1,
    CARD_NVIDIA_GEFORCE_GTX465      = 0x06c4,
    CARD_NVIDIA_GEFORCE_GTX470      = 0x06cd,
    CARD_NVIDIA_GEFORCE_GTX480      = 0x06c0,
    CARD_NVIDIA_GEFORCE_GT520       = 0x1040,
    CARD_NVIDIA_GEFORCE_GT525M      = 0x0dec,
    CARD_NVIDIA_GEFORCE_GT540M      = 0x0df4,
    CARD_NVIDIA_GEFORCE_GTX550      = 0x1244,
    CARD_NVIDIA_GEFORCE_GT555M      = 0x04b8,
    CARD_NVIDIA_GEFORCE_GTX560TI    = 0x1200,
    CARD_NVIDIA_GEFORCE_GTX560M     = 0x1251,
    CARD_NVIDIA_GEFORCE_GTX560      = 0x1201,
    CARD_NVIDIA_GEFORCE_GTX570      = 0x1081,
    CARD_NVIDIA_GEFORCE_GTX580      = 0x1080,
    CARD_NVIDIA_GEFORCE_GT610       = 0x104a,
    CARD_NVIDIA_GEFORCE_GT630       = 0x0f00,
    CARD_NVIDIA_GEFORCE_GT630M      = 0x0de9,
    CARD_NVIDIA_GEFORCE_GT640       = 0x0fc1,
    CARD_NVIDIA_GEFORCE_GT640M      = 0x0fd2,
    CARD_NVIDIA_GEFORCE_GT650M      = 0x0fd1,
    CARD_NVIDIA_GEFORCE_GTX650      = 0x0fc6,
    CARD_NVIDIA_GEFORCE_GTX650TI    = 0x11c6,
    CARD_NVIDIA_GEFORCE_GTX660      = 0x11c0,
    CARD_NVIDIA_GEFORCE_GTX660M     = 0x0fd4,
    CARD_NVIDIA_GEFORCE_GTX660TI    = 0x1183,
    CARD_NVIDIA_GEFORCE_GTX670      = 0x1189,
    CARD_NVIDIA_GEFORCE_GTX670MX    = 0x11a1,
    CARD_NVIDIA_GEFORCE_GTX675MX_1  = 0x11a7,
    CARD_NVIDIA_GEFORCE_GTX675MX_2  = 0x11a2,
    CARD_NVIDIA_GEFORCE_GTX680      = 0x1180,
    CARD_NVIDIA_GEFORCE_GTX690      = 0x1188,
    CARD_NVIDIA_GEFORCE_GT720       = 0x128b,
    CARD_NVIDIA_GEFORCE_GT730       = 0x1287,
    CARD_NVIDIA_GEFORCE_GT730M      = 0x0fe1,
    CARD_NVIDIA_GEFORCE_GT740M      = 0x1292,
    CARD_NVIDIA_GEFORCE_GT750M      = 0x0fe9,
    CARD_NVIDIA_GEFORCE_GT755M      = 0x0fcd,
    CARD_NVIDIA_GEFORCE_GTX750      = 0x1381,
    CARD_NVIDIA_GEFORCE_GTX750TI    = 0x1380,
    CARD_NVIDIA_GEFORCE_GTX760      = 0x1187,
    CARD_NVIDIA_GEFORCE_GTX760TI    = 0x1193,
    CARD_NVIDIA_GEFORCE_GTX765M     = 0x11e2,
    CARD_NVIDIA_GEFORCE_GTX770M     = 0x11e0,
    CARD_NVIDIA_GEFORCE_GTX770      = 0x1184,
    CARD_NVIDIA_GEFORCE_GTX775M     = 0x119d,
    CARD_NVIDIA_GEFORCE_GTX780      = 0x1004,
    CARD_NVIDIA_GEFORCE_GTX780M     = 0x119e,
    CARD_NVIDIA_GEFORCE_GTX780TI    = 0x100a,
    CARD_NVIDIA_GEFORCE_GTXTITAN    = 0x1005,
    CARD_NVIDIA_GEFORCE_GTXTITANB   = 0x100c,
    CARD_NVIDIA_GEFORCE_GTXTITANX   = 0x17c2,
    CARD_NVIDIA_GEFORCE_GTXTITANZ   = 0x1001,
    CARD_NVIDIA_GEFORCE_820M        = 0x0fed,
    CARD_NVIDIA_GEFORCE_830M        = 0x1340,
    CARD_NVIDIA_GEFORCE_840M        = 0x1341,
    CARD_NVIDIA_GEFORCE_845M        = 0x1344,
    CARD_NVIDIA_GEFORCE_GTX850M     = 0x1391,
    CARD_NVIDIA_GEFORCE_GTX860M     = 0x1392, /* Other PCI ID 0x119a */
    CARD_NVIDIA_GEFORCE_GTX870M     = 0x1199,
    CARD_NVIDIA_GEFORCE_GTX880M     = 0x1198,
    CARD_NVIDIA_GEFORCE_940M        = 0x1347,
    CARD_NVIDIA_GEFORCE_GTX950      = 0x1402,
    CARD_NVIDIA_GEFORCE_GTX950M     = 0x139a,
    CARD_NVIDIA_GEFORCE_GTX960      = 0x1401,
    CARD_NVIDIA_GEFORCE_GTX960M     = 0x139b,
    CARD_NVIDIA_GEFORCE_GTX970      = 0x13c2,
    CARD_NVIDIA_GEFORCE_GTX970M     = 0x13d8,
    CARD_NVIDIA_GEFORCE_GTX980      = 0x13c0,
    CARD_NVIDIA_GEFORCE_GTX980TI    = 0x17c8,
    CARD_NVIDIA_GEFORCE_GT1030      = 0x1d01,
    CARD_NVIDIA_GEFORCE_GTX1050     = 0x1c81,
    CARD_NVIDIA_GEFORCE_GTX1050TI   = 0x1c82,
    CARD_NVIDIA_GEFORCE_GTX1060_3GB = 0x1c02,
    CARD_NVIDIA_GEFORCE_GTX1060     = 0x1c03,
    CARD_NVIDIA_GEFORCE_GTX1060M    = 0x1c20,
    CARD_NVIDIA_GEFORCE_GTX1070     = 0x1b81,
    CARD_NVIDIA_GEFORCE_GTX1070M    = 0x1be1,
    CARD_NVIDIA_GEFORCE_GTX1080     = 0x1b80,
    CARD_NVIDIA_GEFORCE_GTX1080M    = 0x1be0,
    CARD_NVIDIA_GEFORCE_GTX1080TI   = 0x1b06,
    CARD_NVIDIA_TITANX_PASCAL       = 0x1b00,
    CARD_NVIDIA_TITANV              = 0x1d81,
    CARD_NVIDIA_GEFORCE_GTX1650     = 0x1f82,
    CARD_NVIDIA_GEFORCE_GTX1650SUPER= 0x2187,
    CARD_NVIDIA_GEFORCE_GTX1660SUPER= 0x21c4,
    CARD_NVIDIA_GEFORCE_GTX1660TI   = 0x2182,
    CARD_NVIDIA_GEFORCE_RTX2060     = 0x1f08,
    CARD_NVIDIA_GEFORCE_RTX2070     = 0x1f07,
    CARD_NVIDIA_GEFORCE_RTX2080     = 0x1e87,
    CARD_NVIDIA_GEFORCE_RTX2080TI   = 0x1e07,
    CARD_NVIDIA_GEFORCE_RTX3050     = 0x2507,
    CARD_NVIDIA_GEFORCE_RTX3060     = 0x2544,
    CARD_NVIDIA_GEFORCE_RTX3060_LHR = 0x2504,
    CARD_NVIDIA_GEFORCE_RTX3060TI_GA103 = 0x2414,
    CARD_NVIDIA_GEFORCE_RTX3060TI_GA104 = 0x2486,
    CARD_NVIDIA_GEFORCE_RTX3060TI_GA104_LHR = 0x2489,
    CARD_NVIDIA_GEFORCE_RTX3070     = 0x2484,
    CARD_NVIDIA_GEFORCE_RTX3070_LHR = 0x2488,
    CARD_NVIDIA_GEFORCE_RTX3070_MOBILE = 0x249d,
    CARD_NVIDIA_GEFORCE_RTX3070TI   = 0x2482,
    CARD_NVIDIA_GEFORCE_RTX3080_10GB = 0x2206,
    CARD_NVIDIA_GEFORCE_RTX3080_10GB_LHR = 0x2216,
    CARD_NVIDIA_GEFORCE_RTX3080_12GB = 0x220a,
    CARD_NVIDIA_GEFORCE_RTX3080TI   = 0x2208,
    CARD_NVIDIA_GEFORCE_RTX3090     = 0x2204,
    CARD_NVIDIA_GEFORCE_RTX3090TI   = 0x2203,
    CARD_NVIDIA_TESLA_T4            = 0x1eb8,
    CARD_NVIDIA_AMPERE_A10          = 0x2236,
    CARD_NVIDIA_GEFORCE_RTX4060     = 0x2882,
    CARD_NVIDIA_GEFORCE_RTX4060TI8G = 0x2803,
    CARD_NVIDIA_GEFORCE_RTX4060TI16G = 0x2805,
    CARD_NVIDIA_GEFORCE_RTX4070     = 0x2786,
    CARD_NVIDIA_GEFORCE_RTX4070SUPER = 0x2783,
    CARD_NVIDIA_GEFORCE_RTX4070TI   = 0x2782,
    CARD_NVIDIA_GEFORCE_RTX4070TISUPER = 0x2705,
    CARD_NVIDIA_GEFORCE_RTX4080     = 0x2704,
    CARD_NVIDIA_GEFORCE_RTX4080SUPER = 0x2702,
    CARD_NVIDIA_GEFORCE_RTX4090     = 0x2684,

    CARD_REDHAT_VIRGL               = 0x1010,

    CARD_VMWARE_SVGA3D              = 0x0405,

    CARD_INTEL_830M                 = 0x3577,
    CARD_INTEL_855GM                = 0x3582,
    CARD_INTEL_845G                 = 0x2562,
    CARD_INTEL_865G                 = 0x2572,
    CARD_INTEL_915G                 = 0x2582,
    CARD_INTEL_E7221G               = 0x258a,
    CARD_INTEL_915GM                = 0x2592,
    CARD_INTEL_945G                 = 0x2772,
    CARD_INTEL_945GM                = 0x27a2,
    CARD_INTEL_945GME               = 0x27ae,
    CARD_INTEL_Q35                  = 0x29b2,
    CARD_INTEL_G33                  = 0x29c2,
    CARD_INTEL_Q33                  = 0x29d2,
    CARD_INTEL_PNVG                 = 0xa001,
    CARD_INTEL_PNVM                 = 0xa011,
    CARD_INTEL_965Q                 = 0x2992,
    CARD_INTEL_965G                 = 0x2982,
    CARD_INTEL_946GZ                = 0x2972,
    CARD_INTEL_965GM                = 0x2a02,
    CARD_INTEL_965GME               = 0x2a12,
    CARD_INTEL_GM45                 = 0x2a42,
    CARD_INTEL_IGD                  = 0x2e02,
    CARD_INTEL_Q45                  = 0x2e12,
    CARD_INTEL_G45                  = 0x2e22,
    CARD_INTEL_G41                  = 0x2e32,
    CARD_INTEL_B43                  = 0x2e92,
    CARD_INTEL_ILKD                 = 0x0042,
    CARD_INTEL_ILKM                 = 0x0046,
    CARD_INTEL_SNBD                 = 0x0122,
    CARD_INTEL_SNBM                 = 0x0126,
    CARD_INTEL_SNBS                 = 0x010a,
    CARD_INTEL_IVBD                 = 0x0162,
    CARD_INTEL_IVBM                 = 0x0166,
    CARD_INTEL_IVBS                 = 0x015a,
    CARD_INTEL_HWD                  = 0x0412,
    CARD_INTEL_HWM                  = 0x0416,
    CARD_INTEL_HD5000_1             = 0x0a26,
    CARD_INTEL_HD5000_2             = 0x0422,
    CARD_INTEL_I5100_1              = 0x0a22,
    CARD_INTEL_I5100_2              = 0x0a2a,
    CARD_INTEL_I5100_3              = 0x0a2b,
    CARD_INTEL_I5100_4              = 0x0a2e,
    CARD_INTEL_IP5200_1             = 0x0d22,
    CARD_INTEL_IP5200_2             = 0x0d26,
    CARD_INTEL_IP5200_3             = 0x0d2a,
    CARD_INTEL_IP5200_4             = 0x0d2b,
    CARD_INTEL_IP5200_5             = 0x0d2e,
    CARD_INTEL_IP5200_6             = 0x0c22,
    CARD_INTEL_HD5300               = 0x161e,
    CARD_INTEL_HD5500               = 0x1616,
    CARD_INTEL_HD5600               = 0x1612,
    CARD_INTEL_HD6000               = 0x1626,
    CARD_INTEL_I6100                = 0x162b,
    CARD_INTEL_IP6200               = 0x1622,
    CARD_INTEL_IPP6300              = 0x162a,
    CARD_INTEL_HD510_1              = 0x1902,
    CARD_INTEL_HD510_2              = 0x1906,
    CARD_INTEL_HD510_3              = 0x190b,
    CARD_INTEL_HD515                = 0x191e,
    CARD_INTEL_HD520_1              = 0x1916,
    CARD_INTEL_HD520_2              = 0x1921,
    CARD_INTEL_HD530_1              = 0x1912,
    CARD_INTEL_HD530_2              = 0x191b,
    CARD_INTEL_HDP530               = 0x191d,
    CARD_INTEL_I540                 = 0x1926,
    CARD_INTEL_I550                 = 0x1927,
    CARD_INTEL_I555                 = 0x192b,
    CARD_INTEL_IP555                = 0x192d,
    CARD_INTEL_IP580_1              = 0x1932,
    CARD_INTEL_IP580_2              = 0x193b,
    CARD_INTEL_IPP580_1             = 0x193a,
    CARD_INTEL_IPP580_2             = 0x193d,
    CARD_INTEL_UHD617               = 0x87c0,
    CARD_INTEL_UHD620               = 0x3ea0,
    CARD_INTEL_HD615                = 0x591e,
    CARD_INTEL_HD620                = 0x5916,
    CARD_INTEL_HD630_1              = 0x5912,
    CARD_INTEL_HD630_2              = 0x591b,
    CARD_INTEL_UHD630_1             = 0x3e9b,
    CARD_INTEL_UHD630_2             = 0x3e91,
};

/* The driver names reflect the lowest GPU supported
 * by a certain driver, so DRIVER_AMD_R300 supports
 * R3xx, R4xx and R5xx GPUs. */
enum wined3d_display_driver
{
    DRIVER_AMD_RAGE_128PRO,
    DRIVER_AMD_R100,
    DRIVER_AMD_R300,
    DRIVER_AMD_R600,
    DRIVER_AMD_RX,
    DRIVER_INTEL_GMA800,
    DRIVER_INTEL_GMA900,
    DRIVER_INTEL_GMA950,
    DRIVER_INTEL_GMA3000,
    DRIVER_INTEL_HD4000,
    DRIVER_NVIDIA_TNT,
    DRIVER_NVIDIA_GEFORCE2MX,
    DRIVER_NVIDIA_GEFORCEFX,
    DRIVER_NVIDIA_GEFORCE6,
    DRIVER_NVIDIA_GEFORCE8,
    DRIVER_NVIDIA_FERMI,
    DRIVER_NVIDIA_KEPLER,
    DRIVER_REDHAT_VIRGL,
    DRIVER_VMWARE,
    DRIVER_WINE,
    DRIVER_UNKNOWN,
};

struct wined3d_gpu_description
{
    enum wined3d_pci_vendor vendor;
    enum wined3d_pci_device device;
    const char *description;
    enum wined3d_display_driver driver;
    unsigned int vidmem;
};

const struct wined3d_gpu_description *wined3d_get_gpu_description(enum wined3d_pci_vendor vendor,
        enum wined3d_pci_device device);
const struct wined3d_gpu_description *wined3d_get_user_override_gpu_description(enum wined3d_pci_vendor vendor,
        enum wined3d_pci_device device);
enum wined3d_pci_device wined3d_gpu_from_feature_level(enum wined3d_pci_vendor *vendor,
        enum wined3d_feature_level feature_level);

/* 512 in Direct3D 8/9, 128 in DXGI. */
#define WINED3D_MAX_DEVICE_IDENTIFIER_LENGTH 512

struct wined3d_driver_info
{
    enum wined3d_pci_vendor vendor;
    enum wined3d_pci_device device;
    const char *name;
    char description[WINED3D_MAX_DEVICE_IDENTIFIER_LENGTH];
    UINT64 vram_bytes;
    UINT64 sysmem_bytes;
    unsigned int version_high;
    unsigned int version_low;
};

bool wined3d_driver_info_init(struct wined3d_driver_info *driver_info,
        const struct wined3d_gpu_description *gpu_description, enum wined3d_feature_level feature_level,
        UINT64 vram_bytes, UINT64 sysmem_bytes);

#define UPLOAD_BO_UPLOAD_ON_UNMAP   0x1
#define UPLOAD_BO_RENAME_ON_UNMAP   0x2
#define UPLOAD_BO_FREE_ON_UNMAP     0x4

struct upload_bo
{
    struct wined3d_const_bo_address addr;
    uint32_t flags;
};

struct wined3d_adapter_ops
{
    void (*adapter_destroy)(struct wined3d_adapter *adapter);
    HRESULT (*adapter_create_device)(struct wined3d *wined3d, const struct wined3d_adapter *adapter,
            enum wined3d_device_type device_type, HWND focus_window, unsigned int flags,
            BYTE surface_alignment, const enum wined3d_feature_level *levels, unsigned int level_count,
            struct wined3d_device_parent *device_parent, struct wined3d_device **device);
    void (*adapter_destroy_device)(struct wined3d_device *device);
    struct wined3d_context *(*adapter_acquire_context)(struct wined3d_device *device,
            struct wined3d_texture *texture, unsigned int sub_resource_idx);
    void (*adapter_release_context)(struct wined3d_context *context);
    void (*adapter_get_wined3d_caps)(const struct wined3d_adapter *adapter, struct wined3d_caps *caps);
    BOOL (*adapter_check_format)(const struct wined3d_adapter *adapter,
            const struct wined3d_format *adapter_format, const struct wined3d_format *rt_format,
            const struct wined3d_format *ds_format);
    HRESULT (*adapter_init_3d)(struct wined3d_device *device);
    void (*adapter_uninit_3d)(struct wined3d_device *device);
    void *(*adapter_map_bo_address)(struct wined3d_context *context,
            const struct wined3d_bo_address *data, size_t size, uint32_t map_flags);
    void (*adapter_unmap_bo_address)(struct wined3d_context *context, const struct wined3d_bo_address *data,
            unsigned int range_count, const struct wined3d_range *ranges);
    void (*adapter_copy_bo_address)(struct wined3d_context *context,
            const struct wined3d_bo_address *dst, const struct wined3d_bo_address *src,
            unsigned int range_count, const struct wined3d_range *ranges, uint32_t map_flags);
    void (*adapter_flush_bo_address)(struct wined3d_context *context,
            const struct wined3d_const_bo_address *data, size_t size);
    bool (*adapter_alloc_bo)(struct wined3d_device *device, struct wined3d_resource *resource,
            unsigned int sub_resource_idx, struct wined3d_bo_address *addr);
    void (*adapter_destroy_bo)(struct wined3d_context *context, struct wined3d_bo *bo);
    HRESULT (*adapter_create_swapchain)(struct wined3d_device *device,
            const struct wined3d_swapchain_desc *desc,
            struct wined3d_swapchain_state_parent *state_parent, void *parent,
            const struct wined3d_parent_ops *parent_ops, struct wined3d_swapchain **swapchain);
    void (*adapter_destroy_swapchain)(struct wined3d_swapchain *swapchain);
    HRESULT (*adapter_create_buffer)(struct wined3d_device *device, const struct wined3d_buffer_desc *desc,
            const struct wined3d_sub_resource_data *data, void *parent, const struct wined3d_parent_ops *parent_ops,
            struct wined3d_buffer **buffer);
    void (*adapter_destroy_buffer)(struct wined3d_buffer *buffer);
    HRESULT (*adapter_create_texture)(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
            unsigned int layer_count, unsigned int level_count, uint32_t flags, void *parent,
            const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture);
    void (*adapter_destroy_texture)(struct wined3d_texture *texture);
    HRESULT (*adapter_create_rendertarget_view)(const struct wined3d_view_desc *desc,
            struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
            struct wined3d_rendertarget_view **view);
    void (*adapter_destroy_rendertarget_view)(struct wined3d_rendertarget_view *view);
    HRESULT (*adapter_create_shader_resource_view)(const struct wined3d_view_desc *desc,
            struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
            struct wined3d_shader_resource_view **view);
    void (*adapter_destroy_shader_resource_view)(struct wined3d_shader_resource_view *view);
    HRESULT (*adapter_create_unordered_access_view)(const struct wined3d_view_desc *desc,
            struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
            struct wined3d_unordered_access_view **view);
    void (*adapter_destroy_unordered_access_view)(struct wined3d_unordered_access_view *view);
    HRESULT (*adapter_create_sampler)(struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
            void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_sampler **sampler);
    void (*adapter_destroy_sampler)(struct wined3d_sampler *sampler);
    HRESULT (*adapter_create_query)(struct wined3d_device *device, enum wined3d_query_type type,
            void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query);
    void (*adapter_destroy_query)(struct wined3d_query *query);
    void (*adapter_flush_context)(struct wined3d_context *context);
    void (*adapter_draw_primitive)(struct wined3d_device *device, const struct wined3d_state *state,
            const struct wined3d_draw_parameters *parameters);
    void (*adapter_dispatch_compute)(struct wined3d_device *device, const struct wined3d_state *state,
            const struct wined3d_dispatch_parameters *parameters);
    void (*adapter_clear_uav)(struct wined3d_context *context,
            struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value, bool fp);
    void (*adapter_generate_mipmap)(struct wined3d_context *context, struct wined3d_shader_resource_view *view);
};

struct wined3d_output
{
    unsigned int ordinal;
    WCHAR device_name[CCHDEVICENAME];
    struct wined3d_adapter *adapter;
    enum wined3d_format_id screen_format;

    D3DKMT_HANDLE kmt_device;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID vidpn_source_id;

    struct wined3d_display_mode *modes;
    SIZE_T mode_count, modes_size;
    bool modes_valid;
};

HRESULT wined3d_output_get_gamma_ramp(struct wined3d_output *output, struct wined3d_gamma_ramp *ramp);

#ifdef _WIN64
#define MAX_PERSISTENT_MAPPED_BYTES I64_MAX
#else
#define MAX_PERSISTENT_MAPPED_BYTES (128 * 1024 * 1024)
#endif

/* The adapter structure */
struct wined3d_adapter
{
    unsigned int ordinal;

    struct wined3d_d3d_info d3d_info;
    struct wined3d_driver_info driver_info;
    struct wined3d_output *outputs;
    SIZE_T output_count, outputs_size;
    D3DKMT_HANDLE kmt_adapter;
    UINT64 vram_bytes_used;
    GUID driver_uuid;
    GUID device_uuid;
    LUID luid;

    void *formats;
    size_t format_size;

    ssize_t mapped_size;

    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    const struct wined3d_state_entry_template *misc_state_template;
    const struct wined3d_shader_backend_ops *shader_backend;
    const struct wined3d_adapter_ops *adapter_ops;
};

BOOL wined3d_adapter_init(struct wined3d_adapter *adapter, unsigned int ordinal, const LUID *luid,
        const struct wined3d_adapter_ops *adapter_ops);
void wined3d_adapter_cleanup(struct wined3d_adapter *adapter);
BOOL wined3d_get_primary_adapter_luid(LUID *luid);

struct wined3d_adapter *wined3d_adapter_vk_create(unsigned int ordinal,
        unsigned int wined3d_creation_flags);

struct wined3d_adapter *wined3d_adapter_gl_create(unsigned int ordinal,
        unsigned int wined3d_creation_flags);

BOOL wined3d_adapter_no3d_init_format_info(struct wined3d_adapter *adapter);
ssize_t adapter_adjust_mapped_memory(struct wined3d_adapter *adapter, ssize_t size);
UINT64 adapter_adjust_memory(struct wined3d_adapter *adapter, INT64 amount);

enum wined3d_projection_type
{
    WINED3D_PROJECTION_NONE    = 0,
    WINED3D_PROJECTION_COUNT3  = 1,
    WINED3D_PROJECTION_COUNT4  = 2
};

/*****************************************************************************
 * Fixed function pipeline replacements
 */
#define ARG_UNUSED          0xff
struct texture_stage_op
{
    unsigned                cop : 8;
    unsigned                carg1 : 8;
    unsigned                carg2 : 8;
    unsigned                carg0 : 8;

    unsigned                aop : 8;
    unsigned                aarg1 : 8;
    unsigned                aarg2 : 8;
    unsigned                aarg0 : 8;

    struct color_fixup_desc color_fixup;
    unsigned                tex_type : 3;
    unsigned                tmp_dst : 1;
    unsigned                projected : 2;
    unsigned                padding : 10;
};

struct ffp_frag_settings
{
    struct texture_stage_op op[WINED3D_MAX_FFP_TEXTURES];
    unsigned char fog; /* enum wined3d_ffp_ps_fog_mode */
    unsigned char sRGB_write;
    unsigned char texcoords_initialized;
    unsigned char color_key_enabled : 1;
    unsigned char pointsprite : 1;
    unsigned char flatshading : 1;
    unsigned char alpha_test_func : 3;
    unsigned char padding : 2;
};

struct ffp_frag_desc
{
    struct wine_rb_entry entry;
    struct ffp_frag_settings    settings;
};

int wined3d_ffp_frag_program_key_compare(const void *key, const struct wine_rb_entry *entry);
int wined3d_ffp_vertex_program_key_compare(const void *key, const struct wine_rb_entry *entry);

struct wined3d_ffp_ps
{
    struct ffp_frag_desc entry;
    struct wined3d_shader *shader;
};

extern const struct wined3d_parent_ops wined3d_null_parent_ops;

void wined3d_ffp_get_fs_settings(const struct wined3d_state *state,
        const struct wined3d_d3d_info *d3d_info, struct ffp_frag_settings *settings);
const struct ffp_frag_desc *find_ffp_frag_shader(const struct wine_rb_tree *fragment_shaders,
        const struct ffp_frag_settings *settings);
void add_ffp_frag_shader(struct wine_rb_tree *shaders, struct ffp_frag_desc *desc);
void wined3d_ftoa(float value, char *s);

enum wined3d_ffp_vs_fog_mode
{
    WINED3D_FFP_VS_FOG_OFF      = 0,
    WINED3D_FFP_VS_FOG_FOGCOORD = 1,
    WINED3D_FFP_VS_FOG_DEPTH    = 2,
    WINED3D_FFP_VS_FOG_RANGE    = 3,
};

#define WINED3D_FFP_TCI_SHIFT               16
#define WINED3D_FFP_TCI_MASK                0xffu

#define WINED3D_FFP_LIGHT_TYPE_SHIFT(idx)   (3 * (idx))
#define WINED3D_FFP_LIGHT_TYPE_MASK         0x7u

struct wined3d_ffp_vs_settings
{
    DWORD point_light_count          : 4;
    DWORD spot_light_count           : 4;
    DWORD directional_light_count    : 4;
    DWORD parallel_point_light_count : 4;
    DWORD diffuse_source  : 2;
    DWORD emissive_source : 2;
    DWORD ambient_source  : 2;
    DWORD specular_source : 2;
    DWORD transformed     : 1;
    DWORD vertexblends    : 2;
    DWORD clipping        : 1;
    DWORD normal          : 1;
    DWORD normalize       : 1;
    DWORD lighting        : 1;
    DWORD localviewer     : 1;

    DWORD point_size      : 1;
    DWORD per_vertex_point_size : 1;
    DWORD fog_mode        : 2;
    DWORD texcoords       : 8;  /* WINED3D_MAX_FFP_TEXTURES */
    DWORD ortho_fog       : 1;
    DWORD flatshading     : 1;
    DWORD specular_enable : 1;
    DWORD diffuse         : 1;
    DWORD padding         : 16;

    DWORD swizzle_map; /* MAX_ATTRIBS, 32 */

    unsigned int texgen[WINED3D_MAX_FFP_TEXTURES];
};

struct wined3d_ffp_vs_desc
{
    struct wine_rb_entry entry;
    struct wined3d_ffp_vs_settings settings;
};

struct wined3d_ffp_vs
{
    struct wined3d_ffp_vs_desc entry;
    struct wined3d_shader *shader;
};

void wined3d_ffp_get_vs_settings(const struct wined3d_state *state, const struct wined3d_stream_info *si,
        const struct wined3d_d3d_info *d3d_info, struct wined3d_ffp_vs_settings *settings);

struct wined3d
{
    LONG ref;
    unsigned int flags;
    unsigned int adapter_count;
    struct wined3d_adapter *adapters[1];
};

BOOL wined3d_filter_messages(HWND window, BOOL filter);
HRESULT wined3d_init(struct wined3d *wined3d, uint32_t flags);
void wined3d_unregister_window(HWND window);

BOOL wined3d_get_app_name(char *app_name, unsigned int app_name_size);

/* Direct3D 1-9 shader constants are submitted by internally feeding them into
 * wined3d_buffer objects, which are updated with
 * wined3d_device_context_emit_update_sub_resource().
 * This allows Vulkan backend shaders to read from the buffers as they would
 * any other uniform buffer, with no extra code needed to set up the descriptor
 * set and bind buffers. Other renderers simply map the buffers as CPU buffers
 * and read from them directly.
 *
 * The "FFP" buffer contains constants that are part of the D3D1-9 FFP pipe,
 * i.e. only used when shaders are disabled.
 */

struct wined3d_ffp_vs_constants
{
    union wined3d_ffp_vs_modelview_matrices
    {
        struct wined3d_matrix modelview_matrices[MAX_VERTEX_BLENDS];
        struct
        {
            struct wined3d_matrix modelview_matrix;
            struct wined3d_matrix normal_matrix;
        } not_blended;
    } modelview;
    struct wined3d_matrix projection_matrix;
    struct wined3d_matrix texture_matrices[WINED3D_MAX_FFP_TEXTURES];
    struct wined3d_ffp_point_constants
    {
        float scale_const, scale_linear, scale_quad;
        float size;
    } point;
    struct wined3d_material material;
    float padding[3]; /* For the HLSL backend. */
    struct wined3d_ffp_light_constants
    {
        struct wined3d_color ambient;
        struct wined3d_light_constants lights[8];
    } light;
};

struct wined3d_ffp_ps_constants
{
    struct wined3d_color texture_constants[WINED3D_MAX_FFP_TEXTURES];
    struct wined3d_color texture_factor;
    /* (1, 1, 1, 0) or (0, 0, 0, 0), which shaders will multiply with the
     * specular color. */
    struct wined3d_color specular_enable;
    struct wined3d_color color_key[2];
    struct wined3d_ffp_bumpenv_constants
    {
        struct
        {
            float _00, _01, _10, _11;
        } matrices[WINED3D_MAX_FFP_TEXTURES];
        float lscale[WINED3D_MAX_FFP_TEXTURES];
        float loffset[WINED3D_MAX_FFP_TEXTURES];
    } bumpenv;

    /* States not used by the HLSL pipeline. */
    float alpha_test_ref;
    float padding[3]; /* to align to 16 bytes */
};

/* Float/int/bool constants are bound to VKD3D_SHADER_D3DBC_*_CONSTANT_REGISTER
 * which have the values 0/1/2 respectively.
 * FFP uniform constants active even with shaders (e.g. alpha test ref) need to
 * be additionally bound to a different slot. */
#define WINED3D_FFP_CONSTANTS_EXTRA_REGISTER 3

enum wined3d_push_constants
{
    WINED3D_PUSH_CONSTANTS_VS_F,
    WINED3D_PUSH_CONSTANTS_PS_F,
    WINED3D_PUSH_CONSTANTS_VS_I,
    WINED3D_PUSH_CONSTANTS_PS_I,
    WINED3D_PUSH_CONSTANTS_VS_B,
    WINED3D_PUSH_CONSTANTS_PS_B,
    WINED3D_PUSH_CONSTANTS_VS_FFP,
    WINED3D_PUSH_CONSTANTS_PS_FFP,
    WINED3D_PUSH_CONSTANTS_COUNT,
};

struct wined3d_blend_state
{
    LONG refcount;
    struct wined3d_blend_state_desc desc;
    BOOL dual_source;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    struct wined3d_device *device;
    struct wine_rb_entry entry;
};

static inline unsigned int wined3d_blend_state_get_writemask(const struct wined3d_blend_state *state,
        unsigned int index)
{
    if (!state)
        return 0xf;
    if (!state->desc.independent)
        index = 0;
    return state->desc.rt[index].writemask;
}

struct wined3d_depth_stencil_state
{
    LONG refcount;
    struct wined3d_depth_stencil_state_desc desc;

    bool writes_ds;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    struct wined3d_device *device;
    struct wine_rb_entry entry;
};

struct wined3d_rasterizer_state
{
    LONG refcount;
    struct wined3d_rasterizer_state_desc desc;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    struct wined3d_device *device;
    struct wine_rb_entry entry;
};

struct wined3d_light_state
{
    struct rb_tree lights_tree;
    const struct wined3d_light_info *lights[WINED3D_MAX_ACTIVE_LIGHTS];
};

#define WINED3D_STATE_NO_REF        0x00000001
#define WINED3D_STATE_INIT_DEFAULT  0x00000002

struct wined3d_state
{
    enum wined3d_feature_level feature_level;
    uint32_t flags;
    struct wined3d_fb_state fb;

    struct wined3d_vertex_declaration *vertex_declaration;
    struct wined3d_stream_output stream_output[WINED3D_MAX_STREAM_OUTPUT_BUFFERS];
    struct wined3d_stream_state streams[WINED3D_MAX_STREAMS];
    struct wined3d_buffer *index_buffer;
    enum wined3d_format_id index_format;
    unsigned int index_offset;
    int base_vertex_index;
    int load_base_vertex_index; /* Non-indexed drawing needs 0 here, indexed needs base_vertex_index. */
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;
    struct wined3d_query *predicate;
    BOOL predicate_value;

    struct wined3d_shader *shader[WINED3D_SHADER_TYPE_COUNT];
    struct wined3d_constant_buffer_state cb[WINED3D_SHADER_TYPE_COUNT][MAX_CONSTANT_BUFFERS];
    struct wined3d_sampler *sampler[WINED3D_SHADER_TYPE_COUNT][MAX_SAMPLER_OBJECTS];
    struct wined3d_shader_resource_view *shader_resource_view[WINED3D_SHADER_TYPE_COUNT][MAX_SHADER_RESOURCE_VIEWS];
    struct wined3d_unordered_access_view *unordered_access_view[WINED3D_PIPELINE_COUNT][MAX_UNORDERED_ACCESS_VIEWS];

    uint32_t texture_states[WINED3D_MAX_FFP_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];

    struct wined3d_matrix transforms[WINED3D_HIGHEST_TRANSFORM_STATE + 1];
    struct wined3d_vec4 clip_planes[WINED3D_MAX_CLIP_DISTANCES];
    struct wined3d_viewport viewports[WINED3D_MAX_VIEWPORTS];
    unsigned int viewport_count;
    RECT scissor_rects[WINED3D_MAX_VIEWPORTS];
    unsigned int scissor_rect_count;

    struct wined3d_light_state light_state;

    unsigned int render_states[WINEHIGHEST_RENDER_STATE + 1];
    struct wined3d_blend_state *blend_state;
    struct wined3d_color blend_factor;
    uint32_t sample_mask;
    struct wined3d_depth_stencil_state *depth_stencil_state;
    unsigned int stencil_ref;
    bool depth_bounds_enable;
    float depth_bounds_min, depth_bounds_max;
    struct wined3d_rasterizer_state *rasterizer_state;
};

void state_cleanup(struct wined3d_state *state);
void state_init(struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info,
        uint32_t flags, enum wined3d_feature_level feature_level);
void state_unbind_resources(struct wined3d_state *state);

static inline void wined3d_state_reset(struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info)
{
    enum wined3d_feature_level feature_level = state->feature_level;
    uint32_t flags = state->flags;

    memset(state, 0, sizeof(*state));
    state_init(state, d3d_info, flags, feature_level);
}

static inline bool wined3d_state_uses_depth_buffer(const struct wined3d_state *state)
{
    if (!state->depth_stencil_state)
        return true;
    return state->depth_stencil_state->desc.depth || state->depth_stencil_state->desc.depth_write
            || state->depth_stencil_state->desc.stencil;
}

#define WINED3D_UNMAPPED_STAGE ~0u

/* Multithreaded flag. Removed from the public header to signal that
 * wined3d_device_create() ignores it. */
#define WINED3DCREATE_MULTITHREADED 0x00000004

struct wined3d_so_desc_entry
{
    struct wine_rb_entry entry;
    struct wined3d_stream_output_desc desc;
    struct wined3d_stream_output_element elements[1];
};

struct wined3d_device
{
    LONG ref;

    /* WineD3D Information  */
    struct wined3d_device_parent *device_parent;
    struct wined3d *wined3d;
    struct wined3d_adapter *adapter;

    const struct wined3d_shader_backend_ops *shader_backend;
    void *shader_priv;
    void *fragment_priv;
    void *vertex_priv;
    struct wined3d_state_entry state_table[STATE_HIGHEST + 1];
    /* Array of functions for states which are handled by more than one pipeline part */
    APPLYSTATEFUNC *multistate_funcs[STATE_HIGHEST + 1];
    struct wined3d_blitter *blitter;

    BYTE bCursorVisible : 1;
    BYTE d3d_initialized : 1;
    BYTE inScene : 1;                   /* A flag to check for proper BeginScene / EndScene call pairs */
    BYTE softwareVertexProcessing : 1;  /* process vertex shaders using software or hardware */
    BYTE restore_screensaver : 1;
    BYTE padding : 3;

    unsigned char           surface_alignment; /* Line Alignment of surfaces                      */

    WORD padding2 : 16;

    /* Internal use fields  */
    struct wined3d_device_creation_parameters create_parms;
    HWND focus_window;

    struct wined3d_rendertarget_view *back_buffer_view;
    struct wined3d_swapchain **swapchains;
    UINT swapchain_count;
    unsigned int max_frame_latency;

    struct list             resources; /* a linked list to track resources created by the device */
    struct list             shaders;   /* a linked list to track shaders (pixel and vertex)      */
    struct wine_rb_tree so_descs;
    struct wine_rb_tree samplers, rasterizer_states, blend_states, depth_stencil_states;
    struct wine_rb_tree ffp_vertex_shaders, ffp_pixel_shaders;

    /* Render Target Support */
    struct wined3d_rendertarget_view *auto_depth_stencil_view;

    /* Cursor management */
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;
    UINT                    cursorWidth, cursorHeight;
    struct wined3d_texture *cursor_texture;
    HCURSOR                 hardwareCursor;

    /* The Wine logo texture */
    struct wined3d_texture *logo_texture;

    /* Default sampler used to emulate the direct resource access without using wined3d_sampler */
    struct wined3d_sampler *default_sampler;
    struct wined3d_sampler *null_sampler;

    /* Command stream */
    struct wined3d_cs *cs;

    struct wined3d_buffer *push_constants[WINED3D_PUSH_CONSTANTS_COUNT];

    /* Context management */
    struct wined3d_context **contexts;
    UINT context_count;

    CRITICAL_SECTION bo_map_lock;
};

void wined3d_device_cleanup(struct wined3d_device *device);
BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context);
void device_context_remove(struct wined3d_device *device, struct wined3d_context *context);
void wined3d_device_create_default_samplers(struct wined3d_device *device,
        struct wined3d_context *context);
void wined3d_device_destroy_default_samplers(struct wined3d_device *device);
HRESULT wined3d_device_init(struct wined3d_device *device, struct wined3d *wined3d,
        unsigned int adapter_idx, enum wined3d_device_type device_type, HWND focus_window, unsigned int flags,
        BYTE surface_alignment, const enum wined3d_feature_level *levels, unsigned int level_count,
        const BOOL *supported_extensions, struct wined3d_device_parent *device_parent);
LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc);
void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource);
void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource);
void device_invalidate_state(const struct wined3d_device *device, unsigned int state);
HRESULT wined3d_device_set_implicit_swapchain(struct wined3d_device *device,
        struct wined3d_swapchain *swapchain);
void wined3d_device_uninit_3d(struct wined3d_device *device);

static inline void wined3d_device_bo_map_lock(struct wined3d_device *device)
{
    EnterCriticalSection(&device->bo_map_lock);
}

static inline void wined3d_device_bo_map_unlock(struct wined3d_device *device)
{
    LeaveCriticalSection(&device->bo_map_lock);
}

struct wined3d_device_no3d
{
    struct wined3d_device d;

    struct wined3d_context context_no3d;
};

static inline struct wined3d_device_no3d *wined3d_device_no3d(struct wined3d_device *device)
{
    return CONTAINING_RECORD(device, struct wined3d_device_no3d, d);
}

#define WINED3D_ALLOCATOR_CHUNK_SIZE        (64 * 1024 * 1024)
#define WINED3D_ALLOCATOR_CHUNK_ORDER_COUNT 15
#define WINED3D_ALLOCATOR_MIN_BLOCK_SIZE    (WINED3D_ALLOCATOR_CHUNK_SIZE >> (WINED3D_ALLOCATOR_CHUNK_ORDER_COUNT - 1))
#define WINED3D_SLAB_BO_MIN_OBJECT_ALIGN    16
#define WINED3D_RETIRED_BO_SIZE_THRESHOLD   (64 * 1024 * 1024)

struct wined3d_allocator_chunk
{
    struct list entry;
    struct list available[WINED3D_ALLOCATOR_CHUNK_ORDER_COUNT];
    struct wined3d_allocator *allocator;
    unsigned int map_count;
    void *map_ptr;
};

void wined3d_allocator_chunk_cleanup(struct wined3d_allocator_chunk *chunk);
bool wined3d_allocator_chunk_init(struct wined3d_allocator_chunk *chunk,
        struct wined3d_allocator *allocator);

struct wined3d_allocator_block
{
    struct list entry;
    struct wined3d_allocator_chunk *chunk;
    struct wined3d_allocator_block *parent, *sibling;
    unsigned int order;
    size_t offset;
    bool free;
};

void wined3d_allocator_block_free(struct wined3d_allocator_block *block);

struct wined3d_allocator_pool
{
    struct list chunks;
};

struct wined3d_allocator_ops
{
    struct wined3d_allocator_chunk *(*allocator_create_chunk)(struct wined3d_allocator *allocator,
            struct wined3d_context *context, unsigned int memory_type, size_t chunk_size);
    void (*allocator_destroy_chunk)(struct wined3d_allocator_chunk *chunk);
};

struct wined3d_allocator
{
    const struct wined3d_allocator_ops *ops;
    struct wined3d_allocator_pool *pools;
    size_t pool_count;
    struct wined3d_allocator_block *free;
};

struct wined3d_allocator_block *wined3d_allocator_allocate(struct wined3d_allocator *allocator,
        struct wined3d_context *context, unsigned int memory_type, size_t size);
void wined3d_allocator_cleanup(struct wined3d_allocator *allocator);
bool wined3d_allocator_init(struct wined3d_allocator *allocator,
        size_t pool_count, const struct wined3d_allocator_ops *allocator_ops);

const char *wined3d_debug_resource_access(uint32_t access);
const char *wined3d_debug_bind_flags(uint32_t bind_flags);
const char *wined3d_debug_view_desc(const struct wined3d_view_desc *d,
        const struct wined3d_resource *resource);

static inline ULONG wined3d_atomic_decrement_mutex_lock(volatile LONG *refcount)
{
    ULONG count, old_count = *refcount;
    do
    {
        if ((count = old_count) == 1)
        {
            wined3d_mutex_lock();
            count = InterlockedDecrement(refcount);
            if (count) wined3d_mutex_unlock();
            return count;
        }

        old_count = InterlockedCompareExchange(refcount, count - 1, count);
    }
    while (old_count != count);

    return count - 1;
}

#define CLIENT_BO_DISCARDED ((struct wined3d_bo *)~(UINT_PTR)0)

struct wined3d_client_resource
{
    /* The resource's persistently mapped address, which we may use to perform
     * NOOVERWRITE maps from the client thread. */
    struct wined3d_bo_address addr;

    /* The currently mapped upload BO, if applicable, and box. */
    struct upload_bo mapped_upload;
    struct wined3d_box mapped_box;
};

static inline BOOL wined3d_resource_access_is_managed(unsigned int access)
{
    return !(~access & (WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU));
}

struct wined3d_resource_ops
{
    ULONG (*resource_incref)(struct wined3d_resource *resource);
    ULONG (*resource_decref)(struct wined3d_resource *resource);
    void (*resource_preload)(struct wined3d_resource *resource);
    void (*resource_unload)(struct wined3d_resource *resource);
    HRESULT (*resource_sub_resource_get_desc)(struct wined3d_resource *resource,
            unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc);
    void (*resource_sub_resource_get_map_pitch)(struct wined3d_resource *resource,
            unsigned int sub_resource_idx, unsigned int *row_pitch, unsigned int *slice_pitch);
    HRESULT (*resource_sub_resource_map)(struct wined3d_resource *resource, unsigned int sub_resource_idx,
            void **map_ptr, const struct wined3d_box *box, uint32_t flags);
    HRESULT (*resource_sub_resource_unmap)(struct wined3d_resource *resource, unsigned int sub_resource_idx);
};

struct wined3d_resource
{
    LONG ref;
    LONG bind_count;
    LONG map_count;
    ULONG access_time;
    struct wined3d_device *device;
    enum wined3d_resource_type type;
    enum wined3d_gl_resource_type gl_type;
    const struct wined3d_format *format;
    unsigned int format_attrs;
    unsigned int format_caps;
    enum wined3d_multisample_type multisample_type;
    UINT multisample_quality;
    uint32_t usage;
    unsigned int bind_flags;
    unsigned int access;
    WORD draw_binding;
    WORD map_binding;
    UINT width;
    UINT height;
    UINT depth;
    UINT size;
    unsigned int priority;
    void *heap_pointer;
    void *heap_memory;

    uint32_t pin_sysmem : 1;

    struct wined3d_client_resource client;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    const struct wined3d_resource_ops *resource_ops;

    struct list resource_list_entry;

    LONG srv_bind_count_device;
    LONG rtv_bind_count_device;
};

static inline ULONG wined3d_resource_incref(struct wined3d_resource *resource)
{
    return resource->resource_ops->resource_incref(resource);
}

static inline ULONG wined3d_resource_decref(struct wined3d_resource *resource)
{
    return resource->resource_ops->resource_decref(resource);
}

static inline HRESULT wined3d_resource_get_sub_resource_desc(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    return resource->resource_ops->resource_sub_resource_get_desc(resource, sub_resource_idx, desc);
}

static inline void wined3d_resource_get_sub_resource_map_pitch(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    resource->resource_ops->resource_sub_resource_get_map_pitch(resource, sub_resource_idx, row_pitch, slice_pitch);
}

void resource_cleanup(struct wined3d_resource *resource);
HRESULT resource_init(struct wined3d_resource *resource, struct wined3d_device *device,
        enum wined3d_resource_type type, const struct wined3d_format *format,
        enum wined3d_multisample_type multisample_type, unsigned int multisample_quality, unsigned int usage,
        unsigned int bind_flags, unsigned int access, unsigned int width, unsigned int height, unsigned int depth,
        unsigned int size, void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops);
void *resource_offset_map_pointer(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        uint8_t *base_memory, const struct wined3d_box *box);
void resource_unload(struct wined3d_resource *resource);
HRESULT wined3d_resource_check_box_dimensions(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, const struct wined3d_box *box);
void wined3d_resource_free_sysmem(struct wined3d_resource *resource);
const struct wined3d_format *wined3d_resource_get_decompress_format(
        const struct wined3d_resource *resource);
unsigned int wined3d_resource_get_sample_count(const struct wined3d_resource *resource);
BOOL wined3d_resource_is_offscreen(struct wined3d_resource *resource);
BOOL wined3d_resource_prepare_sysmem(struct wined3d_resource *resource);
void wined3d_resource_update_draw_binding(struct wined3d_resource *resource);
void wined3d_resource_memory_colour_fill(struct wined3d_resource *resource,
        const struct wined3d_map_desc *map, const struct wined3d_color *colour,
        const struct wined3d_box *box, bool full_subresource);

/* Tests show that the start address of resources is 32 byte aligned */
#define RESOURCE_ALIGNMENT 16
#define WINED3D_CONSTANT_BUFFER_ALIGNMENT 16

#define WINED3D_LOCATION_DISCARDED      0x00000001
#define WINED3D_LOCATION_SYSMEM         0x00000002
#define WINED3D_LOCATION_CLEARED        0x00000004
#define WINED3D_LOCATION_BUFFER         0x00000008
#define WINED3D_LOCATION_TEXTURE_RGB    0x00000010
#define WINED3D_LOCATION_TEXTURE_SRGB   0x00000020
#define WINED3D_LOCATION_DRAWABLE       0x00000040
#define WINED3D_LOCATION_RB_MULTISAMPLE 0x00000080
#define WINED3D_LOCATION_RB_RESOLVED    0x00000100

const char *wined3d_debug_location(uint32_t location);

struct wined3d_texture_ops
{
    BOOL (*texture_prepare_location)(struct wined3d_texture *texture, unsigned int sub_resource_idx,
            struct wined3d_context *context, unsigned int location);
    BOOL (*texture_load_location)(struct wined3d_texture *texture, unsigned int sub_resource_idx,
            struct wined3d_context *context, unsigned int location);
    void (*texture_unload_location)(struct wined3d_texture *texture,
            struct wined3d_context *context, unsigned int location);
    void (*texture_upload_data)(struct wined3d_context *context, const struct wined3d_const_bo_address *src_bo_addr,
            const struct wined3d_format *src_format, const struct wined3d_box *src_box, unsigned int src_row_pitch,
            unsigned int src_slice_pitch, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
            unsigned int dst_location, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z);
    void (*texture_download_data)(struct wined3d_context *context, struct wined3d_texture *src_texture,
            unsigned int src_sub_resource_idx, unsigned int src_location, const struct wined3d_box *src_box,
            const struct wined3d_bo_address *dst_bo_addr, const struct wined3d_format *dst_format,
            unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
            unsigned int dst_row_pitch, unsigned int dst_slice_pitch);
};

#define WINED3D_TEXTURE_COND_NP2            0x00000001
#define WINED3D_TEXTURE_IS_SRGB             0x00000008
#define WINED3D_TEXTURE_RGB_ALLOCATED       0x00000010
#define WINED3D_TEXTURE_RGB_VALID           0x00000020
#define WINED3D_TEXTURE_SRGB_ALLOCATED      0x00000040
#define WINED3D_TEXTURE_SRGB_VALID          0x00000080
#define WINED3D_TEXTURE_CONVERTED           0x00000100
#define WINED3D_TEXTURE_GET_DC_LENIENT      0x00000800
#define WINED3D_TEXTURE_DC_IN_USE           0x00001000
#define WINED3D_TEXTURE_DISCARD             0x00002000
#define WINED3D_TEXTURE_GET_DC              0x00004000
#define WINED3D_TEXTURE_GENERATE_MIPMAPS    0x00008000
#define WINED3D_TEXTURE_DOWNLOADABLE        0x00010000

#define WINED3D_TEXTURE_ASYNC_COLOR_KEY     0x00000001

struct wined3d_texture
{
    struct wined3d_resource resource;
    const struct wined3d_texture_ops *texture_ops;
    struct wined3d_swapchain *swapchain;
    UINT layer_count;
    unsigned int level_count;
    unsigned int download_count;
    unsigned int sysmem_count;
    unsigned int lod;
    uint32_t flags;
    DWORD update_map_binding;

    unsigned int row_pitch;
    unsigned int slice_pitch;

    struct wined3d_shader_resource_view *identity_srv;

    /* May only be accessed from the command stream worker thread. */
    struct wined3d_texture_async
    {
        uint32_t flags;

        /* Color keys for DDraw */
        struct wined3d_color_key dst_blt_color_key;
        struct wined3d_color_key src_blt_color_key;
        struct wined3d_color_key dst_overlay_color_key;
        struct wined3d_color_key src_overlay_color_key;
        struct wined3d_color_key gl_color_key;
        DWORD color_key_flags;
    } async;

    /* Color key field accessed from the client side. */
    struct wined3d_color_key src_blt_color_key;
    uint32_t color_key_flags;

    struct wined3d_dirty_regions
    {
        struct wined3d_box *boxes;
        SIZE_T boxes_size;
        unsigned int box_count;
    } *dirty_regions;

    struct wined3d_overlay_info
    {
        struct list entry;
        struct list overlays;
        struct wined3d_texture *dst_texture;
        unsigned int dst_sub_resource_idx;
        RECT src_rect;
        RECT dst_rect;
    } *overlay_info;

    struct wined3d_dc_info
    {
        HBITMAP bitmap;
        HDC dc;
    } *dc_info;

    struct wined3d_texture_sub_resource
    {
        void *parent;
        const struct wined3d_parent_ops *parent_ops;

        unsigned int offset;
        unsigned int size;

        unsigned int map_count;
        uint32_t map_flags;
        DWORD locations;
        struct wined3d_bo *bo;
        union
        {
            struct wined3d_color colour;
            struct
            {
                float depth;
                unsigned int stencil;
            };
        } clear_value;

        void *user_memory;
    } *sub_resources;
};

static inline void *wined3d_texture_allocate_object_memory(SIZE_T s, SIZE_T level_count, SIZE_T layer_count)
{
    struct wined3d_texture *t;

    if (level_count > ((~(SIZE_T)0 - s) / sizeof(*t->sub_resources)) / layer_count)
        return NULL;

    return calloc(1, s + level_count * layer_count * sizeof(*t->sub_resources));
}

static inline struct wined3d_texture *texture_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_texture, resource);
}

static inline unsigned int wined3d_texture_get_level_width(const struct wined3d_texture *texture,
        unsigned int level)
{
    return max(1, texture->resource.width >> level);
}

static inline unsigned int wined3d_texture_get_level_height(const struct wined3d_texture *texture,
        unsigned int level)
{
    return max(1, texture->resource.height >> level);
}

static inline unsigned int wined3d_texture_get_level_depth(const struct wined3d_texture *texture,
        unsigned int level)
{
    return max(1, texture->resource.depth >> level);
}

static inline void wined3d_texture_get_level_box(const struct wined3d_texture *texture,
        unsigned int level, struct wined3d_box *box)
{
    wined3d_box_set(box, 0, 0,
            wined3d_texture_get_level_width(texture, level),
            wined3d_texture_get_level_height(texture, level),
            0, wined3d_texture_get_level_depth(texture, level));
}

static inline bool wined3d_texture_is_full_rect(const struct wined3d_texture *texture,
        unsigned int level, const RECT *r)
{
    unsigned int t;

    t = wined3d_texture_get_level_width(texture, level);
    if ((r->left && r->right) || abs(r->right - r->left) != t)
        return false;
    t = wined3d_texture_get_level_height(texture, level);
    if ((r->top && r->bottom) || abs(r->bottom - r->top) != t)
        return false;
    return true;
}

HRESULT texture2d_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box, uint32_t flags,
        const struct wined3d_blt_fx *blt_fx, enum wined3d_texture_filter_type filter);

void wined3d_texture_cleanup(struct wined3d_texture *texture);
void wined3d_texture_download_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx);
void wined3d_texture_get_bo_address(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_bo_address *data, uint32_t location);
void wined3d_texture_invalidate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, uint32_t location);
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb);
BOOL wined3d_texture_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t location);
BOOL wined3d_texture_prepare_location(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, uint32_t location);
void wined3d_texture_set_swapchain(struct wined3d_texture *texture,
        struct wined3d_swapchain *swapchain);
BOOL wined3d_texture_can_use_pbo(const struct wined3d_texture *texture, const struct wined3d_d3d_info *d3d_info)
       ;
void wined3d_texture_sub_resources_destroyed(struct wined3d_texture *texture);
void wined3d_texture_translate_drawable_coords(const struct wined3d_texture *texture,
        HWND window, RECT *rect);
void wined3d_texture_update_sub_resource(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, const struct upload_bo *upload_bo, const struct wined3d_box *box,
        unsigned int row_pitch, unsigned int slice_pitch);
void wined3d_texture_upload_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box);
void wined3d_texture_validate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, uint32_t location);
bool wined3d_texture_validate_sub_resource_idx(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx);
void wined3d_texture_clear_dirty_regions(struct wined3d_texture *texture);

HRESULT wined3d_texture_no3d_init(struct wined3d_texture *texture_no3d, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_sampler
{
    struct wine_rb_entry entry;
    LONG refcount;
    struct wined3d_device *device;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    struct wined3d_sampler_desc desc;
};

struct wined3d_vertex_declaration_element
{
    const struct wined3d_format *format;
    BOOL ffp_valid;
    unsigned int input_slot;
    unsigned int offset;
    unsigned int output_slot;
    enum wined3d_input_classification input_slot_class;
    unsigned int instance_data_step_rate;
    BYTE method;
    BYTE usage;
    BYTE usage_idx;
};

struct wined3d_vertex_declaration
{
    LONG ref;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    struct wined3d_device *device;

    struct wined3d_vertex_declaration_element *elements;
    unsigned int element_count;

    bool position_transformed;
    bool point_size;
    bool diffuse;
    bool specular;
    bool normal;
    uint8_t texcoords;
};

bool wined3d_light_state_enable_light(struct wined3d_light_state *state, const struct wined3d_d3d_info *d3d_info,
        struct wined3d_light_info *light_info, BOOL enable);
struct wined3d_light_info *wined3d_light_state_get_light(const struct wined3d_light_state *state,
        unsigned int idx);

enum wined3d_cs_queue_id
{
    WINED3D_CS_QUEUE_DEFAULT = 0,
    WINED3D_CS_QUEUE_MAP,
    WINED3D_CS_QUEUE_COUNT,
};

#define WINED3D_CS_QUERY_POLL_INTERVAL  100u
#if defined(_WIN64)
#define WINED3D_CS_QUEUE_SIZE           0x1000000u
#else
#define WINED3D_CS_QUEUE_SIZE           0x400000u
#endif
#define WINED3D_CS_SPIN_COUNT           2000u
/* How long to wait for commands when there are active queries, in µs. */
#define WINED3D_CS_COMMAND_WAIT_WITH_QUERIES_TIMEOUT 100
/* How long to wait for the CS from the client thread, in µs. */
#define WINED3D_CS_CLIENT_WAIT_TIMEOUT  0
#define WINED3D_CS_QUEUE_MASK           (WINED3D_CS_QUEUE_SIZE - 1)

C_ASSERT(!(WINED3D_CS_QUEUE_SIZE & (WINED3D_CS_QUEUE_SIZE - 1)));

struct wined3d_cs_queue
{
    ULONG head, tail;
    BYTE data[WINED3D_CS_QUEUE_SIZE];
};

struct wined3d_device_context_ops
{
    void *(*require_space)(struct wined3d_device_context *context, size_t size, enum wined3d_cs_queue_id queue_id);
    void (*submit)(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id);
    void (*finish)(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id);
    bool (*map_upload_bo)(struct wined3d_device_context *context, struct wined3d_resource *resource,
            unsigned int sub_resource_idx, struct wined3d_map_desc *map_desc,
            const struct wined3d_box *box, uint32_t flags);
    bool (*unmap_upload_bo)(struct wined3d_device_context *context, struct wined3d_resource *resource,
            unsigned int sub_resource_idx, struct wined3d_box *box, struct upload_bo *upload_bo);
    void (*issue_query)(struct wined3d_device_context *context, struct wined3d_query *query, unsigned int flags);
    void (*flush)(struct wined3d_device_context *context);
    void (*reference_resource)(struct wined3d_device_context *context, struct wined3d_resource *resource);
    void (*reference_command_list)(struct wined3d_device_context *context, struct wined3d_command_list *list);
};

struct wined3d_device_context
{
    const struct wined3d_device_context_ops *ops;
    struct wined3d_device *device;
    struct wined3d_state *state;
};

struct wined3d_cs
{
    struct wined3d_device_context c;

    struct wined3d_state state;
    HMODULE wined3d_module;
    HANDLE thread;
    DWORD thread_id;
    BOOL serialize_commands;

    struct wined3d_cs_queue queue[WINED3D_CS_QUEUE_COUNT];
    size_t data_size, start, end;
    void *data;
    struct list query_poll_list;
    BOOL queries_flushed;

    HANDLE event, present_event;
    LONG waiting_for_event;
    LONG waiting_for_present;
    LONG pending_presents;
};

static inline void wined3d_device_context_lock(struct wined3d_device_context *context)
{
    if (context == &context->device->cs->c)
        wined3d_mutex_lock();
}

static inline void wined3d_device_context_unlock(struct wined3d_device_context *context)
{
    if (context == &context->device->cs->c)
        wined3d_mutex_unlock();
}

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device,
        const enum wined3d_feature_level *levels, unsigned int level_count);
void wined3d_cs_destroy(struct wined3d_cs *cs);
void wined3d_cs_destroy_object(struct wined3d_cs *cs,
        void (*callback)(void *object), void *object);
void wined3d_cs_emit_add_dirty_texture_region(struct wined3d_cs *cs,
        struct wined3d_texture *texture, unsigned int layer);
void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        uint32_t flags, const struct wined3d_color *color, float depth, DWORD stencil);
void wined3d_device_context_emit_clear_uav(struct wined3d_device_context *context,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value, bool fp);
void wined3d_cs_emit_preload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource);
void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain, const RECT *src_rect,
        const RECT *dst_rect, HWND dst_window_override, unsigned int swap_interval, uint32_t flags);
void wined3d_cs_emit_set_color_key(struct wined3d_cs *cs, struct wined3d_texture *texture,
        WORD flags, const struct wined3d_color_key *color_key);
void wined3d_cs_emit_set_render_state(struct wined3d_cs *cs,
        enum wined3d_render_state state, DWORD value);
void wined3d_cs_emit_unload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource);
void wined3d_cs_init_object(struct wined3d_cs *cs,
        void (*callback)(void *object), void *object);
void wined3d_cs_map_bo_address(struct wined3d_cs *cs,
        struct wined3d_bo_address *addr, size_t size, unsigned int flags);
void wined3d_device_context_set_depth_bounds(struct wined3d_device_context *context,
        bool enable, float min_depth, float max_depth);

static inline void wined3d_cs_finish(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
    cs->c.ops->finish(&cs->c, queue_id);
}

void wined3d_device_context_emit_blt_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx, const struct wined3d_box *dst_box,
        struct wined3d_resource *src_resource, unsigned int src_sub_resource_idx, const struct wined3d_box *src_box,
        unsigned int flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter);
void wined3d_device_context_emit_clear_rendertarget_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view, const RECT *rect, unsigned int flags,
        const struct wined3d_color *color, float depth, unsigned int stencil);
void wined3d_device_context_emit_copy_uav_counter(struct wined3d_device_context *context,
        struct wined3d_buffer *dst_buffer, unsigned int offset,
        struct wined3d_unordered_access_view *uav);
void wined3d_device_context_emit_draw(struct wined3d_device_context *context,
        enum wined3d_primitive_type primitive_type, unsigned int patch_vertex_count, int base_vertex_idx,
        unsigned int start_idx, unsigned int index_count, unsigned int start_instance, unsigned int instance_count,
        bool indexed);
void wined3d_device_context_emit_execute_command_list(struct wined3d_device_context *context,
        struct wined3d_command_list *list, bool restore_state);
void wined3d_device_context_emit_generate_mipmaps(struct wined3d_device_context *context,
        struct wined3d_shader_resource_view *view);
HRESULT wined3d_device_context_emit_map(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, struct wined3d_map_desc *map_desc,
        const struct wined3d_box *box, unsigned int flags);
void wined3d_device_context_emit_reset_state(struct wined3d_device_context *context, bool invalidate);
void wined3d_device_context_emit_set_blend_state(struct wined3d_device_context *context,
        struct wined3d_blend_state *state, const struct wined3d_color *blend_factor,
        unsigned int sample_mask);
void wined3d_device_context_emit_set_clip_plane(struct wined3d_device_context *context, unsigned int plane_idx,
        const struct wined3d_vec4 *plane);
void wined3d_device_context_emit_set_constant_buffers(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        const struct wined3d_constant_buffer_state *buffers);
void wined3d_device_context_emit_set_depth_stencil_state(struct wined3d_device_context *context,
        struct wined3d_depth_stencil_state *state, unsigned int stencil_ref);
void wined3d_device_context_emit_set_depth_stencil_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view);
void wined3d_device_context_emit_set_feature_level(struct wined3d_device_context *context,
        enum wined3d_feature_level level);
void wined3d_device_context_emit_set_index_buffer(struct wined3d_device_context *context, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id, unsigned int offset);
void wined3d_device_context_emit_set_light(struct wined3d_device_context *context,
        const struct wined3d_light_info *light);
void wined3d_device_context_emit_set_light_enable(struct wined3d_device_context *context, unsigned int idx,
        BOOL enable);
void wined3d_device_context_emit_set_predication(struct wined3d_device_context *context,
        struct wined3d_query *predicate, BOOL value);
void wined3d_device_context_emit_set_rasterizer_state(struct wined3d_device_context *context,
        struct wined3d_rasterizer_state *rasterizer_state);
void wined3d_device_context_emit_set_render_state(struct wined3d_device_context *context,
        enum wined3d_render_state state, unsigned int value);
void wined3d_device_context_emit_set_rendertarget_views(struct wined3d_device_context *context, unsigned int start_idx,
        unsigned int count, struct wined3d_rendertarget_view *const *views);
void wined3d_device_context_emit_set_samplers(struct wined3d_device_context *context, enum wined3d_shader_type type,
        unsigned int start_idx, unsigned int count, struct wined3d_sampler *const *samplers);
void wined3d_device_context_emit_set_scissor_rects(struct wined3d_device_context *context,
        unsigned int rect_count, const RECT *rects);
void wined3d_device_context_emit_set_shader(struct wined3d_device_context *context, enum wined3d_shader_type type,
        struct wined3d_shader *shader);
void wined3d_device_context_emit_set_shader_resource_views(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        struct wined3d_shader_resource_view *const *views);
void wined3d_device_context_emit_set_stream_outputs(struct wined3d_device_context *context,
        const struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS]);
void wined3d_device_context_emit_set_stream_sources(struct wined3d_device_context *context,
        unsigned int start_idx, unsigned int count, const struct wined3d_stream_state *streams);
void wined3d_device_context_emit_set_texture(struct wined3d_device_context *context,
        enum wined3d_shader_type shader_type, unsigned int bind_index,
        struct wined3d_shader_resource_view *view);
void wined3d_device_context_emit_set_texture_state(struct wined3d_device_context *context, unsigned int stage,
        enum wined3d_texture_stage_state state, unsigned int value);
void wined3d_device_context_emit_set_transform(struct wined3d_device_context *context,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix);
void wined3d_device_context_emit_set_unordered_access_views(struct wined3d_device_context *context,
        enum wined3d_pipeline pipeline, unsigned int start_idx, unsigned int count,
        struct wined3d_unordered_access_view *const *views, const unsigned int *initial_count);
void wined3d_device_context_emit_set_vertex_declaration(struct wined3d_device_context *context,
        struct wined3d_vertex_declaration *declaration);
void wined3d_device_context_emit_set_viewports(struct wined3d_device_context *context, unsigned int viewport_count,
        const struct wined3d_viewport *viewports);
void wined3d_device_context_emit_update_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, const struct wined3d_box *box,
        const void *data, unsigned int row_pitch, unsigned int slice_pitch);
HRESULT wined3d_device_context_emit_unmap(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx);
void wined3d_device_context_push_constants(struct wined3d_device_context *context,
        enum wined3d_push_constants type, uint32_t update_mask, unsigned int start_idx,
        unsigned int count, const void *constants);

static inline void wined3d_resource_reference(struct wined3d_resource *resource)
{
    const struct wined3d_cs *cs = resource->device->cs;
    resource->access_time = cs->queue[WINED3D_CS_QUEUE_DEFAULT].head;
}

#define WINED3D_PAUSE_SPIN_COUNT 200u

static inline void wined3d_pause(unsigned int *spin_count)
{
    static const LARGE_INTEGER timeout = {.QuadPart = WINED3D_CS_CLIENT_WAIT_TIMEOUT * -10};

    if (++*spin_count >= WINED3D_PAUSE_SPIN_COUNT)
        NtDelayExecution(FALSE, &timeout);
}

static inline BOOL wined3d_ge_wrap(ULONG x, ULONG y)
{
    return (x - y) < UINT_MAX / 2;
}
C_ASSERT(WINED3D_CS_QUEUE_SIZE < UINT_MAX / 4);

static inline void wined3d_resource_wait_idle(const struct wined3d_resource *resource)
{
    const struct wined3d_cs *cs = resource->device->cs;
    ULONG access_time, tail, head;
    unsigned int spin_count = 0;

    if (!cs->thread || cs->thread_id == GetCurrentThreadId())
        return;

    access_time = resource->access_time;
    head = cs->queue[WINED3D_CS_QUEUE_DEFAULT].head;

    /* The basic idea is that a resource is busy if tail < access_time <= head.
     * But we have to be careful about wrap-around of the head and tail. The
     * wined3d_ge_wrap function considers x >= y if x - y is smaller than half the
     * UINT range. Head is at most WINED3D_CS_QUEUE_SIZE ahead of tail, because
     * otherwise the queue memory is considered full and queue_require_space
     * stalls. Thus wined3d_ge_wrap(head, tail) is always true. The C_ASSERT above
     * ensures this in case we decide to grow the queue size in the future.
     *
     * It is possible that a resource has not been used for a long time and is idle, but the head and
     * tail wrapped around in such a way that the previously set access time falls between head and tail.
     * In this case we will incorrectly wait for the resource. Because we use the entire 32 bits of the
     * counters and not just the bits needed to address the actual queue memory, this should happen rarely.
     * If it turns out to be a problem we can switch to 64 bit counters or attempt to somehow mark the
     * access time of resources invalid. CS packets are at least 4 byte aligned, so we could use the lower
     * 2 bits in access_time for such a marker.
     *
     * Note that the access time is set before the command is submitted, so we have to wait until the
     * tail is bigger than access_time, not equal. */

    if (!wined3d_ge_wrap(head, access_time))
        return;

    for (;;)
    {
        tail = *(volatile ULONG *)&cs->queue[WINED3D_CS_QUEUE_DEFAULT].tail;
        if (head == tail) /* Queue empty. */
            break;

        if (!wined3d_ge_wrap(access_time, tail) && access_time != tail)
            break;

        wined3d_pause(&spin_count);
    }
}

struct wined3d_buffer
{
    struct wined3d_resource resource;
    const struct wined3d_buffer_ops *buffer_ops;

    unsigned int structure_byte_stride;
    uint32_t flags;
    DWORD locations;
    void *map_ptr;
    struct wined3d_bo *buffer_object;
    struct wined3d_bo_user bo_user;

    /* For buffers which are GPU accessible but for which BUFFER is not
     * currently a valid location, this is a list of areas that need to be
     * uploaded to BUFFER. */
    struct wined3d_range *dirty_ranges;
    SIZE_T dirty_range_count, dirty_ranges_capacity;
};

static inline struct wined3d_buffer *buffer_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_buffer, resource);
}

static inline void wined3d_buffer_validate_user(struct wined3d_buffer *buffer)
{
    if (buffer->bo_user.valid)
        return;
    buffer->bo_user.valid = true;
    list_add_head(&buffer->buffer_object->users, &buffer->bo_user.entry);
}

void wined3d_buffer_acquire_bo_for_write(struct wined3d_buffer *buffer,
        struct wined3d_context *context);
void wined3d_buffer_cleanup(struct wined3d_buffer *buffer);
void wined3d_buffer_copy(struct wined3d_buffer *dst_buffer, unsigned int dst_offset,
        struct wined3d_buffer *src_buffer, unsigned int src_offset, unsigned int size);
void wined3d_buffer_copy_bo_address(struct wined3d_buffer *dst_buffer, struct wined3d_context *context,
        unsigned int dst_offset, const struct wined3d_const_bo_address *src_addr, unsigned int size);
DWORD wined3d_buffer_get_memory(struct wined3d_buffer *buffer, struct wined3d_context *context,
        struct wined3d_bo_address *data);
void wined3d_buffer_invalidate_location(struct wined3d_buffer *buffer, uint32_t location);
void wined3d_buffer_load(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct wined3d_state *state);
BOOL wined3d_buffer_load_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, uint32_t location);
void *wined3d_buffer_load_sysmem(struct wined3d_buffer *buffer, struct wined3d_context *context);
BOOL wined3d_buffer_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location);
void wined3d_buffer_update_sub_resource(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct upload_bo *upload_bo, unsigned int offset, unsigned int size);
void wined3d_buffer_validate_location(struct wined3d_buffer *buffer, uint32_t location);

HRESULT wined3d_buffer_no3d_init(struct wined3d_buffer *buffer_no3d, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_rendertarget_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    const struct wined3d_format *format;
    unsigned int format_attrs;
    unsigned int format_caps;
    unsigned int sub_resource_idx;
    unsigned int layer_count;

    unsigned int width;
    unsigned int height;

    struct wined3d_view_desc desc;
};

void wined3d_rendertarget_view_cleanup(struct wined3d_rendertarget_view *view);
void wined3d_rendertarget_view_get_drawable_size(const struct wined3d_rendertarget_view *view,
        const struct wined3d_context *context, unsigned int *width, unsigned int *height);
void wined3d_rendertarget_view_get_box(struct wined3d_rendertarget_view *view,
        struct wined3d_box *box);
void wined3d_rendertarget_view_invalidate_location(struct wined3d_rendertarget_view *view,
        uint32_t location);
void wined3d_rendertarget_view_load_location(struct wined3d_rendertarget_view *view,
        struct wined3d_context *context, uint32_t location);
void wined3d_rendertarget_view_prepare_location(struct wined3d_rendertarget_view *view,
        struct wined3d_context *context, uint32_t location);
void wined3d_rendertarget_view_validate_location(struct wined3d_rendertarget_view *view,
        uint32_t location);
DWORD wined3d_rendertarget_view_get_locations(const struct wined3d_rendertarget_view *view)
       ;

HRESULT wined3d_rendertarget_view_no3d_init(struct wined3d_rendertarget_view *view_no3d,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_shader_resource_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    const struct wined3d_format *format;

    struct wined3d_view_desc desc;
};

void wined3d_shader_resource_view_cleanup(struct wined3d_shader_resource_view *view);
void wined3d_shader_resource_view_destroy(struct wined3d_shader_resource_view *view);

static inline struct wined3d_texture *wined3d_state_get_ffp_texture(const struct wined3d_state *state, unsigned int idx)
{
    struct wined3d_shader_resource_view *view = state->shader_resource_view[WINED3D_SHADER_TYPE_PIXEL][idx];

    assert(idx <= WINED3D_MAX_FFP_TEXTURES);
    return view ? texture_from_resource(view->resource) : NULL;
}

struct wined3d_unordered_access_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    const struct wined3d_format *format;

    struct wined3d_view_desc desc;
    struct wined3d_bo *counter_bo;
};

void wined3d_unordered_access_view_cleanup(struct wined3d_unordered_access_view *view);
void wined3d_unordered_access_view_copy_counter(struct wined3d_unordered_access_view *view,
        struct wined3d_buffer *buffer, unsigned int offset, struct wined3d_context *context);
void wined3d_unordered_access_view_invalidate_location(struct wined3d_unordered_access_view *view,
        uint32_t location);
void wined3d_unordered_access_view_set_counter(struct wined3d_unordered_access_view *view,
        unsigned int value);

struct wined3d_swapchain_state
{
    struct wined3d *wined3d;
    struct wined3d_swapchain_desc desc;
    struct wined3d_swapchain_state_parent *parent;

    struct wined3d_display_mode original_mode, d3d_mode;
    RECT original_window_rect;

    /* Window styles to restore when switching fullscreen mode. */
    LONG style;
    LONG exstyle;
    HWND device_window;
};

void wined3d_swapchain_state_cleanup(struct wined3d_swapchain_state *state);
void wined3d_swapchain_state_register(struct wined3d_swapchain_state *state);
void wined3d_swapchain_state_restore_from_fullscreen(struct wined3d_swapchain_state *state,
        HWND window, const RECT *window_rect);
HRESULT wined3d_swapchain_state_setup_fullscreen(struct wined3d_swapchain_state *state,
        HWND window, int x, int y, int width, int height);

struct wined3d_swapchain_ops
{
    void (*swapchain_present)(struct wined3d_swapchain *swapchain,
            const RECT *src_rect, const RECT *dst_rect, unsigned int swap_interval, uint32_t flags);
    void (*swapchain_frontbuffer_updated)(struct wined3d_swapchain *swapchain);
};

struct wined3d_swapchain
{
    LONG ref;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    const struct wined3d_swapchain_ops *swapchain_ops;
    struct wined3d_device *device;

    struct wined3d_texture **back_buffers;
    struct wined3d_texture *front_buffer;
    struct wined3d_gamma_ramp orig_gamma;
    bool reapply_mode;
    const struct wined3d_format *ds_format;
    struct wined3d_palette *palette;
    RECT front_buffer_update;
    unsigned int swap_interval;
    unsigned int max_frame_latency;

    /* Performance tracking */
    LARGE_INTEGER last_present_time;
    LONG prev_time, frames;

    struct wined3d_swapchain_state state;
    HWND win_handle;
};

void wined3d_swapchain_activate(struct wined3d_swapchain *swapchain, BOOL activate);
void wined3d_swapchain_cleanup(struct wined3d_swapchain *swapchain);
struct wined3d_output * wined3d_swapchain_get_output(const struct wined3d_swapchain *swapchain);
void swapchain_update_draw_bindings(struct wined3d_swapchain *swapchain);
void swapchain_set_max_frame_latency(struct wined3d_swapchain *swapchain,
        const struct wined3d_device *device);

HRESULT wined3d_swapchain_no3d_init(struct wined3d_swapchain *swapchain_no3d,
        struct wined3d_device *device, const struct wined3d_swapchain_desc *desc,
        struct wined3d_swapchain_state_parent *state_parent, void *parent,
        const struct wined3d_parent_ops *parent_ops);

/*****************************************************************************
 * Utility function prototypes
 */

/* Trace routines */
const char *debug_bo_address(const struct wined3d_bo_address *address);
const char *debug_box(const struct wined3d_box *box);
const char *debug_color(const struct wined3d_color *color);
const char *debug_const_bo_address(const struct wined3d_const_bo_address *address);
const char *debug_d3dshaderinstructionhandler(enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx);
const char *debug_d3dformat(enum wined3d_format_id format_id);
const char *debug_d3ddevicetype(enum wined3d_device_type device_type);
const char *debug_d3dresourcetype(enum wined3d_resource_type resource_type);
const char *debug_d3dusage(uint32_t usage);
const char *debug_d3ddeclmethod(enum wined3d_decl_method method);
const char *debug_d3ddeclusage(enum wined3d_decl_usage usage);
const char *debug_d3dinput_classification(enum wined3d_input_classification classification);
const char *debug_d3dprimitivetype(enum wined3d_primitive_type primitive_type);
const char *debug_d3drenderstate(enum wined3d_render_state state);
const char *debug_d3dsamplerstate(enum wined3d_sampler_state state);
const char *debug_d3dstate(uint32_t state);
const char *debug_d3dtexturefiltertype(enum wined3d_texture_filter_type filter_type);
const char *debug_d3dtexturestate(enum wined3d_texture_stage_state state);
const char *debug_d3dtop(enum wined3d_texture_op d3dtop);
const char *debug_d3dtstype(enum wined3d_transform_state tstype);
const char *debug_ivec4(const struct wined3d_ivec4 *v);
const char *debug_uvec4(const struct wined3d_uvec4 *v);
const char *debug_shader_type(enum wined3d_shader_type shader_type);
const char *debug_vec4(const struct wined3d_vec4 *v);
const char *wined3d_debug_feature_level(enum wined3d_feature_level level);
void dump_color_fixup_desc(struct color_fixup_desc fixup);

BOOL is_invalid_op(const struct wined3d_state *state, int stage,
        enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3);
void state_nop(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);
void state_srgbwrite(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);

void state_clipping(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);
void clipplane(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);
void state_shademode(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id);

/* Math utils */
void multiply_matrix(struct wined3d_matrix *dest, const struct wined3d_matrix *src1,
        const struct wined3d_matrix *src2);

void wined3d_release_dc(HWND window, HDC dc);

struct wined3d_shader_lconst
{
    struct list entry;
    unsigned int idx;
    unsigned int value[4];
};

struct wined3d_shader_limits
{
    unsigned int sampler;
    unsigned int constant_int;
    unsigned int constant_float;
    unsigned int constant_bool;
    unsigned int packed_output;
    unsigned int packed_input;
};

#define wined3d_lock_init(lock, name) wined3d_lock_init_(lock, __FILE__ ": " name)
static inline void wined3d_lock_init_(CRITICAL_SECTION *lock, const char *name)
{
    InitializeCriticalSectionEx(lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    if (lock->DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        lock->DebugInfo->Spare[0] = (DWORD_PTR)name;
}

static inline void wined3d_lock_cleanup(CRITICAL_SECTION *lock)
{
    if (lock->DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        lock->DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(lock);
}

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

struct wined3d_string_buffer_list
{
    struct list list;
};

struct wined3d_string_buffer *string_buffer_get(struct wined3d_string_buffer_list *list);
void string_buffer_sprintf(struct wined3d_string_buffer *buffer, const char *format, ...) PRINTF_ATTR(2, 3);
void string_buffer_release(struct wined3d_string_buffer_list *list, struct wined3d_string_buffer *buffer);
void string_buffer_list_init(struct wined3d_string_buffer_list *list);
void string_buffer_list_cleanup(struct wined3d_string_buffer_list *list);

int shader_addline(struct wined3d_string_buffer *buffer, const char *fmt, ...) PRINTF_ATTR(2,3);
BOOL string_buffer_resize(struct wined3d_string_buffer *buffer, int rc);
int shader_vaddline(struct wined3d_string_buffer *buffer, const char *fmt, va_list args);

struct wined3d_shader_phase
{
    const DWORD *start;
    const DWORD *end;
    unsigned int instance_count;
    unsigned int temporary_count;
};

struct wined3d_vertex_shader
{
    struct wined3d_shader_attribute attributes[MAX_ATTRIBS];
};

struct wined3d_hull_shader
{
    struct
    {
        struct wined3d_shader_phase *control_point;
        unsigned int fork_count;
        unsigned int join_count;
        struct wined3d_shader_phase *fork;
        SIZE_T fork_size;
        struct wined3d_shader_phase *join;
        SIZE_T join_size;
    } phases;
    unsigned int output_vertex_count;
    enum wined3d_tessellator_output_primitive tessellator_output_primitive;
    enum wined3d_tessellator_partitioning tessellator_partitioning;
};

struct wined3d_domain_shader
{
    enum wined3d_tessellator_domain tessellator_domain;
};

struct wined3d_geometry_shader
{
    enum wined3d_primitive_type input_type;
    enum wined3d_primitive_type output_type;
    unsigned int vertices_out;
    unsigned int instance_count;

    const struct wined3d_stream_output_desc *so_desc;
};

struct wined3d_pixel_shader
{
    /* Pixel shader input semantics */
    unsigned int input_reg_map[MAX_REG_INPUT];
    DWORD input_reg_used; /* MAX_REG_INPUT, 32 */
    unsigned int declared_in_count;

    /* Some information about the shader behavior */
    BOOL color0_mov;
    unsigned int color0_reg;

    BOOL force_early_depth_stencil;
    enum wined3d_shader_register_type depth_output;
    uint32_t interpolation_mode[WINED3D_PACKED_INTERPOLATION_SIZE];
};

struct wined3d_compute_shader
{
    struct wined3d_shader_thread_group_size thread_group_size;
};

struct wined3d_shader
{
    LONG ref;
    const struct wined3d_shader_limits *limits;
    const DWORD *function;
    unsigned int functionLength;
    void *byte_code;
    unsigned int byte_code_size;
    bool load_local_constsF;
    bool is_ffp_vs;
    bool is_ffp_ps;
    enum vkd3d_shader_source_type source_type;
    const struct wined3d_shader_frontend *frontend;
    void *frontend_data;
    void *backend_data;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    /* Programs this shader is linked with */
    struct list linked_programs;

    /* Immediate constants (override global ones) */
    struct list constantsB;
    struct list constantsF;
    struct list constantsI;
    struct wined3d_shader_reg_maps reg_maps;
    BOOL lconst_inf_or_nan;

    struct wined3d_shader_signature input_signature;
    struct wined3d_shader_signature output_signature;
    struct wined3d_shader_signature patch_constant_signature;

    /* Pointer to the parent device */
    struct wined3d_device *device;
    struct list shader_list_entry;

    union
    {
        struct wined3d_vertex_shader vs;
        struct wined3d_hull_shader hs;
        struct wined3d_domain_shader ds;
        struct wined3d_geometry_shader gs;
        struct wined3d_pixel_shader ps;
        struct wined3d_compute_shader cs;
    } u;
};

HRESULT wined3d_shader_create_ffp_vs(struct wined3d_device *device,
        const struct wined3d_ffp_vs_settings *settings, struct wined3d_shader **shader);
HRESULT wined3d_shader_create_ffp_ps(struct wined3d_device *device,
        const struct ffp_frag_settings *settings, struct wined3d_shader **shader);

enum wined3d_shader_resource_type pixelshader_get_resource_type(const struct wined3d_shader_reg_maps *reg_maps,
        unsigned int resource_idx, DWORD tex_types);
void find_ps_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        BOOL position_transformed, struct ps_compile_args *args,
        const struct wined3d_context *context);

bool vshader_get_input(const struct wined3d_shader *shader,
        uint8_t usage_req, uint8_t usage_idx_req, unsigned int *regnum);
void find_vs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        struct vs_compile_args *args, const struct wined3d_context *context);

void find_ds_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        struct ds_compile_args *args, const struct wined3d_context *context);

void find_gs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        struct gs_compile_args *args, const struct wined3d_context *context);

void string_buffer_clear(struct wined3d_string_buffer *buffer);
BOOL string_buffer_init(struct wined3d_string_buffer *buffer);
void string_buffer_free(struct wined3d_string_buffer *buffer);
unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps,
        unsigned int max);
HRESULT shader_generate_code(const struct wined3d_shader *shader, struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, void *backend_ctx,
        const DWORD *start, const DWORD *end);
BOOL shader_match_semantic(const char *semantic_name, enum wined3d_decl_usage usage);

enum vkd3d_shader_visibility vkd3d_shader_visibility_from_wined3d(enum wined3d_shader_type shader_type);

bool ffp_hlsl_compile_vs(const struct wined3d_ffp_vs_settings *settings,
        struct wined3d_shader_desc *shader_desc, struct wined3d_device *device);
bool ffp_hlsl_compile_ps(const struct ffp_frag_settings *settings, struct wined3d_shader_desc *shader_desc);

static inline BOOL shader_is_scalar(const struct wined3d_shader_register *reg)
{
    switch (reg->type)
    {
        case WINED3DSPR_RASTOUT:
            /* oFog & oPts */
            if (reg->idx[0].offset)
                return TRUE;
            /* oPos */
            return FALSE;

        case WINED3DSPR_CONSTBOOL:  /* b# */
        case WINED3DSPR_DEPTHOUT:   /* oDepth */
        case WINED3DSPR_DEPTHOUTGE:
        case WINED3DSPR_DEPTHOUTLE:
        case WINED3DSPR_LOOP:       /* aL */
        case WINED3DSPR_OUTPOINTID:
        case WINED3DSPR_PREDICATE:  /* p0 */
        case WINED3DSPR_PRIMID:     /* primID */
        case WINED3DSPR_COVERAGE: /* vCoverage */
        case WINED3DSPR_SAMPLEMASK: /* oMask */
        case WINED3DSPR_STENCILREF: /* oStencilRef */
            return TRUE;

        case WINED3DSPR_MISCTYPE:
            switch (reg->idx[0].offset)
            {
                case 0: /* vPos */
                    return FALSE;
                case 1: /* vFace */
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DSPR_IMMCONST:
            return reg->immconst_type == WINED3D_IMMCONST_SCALAR;

        default:
            return FALSE;
    }
}

static inline void shader_get_position_fixup(const struct wined3d_context *context,
        const struct wined3d_state *state, unsigned int fixup_count, float *position_fixup)
{
    float center_offset, x = 0.0f, y = 0.0f;
    unsigned int i;

    /* See get_projection_matrix() in glsl_shader.c for a discussion of the position fixup.
     * This function here also applies to d3d10+ which does not need adjustment for
     * integer pixel centers, but it may need the filling convention offset. */
    if (context->d3d_info->wined3d_creation_flags & WINED3D_PIXEL_CENTER_INTEGER)
        center_offset = 1.0f;
    else
        center_offset = 0.0f;

    center_offset += context->d3d_info->filling_convention_offset;

    for (i = 0; i < fixup_count; ++i)
    {
        position_fixup[4 * i    ] = 1.0f;
        position_fixup[4 * i + 1] = -1.0f;
        if (!context->d3d_info->subpixel_viewport)
        {
            double dummy;
            x = modf(state->viewports[i].x, &dummy) * 2.0f;
            y = modf(state->viewports[i].y, &dummy) * 2.0f;
        }
        position_fixup[4 * i + 2] = (center_offset + x) / state->viewports[i].width;
        position_fixup[4 * i + 3] = (center_offset + y) / state->viewports[i].height;
    }
}

static inline BOOL shader_constant_is_local(const struct wined3d_shader *shader, DWORD reg)
{
    struct wined3d_shader_lconst *lconst;

    if (shader->load_local_constsF)
        return FALSE;

    LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
    {
        if (lconst->idx == reg)
            return TRUE;
    }

    return FALSE;
}

static inline BOOL shader_sampler_is_shadow(const struct wined3d_shader *shader,
        const struct ps_compile_args *ps_args, unsigned int resource_idx, unsigned int sampler_idx)
{
    const struct wined3d_shader_version *version = &shader->reg_maps.shader_version;

    if (version->major >= 4)
        return shader->reg_maps.sampler_comparison_mode & (1u << sampler_idx);
    else
        return version->type == WINED3D_SHADER_TYPE_PIXEL && (ps_args->shadow & (1u << resource_idx));
}

void get_identity_matrix(struct wined3d_matrix *mat);
void get_modelview_matrix(const struct wined3d_stateblock_state *state, unsigned int index, struct wined3d_matrix *mat);
void get_texture_matrix(const struct wined3d_stateblock_state *state,
        const unsigned int tex, struct wined3d_matrix *mat);
void get_pointsize_minmax(const struct wined3d_context *context, const struct wined3d_state *state,
        float *out_min, float *out_max);
void get_fog_start_end(const struct wined3d_context *context, const struct wined3d_state *state,
        float *start, float *end);

struct wined3d_palette
{
    LONG ref;
    struct wined3d_device *device;

    unsigned int size;
    RGBQUAD colors[256];
    uint32_t flags;
};

/* DirectDraw utility functions */
extern enum wined3d_format_id pixelformat_for_depth(DWORD depth);

/*****************************************************************************
 * Pixel format management
 */

/* Pixel format attributes */
#define WINED3D_FORMAT_ATTR_FLOAT                   0x00000001
#define WINED3D_FORMAT_ATTR_INTEGER                 0x00000002
#define WINED3D_FORMAT_ATTR_NORMALISED              0x00000004
#define WINED3D_FORMAT_ATTR_BUMPMAP                 0x00000008
#define WINED3D_FORMAT_ATTR_EXTENSION               0x00000010
#define WINED3D_FORMAT_ATTR_BLOCKS                  0x00000020
#define WINED3D_FORMAT_ATTR_BLOCKS_NO_VERIFY        0x00000040
#define WINED3D_FORMAT_ATTR_COMPRESSED              0x00000080
#define WINED3D_FORMAT_ATTR_BROKEN_PITCH            0x00000100
#define WINED3D_FORMAT_ATTR_HEIGHT_SCALE            0x00000200
#define WINED3D_FORMAT_ATTR_MAPPABLE                0x00000400
#define WINED3D_FORMAT_ATTR_CAST_TO_BLOCK           0x00000800

/* Pixel format capabilities */
#define WINED3D_FORMAT_CAP_POSTPIXELSHADER_BLENDING     0x00000001
#define WINED3D_FORMAT_CAP_FILTERING                    0x00000002
#define WINED3D_FORMAT_CAP_UNORDERED_ACCESS             0x00000004
#define WINED3D_FORMAT_CAP_DEPTH_STENCIL                0x00000008
#define WINED3D_FORMAT_CAP_RENDERTARGET                 0x00000010
#define WINED3D_FORMAT_CAP_FBO_ATTACHABLE               0x00000020
#define WINED3D_FORMAT_CAP_FBO_ATTACHABLE_SRGB          0x00000040
#define WINED3D_FORMAT_CAP_DECOMPRESS                   0x00000080
#define WINED3D_FORMAT_CAP_SRGB_READ                    0x00000100
#define WINED3D_FORMAT_CAP_SRGB_WRITE                   0x00000200
#define WINED3D_FORMAT_CAP_VTF                          0x00000400
#define WINED3D_FORMAT_CAP_SHADOW                       0x00000800
#define WINED3D_FORMAT_CAP_TEXTURE                      0x00001000
#define WINED3D_FORMAT_CAP_GEN_MIPMAP                   0x00002000
#define WINED3D_FORMAT_CAP_VERTEX_ATTRIBUTE             0x00004000
#define WINED3D_FORMAT_CAP_BLIT                         0x00008000
#define WINED3D_FORMAT_CAP_INDEX_BUFFER                 0x00010000

struct wined3d_rational
{
    UINT numerator;
    UINT denominator;
};

struct wined3d_color_key_conversion
{
    enum wined3d_format_id dst_format;
    void (*convert)(const BYTE *src, unsigned int src_pitch, BYTE *dst, unsigned int dst_pitch,
            unsigned int width, unsigned int height, const struct wined3d_color_key *colour_key);
};

enum wined3d_channel_type
{
    WINED3D_CHANNEL_TYPE_NONE,
    WINED3D_CHANNEL_TYPE_UNORM,
    WINED3D_CHANNEL_TYPE_SNORM,
    WINED3D_CHANNEL_TYPE_UINT,
    WINED3D_CHANNEL_TYPE_SINT,
    WINED3D_CHANNEL_TYPE_FLOAT,
    WINED3D_CHANNEL_TYPE_DEPTH,
    WINED3D_CHANNEL_TYPE_STENCIL,
    WINED3D_CHANNEL_TYPE_UNUSED,
};

struct wined3d_format
{
    enum wined3d_format_id id;

    D3DDDIFORMAT ddi_format;
    unsigned int component_count;
    DWORD red_size;
    DWORD green_size;
    DWORD blue_size;
    DWORD alpha_size;
    DWORD red_offset;
    DWORD green_offset;
    DWORD blue_offset;
    DWORD alpha_offset;
    UINT byte_count;
    BYTE depth_size;
    BYTE stencil_size;

    UINT block_width;
    UINT block_height;
    UINT block_byte_count;

    enum wined3d_ffp_emit_idx emit_idx;

    UINT  conv_byte_count;
    DWORD multisample_types;
    unsigned int attrs;
    unsigned int caps[WINED3D_GL_RES_TYPE_COUNT];
    float depth_bias_scale;
    struct wined3d_rational height_scale;
    struct color_fixup_desc color_fixup;
    void (*upload)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
    void (*download)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
    void (*decompress)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);

    enum wined3d_format_id typeless_id;
};

const struct wined3d_format *wined3d_get_format(const struct wined3d_adapter *adapter,
        enum wined3d_format_id format_id, unsigned int bind_flags);
enum wined3d_format_id wined3d_get_typed_format_id(const struct wined3d_adapter *adapter,
        const struct wined3d_format *format, enum wined3d_channel_type channel_type);
void wined3d_format_calculate_pitch(const struct wined3d_format *format, unsigned int alignment,
        unsigned int width, unsigned int height, unsigned int *row_pitch, unsigned int *slice_pitch);
UINT wined3d_format_calculate_size(const struct wined3d_format *format,
        UINT alignment, UINT width, UINT height, UINT depth);
void wined3d_format_convert_from_float(const struct wined3d_format *format,
        const struct wined3d_color *color, void *ret);
void wined3d_format_copy_data(const struct wined3d_format *format, const uint8_t *src,
        unsigned int src_row_pitch, unsigned int src_slice_pitch, uint8_t *dst, unsigned int dst_row_pitch,
        unsigned int dst_slice_pitch, unsigned int w, unsigned int h, unsigned int d);
void wined3d_format_get_float_color_key(const struct wined3d_format *format,
        const struct wined3d_color_key *key, struct wined3d_color *float_colors);
BOOL wined3d_format_is_depth_view(enum wined3d_format_id resource_format_id,
        enum wined3d_format_id view_format_id);
const struct wined3d_color_key_conversion * wined3d_format_get_color_key_conversion(
        const struct wined3d_texture *texture, BOOL need_alpha_ck);
uint32_t wined3d_format_pack(const struct wined3d_format *format, const struct wined3d_uvec4 *value);
BOOL wined3d_formats_are_srgb_variants(enum wined3d_format_id format1,
        enum wined3d_format_id format2);

void wined3d_stateblock_invalidate_texture_lod(struct wined3d_stateblock *stateblock,
        struct wined3d_texture *texture);

BOOL wined3d_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size);

static inline BOOL wined3d_format_is_typeless(const struct wined3d_format *format)
{
    return format->id == format->typeless_id && format->id != WINED3DFMT_UNKNOWN;
}

static inline BOOL use_vs(const struct wined3d_state *state)
{
    /* Check state->vertex_declaration to allow this to be used before the
     * stream info is validated, for example in device_update_tex_unit_map(). */
    return state->shader[WINED3D_SHADER_TYPE_VERTEX]
            && (!state->vertex_declaration || !state->vertex_declaration->position_transformed);
}

static inline BOOL use_ps(const struct wined3d_state *state)
{
    return !!state->shader[WINED3D_SHADER_TYPE_PIXEL];
}

static inline BOOL use_transform_feedback(const struct wined3d_state *state)
{
    const struct wined3d_shader *shader;

    if (!(shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]))
        return FALSE;
    return !!shader->u.gs.so_desc;
}

static inline void context_apply_state(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_state_entry *state_table = context->state_table;
    unsigned int rep = state_table[state_id].representative;
    state_table[rep].apply(context, state, rep);
}

static inline BOOL needs_separate_srgb_gl_texture(const struct wined3d_context *context,
        const struct wined3d_texture *texture)
{
    if (!(context->d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL))
        return FALSE;

    if (!context->d3d_info->srgb_read_control
            && (texture->resource.bind_flags & WINED3D_BIND_SHADER_RESOURCE)
            && (texture->resource.format_caps & WINED3D_FORMAT_CAP_SRGB_READ))
        return TRUE;

    if (!context->d3d_info->srgb_write_control
            && (texture->resource.bind_flags & WINED3D_BIND_RENDER_TARGET)
            && (texture->resource.format_caps & WINED3D_FORMAT_CAP_SRGB_WRITE))
        return TRUE;

    return FALSE;
}

static inline BOOL needs_srgb_write(const struct wined3d_d3d_info *d3d_info,
        const struct wined3d_state *state, const struct wined3d_fb_state *fb)
{
    return (!(d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
            || state->render_states[WINED3D_RS_SRGBWRITEENABLE])
            && fb->render_targets[0] && fb->render_targets[0]->format_caps & WINED3D_FORMAT_CAP_SRGB_WRITE;
}

static inline BOOL can_use_texture_swizzle(const struct wined3d_d3d_info *d3d_info, const struct wined3d_format *format)
{
    return d3d_info->texture_swizzle && !is_complex_fixup(format->color_fixup) && !is_scaling_fixup(format->color_fixup);
}

static inline BOOL is_rasterization_disabled(const struct wined3d_shader *geometry_shader)
{
    return geometry_shader && geometry_shader->u.gs.so_desc
            && geometry_shader->u.gs.so_desc->rasterizer_stream_idx == WINED3D_NO_RASTERIZER_STREAM;
}

static inline uint32_t wined3d_mask_from_size(unsigned int size)
{
    return size < 32 ? (1u << size) - 1 : ~0u;
}

static inline DWORD wined3d_extract_bits(const uint32_t *bitstream,
        unsigned int offset, unsigned int count)
{
    const unsigned int word_bit_count = sizeof(*bitstream) * CHAR_BIT;
    const unsigned int idx = offset / word_bit_count;
    const unsigned int shift = offset % word_bit_count;
    DWORD mask = (1u << count) - 1;
    DWORD ret;

    ret = (bitstream[idx] >> shift) & mask;
    if (shift + count > word_bit_count)
    {
        const unsigned int extracted_bit_count = word_bit_count - shift;
        const unsigned int remaining_bit_count = count - extracted_bit_count;
        mask = (1u << remaining_bit_count) - 1;
        ret |= (bitstream[idx + 1] & mask) << extracted_bit_count;
    }
    return ret;
}

static inline void wined3d_insert_bits(uint32_t *bitstream,
        unsigned int offset, unsigned int count, DWORD bits)
{
    const unsigned int word_bit_count = sizeof(*bitstream) * CHAR_BIT;
    const unsigned int idx = offset / word_bit_count;
    const unsigned int shift = offset % word_bit_count;
    DWORD mask = (1u << count) - 1;

    bitstream[idx] |= (bits & mask) << shift;
    if (shift + count > word_bit_count)
    {
        const unsigned int inserted_bit_count = word_bit_count - shift;
        const unsigned int remaining_bit_count = count - inserted_bit_count;
        mask = (1u << remaining_bit_count) - 1;
        bitstream[idx + 1] |= (bits >> inserted_bit_count) & mask;
    }
}

static inline void wined3d_from_cs(const struct wined3d_cs *cs)
{
    if (cs->thread)
        assert(cs->thread_id == GetCurrentThreadId());
}

static inline void wined3d_not_from_cs(const struct wined3d_cs *cs)
{
    assert(cs->thread_id != GetCurrentThreadId());
}

static inline enum wined3d_material_color_source validate_material_colour_source(
        const struct wined3d_vertex_declaration *vdecl, enum wined3d_material_color_source source)
{
    if (source == WINED3D_MCS_COLOR1 && vdecl->diffuse)
        return source;
    if (source == WINED3D_MCS_COLOR2 && vdecl->specular)
        return source;
    return WINED3D_MCS_MATERIAL;
}

static inline void wined3d_get_material_colour_source(enum wined3d_material_color_source *diffuse,
        enum wined3d_material_color_source *emissive, enum wined3d_material_color_source *ambient,
        enum wined3d_material_color_source *specular, const struct wined3d_state *state)
{
    const struct wined3d_vertex_declaration *vdecl = state->vertex_declaration;

    if (!state->render_states[WINED3D_RS_LIGHTING])
    {
        *diffuse = WINED3D_MCS_COLOR1;
        *specular = WINED3D_MCS_COLOR2;
        *emissive = *ambient = WINED3D_MCS_MATERIAL;

        return;
    }

    if (!state->render_states[WINED3D_RS_COLORVERTEX])
    {
        *diffuse = *emissive = *ambient = *specular = WINED3D_MCS_MATERIAL;

        return;
    }

    *diffuse = validate_material_colour_source(vdecl, state->render_states[WINED3D_RS_DIFFUSEMATERIALSOURCE]);
    *emissive = validate_material_colour_source(vdecl, state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE]);
    *ambient = validate_material_colour_source(vdecl, state->render_states[WINED3D_RS_AMBIENTMATERIALSOURCE]);
    *specular = validate_material_colour_source(vdecl, state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE]);
}

static inline void wined3d_vec4_transform(struct wined3d_vec4 *dst,
        const struct wined3d_vec4 *v, const struct wined3d_matrix *m)
{
    struct wined3d_vec4 tmp;

    tmp.x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + v->w * m->_41;
    tmp.y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + v->w * m->_42;
    tmp.z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + v->w * m->_43;
    tmp.w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + v->w * m->_44;

    *dst = tmp;
}

BOOL invert_matrix(struct wined3d_matrix *out, const struct wined3d_matrix *m);

void compute_normal_matrix(struct wined3d_matrix *normal_matrix, BOOL legacy_lighting,
        const struct wined3d_matrix *modelview);

static inline struct wined3d_context *context_acquire(struct wined3d_device *device,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    wined3d_from_cs(device->cs);

    return device->adapter->adapter_ops->adapter_acquire_context(device, texture, sub_resource_idx);
}

static inline void context_release(struct wined3d_context *context)
{
    context->device->adapter->adapter_ops->adapter_release_context(context);
}

static inline float wined3d_get_float_state(const struct wined3d_state *state, enum wined3d_render_state rs)
{
    union
    {
        DWORD d;
        float f;
    }
    tmpvalue;

    tmpvalue.d = state->render_states[rs];
    return tmpvalue.f;
}

static inline void *wined3d_context_map_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *data, size_t size, uint32_t map_flags)
{
    return context->device->adapter->adapter_ops->adapter_map_bo_address(context, data, size, map_flags);
}

static inline void wined3d_context_unmap_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *data, unsigned int range_count, const struct wined3d_range *ranges)
{
    context->device->adapter->adapter_ops->adapter_unmap_bo_address(context, data, range_count, ranges);
}

static inline void wined3d_context_copy_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *dst, const struct wined3d_bo_address *src,
        unsigned int range_count, const struct wined3d_range *ranges, uint32_t map_flags)
{
    context->device->adapter->adapter_ops->adapter_copy_bo_address(context, dst, src, range_count, ranges, map_flags);
}

static inline void wined3d_context_flush_bo_address(struct wined3d_context *context,
        const struct wined3d_const_bo_address *data, size_t size)
{
    context->device->adapter->adapter_ops->adapter_flush_bo_address(context, data, size);
}

static inline void wined3d_context_destroy_bo(struct wined3d_context *context, struct wined3d_bo *bo)
{
    context->device->adapter->adapter_ops->adapter_destroy_bo(context, bo);
}

static inline BOOL wined3d_dsv_srv_conflict(const struct wined3d_rendertarget_view *dsv,
        const struct wined3d_format *srv_format)
{
    return !srv_format || (srv_format->red_size && !(dsv->desc.flags & WINED3D_VIEW_READ_ONLY_DEPTH))
            || (srv_format->green_size && !(dsv->desc.flags & WINED3D_VIEW_READ_ONLY_STENCIL));
}

static inline unsigned int wined3d_bind_layer_count(const struct wined3d_texture *texture)
{
    return texture->resource.type == WINED3D_RTYPE_TEXTURE_3D ? texture->resource.depth : texture->layer_count;
}

static inline bool wined3d_srv_all_subresources(const struct wined3d_shader_resource_view *srv)
{
    struct wined3d_texture *texture;

    if (srv->resource->type == WINED3D_RTYPE_BUFFER)
        return TRUE;

    if (srv->desc.u.texture.layer_idx || srv->desc.u.texture.level_idx)
        return FALSE;

    texture = texture_from_resource(srv->resource);
    return srv->desc.u.texture.level_count == texture->level_count
            && srv->desc.u.texture.layer_count == wined3d_bind_layer_count(texture);
}

static inline bool wined3d_rtv_all_subresources(const struct wined3d_rendertarget_view *rtv)
{
    struct wined3d_texture *texture;

    if (rtv->resource->type == WINED3D_RTYPE_BUFFER)
        return TRUE;

    if (rtv->sub_resource_idx)
        return FALSE;

    texture = texture_from_resource(rtv->resource);
    return texture->level_count == 1 && rtv->layer_count == wined3d_bind_layer_count(texture);
}

static inline void wined3d_srv_bind_count_inc(struct wined3d_shader_resource_view *srv)
{
    InterlockedIncrement(&srv->resource->srv_bind_count_device);
}

static inline void wined3d_srv_bind_count_dec(struct wined3d_shader_resource_view *srv)
{
    InterlockedDecrement(&srv->resource->srv_bind_count_device);
}

static inline void wined3d_rtv_bind_count_inc(struct wined3d_rendertarget_view *rtv)
{
    InterlockedIncrement(&rtv->resource->rtv_bind_count_device);
}

static inline void wined3d_rtv_bind_count_dec(struct wined3d_rendertarget_view *rtv)
{
    InterlockedDecrement(&rtv->resource->rtv_bind_count_device);
}

static inline bool wined3d_rtv_overlaps_srv(const struct wined3d_rendertarget_view *rtv,
        const struct wined3d_shader_resource_view *srv)
{
    if (rtv->resource != srv->resource)
        return false;

    if (wined3d_srv_all_subresources(srv) || wined3d_rtv_all_subresources(rtv))
        return true;

    return rtv->sub_resource_idx >= srv->desc.u.texture.level_idx
            && rtv->sub_resource_idx < srv->desc.u.texture.level_idx + srv->desc.u.texture.level_count
            && rtv->layer_count >= srv->desc.u.texture.layer_idx;
}

static inline bool wined3d_is_srv_rtv_bound(const struct wined3d_state *state,
        const struct wined3d_shader_resource_view *srv)
{
    unsigned int i;

    if (!srv->resource->rtv_bind_count_device)
        return false;

    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        const struct wined3d_rendertarget_view *rtv;

        if ((rtv = state->fb.render_targets[i]) && wined3d_rtv_overlaps_srv(rtv, srv))
            return true;
    }

    return false;
}

static inline void wined3d_viewport_get_z_range(const struct wined3d_viewport *vp, float *min_z, float *max_z)
{
    *min_z = vp->min_z;

    /* The magic constant is derived from tests. */
    *max_z = max(vp->max_z, vp->min_z + 0.001f);
}

static inline BOOL wined3d_bitmap_clear(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] &= ~(1u << (idx & 0x1f));
}

static inline BOOL wined3d_bitmap_set(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] |= (1u << (idx & 0x1f));
}

static inline BOOL wined3d_bitmap_is_set(const uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] & (1u << (idx & 0x1f));
}

static inline unsigned int wined3d_bitmap_ffs_xor(const uint32_t *bitmap, unsigned int bit_count,
        unsigned int start, uint32_t xor_mask)
{
    const unsigned int word_bit_count = sizeof(*bitmap) * CHAR_BIT;
    const uint32_t *ptr, *end_ptr;
    uint32_t map, mask;

    assert(bit_count < word_bit_count || !(bit_count % word_bit_count));

    ptr = bitmap + start / word_bit_count;
    end_ptr = bitmap + (bit_count + word_bit_count - 1) / word_bit_count;

    if (ptr >= end_ptr)
        return ~0u;

    mask = ~0u << start % word_bit_count;
    map = (*ptr ^ xor_mask) & mask;
    while (!map)
    {
        if (++ptr == end_ptr)
            return ~0u;
        map = *ptr ^ xor_mask;
    }
    return (ptr - bitmap) * word_bit_count + wined3d_bit_scan(&map);
}

static inline unsigned int wined3d_bitmap_ffs(const uint32_t *bitmap, unsigned int bit_count, unsigned int start)
{
    return wined3d_bitmap_ffs_xor(bitmap, bit_count, start, 0);
}

static inline unsigned int wined3d_bitmap_ffz(const uint32_t *bitmap, unsigned int bit_count, unsigned int start)
{
    return wined3d_bitmap_ffs_xor(bitmap, bit_count, start, ~0u);
}

static inline BOOL wined3d_bitmap_get_range(const uint32_t *bitmap, unsigned int bit_count,
        unsigned int start, struct wined3d_range *range)
{
    unsigned int range_start, range_end;

    range_start = wined3d_bitmap_ffs(bitmap, bit_count, start);
    if (range_start == ~0u)
        return FALSE;

    range_end = wined3d_bitmap_ffz(bitmap, bit_count, range_start + 1);
    if (range_end == ~0u)
        range_end = bit_count;

    range->offset = range_start;
    range->size = range_end - range_start;
    return TRUE;
}

static inline bool wined3d_context_is_graphics_state_dirty(const struct wined3d_context *context, unsigned int state_id)
{
    return wined3d_bitmap_is_set(context->dirty_graphics_states, state_id);
}

static inline bool wined3d_context_is_compute_state_dirty(const struct wined3d_context *context, unsigned int state_id)
{
    return wined3d_bitmap_is_set(context->dirty_compute_states, state_id - STATE_COMPUTE_OFFSET);
}

static inline bool isStateDirty(const struct wined3d_context *context, unsigned int state_id)
{
    return wined3d_context_is_graphics_state_dirty(context, state_id);
}

static inline bool wined3d_primitive_type_is_list(enum wined3d_primitive_type t)
{
    return t == WINED3D_PT_POINTLIST
            || t == WINED3D_PT_LINELIST
            || t == WINED3D_PT_TRIANGLELIST
            || t == WINED3D_PT_LINELIST_ADJ
            || t == WINED3D_PT_TRIANGLELIST_ADJ
            || t == WINED3D_PT_PATCH;
}

static inline bool wined3d_map_persistent(void)
{
    return sizeof(void *) >= sizeof(uint64_t);
}

/* The WNDCLASS-Name for the fake window which we use to retrieve the GL capabilities */
#define WINED3D_OPENGL_WINDOW_CLASS_NAME "WineD3D_OpenGL"

extern CRITICAL_SECTION wined3d_command_cs;

#endif
