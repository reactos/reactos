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

#include <wine/config.h>
#include <wine/port.h>

#ifdef USE_WIN32_OPENGL
#define WINE_GLAPI __stdcall
#else
#define WINE_GLAPI
#endif

#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <objbase.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/rbtree.h>
#include <wine/wined3d.h>
#include "wined3d_gl.h"
#include <wine/wgl_driver.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

/* Driver quirks */
#define WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT       0x00000001
#define WINED3D_QUIRK_SET_TEXCOORD_W            0x00000002
#define WINED3D_QUIRK_GLSL_CLIP_VARYING         0x00000004
#define WINED3D_QUIRK_ALLOWS_SPECULAR_ALPHA     0x00000008
#define WINED3D_QUIRK_NV_CLIP_BROKEN            0x00000010
#define WINED3D_QUIRK_FBO_TEX_UPDATE            0x00000020
#define WINED3D_QUIRK_BROKEN_RGBA16             0x00000040
#define WINED3D_QUIRK_INFO_LOG_SPAM             0x00000080
#define WINED3D_QUIRK_LIMITED_TEX_FILTERING     0x00000100
#define WINED3D_QUIRK_BROKEN_ARB_FOG            0x00000200

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

static const struct color_fixup_desc COLOR_FIXUP_IDENTITY =
        {0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_Z, 0, CHANNEL_SOURCE_W};

static inline struct color_fixup_desc create_color_fixup_desc(
        int sign0, enum fixup_channel_source src0, int sign1, enum fixup_channel_source src1,
        int sign2, enum fixup_channel_source src2, int sign3, enum fixup_channel_source src3)
{
    struct color_fixup_desc fixup =
    {
        sign0, src0,
        sign1, src1,
        sign2, src2,
        sign3, src3,
    };
    return fixup;
}

static inline struct color_fixup_desc create_complex_fixup_desc(enum complex_fixup complex_fixup)
{
    struct color_fixup_desc fixup =
    {
        0, complex_fixup & (1 << 0) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0, complex_fixup & (1 << 1) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0, complex_fixup & (1 << 2) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
        0, complex_fixup & (1 << 3) ? CHANNEL_SOURCE_COMPLEX1 : CHANNEL_SOURCE_COMPLEX0,
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
    if (fixup.x_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1 << 0);
    if (fixup.y_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1 << 1);
    if (fixup.z_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1 << 2);
    if (fixup.w_source == CHANNEL_SOURCE_COMPLEX1) complex_fixup |= (1 << 3);
    return complex_fixup;
}

void *wined3d_rb_alloc(size_t size) DECLSPEC_HIDDEN;
void *wined3d_rb_realloc(void *ptr, size_t size) DECLSPEC_HIDDEN;
void wined3d_rb_free(void *ptr) DECLSPEC_HIDDEN;

/* Device caps */
#define MAX_STREAM_OUT              4
#define MAX_STREAMS                 16
#define MAX_TEXTURES                8
#define MAX_FRAGMENT_SAMPLERS       16
#define MAX_VERTEX_SAMPLERS         4
#define MAX_COMBINED_SAMPLERS       (MAX_FRAGMENT_SAMPLERS + MAX_VERTEX_SAMPLERS)
#define MAX_ACTIVE_LIGHTS           8
#define MAX_CLIPPLANES              WINED3DMAXUSERCLIPPLANES
#define MAX_CONSTANT_BUFFERS        15
#define MAX_SAMPLER_OBJECTS         16
#define MAX_SHADER_RESOURCE_VIEWS   128
#define MAX_VERTEX_BLENDS           4

struct min_lookup
{
    GLenum mip[WINED3D_TEXF_LINEAR + 1];
};

extern const struct min_lookup minMipLookup[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;
extern const GLenum magLookup[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;

GLenum wined3d_gl_compare_func(enum wined3d_cmp_func f) DECLSPEC_HIDDEN;

static inline GLenum wined3d_gl_mag_filter(enum wined3d_texture_filter_type mag_filter)
{
    return magLookup[mag_filter];
}

static inline GLenum wined3d_gl_min_mip_filter(enum wined3d_texture_filter_type min_filter,
        enum wined3d_texture_filter_type mip_filter)
{
    return minMipLookup[min_filter].mip[mip_filter];
}

/* float_16_to_32() and float_32_to_16() (see implementation in
 * surface_base.c) convert 16 bit floats in the FLOAT16 data type
 * to standard C floats and vice versa. They do not depend on the encoding
 * of the C float, so they are platform independent, but slow. On x86 and
 * other IEEE 754 compliant platforms the conversion can be accelerated by
 * bit shifting the exponent and mantissa. There are also some SSE-based
 * assembly routines out there.
 *
 * See GL_NV_half_float for a reference of the FLOAT16 / GL_HALF format
 */
static inline float float_16_to_32(const unsigned short *in)
{
    const unsigned short s = ((*in) & 0x8000);
    const unsigned short e = ((*in) & 0x7c00) >> 10;
    const unsigned short m = (*in) & 0x3ff;
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
    const float sgn = in & 0x800000 ? -1.0f : 1.0f;
    const unsigned short e = (in & 0x780000) >> 19;
    const unsigned int m = in & 0x7ffff;

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

#define ORM_BACKBUFFER  0
#define ORM_FBO         1

#define PCI_VENDOR_NONE 0xffff /* e.g. 0x8086 for Intel and 0x10de for Nvidia */
#define PCI_DEVICE_NONE 0xffff /* e.g. 0x14f for a Geforce6200 */

/* NOTE: When adding fields to this structure, make sure to update the default
 * values in wined3d_main.c as well. */
struct wined3d_settings
{
    DWORD max_gl_version;
    BOOL glslRequested;
    int offscreen_rendering_mode;
    unsigned short pci_vendor_id;
    unsigned short pci_device_id;
    /* Memory tracking and object counting. */
    unsigned int msaa_quality_levels;
    UINT64 emulated_textureram;
    char *logo;
    int allow_multisampling;
    BOOL strict_draw_ordering;
    BOOL always_offscreen;
    unsigned int max_sm_vs;
    unsigned int max_sm_gs;
    unsigned int max_sm_ps;
    BOOL no_3d;
#if defined(STAGING_CSMT)
    BOOL cs_multithreaded;
    BOOL ignore_rt_map;
#endif /* STAGING_CSMT */
};

extern struct wined3d_settings wined3d_settings DECLSPEC_HIDDEN;

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
};

#define WINED3D_SHADER_CONST_VS_F            0x00000001
#define WINED3D_SHADER_CONST_VS_I            0x00000002
#define WINED3D_SHADER_CONST_VS_B            0x00000004
#define WINED3D_SHADER_CONST_VS_POINTSIZE    0x00000008
#define WINED3D_SHADER_CONST_VS_POS_FIXUP    0x00000010
#define WINED3D_SHADER_CONST_PS_F            0x00000020
#define WINED3D_SHADER_CONST_PS_I            0x00000040
#define WINED3D_SHADER_CONST_PS_B            0x00000080
#define WINED3D_SHADER_CONST_PS_BUMP_ENV     0x00000100
#define WINED3D_SHADER_CONST_PS_FOG          0x00000200
#define WINED3D_SHADER_CONST_PS_Y_CORR       0x00000400
#define WINED3D_SHADER_CONST_PS_NP2_FIXUP    0x00000800
#define WINED3D_SHADER_CONST_FFP_MODELVIEW   0x00001000
#define WINED3D_SHADER_CONST_FFP_VERTEXBLEND 0x00002000
#define WINED3D_SHADER_CONST_FFP_PROJ        0x00004000
#define WINED3D_SHADER_CONST_FFP_TEXMATRIX   0x00008000
#define WINED3D_SHADER_CONST_FFP_MATERIAL    0x00010000
#define WINED3D_SHADER_CONST_FFP_LIGHTS      0x00020000
#define WINED3D_SHADER_CONST_FFP_PS          0x00040000
#define WINED3D_SHADER_CONST_FFP_COLOR_KEY   0x00080000

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
    WINED3DSPR_PRIMID,
    WINED3DSPR_NULL,
    WINED3DSPR_RESOURCE,
};

enum wined3d_data_type
{
    WINED3D_DATA_FLOAT,
    WINED3D_DATA_INT,
    WINED3D_DATA_RESOURCE,
    WINED3D_DATA_SAMPLER,
    WINED3D_DATA_UINT,
    WINED3D_DATA_UNORM,
    WINED3D_DATA_SNORM,
};

enum wined3d_immconst_type
{
    WINED3D_IMMCONST_SCALAR,
    WINED3D_IMMCONST_VEC4,
};

#define WINED3DSP_NOSWIZZLE (0 | (1 << 2) | (2 << 4) | (3 << 6))

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

#define WINED3DSP_WRITEMASK_0   0x1 /* .x r */
#define WINED3DSP_WRITEMASK_1   0x2 /* .y g */
#define WINED3DSP_WRITEMASK_2   0x4 /* .z b */
#define WINED3DSP_WRITEMASK_3   0x8 /* .w a */
#define WINED3DSP_WRITEMASK_ALL 0xf /* all */

enum wined3d_shader_dst_modifier
{
    WINED3DSPDM_NONE = 0,
    WINED3DSPDM_SATURATE = 1,
    WINED3DSPDM_PARTIALPRECISION = 2,
    WINED3DSPDM_MSAMPCENTROID = 4,
};

/* Undocumented opcode control to identify projective texture lookups in ps 2.0 and later */
#define WINED3DSI_TEXLD_PROJECT     0x1
#define WINED3DSI_TEXLD_BIAS        0x2
#define WINED3DSI_INDEXED_DYNAMIC   0x4

enum wined3d_shader_rel_op
{
    WINED3D_SHADER_REL_OP_GT = 1,
    WINED3D_SHADER_REL_OP_EQ = 2,
    WINED3D_SHADER_REL_OP_GE = 3,
    WINED3D_SHADER_REL_OP_LT = 4,
    WINED3D_SHADER_REL_OP_NE = 5,
    WINED3D_SHADER_REL_OP_LE = 6,
};

#define WINED3D_SM1_VS  0xfffe
#define WINED3D_SM1_PS  0xffff
#define WINED3D_SM4_PS  0x0000
#define WINED3D_SM4_VS  0x0001
#define WINED3D_SM4_GS  0x0002

/* Shader version tokens, and shader end tokens */
#define WINED3DPS_VERSION(major, minor) ((WINED3D_SM1_PS << 16) | ((major) << 8) | (minor))
#define WINED3DVS_VERSION(major, minor) ((WINED3D_SM1_VS << 16) | ((major) << 8) | (minor))

/* Shader backends */

/* TODO: Make this dynamic, based on shader limits ? */
#define MAX_ATTRIBS 16
#define MAX_REG_ADDR 1
#define MAX_REG_TEMP 32
#define MAX_REG_TEXCRD 8
#define MAX_REG_INPUT 32
#define MAX_REG_OUTPUT 32
#define MAX_CONST_I 16
#define MAX_CONST_B 16
#define WINED3D_MAX_CBS 15

/* FIXME: This needs to go up to 2048 for
 * Shader model 3 according to msdn (and for software shaders) */
#define MAX_LABELS 16

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
    WINED3DSIH_BEM,
    WINED3DSIH_BREAK,
    WINED3DSIH_BREAKC,
    WINED3DSIH_BREAKP,
    WINED3DSIH_CALL,
    WINED3DSIH_CALLNZ,
    WINED3DSIH_CMP,
    WINED3DSIH_CND,
    WINED3DSIH_CRS,
    WINED3DSIH_CUT,
    WINED3DSIH_DCL,
    WINED3DSIH_DCL_CONSTANT_BUFFER,
    WINED3DSIH_DCL_INPUT_PRIMITIVE,
    WINED3DSIH_DCL_OUTPUT_TOPOLOGY,
    WINED3DSIH_DCL_VERTICES_OUT,
    WINED3DSIH_DEF,
    WINED3DSIH_DEFB,
    WINED3DSIH_DEFI,
    WINED3DSIH_DIV,
    WINED3DSIH_DP2,
    WINED3DSIH_DP2ADD,
    WINED3DSIH_DP3,
    WINED3DSIH_DP4,
    WINED3DSIH_DST,
    WINED3DSIH_DSX,
    WINED3DSIH_DSY,
    WINED3DSIH_ELSE,
    WINED3DSIH_EMIT,
    WINED3DSIH_ENDIF,
    WINED3DSIH_ENDLOOP,
    WINED3DSIH_ENDREP,
    WINED3DSIH_EQ,
    WINED3DSIH_EXP,
    WINED3DSIH_EXPP,
    WINED3DSIH_FRC,
    WINED3DSIH_FTOI,
    WINED3DSIH_GE,
    WINED3DSIH_IADD,
    WINED3DSIH_IEQ,
    WINED3DSIH_IF,
    WINED3DSIH_IFC,
    WINED3DSIH_IGE,
    WINED3DSIH_IMUL,
    WINED3DSIH_ISHL,
    WINED3DSIH_ITOF,
    WINED3DSIH_LABEL,
    WINED3DSIH_LD,
    WINED3DSIH_LIT,
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
    WINED3DSIH_NRM,
    WINED3DSIH_OR,
    WINED3DSIH_PHASE,
    WINED3DSIH_POW,
    WINED3DSIH_RCP,
    WINED3DSIH_REP,
    WINED3DSIH_RET,
    WINED3DSIH_ROUND_NI,
    WINED3DSIH_RSQ,
    WINED3DSIH_SAMPLE,
    WINED3DSIH_SAMPLE_GRAD,
    WINED3DSIH_SAMPLE_LOD,
    WINED3DSIH_SETP,
    WINED3DSIH_SGE,
    WINED3DSIH_SGN,
    WINED3DSIH_SINCOS,
    WINED3DSIH_SLT,
    WINED3DSIH_SQRT,
    WINED3DSIH_SUB,
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
    WINED3DSIH_UDIV,
    WINED3DSIH_UGE,
    WINED3DSIH_USHR,
    WINED3DSIH_UTOF,
    WINED3DSIH_XOR,
    WINED3DSIH_TABLE_SIZE
};

enum wined3d_shader_type
{
    WINED3D_SHADER_TYPE_PIXEL,
    WINED3D_SHADER_TYPE_VERTEX,
    WINED3D_SHADER_TYPE_GEOMETRY,
    WINED3D_SHADER_TYPE_COUNT,
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
};

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

#define WINED3D_SHADER_VERSION(major, minor) (((major) << 8) | (minor))

struct wined3d_shader_reg_maps
{
    struct wined3d_shader_version shader_version;
    BYTE texcoord;                          /* MAX_REG_TEXCRD, 8 */
    BYTE address;                           /* MAX_REG_ADDR, 1 */
    WORD labels;                            /* MAX_LABELS, 16 */
    DWORD temporary;                        /* MAX_REG_TEMP, 32 */
    DWORD *constf;                          /* pixel, vertex */
    DWORD texcoord_mask[MAX_REG_TEXCRD];    /* vertex < 3.0 */
    DWORD input_registers;                  /* max(MAX_REG_INPUT, MAX_ATTRIBS), 32 */
    DWORD output_registers;                 /* MAX_REG_OUTPUT, 32 */
    WORD integer_constants;                 /* MAX_CONST_I, 16 */
    WORD boolean_constants;                 /* MAX_CONST_B, 16 */
    WORD local_int_consts;                  /* MAX_CONST_I, 16 */
    WORD local_bool_consts;                 /* MAX_CONST_B, 16 */
    UINT cb_sizes[WINED3D_MAX_CBS];

    struct wined3d_shader_resource_info resource_info[max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS)];
    struct wined3d_shader_sampler_map sampler_map;
    BYTE bumpmat;                           /* MAX_TEXTURES, 8 */
    BYTE luminanceparams;                   /* MAX_TEXTURES, 8 */

    WORD usesnrm        : 1;
    WORD vpos           : 1;
    WORD usesdsx        : 1;
    WORD usesdsy        : 1;
    WORD usestexldd     : 1;
    WORD usesmova       : 1;
    WORD usesfacing     : 1;
    WORD usesrelconstF  : 1;
    WORD fog            : 1;
    WORD usestexldl     : 1;
    WORD usesifc        : 1;
    WORD usescall       : 1;
    WORD usespow        : 1;
    WORD point_size     : 1;
    WORD padding        : 2;

    DWORD rt_mask; /* Used render targets, 32 max. */

    /* Whether or not loops are used in this shader, and nesting depth */
    unsigned loop_depth;
    UINT min_rel_offset, max_rel_offset;
};

/* Keeps track of details for TEX_M#x# instructions which need to maintain
 * state information between multiple instructions. */
struct wined3d_shader_tex_mx
{
    unsigned int current_row;
    DWORD texcoord_w[2];
};

struct wined3d_shader_loop_state
{
    UINT current_depth;
    UINT current_reg;
};

struct wined3d_shader_context
{
    const struct wined3d_shader *shader;
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_shader_reg_maps *reg_maps;
    struct wined3d_string_buffer *buffer;
    struct wined3d_shader_tex_mx *tex_mx;
    struct wined3d_shader_loop_state *loop_state;
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
    DWORD immconst_data[4];
};

struct wined3d_shader_dst_param
{
    struct wined3d_shader_register reg;
    DWORD write_mask;
    DWORD modifiers;
    DWORD shift;
};

struct wined3d_shader_src_param
{
    struct wined3d_shader_register reg;
    DWORD swizzle;
    enum wined3d_shader_src_modifier modifiers;
};

struct wined3d_shader_semantic
{
    enum wined3d_decl_usage usage;
    UINT usage_idx;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_data_type resource_data_type;
    struct wined3d_shader_dst_param reg;
};

struct wined3d_shader_instruction
{
    const struct wined3d_shader_context *ctx;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    DWORD flags;
    BOOL coissue;
    const struct wined3d_shader_src_param *predicate;
    UINT dst_count;
    const struct wined3d_shader_dst_param *dst;
    UINT src_count;
    const struct wined3d_shader_src_param *src;
    union
    {
        struct wined3d_shader_semantic semantic;
        enum wined3d_primitive_type primitive_type;
        struct wined3d_shader_src_param src;
        UINT count;
    } declaration;
};

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
    void *(*shader_init)(const DWORD *ptr, const struct wined3d_shader_signature *output_signature);
    void (*shader_free)(void *data);
    void (*shader_read_header)(void *data, const DWORD **ptr, struct wined3d_shader_version *shader_version);
    void (*shader_read_instruction)(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins);
    BOOL (*shader_is_end)(void *data, const DWORD **ptr);
};

extern const struct wined3d_shader_frontend sm1_shader_frontend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_frontend sm4_shader_frontend DECLSPEC_HIDDEN;

typedef void (*SHADER_HANDLER)(const struct wined3d_shader_instruction *);

#define WINED3D_SHADER_CAP_VS_CLIPPING      0x00000001
#define WINED3D_SHADER_CAP_SRGB_WRITE       0x00000002

struct shader_caps
{
    UINT vs_version;
    UINT gs_version;
    UINT ps_version;

    DWORD vs_uniform_count;
    DWORD ps_uniform_count;
    float ps_1x_max_value;

    DWORD wined3d_caps;
};

enum wined3d_gl_resource_type
{
    WINED3D_GL_RES_TYPE_TEX_1D          = 0,
    WINED3D_GL_RES_TYPE_TEX_2D          = 1,
    WINED3D_GL_RES_TYPE_TEX_3D          = 2,
    WINED3D_GL_RES_TYPE_TEX_CUBE        = 3,
    WINED3D_GL_RES_TYPE_TEX_RECT        = 4,
    WINED3D_GL_RES_TYPE_BUFFER          = 5,
    WINED3D_GL_RES_TYPE_RB              = 6,
    WINED3D_GL_RES_TYPE_COUNT           = 7,
};

enum vertexprocessing_mode {
    fixedfunction,
    vertexshader,
    pretransformed
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

#define WINED3D_PSARGS_PROJECTED (1 << 3)
#define WINED3D_PSARGS_TEXTRANSFORM_SHIFT 4
#define WINED3D_PSARGS_TEXTRANSFORM_MASK 0xf
#define WINED3D_PSARGS_TEXTYPE_SHIFT 2
#define WINED3D_PSARGS_TEXTYPE_MASK 0x3

/* Used for Shader Model 1 pixel shaders to track the bound texture
 * type. 2D and RECT textures are separated through NP2 fixup. */
enum wined3d_shader_tex_types
{
    WINED3D_SHADER_TEX_2D   = 0,
    WINED3D_SHADER_TEX_3D   = 1,
    WINED3D_SHADER_TEX_CUBE = 2,
};

struct ps_compile_args {
    struct color_fixup_desc     color_fixup[MAX_FRAGMENT_SAMPLERS];
    enum vertexprocessing_mode  vp_mode;
    enum wined3d_ffp_ps_fog_mode fog;
    WORD                        tex_transform; /* ps 1.0-1.3, 4 textures */
    WORD                        tex_types; /* ps 1.0 - 1.4, 6 textures */
    WORD                        srgb_correction;
    WORD                        np2_fixup;
    /* Bitmap for NP2 texcoord fixups (16 samplers max currently).
       D3D9 has a limit of 16 samplers and the fixup is superfluous
       in D3D10 (unconditional NP2 support mandatory). */
    WORD shadow; /* MAX_FRAGMENT_SAMPLERS, 16 */
    BOOL pointsprite;
};

enum fog_src_type {
    VS_FOG_Z        = 0,
    VS_FOG_COORD    = 1
};

struct vs_compile_args
{
    BYTE fog_src;
    BYTE clip_enabled : 1;
    BYTE point_size : 1;
    BYTE per_vertex_point_size : 1;
    BYTE padding : 5;
    WORD swizzle_map;   /* MAX_ATTRIBS, 16 */
};

struct wined3d_context;
struct wined3d_state;
struct fragment_pipeline;
struct wined3d_vertex_pipe_ops;

struct wined3d_shader_backend_ops
{
    void (*shader_handle_instruction)(const struct wined3d_shader_instruction *);
    void (*shader_select)(void *shader_priv, struct wined3d_context *context,
            const struct wined3d_state *state);
    void (*shader_disable)(void *shader_priv, struct wined3d_context *context);
    void (*shader_select_depth_blt)(void *shader_priv, const struct wined3d_gl_info *gl_info,
            enum wined3d_gl_resource_type tex_type, const SIZE *ds_mask_size);
    void (*shader_deselect_depth_blt)(void *shader_priv, const struct wined3d_gl_info *gl_info);
    void (*shader_update_float_vertex_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_update_float_pixel_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_load_constants)(void *shader_priv, struct wined3d_context *context,
            const struct wined3d_state *state);
    void (*shader_destroy)(struct wined3d_shader *shader);
    HRESULT (*shader_alloc_private)(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
            const struct fragment_pipeline *fragment_pipe);
    void (*shader_free_private)(struct wined3d_device *device);
    BOOL (*shader_allocate_context_data)(struct wined3d_context *context);
    void (*shader_free_context_data)(struct wined3d_context *context);
    void (*shader_init_context_state)(struct wined3d_context *context);
    void (*shader_get_caps)(const struct wined3d_gl_info *gl_info, struct shader_caps *caps);
    BOOL (*shader_color_fixup_supported)(struct color_fixup_desc fixup);
    BOOL (*shader_has_ffp_proj_control)(void *shader_priv);
};

extern const struct wined3d_shader_backend_ops glsl_shader_backend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_backend_ops arb_program_shader_backend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_backend_ops none_shader_backend DECLSPEC_HIDDEN;

#define GL_EXTCALL(f) (gl_info->gl_ops.ext.p_##f)

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xff)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xff)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xff)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xff)

#define D3DCOLOR_R(dw) (((float) (((dw) >> 16) & 0xff)) / 255.0f)
#define D3DCOLOR_G(dw) (((float) (((dw) >>  8) & 0xff)) / 255.0f)
#define D3DCOLOR_B(dw) (((float) (((dw) >>  0) & 0xff)) / 255.0f)
#define D3DCOLOR_A(dw) (((float) (((dw) >> 24) & 0xff)) / 255.0f)

#define D3DCOLORTOGLFLOAT4(dw, vec) do { \
  (vec)[0] = D3DCOLOR_R(dw); \
  (vec)[1] = D3DCOLOR_G(dw); \
  (vec)[2] = D3DCOLOR_B(dw); \
  (vec)[3] = D3DCOLOR_A(dw); \
} while(0)

#define HIGHEST_TRANSFORMSTATE WINED3D_TS_WORLD_MATRIX(255) /* Highest value in wined3d_transform_state. */

/* Checking of API calls */
/* --------------------- */
#ifndef WINE_NO_DEBUG_MSGS
#define checkGLcall(A)                                              \
do {                                                                \
    GLint err;                                                      \
    if (!__WINE_IS_DEBUG_ON(_ERR, __wine_dbch___default)) break;    \
    err = gl_info->gl_ops.gl.p_glGetError();                        \
    if (err == GL_NO_ERROR) {                                       \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__);        \
                                                                    \
    } else do {                                                     \
        ERR(">>>>>>>>>>>>>>>>> %s (%#x) from %s @ %s / %d\n",       \
            debug_glerror(err), err, A, __FILE__, __LINE__);        \
       err = gl_info->gl_ops.gl.p_glGetError();                     \
    } while (err != GL_NO_ERROR);                                   \
} while(0)
#else
#define checkGLcall(A) do {} while(0)
#endif

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
    WINED3D_FFP_EMIT_FLOAT1 = 0,
    WINED3D_FFP_EMIT_FLOAT2 = 1,
    WINED3D_FFP_EMIT_FLOAT3 = 2,
    WINED3D_FFP_EMIT_FLOAT4 = 3,
    WINED3D_FFP_EMIT_D3DCOLOR = 4,
    WINED3D_FFP_EMIT_UBYTE4 = 5,
    WINED3D_FFP_EMIT_SHORT2 = 6,
    WINED3D_FFP_EMIT_SHORT4 = 7,
    WINED3D_FFP_EMIT_UBYTE4N = 8,
    WINED3D_FFP_EMIT_SHORT2N = 9,
    WINED3D_FFP_EMIT_SHORT4N = 10,
    WINED3D_FFP_EMIT_USHORT2N = 11,
    WINED3D_FFP_EMIT_USHORT4N = 12,
    WINED3D_FFP_EMIT_UDEC3 = 13,
    WINED3D_FFP_EMIT_DEC3N = 14,
    WINED3D_FFP_EMIT_FLOAT16_2 = 15,
    WINED3D_FFP_EMIT_FLOAT16_4 = 16,
    WINED3D_FFP_EMIT_INVALID = 17,
    WINED3D_FFP_EMIT_COUNT = 18
};

struct wined3d_bo_address
{
    GLuint buffer_object;
    BYTE *addr;
};

struct wined3d_const_bo_address
{
    GLuint buffer_object;
    const BYTE *addr;
};

static inline struct wined3d_const_bo_address *wined3d_const_bo_address(struct wined3d_bo_address *data)
{
    return (struct wined3d_const_bo_address *)data;
}

struct wined3d_stream_info_element
{
    const struct wined3d_format *format;
    struct wined3d_bo_address data;
    GLsizei stride;
    unsigned int stream_idx;
    unsigned int divisor;
};

struct wined3d_stream_info
{
    struct wined3d_stream_info_element elements[MAX_ATTRIBS];
    DWORD position_transformed : 1;
    DWORD all_vbo : 1;
    WORD swizzle_map; /* MAX_ATTRIBS, 16 */
    WORD use_map; /* MAX_ATTRIBS, 16 */
};

#if defined(STAGING_CSMT)
void draw_primitive(struct wined3d_device *device, const struct wined3d_state *state,
        UINT start_idx, UINT index_count, UINT start_instance, UINT instance_count,
        BOOL indexed) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
void draw_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count,
        UINT start_instance, UINT instance_count, BOOL indexed) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) DECLSPEC_HIDDEN;

#define eps 1e-8f

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

/* Routines and structures related to state management */

#define STATE_RENDER(a) (a)
#define STATE_IS_RENDER(a) ((a) >= STATE_RENDER(1) && (a) <= STATE_RENDER(WINEHIGHEST_RENDER_STATE))

#define STATE_TEXTURESTAGE(stage, num) (STATE_RENDER(WINEHIGHEST_RENDER_STATE) + 1 + (stage) * (WINED3D_HIGHEST_TEXTURE_STATE + 1) + (num))
#define STATE_IS_TEXTURESTAGE(a) ((a) >= STATE_TEXTURESTAGE(0, 1) && (a) <= STATE_TEXTURESTAGE(MAX_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE))

/* + 1 because samplers start with 0 */
#define STATE_SAMPLER(num) (STATE_TEXTURESTAGE(MAX_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE) + 1 + (num))
#define STATE_IS_SAMPLER(num) ((num) >= STATE_SAMPLER(0) && (num) <= STATE_SAMPLER(MAX_COMBINED_SAMPLERS - 1))

#define STATE_SHADER(a) (STATE_SAMPLER(MAX_COMBINED_SAMPLERS) + (a))
#define STATE_IS_SHADER(a) ((a) >= STATE_SHADER(0) && (a) < STATE_SHADER(WINED3D_SHADER_TYPE_COUNT))

#define STATE_CONSTANT_BUFFER(a) (STATE_SHADER(WINED3D_SHADER_TYPE_COUNT) + (a))
#define STATE_IS_CONSTANT_BUFFER(a) ((a) >= STATE_CONSTANT_BUFFER(0) && (a) < STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COUNT))

#define STATE_SHADER_RESOURCE_BINDING (STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COUNT))
#define STATE_IS_SHADER_RESOURCE_BINDING(a) ((a) == STATE_SHADER_RESOURCE_BINDING)

#define STATE_TRANSFORM(a) (STATE_SHADER_RESOURCE_BINDING + (a))
#define STATE_IS_TRANSFORM(a) ((a) >= STATE_TRANSFORM(1) && (a) <= STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)))

#define STATE_STREAMSRC (STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)) + 1)
#define STATE_IS_STREAMSRC(a) ((a) == STATE_STREAMSRC)
#define STATE_INDEXBUFFER (STATE_STREAMSRC + 1)
#define STATE_IS_INDEXBUFFER(a) ((a) == STATE_INDEXBUFFER)

#define STATE_VDECL (STATE_INDEXBUFFER + 1)
#define STATE_IS_VDECL(a) ((a) == STATE_VDECL)

#define STATE_VIEWPORT (STATE_VDECL + 1)
#define STATE_IS_VIEWPORT(a) ((a) == STATE_VIEWPORT)

#define STATE_LIGHT_TYPE (STATE_VIEWPORT + 1)
#define STATE_IS_LIGHT_TYPE(a) ((a) == STATE_LIGHT_TYPE)
#define STATE_ACTIVELIGHT(a) (STATE_LIGHT_TYPE + 1 + (a))
#define STATE_IS_ACTIVELIGHT(a) ((a) >= STATE_ACTIVELIGHT(0) && (a) < STATE_ACTIVELIGHT(MAX_ACTIVE_LIGHTS))

#define STATE_SCISSORRECT (STATE_ACTIVELIGHT(MAX_ACTIVE_LIGHTS - 1) + 1)
#define STATE_IS_SCISSORRECT(a) ((a) == STATE_SCISSORRECT)

#define STATE_CLIPPLANE(a) (STATE_SCISSORRECT + 1 + (a))
#define STATE_IS_CLIPPLANE(a) ((a) >= STATE_CLIPPLANE(0) && (a) <= STATE_CLIPPLANE(MAX_CLIPPLANES - 1))

#define STATE_MATERIAL (STATE_CLIPPLANE(MAX_CLIPPLANES))
#define STATE_IS_MATERIAL(a) ((a) == STATE_MATERIAL)

#define STATE_FRONTFACE (STATE_MATERIAL + 1)
#define STATE_IS_FRONTFACE(a) ((a) == STATE_FRONTFACE)

#define STATE_POINTSPRITECOORDORIGIN  (STATE_FRONTFACE + 1)
#define STATE_IS_POINTSPRITECOORDORIGIN(a) ((a) == STATE_POINTSPRITECOORDORIGIN)

#define STATE_BASEVERTEXINDEX  (STATE_POINTSPRITECOORDORIGIN + 1)
#define STATE_IS_BASEVERTEXINDEX(a) ((a) == STATE_BASEVERTEXINDEX)

#define STATE_FRAMEBUFFER (STATE_BASEVERTEXINDEX + 1)
#define STATE_IS_FRAMEBUFFER(a) ((a) == STATE_FRAMEBUFFER)

#define STATE_POINT_ENABLE (STATE_FRAMEBUFFER + 1)
#define STATE_IS_POINT_ENABLE(a) ((a) == STATE_POINT_ENABLE)

#define STATE_COLOR_KEY (STATE_POINT_ENABLE + 1)
#define STATE_IS_COLOR_KEY(a) ((a) == STATE_COLOR_KEY)

#define STATE_HIGHEST (STATE_COLOR_KEY)

enum fogsource {
    FOGSOURCE_FFP,
    FOGSOURCE_VS,
    FOGSOURCE_COORD,
};

struct wined3d_occlusion_query
{
    struct list entry;
    GLuint id;
    struct wined3d_context *context;
#if defined(STAGING_CSMT)
    DWORD samples;
    BOOL started;
#endif /* STAGING_CSMT */
};

union wined3d_gl_query_object
{
    GLuint id;
    GLsync sync;
};

struct wined3d_event_query
{
    struct list entry;
    union wined3d_gl_query_object object;
    struct wined3d_context *context;
};

enum wined3d_event_query_result
{
    WINED3D_EVENT_QUERY_OK,
    WINED3D_EVENT_QUERY_WAITING,
    WINED3D_EVENT_QUERY_NOT_STARTED,
    WINED3D_EVENT_QUERY_WRONG_THREAD,
    WINED3D_EVENT_QUERY_ERROR
};

void wined3d_event_query_destroy(struct wined3d_event_query *query) DECLSPEC_HIDDEN;
enum wined3d_event_query_result wined3d_event_query_finish(const struct wined3d_event_query *query,
        const struct wined3d_device *device) DECLSPEC_HIDDEN;
void wined3d_event_query_issue(struct wined3d_event_query *query, const struct wined3d_device *device) DECLSPEC_HIDDEN;
BOOL wined3d_event_query_supported(const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

struct wined3d_timestamp_query
{
    struct list entry;
    GLuint id;
    struct wined3d_context *context;
#if defined(STAGING_CSMT)
    UINT64 timestamp;
};

void context_alloc_timestamp_query(struct wined3d_context *context, struct wined3d_timestamp_query *query) DECLSPEC_HIDDEN;
void context_free_timestamp_query(struct wined3d_timestamp_query *query) DECLSPEC_HIDDEN;

struct wined3d_fb_state
{
    struct wined3d_rendertarget_view **render_targets;
    struct wined3d_rendertarget_view *depth_stencil;
    UINT rt_size;
};

static inline BOOL wined3d_fb_equal(const struct wined3d_fb_state *fb1, const struct wined3d_fb_state *fb2)
{
    UINT i;

    if (fb1->depth_stencil != fb2->depth_stencil)
        return FALSE;
    if (fb1->rt_size != fb2->rt_size)
        return FALSE;
    for (i = 0; i < fb1->rt_size; i++)
        if (fb1->render_targets[i] != fb2->render_targets[i])
            return FALSE;
    return TRUE;
}

static inline void wined3d_fb_copy(struct wined3d_fb_state *dst, const struct wined3d_fb_state *src)
{
    UINT i;

    dst->depth_stencil = src->depth_stencil;
    for (i = 0; i < min(dst->rt_size, src->rt_size); i++)
        dst->render_targets[i] = src->render_targets[i];
}
#else  /* STAGING_CSMT */
};

void context_alloc_timestamp_query(struct wined3d_context *context, struct wined3d_timestamp_query *query) DECLSPEC_HIDDEN;
void context_free_timestamp_query(struct wined3d_timestamp_query *query) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

struct wined3d_context
{
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_d3d_info *d3d_info;
    const struct StateEntry *state_table;
    /* State dirtification
     * dirtyArray is an array that contains markers for dirty states. numDirtyEntries states are dirty, their numbers are in indices
     * 0...numDirtyEntries - 1. isStateDirty is a redundant copy of the dirtyArray. Technically only one of them would be needed,
     * but with the help of both it is easy to find out if a state is dirty(just check the array index), and for applying dirty states
     * only numDirtyEntries array elements have to be checked, not STATE_HIGHEST states.
     */
    DWORD                   dirtyArray[STATE_HIGHEST + 1]; /* Won't get bigger than that, a state is never marked dirty 2 times */
    DWORD                   numDirtyEntries;
    DWORD isStateDirty[STATE_HIGHEST / (sizeof(DWORD) * CHAR_BIT) + 1]; /* Bitmap to find out quickly if a state is dirty */
#if defined(STAGING_CSMT)
    struct wined3d_fb_state current_fb;
#endif /* STAGING_CSMT */

    struct wined3d_swapchain *swapchain;
    struct wined3d_surface *current_rt;
    DWORD                   tid;    /* Thread ID which owns this context at the moment */

    /* Stores some information about the context state for optimization */
    DWORD render_offscreen : 1;
    DWORD last_was_rhw : 1;             /* true iff last draw_primitive was in xyzrhw mode */
    DWORD last_was_pshader : 1;
    DWORD last_was_vshader : 1;
    DWORD namedArraysLoaded : 1;
    DWORD numberedArraysLoaded : 1;
    DWORD last_was_blit : 1;
    DWORD last_was_ckey : 1;
    DWORD fog_coord : 1;
    DWORD fog_enabled : 1;
    DWORD num_untracked_materials : 2;  /* Max value 2 */
    DWORD current : 1;
    DWORD destroyed : 1;
    DWORD valid : 1;
    DWORD use_immediate_mode_draw : 1;
    DWORD texShaderBumpMap : 8;         /* MAX_TEXTURES, 8 */
    DWORD lastWasPow2Texture : 8;       /* MAX_TEXTURES, 8 */
    DWORD fixed_function_usage_map : 8; /* MAX_TEXTURES, 8 */
    DWORD lowest_disabled_stage : 4;    /* Max MAX_TEXTURES, 8 */
    DWORD rebind_fbo : 1;
    DWORD needs_set : 1;
    DWORD update_shader_resource_bindings : 1;
    DWORD padding : 17;
    DWORD shader_update_mask;
    DWORD constant_update_mask;
    DWORD                   numbered_array_mask;
    GLenum                  tracking_parm;     /* Which source is tracking current colour         */
    GLenum                  untracked_materials[2];
    UINT                    blit_w, blit_h;
    enum fogsource          fog_source;
    DWORD active_texture;
    DWORD texture_type[MAX_COMBINED_SAMPLERS];

    UINT instance_count;

    /* The actual opengl context */
    UINT level;
    HGLRC restore_ctx;
    HDC restore_dc;
    int restore_pf;
    HGLRC                   glCtx;
    HWND                    win_handle;
    HDC                     hdc;
    int pixel_format;
    GLint                   aux_buffers;

    void *shader_backend_data;
    void *fragment_pipe_data;

    /* FBOs */
    UINT                    fbo_entry_count;
    struct list             fbo_list;
    struct list             fbo_destroy_list;
    struct fbo_entry        *current_fbo;
    GLuint                  fbo_read_binding;
    GLuint                  fbo_draw_binding;
    struct wined3d_surface **blit_targets;
    GLenum *draw_buffers;
    DWORD draw_buffers_mask; /* Enabled draw buffers, 31 max. */

    /* Queries */
    GLuint *free_occlusion_queries;
    UINT free_occlusion_query_size;
    UINT free_occlusion_query_count;
    struct list occlusion_queries;

    union wined3d_gl_query_object *free_event_queries;
    UINT free_event_query_size;
    UINT free_event_query_count;
    struct list event_queries;

    GLuint *free_timestamp_queries;
    UINT free_timestamp_query_size;
    UINT free_timestamp_query_count;
    struct list timestamp_queries;

    struct wined3d_stream_info stream_info;

    /* Fences for GL_APPLE_flush_buffer_range */
    struct wined3d_event_query *buffer_queries[MAX_ATTRIBS];
    unsigned int num_buffer_queries;

    DWORD                     tex_unit_map[MAX_COMBINED_SAMPLERS];
    DWORD                     rev_tex_unit_map[MAX_COMBINED_SAMPLERS];

    /* Extension emulation */
    GLint                   gl_fog_source;
    GLfloat                 fog_coord_value;
    GLfloat                 color[4], fogstart, fogend, fogcolor[4];
    GLuint                  dummy_arbfp_prog;
#if defined(STAGING_CSMT)

    GLenum                  offscreenBuffer;
#else  /* STAGING_CSMT */
};

struct wined3d_fb_state
{
    struct wined3d_rendertarget_view **render_targets;
    struct wined3d_rendertarget_view *depth_stencil;
#endif /* STAGING_CSMT */
};

typedef void (*APPLYSTATEFUNC)(struct wined3d_context *ctx, const struct wined3d_state *state, DWORD state_id);

struct StateEntry
{
    DWORD representative;
    APPLYSTATEFUNC apply;
};

struct StateEntryTemplate
{
    DWORD state;
    struct StateEntry content;
    enum wined3d_gl_extension extension;
};

#define WINED3D_FRAGMENT_CAP_PROJ_CONTROL   0x00000001
#define WINED3D_FRAGMENT_CAP_SRGB_WRITE     0x00000002
#define WINED3D_FRAGMENT_CAP_COLOR_KEY      0x00000004

struct fragment_caps
{
    DWORD wined3d_caps;
    DWORD PrimitiveMiscCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;
};

struct fragment_pipeline
{
    void (*enable_extension)(const struct wined3d_gl_info *gl_info, BOOL enable);
    void (*get_caps)(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps);
    void *(*alloc_private)(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv);
    void (*free_private)(struct wined3d_device *device);
    BOOL (*allocate_context_data)(struct wined3d_context *context);
    void (*free_context_data)(struct wined3d_context *context);
    BOOL (*color_fixup_supported)(struct color_fixup_desc fixup);
    const struct StateEntryTemplate *states;
};

struct wined3d_vertex_caps
{
    BOOL xyzrhw;
    BOOL ffp_generic_attributes;
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
    void (*vp_enable)(const struct wined3d_gl_info *gl_info, BOOL enable);
    void (*vp_get_caps)(const struct wined3d_gl_info *gl_info, struct wined3d_vertex_caps *caps);
    void *(*vp_alloc)(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv);
    void (*vp_free)(struct wined3d_device *device);
    const struct StateEntryTemplate *vp_states;
};

extern const struct StateEntryTemplate misc_state_template[] DECLSPEC_HIDDEN;
extern const struct fragment_pipeline none_fragment_pipe DECLSPEC_HIDDEN;
extern const struct fragment_pipeline ffp_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline atifs_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline arbfp_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline nvts_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline nvrc_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline glsl_fragment_pipe DECLSPEC_HIDDEN;

extern const struct wined3d_vertex_pipe_ops none_vertex_pipe DECLSPEC_HIDDEN;
extern const struct wined3d_vertex_pipe_ops ffp_vertex_pipe DECLSPEC_HIDDEN;
extern const struct wined3d_vertex_pipe_ops glsl_vertex_pipe DECLSPEC_HIDDEN;

/* "Base" state table */
HRESULT compile_state_table(struct StateEntry *StateTable, APPLYSTATEFUNC **dev_multistate_funcs,
        const struct wined3d_gl_info *gl_info, const struct wined3d_d3d_info *d3d_info,
        const struct wined3d_vertex_pipe_ops *vertex, const struct fragment_pipeline *fragment,
        const struct StateEntryTemplate *misc) DECLSPEC_HIDDEN;

enum wined3d_blit_op
{
    WINED3D_BLIT_OP_COLOR_BLIT,
    WINED3D_BLIT_OP_COLOR_BLIT_CKEY,
    WINED3D_BLIT_OP_COLOR_FILL,
    WINED3D_BLIT_OP_DEPTH_FILL,
    WINED3D_BLIT_OP_DEPTH_BLIT,
};

/* Shaders for color conversions in blits. Do not do blit operations while
 * already under the GL lock. */
struct blit_shader
{
    HRESULT (*alloc_private)(struct wined3d_device *device);
    void (*free_private)(struct wined3d_device *device);
    HRESULT (*set_shader)(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface,
            const struct wined3d_color_key *color_key);
    void (*unset_shader)(const struct wined3d_gl_info *gl_info);
    BOOL (*blit_supported)(const struct wined3d_gl_info *gl_info,
            const struct wined3d_d3d_info *d3d_info, enum wined3d_blit_op blit_op,
            const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
            const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format);
    HRESULT (*color_fill)(struct wined3d_device *device, struct wined3d_surface *dst_surface,
            const RECT *dst_rect, const struct wined3d_color *color);
    HRESULT (*depth_fill)(struct wined3d_device *device,
            struct wined3d_surface *surface, const RECT *rect, float depth);
    void (*blit_surface)(struct wined3d_device *device, DWORD filter,
            struct wined3d_surface *src_surface, const RECT *src_rect,
            struct wined3d_surface *dst_surface, const RECT *dst_rect,
            const struct wined3d_color_key *color_key);
};

extern const struct blit_shader ffp_blit DECLSPEC_HIDDEN;
extern const struct blit_shader arbfp_blit DECLSPEC_HIDDEN;
extern const struct blit_shader cpu_blit DECLSPEC_HIDDEN;

const struct blit_shader *wined3d_select_blitter(const struct wined3d_gl_info *gl_info,
        const struct wined3d_d3d_info *d3d_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
        DECLSPEC_HIDDEN;

struct wined3d_context *context_acquire(const struct wined3d_device *device,
        struct wined3d_surface *target) DECLSPEC_HIDDEN;
void context_alloc_event_query(struct wined3d_context *context,
        struct wined3d_event_query *query) DECLSPEC_HIDDEN;
void context_alloc_occlusion_query(struct wined3d_context *context,
        struct wined3d_occlusion_query *query) DECLSPEC_HIDDEN;
void context_apply_blit_state(struct wined3d_context *context, const struct wined3d_device *device) DECLSPEC_HIDDEN;
BOOL context_apply_clear_state(struct wined3d_context *context, const struct wined3d_device *device,
        UINT rt_count, const struct wined3d_fb_state *fb) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
BOOL context_apply_draw_state(struct wined3d_context *context, const struct wined3d_device *device,
        const struct wined3d_state *state) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
BOOL context_apply_draw_state(struct wined3d_context *context, struct wined3d_device *device) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */
void context_apply_fbo_state_blit(struct wined3d_context *context, GLenum target,
        struct wined3d_surface *render_target, struct wined3d_surface *depth_stencil, DWORD location) DECLSPEC_HIDDEN;
void context_active_texture(struct wined3d_context *context, const struct wined3d_gl_info *gl_info,
        unsigned int unit) DECLSPEC_HIDDEN;
void context_bind_texture(struct wined3d_context *context, GLenum target, GLuint name) DECLSPEC_HIDDEN;
void context_check_fbo_status(const struct wined3d_context *context, GLenum target) DECLSPEC_HIDDEN;
struct wined3d_context *context_create(struct wined3d_swapchain *swapchain, struct wined3d_surface *target,
        const struct wined3d_format *ds_format) DECLSPEC_HIDDEN;
HGLRC context_create_wgl_attribs(const struct wined3d_gl_info *gl_info, HDC hdc, HGLRC share_ctx) DECLSPEC_HIDDEN;
void context_destroy(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
void context_free_event_query(struct wined3d_event_query *query) DECLSPEC_HIDDEN;
void context_free_occlusion_query(struct wined3d_occlusion_query *query) DECLSPEC_HIDDEN;
struct wined3d_context *context_get_current(void) DECLSPEC_HIDDEN;
DWORD context_get_tls_idx(void) DECLSPEC_HIDDEN;
void context_invalidate_state(struct wined3d_context *context, DWORD state_id) DECLSPEC_HIDDEN;
void context_release(struct wined3d_context *context) DECLSPEC_HIDDEN;
void context_resource_released(const struct wined3d_device *device,
        struct wined3d_resource *resource, enum wined3d_resource_type type) DECLSPEC_HIDDEN;
void context_resource_unloaded(const struct wined3d_device *device,
        struct wined3d_resource *resource, enum wined3d_resource_type type) DECLSPEC_HIDDEN;
BOOL context_set_current(struct wined3d_context *ctx) DECLSPEC_HIDDEN;
void context_set_draw_buffer(struct wined3d_context *context, GLenum buffer) DECLSPEC_HIDDEN;
void context_set_tls_idx(DWORD idx) DECLSPEC_HIDDEN;
void context_state_drawbuf(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void context_state_fb(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void context_surface_update(struct wined3d_context *context, const struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void context_stream_info_from_declaration(struct wined3d_context *context,
        const struct wined3d_state *state, struct wined3d_stream_info *stream_info) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Internal representation of a light
 */
struct wined3d_light_info
{
    struct wined3d_light OriginalParms; /* Note D3D8LIGHT == D3D9LIGHT */
    DWORD        OriginalIndex;
    LONG         glIndex;
    BOOL         enabled;

    /* Converted parms to speed up swapping lights */
    struct wined3d_vec4 position;
    struct wined3d_vec4 direction;
    float exponent;
    float cutoff;

    struct list entry;
};

/* The default light parameters */
extern const struct wined3d_light WINED3D_default_light DECLSPEC_HIDDEN;

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
};

enum wined3d_pci_vendor
{
    HW_VENDOR_SOFTWARE              = 0x0000,
    HW_VENDOR_AMD                   = 0x1002,
    HW_VENDOR_NVIDIA                = 0x10de,
    HW_VENDOR_VMWARE                = 0x15ad,
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
    CARD_AMD_RADEON_HD6410D         = 0x9644,
    CARD_AMD_RADEON_HD6550D         = 0x9640,
    CARD_AMD_RADEON_HD6600          = 0x6758,
    CARD_AMD_RADEON_HD6600M         = 0x6741,
    CARD_AMD_RADEON_HD6700          = 0x68ba,
    CARD_AMD_RADEON_HD6800          = 0x6739,
    CARD_AMD_RADEON_HD6900          = 0x6719,
    CARD_AMD_RADEON_HD7660D         = 0x9901,
    CARD_AMD_RADEON_HD7700          = 0x683d,
    CARD_AMD_RADEON_HD7800          = 0x6819,
    CARD_AMD_RADEON_HD7900          = 0x679a,
    CARD_AMD_RADEON_HD8600M         = 0x6660,
    CARD_AMD_RADEON_HD8670          = 0x6610,
    CARD_AMD_RADEON_HD8770          = 0x665c,
    CARD_AMD_RADEON_R3              = 0x9830,
    CARD_AMD_RADEON_R7              = 0x130f,
    CARD_AMD_RADEON_R9              = 0x67b1,

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
    CARD_NVIDIA_GEFORCE_9800GT      = 0x0614,
    CARD_NVIDIA_GEFORCE_210         = 0x0a23,
    CARD_NVIDIA_GEFORCE_GT220       = 0x0a20,
    CARD_NVIDIA_GEFORCE_GT240       = 0x0ca3,
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
    CARD_NVIDIA_GEFORCE_GT430       = 0x0de1,
    CARD_NVIDIA_GEFORCE_GT440       = 0x0de0,
    CARD_NVIDIA_GEFORCE_GTS450      = 0x0dc4,
    CARD_NVIDIA_GEFORCE_GTX460      = 0x0e22,
    CARD_NVIDIA_GEFORCE_GTX460M     = 0x0dd1,
    CARD_NVIDIA_GEFORCE_GTX465      = 0x06c4,
    CARD_NVIDIA_GEFORCE_GTX470      = 0x06cd,
    CARD_NVIDIA_GEFORCE_GTX480      = 0x06c0,
    CARD_NVIDIA_GEFORCE_GT520       = 0x1040,
    CARD_NVIDIA_GEFORCE_GT540M      = 0x0df4,
    CARD_NVIDIA_GEFORCE_GTX550      = 0x1244,
    CARD_NVIDIA_GEFORCE_GT555M      = 0x04b8,
    CARD_NVIDIA_GEFORCE_GTX560TI    = 0x1200,
    CARD_NVIDIA_GEFORCE_GTX560      = 0x1201,
    CARD_NVIDIA_GEFORCE_GTX570      = 0x1081,
    CARD_NVIDIA_GEFORCE_GTX580      = 0x1080,
    CARD_NVIDIA_GEFORCE_GT610       = 0x104a,
    CARD_NVIDIA_GEFORCE_GT630       = 0x0f00,
    CARD_NVIDIA_GEFORCE_GT630M      = 0x0de9,
    CARD_NVIDIA_GEFORCE_GT640M      = 0x0fd2,
    CARD_NVIDIA_GEFORCE_GT650M      = 0x0fd1,
    CARD_NVIDIA_GEFORCE_GTX650      = 0x0fc6,
    CARD_NVIDIA_GEFORCE_GTX650TI    = 0x11c6,
    CARD_NVIDIA_GEFORCE_GTX660      = 0x11c0,
    CARD_NVIDIA_GEFORCE_GTX660M     = 0x0fd4,
    CARD_NVIDIA_GEFORCE_GTX660TI    = 0x1183,
    CARD_NVIDIA_GEFORCE_GTX670      = 0x1189,
    CARD_NVIDIA_GEFORCE_GTX670MX    = 0x11a1,
    CARD_NVIDIA_GEFORCE_GTX680      = 0x1180,
    CARD_NVIDIA_GEFORCE_GT750M      = 0x0fe9,
    CARD_NVIDIA_GEFORCE_GTX750      = 0x1381,
    CARD_NVIDIA_GEFORCE_GTX750TI    = 0x1380,
    CARD_NVIDIA_GEFORCE_GTX760      = 0x1187,
    CARD_NVIDIA_GEFORCE_GTX765M     = 0x11e2,
    CARD_NVIDIA_GEFORCE_GTX770M     = 0x11e0,
    CARD_NVIDIA_GEFORCE_GTX770      = 0x1184,
    CARD_NVIDIA_GEFORCE_GTX780      = 0x1004,
    CARD_NVIDIA_GEFORCE_GTX780TI    = 0x100a,
    CARD_NVIDIA_GEFORCE_GTX970      = 0x13c2,
    CARD_NVIDIA_GEFORCE_GTX970M     = 0x13d8,

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
    CARD_INTEL_HWM                  = 0x0416,
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
    void (WINE_GLAPI *glFramebufferTexture1D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture2D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture3D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level, GLint layer);
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
    UINT buffers;
    UINT lights;
    UINT textures;
    UINT texture_coords;
    UINT vertex_uniform_blocks;
    UINT geometry_uniform_blocks;
    UINT fragment_uniform_blocks;
    UINT fragment_samplers;
    UINT vertex_samplers;
    UINT combined_samplers;
    UINT general_combiners;
    UINT clipplanes;
    UINT texture_size;
    UINT texture3d_size;
    float pointsize_max;
    float pointsize_min;
    UINT blends;
    UINT anisotropy;
    float shininess;
    UINT samples;
    UINT vertex_attribs;

    UINT glsl_varyings;
    UINT glsl_vs_float_constants;
    UINT glsl_ps_float_constants;

    UINT arb_vs_float_constants;
    UINT arb_vs_native_constants;
    UINT arb_vs_instructions;
    UINT arb_vs_temps;
    UINT arb_ps_float_constants;
    UINT arb_ps_local_constants;
    UINT arb_ps_native_constants;
    UINT arb_ps_instructions;
    UINT arb_ps_temps;
};

struct wined3d_gl_info
{
    DWORD selected_gl_version;
    DWORD glsl_version;
    struct wined3d_gl_limits limits;
    DWORD reserved_glsl_constants, reserved_arb_constants;
    DWORD quirks;
    BOOL supported[WINED3D_GL_EXT_COUNT];
    GLint wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP + 1];

    HGLRC (WINAPI *p_wglCreateContextAttribsARB)(HDC dc, HGLRC share, const GLint *attribs);
    struct opengl_funcs gl_ops;
    struct wined3d_fbo_ops fbo_ops;

    struct wined3d_format *formats;
};

struct wined3d_driver_info
{
    enum wined3d_pci_vendor vendor;
    enum wined3d_pci_device device;
    const char *name;
    const char *description;
    UINT64 vram_bytes;
    DWORD version_high;
    DWORD version_low;
};

struct wined3d_d3d_limits
{
    UINT vs_version, gs_version, ps_version;
    DWORD vs_uniform_count;
    DWORD ps_uniform_count;
    UINT ffp_textures;
    UINT ffp_blend_stages;
    UINT ffp_vertex_blend_matrices;
};

typedef void (WINE_GLAPI *wined3d_ffp_attrib_func)(const void *data);
typedef void (WINE_GLAPI *wined3d_ffp_texcoord_func)(GLenum unit, const void *data);
extern wined3d_ffp_attrib_func specular_func_3ubv DECLSPEC_HIDDEN;

struct wined3d_ffp_attrib_ops
{
    wined3d_ffp_attrib_func position[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func diffuse[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func specular[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func normal[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_texcoord_func texcoord[WINED3D_FFP_EMIT_COUNT];
};

struct wined3d_d3d_info
{
    struct wined3d_d3d_limits limits;
    struct wined3d_ffp_attrib_ops ffp_attrib_ops;
    BOOL xyzrhw;
    BOOL ffp_generic_attributes;
    BOOL vs_clipping;
    BOOL shader_color_key;
    DWORD valid_rt_mask;
};

/* The adapter structure */
struct wined3d_adapter
{
    UINT ordinal;
    POINT monitorPoint;
    enum wined3d_format_id screen_format;

    struct wined3d_gl_info  gl_info;
    struct wined3d_d3d_info d3d_info;
    struct wined3d_driver_info driver_info;
    WCHAR                   DeviceName[CCHDEVICENAME]; /* DeviceName for use with e.g. ChangeDisplaySettings */
    unsigned int cfg_count;
    struct wined3d_pixel_format *cfgs;
    UINT64 vram_bytes;
    UINT64 vram_bytes_used;
    LUID luid;

    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct fragment_pipeline *fragment_pipe;
    const struct wined3d_shader_backend_ops *shader_backend;
    const struct blit_shader *blitter;
};

BOOL wined3d_adapter_init_format_info(struct wined3d_adapter *adapter) DECLSPEC_HIDDEN;
UINT64 adapter_adjust_memory(struct wined3d_adapter *adapter, INT64 amount) DECLSPEC_HIDDEN;

BOOL initPixelFormatsNoGL(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
extern void add_gl_compat_wrappers(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

enum projection_types
{
    proj_none    = 0,
    proj_count3  = 1,
    proj_count4  = 2
};

enum dst_arg
{
    resultreg    = 0,
    tempreg      = 1
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
    unsigned                dst : 1;
    unsigned                projected : 2;
    unsigned                padding : 10;
};

struct ffp_frag_settings
{
    struct texture_stage_op op[MAX_TEXTURES];
    enum wined3d_ffp_ps_fog_mode fog;
    unsigned char sRGB_write;
    unsigned char emul_clipplanes;
    unsigned char color_key_enabled;
    unsigned char padding;
};

struct ffp_frag_desc
{
    struct wine_rb_entry entry;
    struct ffp_frag_settings    settings;
};

extern const struct wine_rb_functions wined3d_ffp_frag_program_rb_functions DECLSPEC_HIDDEN;
extern const struct wine_rb_functions wined3d_ffp_vertex_program_rb_functions DECLSPEC_HIDDEN;
extern const struct wined3d_parent_ops wined3d_null_parent_ops DECLSPEC_HIDDEN;

void gen_ffp_frag_op(const struct wined3d_context *context, const struct wined3d_state *state,
        struct ffp_frag_settings *settings, BOOL ignore_textype) DECLSPEC_HIDDEN;
const struct ffp_frag_desc *find_ffp_frag_shader(const struct wine_rb_tree *fragment_shaders,
        const struct ffp_frag_settings *settings) DECLSPEC_HIDDEN;
void add_ffp_frag_shader(struct wine_rb_tree *shaders, struct ffp_frag_desc *desc) DECLSPEC_HIDDEN;
void wined3d_get_draw_rect(const struct wined3d_state *state, RECT *rect) DECLSPEC_HIDDEN;
void wined3d_ftoa(float value, char *s) DECLSPEC_HIDDEN;

extern const float wined3d_srgb_const0[] DECLSPEC_HIDDEN;
extern const float wined3d_srgb_const1[] DECLSPEC_HIDDEN;

enum wined3d_ffp_vs_fog_mode
{
    WINED3D_FFP_VS_FOG_OFF      = 0,
    WINED3D_FFP_VS_FOG_FOGCOORD = 1,
    WINED3D_FFP_VS_FOG_DEPTH    = 2,
    WINED3D_FFP_VS_FOG_RANGE    = 3,
};

#define WINED3D_FFP_TCI_SHIFT               16
#define WINED3D_FFP_TCI_MASK                0xff

#define WINED3D_FFP_LIGHT_TYPE_SHIFT(idx)   (3 * (idx))
#define WINED3D_FFP_LIGHT_TYPE_MASK         0x7

struct wined3d_ffp_vs_settings
{
    DWORD light_type      : 24; /* MAX_ACTIVE_LIGHTS, 8 * 3 */
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
    DWORD texcoords       : 8;  /* MAX_TEXTURES */
    DWORD ortho_fog       : 1;
    DWORD padding         : 11;

    DWORD texgen[MAX_TEXTURES];
};

struct wined3d_ffp_vs_desc
{
    struct wine_rb_entry entry;
    struct wined3d_ffp_vs_settings settings;
};

void wined3d_ffp_get_vs_settings(const struct wined3d_state *state, const struct wined3d_stream_info *si,
        struct wined3d_ffp_vs_settings *settings) DECLSPEC_HIDDEN;

struct wined3d
{
    LONG ref;
    DWORD flags;
    UINT adapter_count;
    struct wined3d_adapter adapters[1];
};

HRESULT wined3d_init(struct wined3d *wined3d, DWORD flags) DECLSPEC_HIDDEN;
BOOL wined3d_register_window(HWND window, struct wined3d_device *device) DECLSPEC_HIDDEN;
void wined3d_unregister_window(HWND window) DECLSPEC_HIDDEN;

struct wined3d_stream_output
{
    struct wined3d_buffer *buffer;
    UINT offset;
};

struct wined3d_stream_state
{
    struct wined3d_buffer *buffer;
    UINT offset;
    UINT stride;
    UINT frequency;
    UINT flags;
};

#define WINED3D_STATE_NO_REF        0x00000001
#define WINED3D_STATE_INIT_DEFAULT  0x00000002

struct wined3d_state
{
    DWORD flags;
#if defined(STAGING_CSMT)
    struct wined3d_fb_state fb;
#else  /* STAGING_CSMT */
    const struct wined3d_fb_state *fb;
#endif /* STAGING_CSMT */

    struct wined3d_vertex_declaration *vertex_declaration;
    struct wined3d_stream_output stream_output[MAX_STREAM_OUT];
    struct wined3d_stream_state streams[MAX_STREAMS + 1 /* tesselated pseudo-stream */];
    struct wined3d_buffer *index_buffer;
    enum wined3d_format_id index_format;
    INT base_vertex_index;
    INT load_base_vertex_index; /* Non-indexed drawing needs 0 here, indexed needs base_vertex_index. */
    GLenum gl_primitive_type;
    struct wined3d_query *predicate;
    BOOL predicate_value;

    struct wined3d_shader *shader[WINED3D_SHADER_TYPE_COUNT];
    struct wined3d_buffer *cb[WINED3D_SHADER_TYPE_COUNT][MAX_CONSTANT_BUFFERS];
    struct wined3d_sampler *sampler[WINED3D_SHADER_TYPE_COUNT][MAX_SAMPLER_OBJECTS];
    struct wined3d_shader_resource_view *shader_resource_view[WINED3D_SHADER_TYPE_COUNT][MAX_SHADER_RESOURCE_VIEWS];

    BOOL vs_consts_b[MAX_CONST_B];
    INT vs_consts_i[MAX_CONST_I * 4];
    float *vs_consts_f;

    BOOL ps_consts_b[MAX_CONST_B];
    INT ps_consts_i[MAX_CONST_I * 4];
    float *ps_consts_f;

    struct wined3d_texture *textures[MAX_COMBINED_SAMPLERS];
    DWORD sampler_states[MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];
    DWORD texture_states[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];

    struct wined3d_matrix transforms[HIGHEST_TRANSFORMSTATE + 1];
    struct wined3d_vec4 clip_planes[MAX_CLIPPLANES];
    struct wined3d_material material;
    struct wined3d_viewport viewport;
    RECT scissor_rect;

    /* Light hashmap. Collisions are handled using linked lists. */
#define LIGHTMAP_SIZE 43
#define LIGHTMAP_HASHFUNC(x) ((x) % LIGHTMAP_SIZE)
    struct list light_map[LIGHTMAP_SIZE];
    const struct wined3d_light_info *lights[MAX_ACTIVE_LIGHTS];

    DWORD render_states[WINEHIGHEST_RENDER_STATE + 1];
};

#if defined(STAGING_CSMT)
struct wined3d_gl_bo
{
    GLuint name;
    GLenum usage;
    GLenum type_hint;
    UINT size;
};

#endif /* STAGING_CSMT */
#define WINED3D_UNMAPPED_STAGE ~0U

/* Multithreaded flag. Removed from the public header to signal that
 * wined3d_device_create() ignores it. */
#define WINED3DCREATE_MULTITHREADED 0x00000004

struct wined3d_device
{
    LONG ref;

    /* WineD3D Information  */
    struct wined3d_device_parent *device_parent;
    struct wined3d *wined3d;
    struct wined3d_adapter *adapter;

    /* Window styles to restore when switching fullscreen mode */
    LONG                    style;
    LONG                    exStyle;

    /* X and GL Information */
    GLenum                  offscreenBuffer;

    const struct wined3d_shader_backend_ops *shader_backend;
    void *shader_priv;
    void *fragment_priv;
    void *vertex_priv;
    void *blit_priv;
    struct StateEntry StateTable[STATE_HIGHEST + 1];
    /* Array of functions for states which are handled by more than one pipeline part */
    APPLYSTATEFUNC *multistate_funcs[STATE_HIGHEST + 1];
    const struct blit_shader *blitter;

    BYTE vertexBlendUsed : 1;           /* To avoid needless setting of the blend matrices */
    BYTE bCursorVisible : 1;
    BYTE d3d_initialized : 1;
    BYTE inScene : 1;                   /* A flag to check for proper BeginScene / EndScene call pairs */
    BYTE softwareVertexProcessing : 1;  /* process vertex shaders using software or hardware */
    BYTE filter_messages : 1;
    BYTE padding : 2;

    unsigned char           surface_alignment; /* Line Alignment of surfaces                      */

    WORD padding2 : 16;

    struct wined3d_state state;
    struct wined3d_state *update_state;
    struct wined3d_stateblock *recording;

    /* Internal use fields  */
    struct wined3d_device_creation_parameters create_parms;
    HWND focus_window;

    struct wined3d_rendertarget_view *back_buffer_view;
    struct wined3d_swapchain **swapchains;
    UINT swapchain_count;
#if defined(STAGING_CSMT)
    struct wined3d_rendertarget_view *auto_depth_stencil_view;

    struct list             resources; /* a linked list to track resources created by the device */
    struct list             shaders;   /* a linked list to track shaders (pixel and vertex)      */
    struct wine_rb_tree samplers;
#else  /* STAGING_CSMT */

    struct list             resources; /* a linked list to track resources created by the device */
    struct list             shaders;   /* a linked list to track shaders (pixel and vertex)      */
    struct wine_rb_tree samplers;

    /* Render Target Support */
    struct wined3d_fb_state fb;
    struct wined3d_surface *onscreen_depth_stencil;
    struct wined3d_rendertarget_view *auto_depth_stencil_view;
#endif /* STAGING_CSMT */

    /* For rendering to a texture using glCopyTexImage */
    GLuint                  depth_blt_texture;

    /* Cursor management */
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;
    UINT                    cursorWidth, cursorHeight;
#if !defined(STAGING_CSMT)
    struct wined3d_texture *cursor_texture;
#endif /* STAGING_CSMT */
    HCURSOR                 hardwareCursor;

    /* The Wine logo texture */
    struct wined3d_texture *logo_texture;

    /* Textures for when no other textures are mapped */
    UINT dummy_texture_2d[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_rect[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_3d[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_cube[MAX_COMBINED_SAMPLERS];

    /* Command stream */
    struct wined3d_cs *cs;

    /* Context management */
    struct wined3d_context **contexts;
    UINT context_count;
};

void device_clear_render_targets(struct wined3d_device *device, UINT rt_count, const struct wined3d_fb_state *fb,
        UINT rect_count, const RECT *rects, const RECT *draw_rect, DWORD flags,
        const struct wined3d_color *color, float depth, DWORD stencil) DECLSPEC_HIDDEN;
BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
void device_context_remove(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
HRESULT device_init(struct wined3d_device *device, struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, HWND focus_window, DWORD flags,
        BYTE surface_alignment, struct wined3d_device_parent *device_parent) DECLSPEC_HIDDEN;
LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc) DECLSPEC_HIDDEN;
void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void device_invalidate_state(const struct wined3d_device *device, DWORD state) DECLSPEC_HIDDEN;
void device_invalidate_shader_constants(const struct wined3d_device *device, DWORD mask) DECLSPEC_HIDDEN;
void device_exec_update_texture(struct wined3d_context *context, struct wined3d_texture *src_texture,
        struct wined3d_texture *dst_texture) DECLSPEC_HIDDEN;
struct wined3d_gl_bo *wined3d_device_get_bo(struct wined3d_device *device, UINT size, GLenum gl_usage,
        GLenum type_hint, struct wined3d_context *context) DECLSPEC_HIDDEN;
void wined3d_device_release_bo(struct wined3d_device *device, struct wined3d_gl_bo *bo,
        const struct wined3d_context *context) DECLSPEC_HIDDEN;
void device_create_dummy_textures(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
void device_delete_opengl_contexts_cs(struct wined3d_device *device,
        struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
void device_switch_onscreen_ds(struct wined3d_device *device, struct wined3d_context *context,
        struct wined3d_surface *depth_stencil) DECLSPEC_HIDDEN;
void device_invalidate_state(const struct wined3d_device *device, DWORD state) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

static inline BOOL isStateDirty(const struct wined3d_context *context, DWORD state)
{
    DWORD idx = state / (sizeof(*context->isStateDirty) * CHAR_BIT);
    BYTE shift = state & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
    return context->isStateDirty[idx] & (1 << shift);
}

#define WINED3D_RESOURCE_ACCESS_GPU     0x1
#define WINED3D_RESOURCE_ACCESS_CPU     0x2

struct wined3d_resource_ops
{
    ULONG (*resource_incref)(struct wined3d_resource *resource);
    ULONG (*resource_decref)(struct wined3d_resource *resource);
    void (*resource_unload)(struct wined3d_resource *resource);
#if defined(STAGING_CSMT)
    void (*resource_location_invalidated)(struct wined3d_resource *resource, DWORD location);
    void (*resource_load_location)(struct wined3d_resource *resource,
            struct wined3d_context *context, DWORD location);
#endif /* STAGING_CSMT */
};

struct wined3d_resource
{
    LONG ref;
    LONG bind_count;
    LONG map_count;
    struct wined3d_device *device;
    enum wined3d_resource_type type;
    enum wined3d_gl_resource_type gl_type;
    const struct wined3d_format *format;
    unsigned int format_flags;
    enum wined3d_multisample_type multisample_type;
    UINT multisample_quality;
    DWORD usage;
    enum wined3d_pool pool;
    DWORD access_flags;
    DWORD draw_binding;
    DWORD map_binding;
    UINT width;
    UINT height;
    UINT depth;
    UINT size;
    DWORD priority;
#if defined(STAGING_CSMT)
    void *heap_memory, *map_heap_memory, *user_memory, *bitmap_data;
    UINT custom_row_pitch, custom_slice_pitch;
    struct wined3d_gl_bo *buffer, *map_buffer;
    struct list resource_list_entry;
    DWORD locations;
    LONG access_fence;
    BOOL unmap_dirtify;
#else  /* STAGING_CSMT */
    void *heap_memory;
    UINT custom_row_pitch, custom_slice_pitch;
    struct list resource_list_entry;
#endif /* STAGING_CSMT */

    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    const struct wined3d_resource_ops *resource_ops;
};

static inline ULONG wined3d_resource_incref(struct wined3d_resource *resource)
{
    return resource->resource_ops->resource_incref(resource);
}

static inline ULONG wined3d_resource_decref(struct wined3d_resource *resource)
{
    return resource->resource_ops->resource_decref(resource);
}

void resource_cleanup(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
HRESULT resource_init(struct wined3d_resource *resource, struct wined3d_device *device,
        enum wined3d_resource_type type, const struct wined3d_format *format,
        enum wined3d_multisample_type multisample_type, UINT multisample_quality,
        DWORD usage, enum wined3d_pool pool, UINT width, UINT height, UINT depth, UINT size,
        void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops) DECLSPEC_HIDDEN;
void resource_unload(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
DWORD wined3d_resource_access_from_location(DWORD location) DECLSPEC_HIDDEN;
BOOL wined3d_resource_allocate_sysmem(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_changed(struct wined3d_resource *resource,
        struct wined3d_gl_bo *swap_buffer, void *swap_heap_memory) DECLSPEC_HIDDEN;
BOOL wined3d_resource_check_block_align(const struct wined3d_resource *resource,
        const struct wined3d_box *box) DECLSPEC_HIDDEN;
void wined3d_resource_cleanup_cs(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_free_bo(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_free_sysmem(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
BYTE *wined3d_resource_get_map_ptr(const struct wined3d_resource *resource,
        const struct wined3d_context *context, DWORD flags) DECLSPEC_HIDDEN;
void wined3d_resource_get_memory(const struct wined3d_resource *resource,
        DWORD location, struct wined3d_bo_address *data) DECLSPEC_HIDDEN;
GLbitfield wined3d_resource_gl_map_flags(DWORD d3d_flags) DECLSPEC_HIDDEN;
void wined3d_resource_invalidate_location(struct wined3d_resource *resource, DWORD location) DECLSPEC_HIDDEN;
BOOL wined3d_resource_is_offscreen(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_load_location(struct wined3d_resource *resource,
        struct wined3d_context *context, DWORD location) DECLSPEC_HIDDEN;
HRESULT wined3d_resource_map(struct wined3d_resource *resource, struct wined3d_map_desc *map_desc,
        const struct wined3d_box *box, DWORD flags) DECLSPEC_HIDDEN;
void *wined3d_resource_map_internal(struct wined3d_resource *resource, DWORD flags) DECLSPEC_HIDDEN;
BOOL wined3d_resource_prepare_map_memory(struct wined3d_resource *resource,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
BOOL wined3d_resource_prepare_system_memory(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_release_map_ptr(const struct wined3d_resource *resource,
        const struct wined3d_context *context) DECLSPEC_HIDDEN;
DWORD wined3d_resource_sanitize_map_flags(const struct wined3d_resource *resource, DWORD flags) DECLSPEC_HIDDEN;
HRESULT wined3d_resource_unmap(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_unmap_internal(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_update_draw_binding(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_validate_location(struct wined3d_resource *resource, DWORD location) DECLSPEC_HIDDEN;

static inline void wined3d_resource_inc_fence(struct wined3d_resource *resource)
{
    InterlockedIncrement(&resource->access_fence);
}

static inline void wined3d_resource_dec_fence(struct wined3d_resource *resource)
{
    InterlockedDecrement(&resource->access_fence);
}

static inline void wined3d_resource_wait_fence(struct wined3d_resource *resource)
{
    while(InterlockedCompareExchange(&resource->access_fence, 0, 0));
}
#else  /* STAGING_CSMT */
BOOL wined3d_resource_allocate_sysmem(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_resource_free_sysmem(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
GLbitfield wined3d_resource_gl_map_flags(DWORD d3d_flags) DECLSPEC_HIDDEN;
GLenum wined3d_resource_gl_legacy_map_flags(DWORD d3d_flags) DECLSPEC_HIDDEN;
BOOL wined3d_resource_is_offscreen(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
DWORD wined3d_resource_sanitize_map_flags(const struct wined3d_resource *resource, DWORD flags) DECLSPEC_HIDDEN;
void wined3d_resource_update_draw_binding(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

/* Tests show that the start address of resources is 32 byte aligned */
#define RESOURCE_ALIGNMENT 16

struct gl_texture
{
    struct wined3d_sampler_desc sampler_desc;
    unsigned int base_level;
    GLuint name;
};

struct wined3d_texture_ops
{
    void (*texture_sub_resource_load)(struct wined3d_resource *sub_resource,
            struct wined3d_context *context, BOOL srgb);
    void (*texture_sub_resource_add_dirty_region)(struct wined3d_resource *sub_resource,
            const struct wined3d_box *dirty_region);
    void (*texture_sub_resource_cleanup)(struct wined3d_resource *sub_resource);
    void (*texture_sub_resource_invalidate_location)(struct wined3d_resource *sub_resource, DWORD location);
    void (*texture_sub_resource_validate_location)(struct wined3d_resource *sub_resource, DWORD location);
    void (*texture_sub_resource_upload_data)(struct wined3d_resource *sub_resource,
            const struct wined3d_context *context, const struct wined3d_sub_resource_data *data);
    void (*texture_prepare_texture)(struct wined3d_texture *texture,
            struct wined3d_context *context, BOOL srgb);
};

#define WINED3D_TEXTURE_COND_NP2            0x00000001
#define WINED3D_TEXTURE_POW2_MAT_IDENT      0x00000002
#define WINED3D_TEXTURE_IS_SRGB             0x00000004
#define WINED3D_TEXTURE_RGB_ALLOCATED       0x00000008
#define WINED3D_TEXTURE_RGB_VALID           0x00000010
#define WINED3D_TEXTURE_SRGB_ALLOCATED      0x00000020
#define WINED3D_TEXTURE_SRGB_VALID          0x00000040
#define WINED3D_TEXTURE_CONVERTED           0x00000080
#define WINED3D_TEXTURE_PIN_SYSMEM          0x00000100
#define WINED3D_TEXTURE_DYNAMIC_MAP         0x00000200
#define WINED3D_TEXTURE_NORMALIZED_COORDS   0x00000400

#define WINED3D_TEXTURE_ASYNC_COLOR_KEY     0x00000001

struct wined3d_texture
{
    struct wined3d_resource resource;
    const struct wined3d_texture_ops *texture_ops;
    struct gl_texture texture_rgb, texture_srgb;
    struct wined3d_resource **sub_resources;
    struct wined3d_swapchain *swapchain;
    UINT layer_count;
    UINT level_count;
    float pow2_matrix[16];
    UINT lod;
    enum wined3d_texture_filter_type filter_type;
    DWORD sampler;
    DWORD flags;
    GLenum target;

    /* May only be accessed from the command stream worker thread. */
    struct wined3d_texture_async
    {
        DWORD flags;

        /* Color keys for DDraw */
        struct wined3d_color_key dst_blt_color_key;
        struct wined3d_color_key src_blt_color_key;
        struct wined3d_color_key dst_overlay_color_key;
        struct wined3d_color_key src_overlay_color_key;
        struct wined3d_color_key gl_color_key;
        DWORD color_key_flags;
    } async;
};

static inline struct wined3d_texture *wined3d_texture_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_texture, resource);
}

static inline struct gl_texture *wined3d_texture_get_gl_texture(struct wined3d_texture *texture,
        BOOL srgb)
{
    return srgb ? &texture->texture_srgb : &texture->texture_rgb;
}

void wined3d_texture_apply_sampler_desc(struct wined3d_texture *texture,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void wined3d_texture_cleanup_cs(struct wined3d_texture *texture) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */
void wined3d_texture_bind(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void wined3d_texture_bind_and_dirtify(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void wined3d_texture_force_reload(struct wined3d_texture *texture) DECLSPEC_HIDDEN;
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void wined3d_texture_prepare_texture(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void wined3d_texture_set_dirty(struct wined3d_texture *texture) DECLSPEC_HIDDEN;
void wined3d_texture_set_swapchain(struct wined3d_texture *texture,
        struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;

#define WINED3D_LOCATION_DISCARDED      0x00000001
#define WINED3D_LOCATION_SYSMEM         0x00000002
#define WINED3D_LOCATION_USER_MEMORY    0x00000004
#define WINED3D_LOCATION_DIB            0x00000008
#define WINED3D_LOCATION_BUFFER         0x00000010
#define WINED3D_LOCATION_TEXTURE_RGB    0x00000020
#define WINED3D_LOCATION_TEXTURE_SRGB   0x00000040
#define WINED3D_LOCATION_DRAWABLE       0x00000080
#define WINED3D_LOCATION_RB_MULTISAMPLE 0x00000100
#define WINED3D_LOCATION_RB_RESOLVED    0x00000200

const char *wined3d_debug_location(DWORD location) DECLSPEC_HIDDEN;

#define WINED3D_VFLAG_CLIENT_STORAGE    0x00000001

struct wined3d_volume
{
    struct wined3d_resource resource;
    struct wined3d_texture *container;

#if defined(STAGING_CSMT)
    DWORD flags;
    GLint texture_level;
    DWORD download_count;
#else  /* STAGING_CSMT */
    DWORD flags, locations;
    GLint texture_level;
    DWORD download_count;
    GLuint pbo;
#endif /* STAGING_CSMT */
};

static inline struct wined3d_volume *volume_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_volume, resource);
}

#if defined(STAGING_CSMT)
HRESULT wined3d_volume_create(struct wined3d_texture *container, const struct wined3d_resource_desc *desc,
        unsigned int level, struct wined3d_volume **volume) DECLSPEC_HIDDEN;
void wined3d_volume_destroy(struct wined3d_volume *volume) DECLSPEC_HIDDEN;
void wined3d_volume_load(struct wined3d_volume *volume, struct wined3d_context *context,
        BOOL srgb_mode) DECLSPEC_HIDDEN;
void wined3d_volume_upload_data(struct wined3d_volume *volume, const struct wined3d_context *context,
        const struct wined3d_const_bo_address *data) DECLSPEC_HIDDEN;
void wined3d_volume_cleanup_cs(struct wined3d_volume *volume) DECLSPEC_HIDDEN;

struct wined3d_surface_dib
{
    HBITMAP DIBsection;
#else  /* STAGING_CSMT */
BOOL volume_prepare_system_memory(struct wined3d_volume *volume) DECLSPEC_HIDDEN;
HRESULT wined3d_volume_create(struct wined3d_texture *container, const struct wined3d_resource_desc *desc,
        unsigned int level, struct wined3d_volume **volume) DECLSPEC_HIDDEN;
void wined3d_volume_destroy(struct wined3d_volume *volume) DECLSPEC_HIDDEN;
void wined3d_volume_load(struct wined3d_volume *volume, struct wined3d_context *context,
        BOOL srgb_mode) DECLSPEC_HIDDEN;
void wined3d_volume_invalidate_location(struct wined3d_volume *volume, DWORD location) DECLSPEC_HIDDEN;
void wined3d_volume_validate_location(struct wined3d_volume *volume, DWORD location) DECLSPEC_HIDDEN;
void wined3d_volume_upload_data(struct wined3d_volume *volume, const struct wined3d_context *context,
        const struct wined3d_const_bo_address *data) DECLSPEC_HIDDEN;

struct wined3d_surface_dib
{
    HBITMAP DIBsection;
    void *bitmap_data;
#endif /* STAGING_CSMT */
    UINT bitmap_size;
};

struct wined3d_renderbuffer_entry
{
    struct list entry;
    GLuint id;
    UINT width;
    UINT height;
};

struct fbo_entry
{
    struct list entry;
    struct wined3d_surface **render_targets;
    struct wined3d_surface *depth_stencil;
    DWORD color_location, ds_location;
    DWORD rt_mask;
    BOOL attached;
    GLuint id;
};

struct wined3d_surface_ops
{
    HRESULT (*surface_private_setup)(struct wined3d_surface *surface);
#if defined(STAGING_CSMT)
    void (*surface_frontbuffer_updated)(struct wined3d_surface *surface);
#else  /* STAGING_CSMT */
    void (*surface_unmap)(struct wined3d_surface *surface);
#endif /* STAGING_CSMT */
};

struct wined3d_surface
{
    struct wined3d_resource resource;
    const struct wined3d_surface_ops *surface_ops;
    struct wined3d_texture *container;
#if defined(STAGING_CSMT)

    DWORD flags;

    UINT pow2Width;
    UINT pow2Height;

#else  /* STAGING_CSMT */
    void *user_memory;
    DWORD locations;

    DWORD flags;

    UINT pow2Width;
    UINT pow2Height;

    /* PBO */
    GLuint                    pbo;
#endif /* STAGING_CSMT */
    GLuint rb_multisample;
    GLuint rb_resolved;
    GLenum texture_target;
    unsigned int texture_level;
    unsigned int texture_layer;

    RECT                      lockedRect;
    int                       lockCount;

    /* For GetDC */
    struct wined3d_surface_dib dib;
    HDC                       hDC;

    struct list               renderbuffers;
    const struct wined3d_renderbuffer_entry *current_renderbuffer;
    SIZE ds_current_size;

    /* DirectDraw Overlay handling */
    RECT                      overlay_srcrect;
    RECT                      overlay_destrect;
    struct wined3d_surface *overlay_dest;
    struct list               overlays;
    struct list               overlay_entry;
};

static inline struct wined3d_surface *surface_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_surface, resource);
}

static inline GLuint surface_get_texture_name(const struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    return srgb && !gl_info->supported[EXT_TEXTURE_SRGB_DECODE]
            ? surface->container->texture_srgb.name : surface->container->texture_rgb.name;
}

void surface_set_dirty(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
HRESULT surface_color_fill(struct wined3d_surface *s,
        const RECT *rect, const struct wined3d_color *color) DECLSPEC_HIDDEN;
GLenum surface_get_gl_buffer(const struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void surface_get_drawable_size(const struct wined3d_surface *surface, const struct wined3d_context *context,
        unsigned int *width, unsigned int *height) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void surface_load(struct wined3d_surface *surface, struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void surface_load_ds_location(struct wined3d_surface *surface,
        struct wined3d_context *context, DWORD location) DECLSPEC_HIDDEN;
void surface_load_fb_texture(struct wined3d_surface *surface, BOOL srgb) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
void surface_invalidate_location(struct wined3d_surface *surface, DWORD location) DECLSPEC_HIDDEN;
void surface_load(struct wined3d_surface *surface, BOOL srgb) DECLSPEC_HIDDEN;
void surface_load_ds_location(struct wined3d_surface *surface,
        struct wined3d_context *context, DWORD location) DECLSPEC_HIDDEN;
void surface_load_fb_texture(struct wined3d_surface *surface, BOOL srgb) DECLSPEC_HIDDEN;
HRESULT surface_load_location(struct wined3d_surface *surface, DWORD location) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */
void surface_modify_ds_location(struct wined3d_surface *surface, DWORD location, UINT w, UINT h) DECLSPEC_HIDDEN;
void surface_prepare_rb(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL multisample) DECLSPEC_HIDDEN;
void surface_set_compatible_renderbuffer(struct wined3d_surface *surface,
        const struct wined3d_surface *rt) DECLSPEC_HIDDEN;
void surface_set_texture_target(struct wined3d_surface *surface, GLenum target, GLint level) DECLSPEC_HIDDEN;
void surface_translate_drawable_coords(const struct wined3d_surface *surface, HWND window, RECT *rect) DECLSPEC_HIDDEN;
HRESULT wined3d_surface_update_desc(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, void *mem, unsigned int pitch) DECLSPEC_HIDDEN;
HRESULT surface_upload_from_surface(struct wined3d_surface *dst_surface, const POINT *dst_point,
        struct wined3d_surface *src_surface, const RECT *src_rect) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
HRESULT wined3d_surface_create(struct wined3d_texture *container, const struct wined3d_resource_desc *desc,
        GLenum target, unsigned int level, unsigned int layer, DWORD flags,
        struct wined3d_surface **surface) DECLSPEC_HIDDEN;
void wined3d_surface_destroy(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_surface_upload_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, const RECT *src_rect, UINT src_pitch, const POINT *dst_point,
        BOOL srgb, const struct wined3d_const_bo_address *data) DECLSPEC_HIDDEN;
void surface_blt_ugly(struct wined3d_surface *dst_surface, const RECT *dst_rect_in,
        struct wined3d_surface *src_surface, const RECT *src_rect_in, DWORD flags,
        const WINEDDBLTFX *fx, enum wined3d_texture_filter_type filter) DECLSPEC_HIDDEN;
BOOL surface_check_block_align(struct wined3d_surface *surface, const RECT *rect) DECLSPEC_HIDDEN;
void wined3d_surface_cleanup_cs(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_surface_getdc_cs(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_surface_releasedc_cs(struct wined3d_surface *surface) DECLSPEC_HIDDEN;

void draw_textured_quad(const struct wined3d_surface *src_surface, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect, enum wined3d_texture_filter_type filter) DECLSPEC_HIDDEN;
void surface_flip(struct wined3d_surface *front, struct wined3d_surface *back) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
void surface_validate_location(struct wined3d_surface *surface, DWORD location) DECLSPEC_HIDDEN;
HRESULT wined3d_surface_create(struct wined3d_texture *container, const struct wined3d_resource_desc *desc,
        GLenum target, unsigned int level, unsigned int layer, DWORD flags,
        struct wined3d_surface **surface) DECLSPEC_HIDDEN;
void wined3d_surface_destroy(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void surface_prepare_map_memory(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_surface_upload_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, const RECT *src_rect, UINT src_pitch, const POINT *dst_point,
        BOOL srgb, const struct wined3d_const_bo_address *data) DECLSPEC_HIDDEN;

void draw_textured_quad(const struct wined3d_surface *src_surface, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect, enum wined3d_texture_filter_type filter) DECLSPEC_HIDDEN;
void flip_surface(struct wined3d_surface *front, struct wined3d_surface *back) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

/* Surface flags: */
#define SFLAG_DIBSECTION        0x00000001 /* Has a DIB section attached for GetDC. */
#define SFLAG_DISCARD           0x00000002 /* ??? */
#define SFLAG_NONPOW2           0x00000004 /* Surface sizes are not a power of 2 */
#define SFLAG_LOST              0x00000008 /* Surface lost flag for ddraw. */
#define SFLAG_CLIENT            0x00000010 /* GL_APPLE_client_storage is used with this surface. */
#define SFLAG_DCINUSE           0x00000020 /* Set between GetDC and ReleaseDC calls. */

struct wined3d_sampler
{
    struct wine_rb_entry entry;
    LONG refcount;
    struct wined3d_device *device;
    void *parent;
    struct wined3d_sampler_desc desc;
    GLuint name;
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
    UINT element_count;

    BOOL position_transformed;
    BOOL half_float_conv_needed;
};

#if defined(STAGING_CSMT)
void wined3d_vertex_declaration_destroy(struct wined3d_vertex_declaration *declaration) DECLSPEC_HIDDEN;

#endif /* STAGING_CSMT */
struct wined3d_saved_states
{
    DWORD transform[(HIGHEST_TRANSFORMSTATE >> 5) + 1];
    WORD streamSource;                          /* MAX_STREAMS, 16 */
    WORD streamFreq;                            /* MAX_STREAMS, 16 */
    DWORD renderState[(WINEHIGHEST_RENDER_STATE >> 5) + 1];
    DWORD textureState[MAX_TEXTURES];           /* WINED3D_HIGHEST_TEXTURE_STATE + 1, 18 */
    WORD samplerState[MAX_COMBINED_SAMPLERS];   /* WINED3D_HIGHEST_SAMPLER_STATE + 1, 14 */
    DWORD clipplane;                            /* WINED3DMAXUSERCLIPPLANES, 32 */
    WORD pixelShaderConstantsB;                 /* MAX_CONST_B, 16 */
    WORD pixelShaderConstantsI;                 /* MAX_CONST_I, 16 */
    BOOL *pixelShaderConstantsF;
    WORD vertexShaderConstantsB;                /* MAX_CONST_B, 16 */
    WORD vertexShaderConstantsI;                /* MAX_CONST_I, 16 */
    BOOL *vertexShaderConstantsF;
    DWORD textures : 20;                        /* MAX_COMBINED_SAMPLERS, 20 */
    DWORD primitive_type : 1;
    DWORD indices : 1;
    DWORD material : 1;
    DWORD viewport : 1;
    DWORD vertexDecl : 1;
    DWORD pixelShader : 1;
    DWORD vertexShader : 1;
    DWORD scissorRect : 1;
    DWORD padding : 4;
};

struct StageState {
    DWORD stage;
    DWORD state;
};

struct wined3d_stateblock
{
    LONG                      ref;     /* Note: Ref counting not required */
    struct wined3d_device *device;

    /* Array indicating whether things have been set or changed */
    struct wined3d_saved_states changed;
    struct wined3d_state state;

    /* Contained state management */
    DWORD                     contained_render_states[WINEHIGHEST_RENDER_STATE + 1];
    unsigned int              num_contained_render_states;
    DWORD                     contained_transform_states[HIGHEST_TRANSFORMSTATE + 1];
    unsigned int              num_contained_transform_states;
    DWORD                     contained_vs_consts_i[MAX_CONST_I];
    unsigned int              num_contained_vs_consts_i;
    DWORD                     contained_vs_consts_b[MAX_CONST_B];
    unsigned int              num_contained_vs_consts_b;
    DWORD                     *contained_vs_consts_f;
    unsigned int              num_contained_vs_consts_f;
    DWORD                     contained_ps_consts_i[MAX_CONST_I];
    unsigned int              num_contained_ps_consts_i;
    DWORD                     contained_ps_consts_b[MAX_CONST_B];
    unsigned int              num_contained_ps_consts_b;
    DWORD                     *contained_ps_consts_f;
    unsigned int              num_contained_ps_consts_f;
    struct StageState         contained_tss_states[MAX_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE + 1)];
    unsigned int              num_contained_tss_states;
    struct StageState         contained_sampler_states[MAX_COMBINED_SAMPLERS * WINED3D_HIGHEST_SAMPLER_STATE];
    unsigned int              num_contained_sampler_states;
};

void stateblock_init_contained_states(struct wined3d_stateblock *stateblock) DECLSPEC_HIDDEN;

void state_cleanup(struct wined3d_state *state) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
HRESULT state_init(struct wined3d_state *state, const struct wined3d_gl_info *gl_info,
        const struct wined3d_d3d_info *d3d_info, DWORD flags) DECLSPEC_HIDDEN;
void state_unbind_resources(struct wined3d_state *state) DECLSPEC_HIDDEN;

struct wined3d_cs_list
{
    struct list blocks;
};

#define WINED3D_CS_QUEUE_SIZE 0x100000
#define WINED3D_CS_SPIN_COUNT 10000000

struct wined3d_cs_queue
{
    LONG head, tail;
    BYTE data[WINED3D_CS_QUEUE_SIZE];
};

struct wined3d_cs_ops
{
    void *(*require_space)(struct wined3d_cs *cs, size_t size);
    void *(*require_space_prio)(struct wined3d_cs *cs, size_t size);
    void (*submit)(struct wined3d_cs *cs, size_t size);
    void (*submit_prio)(struct wined3d_cs *cs, size_t size);
    void (*finish)(struct wined3d_cs *cs);
    void (*finish_prio)(struct wined3d_cs *cs);
};

struct wined3d_cs
{
    const struct wined3d_cs_ops *ops;
    struct wined3d_device *device;
    struct wined3d_state state;
    HANDLE thread;
    DWORD thread_id;
    struct wined3d_surface *onscreen_depth_stencil;

    struct wined3d_cs_queue queue, prio_queue;

    LONG pending_presents;
    struct list query_poll_list;

    HANDLE event;
    BOOL waiting_for_event;
};

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device) DECLSPEC_HIDDEN;
void wined3d_cs_destroy(struct wined3d_cs *cs) DECLSPEC_HIDDEN;
void wined3d_cs_switch_onscreen_ds(struct wined3d_cs *cs, struct wined3d_context *context,
        struct wined3d_surface *depth_stencil) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
HRESULT state_init(struct wined3d_state *state, struct wined3d_fb_state *fb,
        const struct wined3d_gl_info *gl_info, const struct wined3d_d3d_info *d3d_info,
        DWORD flags) DECLSPEC_HIDDEN;
void state_unbind_resources(struct wined3d_state *state) DECLSPEC_HIDDEN;

struct wined3d_cs_ops
{
    void *(*require_space)(struct wined3d_cs *cs, size_t size);
    void (*submit)(struct wined3d_cs *cs);
};

struct wined3d_cs
{
    const struct wined3d_cs_ops *ops;
    struct wined3d_device *device;
    struct wined3d_fb_state fb;
    struct wined3d_state state;

    size_t data_size;
    void *data;
};

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device) DECLSPEC_HIDDEN;
void wined3d_cs_destroy(struct wined3d_cs *cs) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil) DECLSPEC_HIDDEN;
void wined3d_cs_emit_draw(struct wined3d_cs *cs, UINT start_idx, UINT index_count,
        UINT start_instance, UINT instance_count, BOOL indexed) DECLSPEC_HIDDEN;
void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags) DECLSPEC_HIDDEN;
void wined3d_cs_emit_reset_state(struct wined3d_cs *cs) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_clip_plane(struct wined3d_cs *cs, UINT plane_idx,
        const struct wined3d_vec4 *plane) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_color_key(struct wined3d_cs *cs, struct wined3d_texture *texture,
        WORD flags, const struct wined3d_color_key *color_key) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_constant_buffer(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT cb_idx, struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_depth_stencil_view(struct wined3d_cs *cs,
        struct wined3d_rendertarget_view *view) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_index_buffer(struct wined3d_cs *cs, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_material(struct wined3d_cs *cs, const struct wined3d_material *material) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_predication(struct wined3d_cs *cs,
        struct wined3d_query *predicate, BOOL value) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_render_state(struct wined3d_cs *cs,
        enum wined3d_render_state state, DWORD value) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_rendertarget_view(struct wined3d_cs *cs, unsigned int view_idx,
        struct wined3d_rendertarget_view *view) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_shader_resource_view(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT view_idx, struct wined3d_shader_resource_view *view) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_sampler(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT sampler_idx, struct wined3d_sampler *sampler) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_sampler_state(struct wined3d_cs *cs, UINT sampler_idx,
        enum wined3d_sampler_state state, DWORD value) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_scissor_rect(struct wined3d_cs *cs, const RECT *rect) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_shader(struct wined3d_cs *cs, enum wined3d_shader_type type,
        struct wined3d_shader *shader) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_stream_output(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_stream_source(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset, UINT stride) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_stream_source_freq(struct wined3d_cs *cs, UINT stream_idx,
        UINT frequency, UINT flags) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_texture(struct wined3d_cs *cs, UINT stage, struct wined3d_texture *texture) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_texture_state(struct wined3d_cs *cs, UINT stage,
        enum wined3d_texture_stage_state state, DWORD value) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_transform(struct wined3d_cs *cs, enum wined3d_transform_state state,
        const struct wined3d_matrix *matrix) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_vertex_declaration(struct wined3d_cs *cs,
        struct wined3d_vertex_declaration *declaration) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_viewport(struct wined3d_cs *cs, const struct wined3d_viewport *viewport) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void wined3d_cs_emit_set_consts_f(struct wined3d_cs *cs, UINT start_register, const float *constants,
        UINT vector4f_count, enum wined3d_shader_type type) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_consts_b(struct wined3d_cs *cs, UINT start_register,
        const BOOL *constants, UINT bool_count, enum wined3d_shader_type type) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_consts_i(struct wined3d_cs *cs, UINT start_register,
        const int *constants, UINT vector4i_count, enum wined3d_shader_type type) DECLSPEC_HIDDEN;
void wined3d_cs_emit_glfinish(struct wined3d_cs *cs) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_base_vertex_index(struct wined3d_cs *cs,
        UINT base_vertex_index) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_primitive_type(struct wined3d_cs *cs,
        GLenum primitive_type) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_light(struct wined3d_cs *cs, const struct wined3d_light_info *light) DECLSPEC_HIDDEN;
void wined3d_cs_emit_set_light_enable(struct wined3d_cs *cs, UINT idx, BOOL enable) DECLSPEC_HIDDEN;
void wined3d_cs_emit_blt(struct wined3d_cs *cs, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, struct wined3d_surface *src_surface,
        const RECT *src_rect, DWORD flags, const WINEDDBLTFX *fx,
        enum wined3d_texture_filter_type filter) DECLSPEC_HIDDEN;
void wined3d_cs_emit_clear_rtv(struct wined3d_cs *cs, struct wined3d_rendertarget_view *view,
        const RECT *rect, const struct wined3d_color *color) DECLSPEC_HIDDEN;
void wined3d_cs_emit_resource_changed(struct wined3d_cs *cs, struct wined3d_resource *resource,
        struct wined3d_gl_bo *swap_buffer, void *swap_heap_memory) DECLSPEC_HIDDEN;
void *wined3d_cs_emit_resource_map(struct wined3d_cs *cs, struct wined3d_resource *resource,
        DWORD flags) DECLSPEC_HIDDEN;
void wined3d_cs_emit_resource_unmap(struct wined3d_cs *cs, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_cs_emit_query_issue(struct wined3d_cs *cs, struct wined3d_query *query,
        DWORD flags) DECLSPEC_HIDDEN;
void wined3d_cs_emit_query_get_data(struct wined3d_cs *cs, struct wined3d_query *query, void *data,
        UINT data_size, DWORD flags, HRESULT *ret) DECLSPEC_HIDDEN;
void wined3d_cs_emit_query_destroy(struct wined3d_cs *cs, struct wined3d_query *query) DECLSPEC_HIDDEN;
void wined3d_cs_emit_update_surface(struct wined3d_cs *cs, struct wined3d_surface *src,
        const RECT *src_rect, struct wined3d_surface *dst, const POINT *dst_point) DECLSPEC_HIDDEN;
void wined3d_cs_emit_texture_preload(struct wined3d_cs *cs, struct wined3d_texture *texture) DECLSPEC_HIDDEN;
void wined3d_cs_emit_surface_preload(struct wined3d_cs *cs, struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_cs_emit_update_texture(struct wined3d_cs *cs, struct wined3d_texture *src,
        struct wined3d_texture *dst) DECLSPEC_HIDDEN;
void wined3d_cs_emit_evict_resource(struct wined3d_cs *cs, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_cs_emit_buffer_swap_mem(struct wined3d_cs *cs, struct wined3d_buffer *buffer,
        BYTE *mem) DECLSPEC_HIDDEN;
void wined3d_cs_emit_buffer_invalidate_bo_range(struct wined3d_cs *cs,
        struct wined3d_buffer *buffer, UINT offset, UINT size) DECLSPEC_HIDDEN;
void wined3d_cs_emit_buffer_preload(struct wined3d_cs *cs, struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
void wined3d_cs_emit_view_destroy(struct wined3d_cs *cs, struct wined3d_rendertarget_view *view) DECLSPEC_HIDDEN;
void wined3d_cs_emit_vertex_declaration_destroy(struct wined3d_cs *cs,
        struct wined3d_vertex_declaration *declaration) DECLSPEC_HIDDEN;
void wined3d_cs_emit_shader_cleanup(struct wined3d_cs *cs, struct wined3d_shader *shader) DECLSPEC_HIDDEN;
void wined3d_cs_emit_create_vbo(struct wined3d_cs *cs, struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
void wined3d_cs_emit_resource_cleanup(struct wined3d_cs *cs,
        struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void wined3d_cs_emit_buffer_cleanup(struct wined3d_cs *cs, struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
void wined3d_cs_emit_volume_cleanup(struct wined3d_cs *cs, struct wined3d_volume *volume) DECLSPEC_HIDDEN;
void wined3d_cs_emit_surface_cleanup(struct wined3d_cs *cs, struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_cs_emit_texture_cleanup(struct wined3d_cs *cs, struct wined3d_texture *texture) DECLSPEC_HIDDEN;
void wined3d_cs_emit_create_dummy_textures(struct wined3d_cs *cs) DECLSPEC_HIDDEN;
HRESULT wined3d_cs_emit_create_swapchain_context(struct wined3d_cs *cs,
        struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void wined3d_cs_emit_delete_opengl_contexts(struct wined3d_cs *cs,
        struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void wined3d_cs_emit_getdc(struct wined3d_cs *cs, struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void wined3d_cs_emit_releasedc(struct wined3d_cs *cs, struct wined3d_surface *surface) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

/* Direct3D terminology with little modifications. We do not have an issued state
 * because only the driver knows about it, but we have a created state because d3d
 * allows GetData on a created issue, but opengl doesn't
 */
enum query_state {
    QUERY_CREATED,
    QUERY_SIGNALLED,
    QUERY_BUILDING
};

struct wined3d_query_ops
{
    HRESULT (*query_get_data)(struct wined3d_query *query, void *data, DWORD data_size, DWORD flags);
#if defined(STAGING_CSMT)
    BOOL (*query_poll)(struct wined3d_query *query);
    BOOL (*query_issue)(struct wined3d_query *query, DWORD flags);
#else  /* STAGING_CSMT */
    HRESULT (*query_issue)(struct wined3d_query *query, DWORD flags);
#endif /* STAGING_CSMT */
};

struct wined3d_query
{
    LONG ref;

    void *parent;
    const struct wined3d_query_ops *query_ops;
    struct wined3d_device *device;
    enum query_state         state;
    enum wined3d_query_type type;
    DWORD data_size;
    void                     *extendedData;
#if defined(STAGING_CSMT)

    LONG                     counter_main, counter_retrieved;
    struct list              poll_list_entry;
};

void wined3d_query_destroy(struct wined3d_query *query) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
};
#endif /* STAGING_CSMT */

/* TODO: Add tests and support for FLOAT16_4 POSITIONT, D3DCOLOR position, other
 * fixed function semantics as D3DCOLOR or FLOAT16 */
enum wined3d_buffer_conversion_type
{
    CONV_NONE,
    CONV_D3DCOLOR,
    CONV_POSITIONT,
};

struct wined3d_map_range
{
    UINT offset;
    UINT size;
};

struct wined3d_buffer
{
    struct wined3d_resource resource;

    struct wined3d_buffer_desc desc;

    GLuint buffer_object;
    GLenum buffer_object_usage;
    GLenum buffer_type_hint;
    DWORD flags;
#if defined(STAGING_CSMT)
    BOOL ignore_discard;
#endif /* STAGING_CSMT */
    void *map_ptr;

    struct wined3d_map_range *maps;
    ULONG maps_size, modified_areas;
    struct wined3d_event_query *query;

    /* conversion stuff */
    UINT decl_change_count, full_conversion_count;
    UINT draw_count;
    UINT stride;                                            /* 0 if no conversion */
    UINT conversion_stride;                                 /* 0 if no shifted conversion */
    enum wined3d_buffer_conversion_type *conversion_map;    /* NULL if no conversion */
};

static inline struct wined3d_buffer *buffer_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_buffer, resource);
}

void buffer_get_memory(struct wined3d_buffer *buffer, struct wined3d_context *context,
        struct wined3d_bo_address *data) DECLSPEC_HIDDEN;
BYTE *buffer_get_sysmem(struct wined3d_buffer *This, struct wined3d_context *context) DECLSPEC_HIDDEN;
void buffer_internal_preload(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct wined3d_state *state) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void buffer_invalidate_bo_range(struct wined3d_buffer *This, UINT offset, UINT size) DECLSPEC_HIDDEN;
void buffer_swap_mem(struct wined3d_buffer *buffer, BYTE *mem) DECLSPEC_HIDDEN;
void buffer_create_buffer_object(struct wined3d_buffer *This,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void wined3d_buffer_cleanup_cs(struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
#else  /* STAGING_CSMT */
void buffer_mark_used(struct wined3d_buffer *buffer) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

struct wined3d_rendertarget_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;

    const struct wined3d_format *format;
    unsigned int format_flags;
    unsigned int sub_resource_idx;
    unsigned int buffer_offset;

    unsigned int width;
    unsigned int height;
    unsigned int depth;
};

static inline struct wined3d_surface *wined3d_rendertarget_view_get_surface(
        const struct wined3d_rendertarget_view *view)
{
    struct wined3d_resource *resource;
    struct wined3d_texture *texture;

    if (!view)
        return NULL;

    if (view->resource->type != WINED3D_RTYPE_TEXTURE && view->resource->type != WINED3D_RTYPE_CUBE_TEXTURE)
        return NULL;

    texture = wined3d_texture_from_resource(view->resource);
    if (!(resource = wined3d_texture_get_sub_resource(texture, view->sub_resource_idx)))
        return NULL;

    return surface_from_resource(resource);
}

#if defined(STAGING_CSMT)
void wined3d_rendertarget_view_destroy(struct wined3d_rendertarget_view *view) DECLSPEC_HIDDEN;

#endif /* STAGING_CSMT */
struct wined3d_shader_resource_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;
};

struct wined3d_swapchain_ops
{
    void (*swapchain_present)(struct wined3d_swapchain *swapchain, const RECT *src_rect,
#if defined(STAGING_CSMT)
            const RECT *dst_rect, const RGNDATA *dirty_region, DWORD flags,
            struct wined3d_surface *depth_stencil);
#else  /* STAGING_CSMT */
            const RECT *dst_rect, const RGNDATA *dirty_region, DWORD flags);
#endif /* STAGING_CSMT */
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
    struct wined3d_swapchain_desc desc;
    struct wined3d_display_mode original_mode, d3d_mode;
    struct wined3d_gamma_ramp orig_gamma;
    BOOL render_to_fbo, reapply_mode;
    const struct wined3d_format *ds_format;
    struct wined3d_palette *palette;

    LONG prev_time, frames;   /* Performance tracking */

    struct wined3d_context **context;
    unsigned int num_contexts;

    HWND win_handle;
    HWND device_window;

    HDC backup_dc;
    HWND backup_wnd;
};

void x11_copy_to_screen(const struct wined3d_swapchain *swapchain, const RECT *rect) DECLSPEC_HIDDEN;

void wined3d_swapchain_activate(struct wined3d_swapchain *swapchain, BOOL activate) DECLSPEC_HIDDEN;
struct wined3d_context *swapchain_get_context(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_destroy_contexts(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
HDC swapchain_get_backup_dc(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_update_draw_bindings(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_update_render_to_fbo(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
HRESULT swapchain_create_context_cs(struct wined3d_device *device,
        struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

/*****************************************************************************
 * Utility function prototypes
 */

/* Trace routines */
const char *debug_d3dformat(enum wined3d_format_id format_id) DECLSPEC_HIDDEN;
const char *debug_d3ddevicetype(enum wined3d_device_type device_type) DECLSPEC_HIDDEN;
const char *debug_d3dresourcetype(enum wined3d_resource_type resource_type) DECLSPEC_HIDDEN;
const char *debug_d3dusage(DWORD usage) DECLSPEC_HIDDEN;
const char *debug_d3dusagequery(DWORD usagequery) DECLSPEC_HIDDEN;
const char *debug_d3ddeclmethod(enum wined3d_decl_method method) DECLSPEC_HIDDEN;
const char *debug_d3ddeclusage(enum wined3d_decl_usage usage) DECLSPEC_HIDDEN;
const char *debug_d3dinput_classification(enum wined3d_input_classification classification) DECLSPEC_HIDDEN;
const char *debug_d3dprimitivetype(enum wined3d_primitive_type primitive_type) DECLSPEC_HIDDEN;
const char *debug_d3drenderstate(enum wined3d_render_state state) DECLSPEC_HIDDEN;
const char *debug_d3dsamplerstate(enum wined3d_sampler_state state) DECLSPEC_HIDDEN;
const char *debug_d3dstate(DWORD state) DECLSPEC_HIDDEN;
const char *debug_d3dtexturefiltertype(enum wined3d_texture_filter_type filter_type) DECLSPEC_HIDDEN;
const char *debug_d3dtexturestate(enum wined3d_texture_stage_state state) DECLSPEC_HIDDEN;
const char *debug_d3dtstype(enum wined3d_transform_state tstype) DECLSPEC_HIDDEN;
const char *debug_d3dpool(enum wined3d_pool pool) DECLSPEC_HIDDEN;
const char *debug_fbostatus(GLenum status) DECLSPEC_HIDDEN;
const char *debug_glerror(GLenum error) DECLSPEC_HIDDEN;
const char *debug_d3dtop(enum wined3d_texture_op d3dtop) DECLSPEC_HIDDEN;
void dump_color_fixup_desc(struct color_fixup_desc fixup) DECLSPEC_HIDDEN;

BOOL is_invalid_op(const struct wined3d_state *state, int stage,
        enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3) DECLSPEC_HIDDEN;
void set_tex_op_nvrc(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state,
        BOOL is_alpha, int stage, enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3,
        INT texture_idx, DWORD dst) DECLSPEC_HIDDEN;
void texture_activate_dimensions(const struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
void sampler_texdim(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void tex_alphaop(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void apply_pixelshader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_alpha_test(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogcolor(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogdensity(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogstartend(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fog_fragpart(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_nop(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_srgbwrite(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;

void state_clipping(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void clipplane(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_pointsprite_w(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_pointsprite(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;

BOOL getColorBits(const struct wined3d_format *format,
        BYTE *redSize, BYTE *greenSize, BYTE *blueSize, BYTE *alphaSize, BYTE *totalSize) DECLSPEC_HIDDEN;
BOOL getDepthStencilBits(const struct wined3d_format *format,
        BYTE *depthSize, BYTE *stencilSize) DECLSPEC_HIDDEN;
GLenum gl_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type) DECLSPEC_HIDDEN;

/* Math utils */
void multiply_matrix(struct wined3d_matrix *dest, const struct wined3d_matrix *src1,
        const struct wined3d_matrix *src2) DECLSPEC_HIDDEN;
unsigned int count_bits(unsigned int mask) DECLSPEC_HIDDEN;

void wined3d_release_dc(HWND window, HDC dc) DECLSPEC_HIDDEN;

struct wined3d_shader_lconst
{
    struct list entry;
    unsigned int idx;
    DWORD value[4];
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

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

struct wined3d_string_buffer_list
{
    struct list list;
};

struct wined3d_string_buffer *string_buffer_get(struct wined3d_string_buffer_list *list) DECLSPEC_HIDDEN;
void string_buffer_sprintf(struct wined3d_string_buffer *buffer, const char *format, ...) PRINTF_ATTR(2, 3) DECLSPEC_HIDDEN;
void string_buffer_release(struct wined3d_string_buffer_list *list, struct wined3d_string_buffer *buffer) DECLSPEC_HIDDEN;
void string_buffer_list_init(struct wined3d_string_buffer_list *list) DECLSPEC_HIDDEN;
void string_buffer_list_cleanup(struct wined3d_string_buffer_list *list) DECLSPEC_HIDDEN;

int shader_addline(struct wined3d_string_buffer *buffer, const char *fmt, ...) PRINTF_ATTR(2,3) DECLSPEC_HIDDEN;
BOOL string_buffer_resize(struct wined3d_string_buffer *buffer, int rc) DECLSPEC_HIDDEN;
int shader_vaddline(struct wined3d_string_buffer *buffer, const char *fmt, va_list args) DECLSPEC_HIDDEN;

/* Vertex shader utility functions */
BOOL vshader_get_input(const struct wined3d_shader *shader,
        BYTE usage_req, BYTE usage_idx_req, unsigned int *regnum) DECLSPEC_HIDDEN;

struct wined3d_vertex_shader
{
    struct wined3d_shader_attribute attributes[MAX_ATTRIBS];
};

struct wined3d_geometry_shader
{
    enum wined3d_primitive_type input_type;
    enum wined3d_primitive_type output_type;
    UINT vertices_out;
};

struct wined3d_pixel_shader
{
    /* Pixel shader input semantics */
    DWORD input_reg_map[MAX_REG_INPUT];
    BOOL input_reg_used[MAX_REG_INPUT];
    unsigned int declared_in_count;

    /* Some information about the shader behavior */
    BOOL color0_mov;
    DWORD color0_reg;
};

struct wined3d_shader
{
    LONG ref;
    const struct wined3d_shader_limits *limits;
    DWORD *function;
    UINT functionLength;
    BOOL load_local_constsF;
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
    char *signature_strings;

    /* Pointer to the parent device */
    struct wined3d_device *device;
    struct list shader_list_entry;

    union
    {
        struct wined3d_vertex_shader vs;
        struct wined3d_geometry_shader gs;
        struct wined3d_pixel_shader ps;
    } u;
};

void pixelshader_update_resource_types(struct wined3d_shader *shader, WORD tex_types) DECLSPEC_HIDDEN;
void find_ps_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        BOOL position_transformed, struct ps_compile_args *args,
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

void find_vs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        WORD swizzle_map, struct vs_compile_args *args) DECLSPEC_HIDDEN;

void string_buffer_clear(struct wined3d_string_buffer *buffer) DECLSPEC_HIDDEN;
BOOL string_buffer_init(struct wined3d_string_buffer *buffer) DECLSPEC_HIDDEN;
void string_buffer_free(struct wined3d_string_buffer *buffer) DECLSPEC_HIDDEN;
void shader_dump_src_param(const struct wined3d_shader_src_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
void shader_dump_dst_param(const struct wined3d_shader_dst_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps,
        unsigned int max) DECLSPEC_HIDDEN;
void shader_generate_main(const struct wined3d_shader *shader, struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, const DWORD *byte_code, void *backend_ctx) DECLSPEC_HIDDEN;
BOOL shader_match_semantic(const char *semantic_name, enum wined3d_decl_usage usage) DECLSPEC_HIDDEN;
#if defined(STAGING_CSMT)
void shader_cleanup(struct wined3d_shader *shader) DECLSPEC_HIDDEN;
#endif /* STAGING_CSMT */

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

        case WINED3DSPR_DEPTHOUT:   /* oDepth */
        case WINED3DSPR_CONSTBOOL:  /* b# */
        case WINED3DSPR_LOOP:       /* aL */
        case WINED3DSPR_PREDICATE:  /* p0 */
        case WINED3DSPR_PRIMID:     /* primID */
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
        const struct wined3d_state *state, float *position_fixup)
{
    float center_offset;

    if (context->swapchain->device->wined3d->flags & WINED3D_PIXEL_CENTER_INTEGER)
        center_offset = 63.0f / 64.0f;
    else
        center_offset = -1.0f / 64.0f;

    position_fixup[0] = 1.0f;
    position_fixup[1] = 1.0f;
    position_fixup[2] = center_offset / state->viewport.width;
    position_fixup[3] = -center_offset / state->viewport.height;

    if (context->render_offscreen)
    {
        position_fixup[1] *= -1.0f;
        position_fixup[3] *= -1.0f;
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

void get_identity_matrix(struct wined3d_matrix *mat) DECLSPEC_HIDDEN;
void get_modelview_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int index, struct wined3d_matrix *mat) DECLSPEC_HIDDEN;
void get_projection_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        struct wined3d_matrix *mat) DECLSPEC_HIDDEN;
void get_texture_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int tex, struct wined3d_matrix *mat) DECLSPEC_HIDDEN;
void get_pointsize_minmax(const struct wined3d_context *context, const struct wined3d_state *state,
        float *out_min, float *out_max) DECLSPEC_HIDDEN;
void get_pointsize(const struct wined3d_context *context, const struct wined3d_state *state,
        float *out_pointsize, float *out_att) DECLSPEC_HIDDEN;
void get_fog_start_end(const struct wined3d_context *context, const struct wined3d_state *state,
        float *start, float *end) DECLSPEC_HIDDEN;

/* Using additional shader constants (uniforms in GLSL / program environment
 * or local parameters in ARB) is costly:
 * ARB only knows float4 parameters and GLSL compiler are not really smart
 * when it comes to efficiently pack float2 uniforms, so no space is wasted
 * (in fact most compilers map a float2 to a full float4 uniform).
 *
 * For NP2 texcoord fixup we only need 2 floats (width and height) for each
 * 2D texture used in the shader. We therefore pack fixup info for 2 textures
 * into a single shader constant (uniform / program parameter).
 *
 * This structure is shared between the GLSL and the ARB backend.*/
struct ps_np2fixup_info {
    unsigned char     idx[MAX_FRAGMENT_SAMPLERS]; /* indices to the real constant */
    WORD              active; /* bitfield indicating if we can apply the fixup */
    WORD              num_consts;
};

struct wined3d_palette
{
    LONG ref;
    struct wined3d_device *device;

    unsigned int size;
    RGBQUAD colors[256];
    DWORD flags;
};

/* DirectDraw utility functions */
extern enum wined3d_format_id pixelformat_for_depth(DWORD depth) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Pixel format management
 */

/* WineD3D pixel format flags */
#define WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING    0x00000001
#define WINED3DFMT_FLAG_FILTERING                   0x00000002
#define WINED3DFMT_FLAG_DEPTH                       0x00000004
#define WINED3DFMT_FLAG_STENCIL                     0x00000008
#define WINED3DFMT_FLAG_RENDERTARGET                0x00000010
#define WINED3DFMT_FLAG_FBO_ATTACHABLE              0x00000040
#define WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB         0x00000080
#define WINED3DFMT_FLAG_GETDC                       0x00000100
#define WINED3DFMT_FLAG_FLOAT                       0x00000200
#define WINED3DFMT_FLAG_BUMPMAP                     0x00000400
#define WINED3DFMT_FLAG_SRGB_READ                   0x00000800
#define WINED3DFMT_FLAG_SRGB_WRITE                  0x00001000
#define WINED3DFMT_FLAG_VTF                         0x00002000
#define WINED3DFMT_FLAG_SHADOW                      0x00004000
#define WINED3DFMT_FLAG_COMPRESSED                  0x00008000
#define WINED3DFMT_FLAG_BROKEN_PITCH                0x00010000
#define WINED3DFMT_FLAG_BLOCKS                      0x00020000
#define WINED3DFMT_FLAG_HEIGHT_SCALE                0x00040000
#define WINED3DFMT_FLAG_TEXTURE                     0x00080000
#define WINED3DFMT_FLAG_BLOCKS_NO_VERIFY            0x00100000

struct wined3d_rational
{
    UINT numerator;
    UINT denominator;
};

struct wined3d_color_key_conversion
{
    enum wined3d_format_id dst_format;
    void (*convert)(const BYTE *src, unsigned int src_pitch, BYTE *dst, unsigned int dst_pitch, unsigned int width,
            unsigned int height, const struct wined3d_palette *palette, const struct wined3d_color_key *color_key);
};

struct wined3d_format
{
    enum wined3d_format_id id;

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
    GLint component_count;
    GLenum gl_vtx_type;
    GLint gl_vtx_format;
    GLboolean gl_normalized;
    unsigned int component_size;

    GLint glInternal;
    GLint glGammaInternal;
    GLint rtInternal;
    GLint glFormat;
    GLint glType;
    UINT  conv_byte_count;
    unsigned int flags[WINED3D_GL_RES_TYPE_COUNT];
    struct wined3d_rational height_scale;
    struct color_fixup_desc color_fixup;
    void (*convert)(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
            UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth);
};

const struct wined3d_format *wined3d_get_format(const struct wined3d_gl_info *gl_info,
        enum wined3d_format_id format_id) DECLSPEC_HIDDEN;
UINT wined3d_format_calculate_pitch(const struct wined3d_format *format, UINT width) DECLSPEC_HIDDEN;
UINT wined3d_format_calculate_size(const struct wined3d_format *format,
        UINT alignment, UINT width, UINT height, UINT depth) DECLSPEC_HIDDEN;
DWORD wined3d_format_convert_from_float(const struct wined3d_surface *surface,
        const struct wined3d_color *color) DECLSPEC_HIDDEN;
BOOL wined3d_format_convert_color_to_float(const struct wined3d_format *format,
        const struct wined3d_palette *palette, DWORD color, struct wined3d_color *float_color) DECLSPEC_HIDDEN;
const struct wined3d_color_key_conversion * wined3d_format_get_color_key_conversion(
        const struct wined3d_texture *texture, BOOL need_alpha_ck) DECLSPEC_HIDDEN;

static inline BOOL use_vs(const struct wined3d_state *state)
{
    /* Check state->vertex_declaration to allow this to be used before the
     * stream info is validated, for example in device_update_tex_unit_map(). */
    return state->shader[WINED3D_SHADER_TYPE_VERTEX] && !state->vertex_declaration->position_transformed;
}

static inline BOOL use_ps(const struct wined3d_state *state)
{
    return !!state->shader[WINED3D_SHADER_TYPE_PIXEL];
}

static inline void context_apply_state(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct StateEntry *state_table = context->state_table;
    DWORD rep = state_table[state_id].representative;
    state_table[rep].apply(context, state, rep);
}

BOOL wined3d_dxtn_init(void) DECLSPEC_HIDDEN;
void wined3d_dxtn_free(void) DECLSPEC_HIDDEN;

/* The WNDCLASS-Name for the fake window which we use to retrieve the GL capabilities */
#define WINED3D_OPENGL_WINDOW_CLASS_NAME "WineD3D_OpenGL"

#define MAKEDWORD_VERSION(maj, min) (((maj & 0xffff) << 16) | (min & 0xffff))

#endif
