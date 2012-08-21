/*
 * Direct3D wine internal private include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003, 2004 Jason Edmeades
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

#ifndef __WINE_WINED3D_PRIVATE_H
#define __WINE_WINED3D_PRIVATE_H

#include <stdarg.h>
#include <math.h>
#include <limits.h>
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "objbase.h"
#include "wine/wined3d.h"
#include "wined3d_gl.h"
#include "wine/list.h"
#include "wine/rbtree.h"

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
};

#include <pshpack2.h>
struct color_fixup_desc
{
    unsigned x_sign_fixup : 1;
    unsigned x_source : 3;
    unsigned y_sign_fixup : 1;
    unsigned y_source : 3;
    unsigned z_sign_fixup : 1;
    unsigned z_source : 3;
    unsigned w_sign_fixup : 1;
    unsigned w_source : 3;
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
#define MAX_PALETTES            65536
#define MAX_STREAMS             16
#define MAX_TEXTURES            8
#define MAX_FRAGMENT_SAMPLERS   16
#define MAX_VERTEX_SAMPLERS     4
#define MAX_COMBINED_SAMPLERS   (MAX_FRAGMENT_SAMPLERS + MAX_VERTEX_SAMPLERS)
#define MAX_ACTIVE_LIGHTS       8
#define MAX_CLIPPLANES          WINED3DMAXUSERCLIPPLANES

struct min_lookup
{
    GLenum mip[WINED3D_TEXF_LINEAR + 1];
};

extern const struct min_lookup minMipLookup[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;
extern const struct min_lookup minMipLookup_noFilter[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;
extern const struct min_lookup minMipLookup_noMip[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;
extern const GLenum magLookup[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;
extern const GLenum magLookup_noFilter[WINED3D_TEXF_LINEAR + 1] DECLSPEC_HIDDEN;

static inline GLenum wined3d_gl_mag_filter(const GLenum mag_lookup[], enum wined3d_texture_filter_type mag_filter)
{
    return mag_lookup[mag_filter];
}

static inline GLenum wined3d_gl_min_mip_filter(const struct min_lookup min_mip_lookup[],
        enum wined3d_texture_filter_type min_filter, enum wined3d_texture_filter_type mip_filter)
{
    return min_mip_lookup[min_filter].mip[mip_filter];
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
static inline float float_16_to_32(const unsigned short *in) {
    const unsigned short s = ((*in) & 0x8000);
    const unsigned short e = ((*in) & 0x7C00) >> 10;
    const unsigned short m = (*in) & 0x3FF;
    const float sgn = (s ? -1.0f : 1.0f);

    if(e == 0) {
        if(m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -14.0f) * ((float)m / 1024.0f);
    } else if(e < 31) {
        return sgn * powf(2, (float)e - 15.0f) * (1.0f + ((float)m / 1024.0f));
    } else {
        if(m == 0) return sgn * INFINITY; /* +INF / -INF */
        else return NAN; /* NAN */
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
        if (m == 0) return sgn * INFINITY; /* +INF / -INF */
        else return NAN; /* NAN */
    }
}

/**
 * Settings
 */
#define VS_NONE    0
#define VS_HW      1

#define PS_NONE    0
#define PS_HW      1

#define VBO_NONE   0
#define VBO_HW     1

#define ORM_BACKBUFFER  0
#define ORM_FBO         1

#define SHADER_ARB  1
#define SHADER_GLSL 2
#define SHADER_ATI  3
#define SHADER_NONE 4

#define RTL_READDRAW   1
#define RTL_READTEX    2

#define PCI_VENDOR_NONE 0xffff /* e.g. 0x8086 for Intel and 0x10de for Nvidia */
#define PCI_DEVICE_NONE 0xffff /* e.g. 0x14f for a Geforce6200 */

/* NOTE: When adding fields to this structure, make sure to update the default
 * values in wined3d_main.c as well. */
struct wined3d_settings
{
    /* vertex and pixel shader modes */
    int vs_mode;
    int ps_mode;
    /* Ideally, we don't want the user to have to request GLSL. If the
     * hardware supports GLSL, we should use it. However, until it's fully
     * implemented, we'll leave it as a registry setting for developers. */
    BOOL glslRequested;
    int offscreen_rendering_mode;
    int rendertargetlock_mode;
    unsigned short pci_vendor_id;
    unsigned short pci_device_id;
    /* Memory tracking and object counting. */
    unsigned int emulated_textureram;
    char *logo;
    int allow_multisampling;
    BOOL strict_draw_ordering;
    BOOL always_offscreen;
};

extern struct wined3d_settings wined3d_settings DECLSPEC_HIDDEN;

enum wined3d_sampler_texture_type
{
    WINED3DSTT_UNKNOWN = 0,
    WINED3DSTT_1D = 1,
    WINED3DSTT_2D = 2,
    WINED3DSTT_CUBE = 3,
    WINED3DSTT_VOLUME = 4,
};

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
    WINED3DSPR_NULL,
    WINED3DSPR_RESOURCE,
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
#define WINED3DSI_TEXLD_PROJECT 1
#define WINED3DSI_TEXLD_BIAS    2

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
#define MAX_REG_INPUT 12
#define MAX_REG_OUTPUT 12
#define MAX_CONST_I 16
#define MAX_CONST_B 16

/* FIXME: This needs to go up to 2048 for
 * Shader model 3 according to msdn (and for software shaders) */
#define MAX_LABELS 16

#define SHADER_PGMSIZE 65535

struct wined3d_shader_buffer
{
    char *buffer;
    unsigned int bsize;
    unsigned int lineNo;
    BOOL newline;
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
    WINED3DSIH_DEF,
    WINED3DSIH_DEFB,
    WINED3DSIH_DEFI,
    WINED3DSIH_DIV,
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
    WINED3DSIH_NOP,
    WINED3DSIH_NRM,
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
};

struct wined3d_shader_version
{
    enum wined3d_shader_type type;
    BYTE major;
    BYTE minor;
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
    WORD input_registers;                   /* max(MAX_REG_INPUT, MAX_ATTRIBS), 16 */
    WORD output_registers;                  /* MAX_REG_OUTPUT, 12 */
    WORD integer_constants;                 /* MAX_CONST_I, 16 */
    WORD boolean_constants;                 /* MAX_CONST_B, 16 */
    WORD local_int_consts;                  /* MAX_CONST_I, 16 */
    WORD local_bool_consts;                 /* MAX_CONST_B, 16 */

    enum wined3d_sampler_texture_type sampler_type[max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS)];
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
    WORD padding        : 3;

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
    struct wined3d_shader_buffer *buffer;
    struct wined3d_shader_tex_mx *tex_mx;
    struct wined3d_shader_loop_state *loop_state;
    void *backend_data;
};

struct wined3d_shader_register
{
    enum wined3d_shader_register_type type;
    UINT idx;
    UINT array_idx;
    const struct wined3d_shader_src_param *rel_addr;
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

struct wined3d_shader_instruction
{
    const struct wined3d_shader_context *ctx;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    DWORD flags;
    BOOL coissue;
    DWORD predicate;
    UINT dst_count;
    const struct wined3d_shader_dst_param *dst;
    UINT src_count;
    const struct wined3d_shader_src_param *src;
};

struct wined3d_shader_semantic
{
    WINED3DDECLUSAGE usage;
    UINT usage_idx;
    enum wined3d_sampler_texture_type sampler_type;
    struct wined3d_shader_dst_param reg;
};

struct wined3d_shader_attribute
{
    WINED3DDECLUSAGE usage;
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
    void (*shader_read_opcode)(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins, UINT *param_size);
    void (*shader_read_src_param)(void *data, const DWORD **ptr, struct wined3d_shader_src_param *src_param,
            struct wined3d_shader_src_param *src_rel_addr);
    void (*shader_read_dst_param)(void *data, const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
            struct wined3d_shader_src_param *dst_rel_addr);
    void (*shader_read_semantic)(const DWORD **ptr, struct wined3d_shader_semantic *semantic);
    void (*shader_read_comment)(const DWORD **ptr, const char **comment, UINT *comment_size);
    BOOL (*shader_is_end)(void *data, const DWORD **ptr);
};

extern const struct wined3d_shader_frontend sm1_shader_frontend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_frontend sm4_shader_frontend DECLSPEC_HIDDEN;

typedef void (*SHADER_HANDLER)(const struct wined3d_shader_instruction *);

struct shader_caps {
    DWORD               VertexShaderVersion;
    DWORD               MaxVertexShaderConst;

    DWORD               PixelShaderVersion;
    float               PixelShader1xMaxValue;
    DWORD               MaxPixelShaderConst;

    BOOL                VSClipping;
};

enum tex_types
{
    tex_1d       = 0,
    tex_2d       = 1,
    tex_3d       = 2,
    tex_cube     = 3,
    tex_rect     = 4,
    tex_type_count = 5,
};

enum vertexprocessing_mode {
    fixedfunction,
    vertexshader,
    pretransformed
};

#define WINED3D_CONST_NUM_UNUSED ~0U

enum fogmode {
    FOG_OFF,
    FOG_LINEAR,
    FOG_EXP,
    FOG_EXP2
};

/* Stateblock dependent parameters which have to be hardcoded
 * into the shader code
 */

#define WINED3D_PSARGS_PROJECTED (1 << 3)
#define WINED3D_PSARGS_TEXTRANSFORM_SHIFT 4
#define WINED3D_PSARGS_TEXTRANSFORM_MASK 0xf

struct ps_compile_args {
    struct color_fixup_desc     color_fixup[MAX_FRAGMENT_SAMPLERS];
    enum vertexprocessing_mode  vp_mode;
    enum fogmode                fog;
    WORD                        tex_transform; /* ps 1.0-1.3, 4 textures */
    /* Texture types(2D, Cube, 3D) in ps 1.x */
    WORD                        srgb_correction;
    WORD                        np2_fixup;
    /* Bitmap for NP2 texcoord fixups (16 samplers max currently).
       D3D9 has a limit of 16 samplers and the fixup is superfluous
       in D3D10 (unconditional NP2 support mandatory). */
    WORD shadow; /* MAX_FRAGMENT_SAMPLERS, 16 */
};

enum fog_src_type {
    VS_FOG_Z        = 0,
    VS_FOG_COORD    = 1
};

struct vs_compile_args {
    BYTE                        fog_src;
    BYTE                        clip_enabled;
    WORD                        swizzle_map;   /* MAX_ATTRIBS, 16 */
};

struct wined3d_context;
struct wined3d_state;

struct wined3d_shader_backend_ops
{
    void (*shader_handle_instruction)(const struct wined3d_shader_instruction *);
    void (*shader_select)(const struct wined3d_context *context, BOOL usePS, BOOL useVS);
    void (*shader_select_depth_blt)(void *shader_priv, const struct wined3d_gl_info *gl_info,
            enum tex_types tex_type, const SIZE *ds_mask_size);
    void (*shader_deselect_depth_blt)(void *shader_priv, const struct wined3d_gl_info *gl_info);
    void (*shader_update_float_vertex_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_update_float_pixel_constants)(struct wined3d_device *device, UINT start, UINT count);
    void (*shader_load_constants)(const struct wined3d_context *context, char usePS, char useVS);
    void (*shader_load_np2fixup_constants)(void *shader_priv, const struct wined3d_gl_info *gl_info,
            const struct wined3d_state *state);
    void (*shader_destroy)(struct wined3d_shader *shader);
    HRESULT (*shader_alloc_private)(struct wined3d_device *device);
    void (*shader_free_private)(struct wined3d_device *device);
    void (*shader_context_destroyed)(void *shader_priv, const struct wined3d_context *context);
    void (*shader_get_caps)(const struct wined3d_gl_info *gl_info, struct shader_caps *caps);
    BOOL (*shader_color_fixup_supported)(struct color_fixup_desc fixup);
};

extern const struct wined3d_shader_backend_ops glsl_shader_backend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_backend_ops arb_program_shader_backend DECLSPEC_HIDDEN;
extern const struct wined3d_shader_backend_ops none_shader_backend DECLSPEC_HIDDEN;

/* X11 locking */

extern void (CDECL *wine_tsx11_lock_ptr)(void) DECLSPEC_HIDDEN;
extern void (CDECL *wine_tsx11_unlock_ptr)(void) DECLSPEC_HIDDEN;

/* As GLX relies on X, this is needed */
extern int num_lock DECLSPEC_HIDDEN;

#if 0
#define ENTER_GL() ++num_lock; if (num_lock > 1) FIXME("Recursive use of GL lock to: %d\n", num_lock); wine_tsx11_lock_ptr()
#define LEAVE_GL() if (num_lock != 1) FIXME("Recursive use of GL lock: %d\n", num_lock); --num_lock; wine_tsx11_unlock_ptr()
#else
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()
#endif

/*****************************************************************************
 * Defines
 */

/* GL related defines */
/* ------------------ */
#define GL_EXTCALL(f) (gl_info->f)

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xFF)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xFF)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xFF)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xFF)

#define D3DCOLOR_R(dw) (((float) (((dw) >> 16) & 0xFF)) / 255.0f)
#define D3DCOLOR_G(dw) (((float) (((dw) >>  8) & 0xFF)) / 255.0f)
#define D3DCOLOR_B(dw) (((float) (((dw) >>  0) & 0xFF)) / 255.0f)
#define D3DCOLOR_A(dw) (((float) (((dw) >> 24) & 0xFF)) / 255.0f)

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
    err = glGetError();                                             \
    if (err == GL_NO_ERROR) {                                       \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__);        \
                                                                    \
    } else do {                                                     \
        ERR(">>>>>>>>>>>>>>>>> %s (%#x) from %s @ %s / %d\n",       \
            debug_glerror(err), err, A, __FILE__, __LINE__);        \
       err = glGetError();                                          \
    } while (err != GL_NO_ERROR);                                   \
} while(0)
#else
#define checkGLcall(A) do {} while(0)
#endif

/* Trace routines / diagnostics */
/* ---------------------------- */

/* Dump out a matrix and copy it */
#define conv_mat(mat,gl_mat)                                                                \
do {                                                                                        \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));                                              \
} while (0)

/* Trace vector and strided data information */
#define TRACE_STRIDED(si, name) do { if (si->use_map & (1 << name)) \
        TRACE( #name " = (data {%#x:%p}, stride %d, format %s, stream %u)\n", \
        si->elements[name].data.buffer_object, si->elements[name].data.addr, si->elements[name].stride, \
        debug_d3dformat(si->elements[name].format->id), si->elements[name].stream_idx); } while(0)

/* Global variables */
extern const float identity[16] DECLSPEC_HIDDEN;

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
    WINED3D_FFP_EMIT_COUNT = 17
};

struct wined3d_bo_address
{
    GLuint buffer_object;
    const BYTE *addr;
};

struct wined3d_stream_info_element
{
    const struct wined3d_format *format;
    struct wined3d_bo_address data;
    GLsizei stride;
    UINT stream_idx;
};

struct wined3d_stream_info
{
    struct wined3d_stream_info_element elements[MAX_ATTRIBS];
    BOOL position_transformed;
    WORD swizzle_map; /* MAX_ATTRIBS, 16 */
    WORD use_map; /* MAX_ATTRIBS, 16 */
};

/*****************************************************************************
 * Prototypes
 */

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(struct wined3d_device *device, UINT index_count,
        UINT start_idx, UINT idxBytes, const void *idxData) DECLSPEC_HIDDEN;
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) DECLSPEC_HIDDEN;

typedef void (WINE_GLAPI *glAttribFunc)(const void *data);
typedef void (WINE_GLAPI *glMultiTexCoordFunc)(GLenum unit, const void *data);
extern glAttribFunc position_funcs[WINED3D_FFP_EMIT_COUNT] DECLSPEC_HIDDEN;
extern glAttribFunc diffuse_funcs[WINED3D_FFP_EMIT_COUNT] DECLSPEC_HIDDEN;
extern glAttribFunc specular_func_3ubv DECLSPEC_HIDDEN;
extern glAttribFunc specular_funcs[WINED3D_FFP_EMIT_COUNT] DECLSPEC_HIDDEN;
extern glAttribFunc normal_funcs[WINED3D_FFP_EMIT_COUNT] DECLSPEC_HIDDEN;
extern glMultiTexCoordFunc multi_texcoord_funcs[WINED3D_FFP_EMIT_COUNT] DECLSPEC_HIDDEN;

#define eps 1e-8

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

#define STATE_PIXELSHADER (STATE_SAMPLER(MAX_COMBINED_SAMPLERS - 1) + 1)
#define STATE_IS_PIXELSHADER(a) ((a) == STATE_PIXELSHADER)

#define STATE_TRANSFORM(a) (STATE_PIXELSHADER + (a))
#define STATE_IS_TRANSFORM(a) ((a) >= STATE_TRANSFORM(1) && (a) <= STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)))

#define STATE_STREAMSRC (STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)) + 1)
#define STATE_IS_STREAMSRC(a) ((a) == STATE_STREAMSRC)
#define STATE_INDEXBUFFER (STATE_STREAMSRC + 1)
#define STATE_IS_INDEXBUFFER(a) ((a) == STATE_INDEXBUFFER)

#define STATE_VDECL (STATE_INDEXBUFFER + 1)
#define STATE_IS_VDECL(a) ((a) == STATE_VDECL)

#define STATE_VSHADER (STATE_VDECL + 1)
#define STATE_IS_VSHADER(a) ((a) == STATE_VSHADER)

#define STATE_VIEWPORT (STATE_VSHADER + 1)
#define STATE_IS_VIEWPORT(a) ((a) == STATE_VIEWPORT)

#define STATE_VERTEXSHADERCONSTANT (STATE_VIEWPORT + 1)
#define STATE_PIXELSHADERCONSTANT (STATE_VERTEXSHADERCONSTANT + 1)
#define STATE_IS_VERTEXSHADERCONSTANT(a) ((a) == STATE_VERTEXSHADERCONSTANT)
#define STATE_IS_PIXELSHADERCONSTANT(a) ((a) == STATE_PIXELSHADERCONSTANT)

#define STATE_ACTIVELIGHT(a) (STATE_PIXELSHADERCONSTANT + (a) + 1)
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

#define STATE_HIGHEST (STATE_FRAMEBUFFER)

enum fogsource {
    FOGSOURCE_FFP,
    FOGSOURCE_VS,
    FOGSOURCE_COORD,
};

#define WINED3D_MAX_FBO_ENTRIES 64

struct wined3d_occlusion_query
{
    struct list entry;
    GLuint id;
    struct wined3d_context *context;
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

struct wined3d_context
{
    const struct wined3d_gl_info *gl_info;
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

    struct wined3d_swapchain *swapchain;
    struct wined3d_surface *current_rt;
    DWORD                   tid;    /* Thread ID which owns this context at the moment */

    /* Stores some information about the context state for optimization */
    WORD render_offscreen : 1;
    WORD last_was_rhw : 1;              /* true iff last draw_primitive was in xyzrhw mode */
    WORD last_was_pshader : 1;
    WORD last_was_vshader : 1;
    WORD namedArraysLoaded : 1;
    WORD numberedArraysLoaded : 1;
    WORD last_was_blit : 1;
    WORD last_was_ckey : 1;
    WORD fog_coord : 1;
    WORD fog_enabled : 1;
    WORD num_untracked_materials : 2;   /* Max value 2 */
    WORD current : 1;
    WORD destroyed : 1;
    WORD valid : 1;
    WORD padding : 1;
    BYTE texShaderBumpMap;              /* MAX_TEXTURES, 8 */
    BYTE lastWasPow2Texture;            /* MAX_TEXTURES, 8 */
    DWORD                   numbered_array_mask;
    GLenum                  tracking_parm;     /* Which source is tracking current colour         */
    GLenum                  untracked_materials[2];
    UINT                    blit_w, blit_h;
    enum fogsource          fog_source;
    DWORD active_texture;
    DWORD texture_type[MAX_COMBINED_SAMPLERS];

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

    /* FBOs */
    UINT                    fbo_entry_count;
    struct list             fbo_list;
    struct list             fbo_destroy_list;
    struct fbo_entry        *current_fbo;
    GLuint                  fbo_read_binding;
    GLuint                  fbo_draw_binding;
    BOOL rebind_fbo;
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

    /* Extension emulation */
    GLint                   gl_fog_source;
    GLfloat                 fog_coord_value;
    GLfloat                 color[4], fogstart, fogend, fogcolor[4];
    GLuint                  dummy_arbfp_prog;
};

struct wined3d_fb_state
{
    struct wined3d_surface **render_targets;
    struct wined3d_surface *depth_stencil;
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

struct fragment_caps
{
    DWORD PrimitiveMiscCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;
};

struct fragment_pipeline
{
    void (*enable_extension)(BOOL enable);
    void (*get_caps)(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps);
    HRESULT (*alloc_private)(struct wined3d_device *device);
    void (*free_private)(struct wined3d_device *device);
    BOOL (*color_fixup_supported)(struct color_fixup_desc fixup);
    const struct StateEntryTemplate *states;
    BOOL ffp_proj_control;
};

extern const struct StateEntryTemplate misc_state_template[] DECLSPEC_HIDDEN;
extern const struct StateEntryTemplate ffp_vertexstate_template[] DECLSPEC_HIDDEN;
extern const struct fragment_pipeline ffp_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline atifs_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline arbfp_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline nvts_fragment_pipeline DECLSPEC_HIDDEN;
extern const struct fragment_pipeline nvrc_fragment_pipeline DECLSPEC_HIDDEN;

/* "Base" state table */
HRESULT compile_state_table(struct StateEntry *StateTable, APPLYSTATEFUNC **dev_multistate_funcs,
        const struct wined3d_gl_info *gl_info, const struct StateEntryTemplate *vertex,
        const struct fragment_pipeline *fragment, const struct StateEntryTemplate *misc) DECLSPEC_HIDDEN;

enum wined3d_blit_op
{
    WINED3D_BLIT_OP_COLOR_BLIT,
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
    HRESULT (*set_shader)(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface);
    void (*unset_shader)(const struct wined3d_gl_info *gl_info);
    BOOL (*blit_supported)(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
            const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
            const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format);
    HRESULT (*color_fill)(struct wined3d_device *device, struct wined3d_surface *dst_surface,
            const RECT *dst_rect, const struct wined3d_color *color);
    HRESULT (*depth_fill)(struct wined3d_device *device,
            struct wined3d_surface *surface, const RECT *rect, float depth);
};

extern const struct blit_shader ffp_blit DECLSPEC_HIDDEN;
extern const struct blit_shader arbfp_blit DECLSPEC_HIDDEN;
extern const struct blit_shader cpu_blit DECLSPEC_HIDDEN;

const struct blit_shader *wined3d_select_blitter(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
        DECLSPEC_HIDDEN;

/* Temporary blit_shader helper functions */
HRESULT arbfp_blit_surface(struct wined3d_device *device, DWORD filter,
        struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect) DECLSPEC_HIDDEN;

struct wined3d_context *context_acquire(const struct wined3d_device *device,
        struct wined3d_surface *target) DECLSPEC_HIDDEN;
void context_alloc_event_query(struct wined3d_context *context,
        struct wined3d_event_query *query) DECLSPEC_HIDDEN;
void context_alloc_occlusion_query(struct wined3d_context *context,
        struct wined3d_occlusion_query *query) DECLSPEC_HIDDEN;
void context_apply_blit_state(struct wined3d_context *context, const struct wined3d_device *device) DECLSPEC_HIDDEN;
BOOL context_apply_clear_state(struct wined3d_context *context, const struct wined3d_device *device,
        UINT rt_count, const struct wined3d_fb_state *fb) DECLSPEC_HIDDEN;
BOOL context_apply_draw_state(struct wined3d_context *context, struct wined3d_device *device) DECLSPEC_HIDDEN;
void context_apply_fbo_state_blit(struct wined3d_context *context, GLenum target,
        struct wined3d_surface *render_target, struct wined3d_surface *depth_stencil, DWORD location) DECLSPEC_HIDDEN;
void context_active_texture(struct wined3d_context *context, const struct wined3d_gl_info *gl_info,
        unsigned int unit) DECLSPEC_HIDDEN;
void context_bind_texture(struct wined3d_context *context, GLenum target, GLuint name) DECLSPEC_HIDDEN;
void context_check_fbo_status(const struct wined3d_context *context, GLenum target) DECLSPEC_HIDDEN;
struct wined3d_context *context_create(struct wined3d_swapchain *swapchain, struct wined3d_surface *target,
        const struct wined3d_format *ds_format) DECLSPEC_HIDDEN;
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
    float                         lightPosn[4];
    float                         lightDirn[4];
    float                         exponent;
    float                         cutoff;

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
    HW_VENDOR_SOFTWARE                 = 0x0000,
    HW_VENDOR_AMD                      = 0x1002,
    HW_VENDOR_NVIDIA                   = 0x10de,
    HW_VENDOR_INTEL                    = 0x8086,
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
    CARD_AMD_RADEON_HD4350          = 0x954f,
    CARD_AMD_RADEON_HD4550          = 0x9540,
    CARD_AMD_RADEON_HD4600          = 0x9495,
    CARD_AMD_RADEON_HD4650          = 0x9498,
    CARD_AMD_RADEON_HD4670          = 0x9490,
    CARD_AMD_RADEON_HD4700          = 0x944e,
    CARD_AMD_RADEON_HD4770          = 0x94b3,
    CARD_AMD_RADEON_HD4800          = 0x944c, /* Picked one value between 9440, 944c, 9442, 9460 */
    CARD_AMD_RADEON_HD4830          = 0x944c,
    CARD_AMD_RADEON_HD4850          = 0x9442,
    CARD_AMD_RADEON_HD4870          = 0x9440,
    CARD_AMD_RADEON_HD4890          = 0x9460,
    CARD_AMD_RADEON_HD5400          = 0x68f9,
    CARD_AMD_RADEON_HD5600          = 0x68d8,
    CARD_AMD_RADEON_HD5700          = 0x68BE, /* Picked HD5750 */
    CARD_AMD_RADEON_HD5750          = 0x68BE,
    CARD_AMD_RADEON_HD5770          = 0x68B8,
    CARD_AMD_RADEON_HD5800          = 0x6898, /* Picked HD5850 */
    CARD_AMD_RADEON_HD5850          = 0x6898,
    CARD_AMD_RADEON_HD5870          = 0x6899,
    CARD_AMD_RADEON_HD5900          = 0x689c,
    CARD_AMD_RADEON_HD6300          = 0x9803,
    CARD_AMD_RADEON_HD6400          = 0x6770,
    CARD_AMD_RADEON_HD6410D         = 0x9644,
    CARD_AMD_RADEON_HD6550D         = 0x9640,
    CARD_AMD_RADEON_HD6600          = 0x6758,
    CARD_AMD_RADEON_HD6600M         = 0x6741,
    CARD_AMD_RADEON_HD6800          = 0x6739,
    CARD_AMD_RADEON_HD6900          = 0x6719,

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
    CARD_NVIDIA_GEFORCE_7400        = 0x01d8,
    CARD_NVIDIA_GEFORCE_7300        = 0x01d7, /* GeForce Go 7300 */
    CARD_NVIDIA_GEFORCE_7600        = 0x0391,
    CARD_NVIDIA_GEFORCE_7800GT      = 0x0092,
    CARD_NVIDIA_GEFORCE_8100        = 0x084F,
    CARD_NVIDIA_GEFORCE_8200        = 0x0849, /* Other PCI ID 0x084B */
    CARD_NVIDIA_GEFORCE_8300GS      = 0x0423,
    CARD_NVIDIA_GEFORCE_8400GS      = 0x0404,
    CARD_NVIDIA_GEFORCE_8500GT      = 0x0421,
    CARD_NVIDIA_GEFORCE_8600GT      = 0x0402,
    CARD_NVIDIA_GEFORCE_8600MGT     = 0x0407,
    CARD_NVIDIA_GEFORCE_8800GTS     = 0x0193,
    CARD_NVIDIA_GEFORCE_8800GTX     = 0x0191,
    CARD_NVIDIA_GEFORCE_9200        = 0x086d,
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
    CARD_NVIDIA_GEFORCE_GT320M      = 0x0a2d,
    CARD_NVIDIA_GEFORCE_GT325M      = 0x0a35,
    CARD_NVIDIA_GEFORCE_GT330       = 0x0ca0,
    CARD_NVIDIA_GEFORCE_GTS350M     = 0x0cb0,
    CARD_NVIDIA_GEFORCE_GT420       = 0x0de2,
    CARD_NVIDIA_GEFORCE_GT430       = 0x0de1,
    CARD_NVIDIA_GEFORCE_GT440       = 0x0de0,
    CARD_NVIDIA_GEFORCE_GTS450      = 0x0dc4,
    CARD_NVIDIA_GEFORCE_GTX460      = 0x0e22,
    CARD_NVIDIA_GEFORCE_GTX460M     = 0x0dd1,
    CARD_NVIDIA_GEFORCE_GTX465      = 0x06c4,
    CARD_NVIDIA_GEFORCE_GTX470      = 0x06cd,
    CARD_NVIDIA_GEFORCE_GTX480      = 0x06c0,
    CARD_NVIDIA_GEFORCE_GT540M      = 0x0df4,
    CARD_NVIDIA_GEFORCE_GTX550      = 0x1244,
    CARD_NVIDIA_GEFORCE_GT555M      = 0x04b8,
    CARD_NVIDIA_GEFORCE_GTX560TI    = 0x1200,
    CARD_NVIDIA_GEFORCE_GTX560      = 0x1201,
    CARD_NVIDIA_GEFORCE_GTX570      = 0x1081,
    CARD_NVIDIA_GEFORCE_GTX580      = 0x1080,

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
};

struct wined3d_fbo_ops
{
    PGLFNGLISRENDERBUFFERPROC                       glIsRenderbuffer;
    PGLFNGLBINDRENDERBUFFERPROC                     glBindRenderbuffer;
    PGLFNGLDELETERENDERBUFFERSPROC                  glDeleteRenderbuffers;
    PGLFNGLGENRENDERBUFFERSPROC                     glGenRenderbuffers;
    PGLFNGLRENDERBUFFERSTORAGEPROC                  glRenderbufferStorage;
    PGLFNRENDERBUFFERSTORAGEMULTISAMPLEPROC         glRenderbufferStorageMultisample;
    PGLFNGLGETRENDERBUFFERPARAMETERIVPROC           glGetRenderbufferParameteriv;
    PGLFNGLISFRAMEBUFFERPROC                        glIsFramebuffer;
    PGLFNGLBINDFRAMEBUFFERPROC                      glBindFramebuffer;
    PGLFNGLDELETEFRAMEBUFFERSPROC                   glDeleteFramebuffers;
    PGLFNGLGENFRAMEBUFFERSPROC                      glGenFramebuffers;
    PGLFNGLCHECKFRAMEBUFFERSTATUSPROC               glCheckFramebufferStatus;
    PGLFNGLFRAMEBUFFERTEXTURE1DPROC                 glFramebufferTexture1D;
    PGLFNGLFRAMEBUFFERTEXTURE2DPROC                 glFramebufferTexture2D;
    PGLFNGLFRAMEBUFFERTEXTURE3DPROC                 glFramebufferTexture3D;
    PGLFNGLFRAMEBUFFERRENDERBUFFERPROC              glFramebufferRenderbuffer;
    PGLFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC  glGetFramebufferAttachmentParameteriv;
    PGLFNGLBLITFRAMEBUFFERPROC                      glBlitFramebuffer;
    PGLFNGLGENERATEMIPMAPPROC                       glGenerateMipmap;
};

struct wined3d_gl_limits
{
    UINT buffers;
    UINT lights;
    UINT textures;
    UINT texture_stages;
    UINT texture_coords;
    UINT fragment_samplers;
    UINT vertex_samplers;
    UINT combined_samplers;
    UINT general_combiners;
    UINT sampler_stages;
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
    DWORD glsl_version;
    struct wined3d_gl_limits limits;
    DWORD reserved_glsl_constants;
    DWORD quirks;
    BOOL supported[WINED3D_GL_EXT_COUNT];
    GLint wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP + 1];

    struct wined3d_fbo_ops fbo_ops;
#define USE_GL_FUNC(type, pfn, ext, replace) type pfn;
    /* GL function pointers */
    GL_EXT_FUNCS_GEN
    /* WGL function pointers */
    WGL_EXT_FUNCS_GEN
#undef USE_GL_FUNC

    struct wined3d_format *formats;
};

struct wined3d_driver_info
{
    enum wined3d_pci_vendor vendor;
    enum wined3d_pci_device device;
    const char *name;
    const char *description;
    unsigned int vidmem;
    DWORD version_high;
    DWORD version_low;
};

/* The adapter structure */
struct wined3d_adapter
{
    UINT ordinal;
    BOOL                    opengl;

    POINT monitorPoint;
    SIZE screen_size;
    enum wined3d_format_id screen_format;

    struct wined3d_gl_info  gl_info;
    struct wined3d_driver_info driver_info;
    WCHAR                   DeviceName[CCHDEVICENAME]; /* DeviceName for use with e.g. ChangeDisplaySettings */
    unsigned int cfg_count;
    struct wined3d_pixel_format *cfgs;
    unsigned int            TextureRam; /* Amount of texture memory both video ram + AGP/TurboCache/HyperMemory/.. */
    unsigned int            UsedTextureRam;
    LUID luid;

    const struct fragment_pipeline *fragment_pipe;
    const struct wined3d_shader_backend_ops *shader_backend;
    const struct blit_shader *blitter;
};

unsigned int adapter_adjust_memory(struct wined3d_adapter *adapter, int amount) DECLSPEC_HIDDEN;

BOOL initPixelFormats(struct wined3d_gl_info *gl_info, enum wined3d_pci_vendor vendor) DECLSPEC_HIDDEN;
BOOL initPixelFormatsNoGL(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
extern void add_gl_compat_wrappers(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

/*****************************************************************************
 * High order patch management
 */
struct WineD3DRectPatch
{
    UINT                            Handle;
    float                          *mem;
    struct wined3d_strided_data strided;
    struct wined3d_rect_patch_info rect_patch_info;
    float                           numSegs[4];
    char                            has_normals, has_texcoords;
    struct list                     entry;
};

HRESULT tesselate_rectpatch(struct wined3d_device *device, struct WineD3DRectPatch *patch) DECLSPEC_HIDDEN;

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

struct ffp_frag_settings {
    struct texture_stage_op     op[MAX_TEXTURES];
    enum fogmode fog;
    /* Use shorts instead of chars to get dword alignment */
    unsigned short sRGB_write;
    unsigned short emul_clipplanes;
};

struct ffp_frag_desc
{
    struct wine_rb_entry entry;
    struct ffp_frag_settings    settings;
};

extern const struct wine_rb_functions wined3d_ffp_frag_program_rb_functions DECLSPEC_HIDDEN;
extern const struct wined3d_parent_ops wined3d_null_parent_ops DECLSPEC_HIDDEN;

void gen_ffp_frag_op(const struct wined3d_device *device, const struct wined3d_state *state,
        struct ffp_frag_settings *settings, BOOL ignore_textype) DECLSPEC_HIDDEN;
const struct ffp_frag_desc *find_ffp_frag_shader(const struct wine_rb_tree *fragment_shaders,
        const struct ffp_frag_settings *settings) DECLSPEC_HIDDEN;
void add_ffp_frag_shader(struct wine_rb_tree *shaders, struct ffp_frag_desc *desc) DECLSPEC_HIDDEN;
void wined3d_get_draw_rect(const struct wined3d_state *state, RECT *rect) DECLSPEC_HIDDEN;

struct wined3d
{
    LONG ref;
    void *parent;
    DWORD flags;
    UINT dxVersion;
    UINT adapter_count;
    struct wined3d_adapter adapters[1];
};

HRESULT wined3d_init(struct wined3d *wined3d, UINT version, DWORD flags, void *parent) DECLSPEC_HIDDEN;
BOOL wined3d_register_window(HWND window, struct wined3d_device *device) DECLSPEC_HIDDEN;
void wined3d_unregister_window(HWND window) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DDevice implementation structure
 */
#define WINED3D_UNMAPPED_STAGE ~0U

/* Multithreaded flag. Removed from the public header to signal that IWineD3D::CreateDevice ignores it */
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

    /* Selected capabilities */
    int vs_selected_mode;
    int ps_selected_mode;
    const struct wined3d_shader_backend_ops *shader_backend;
    void *shader_priv;
    void *fragment_priv;
    void *blit_priv;
    struct StateEntry StateTable[STATE_HIGHEST + 1];
    /* Array of functions for states which are handled by more than one pipeline part */
    APPLYSTATEFUNC *multistate_funcs[STATE_HIGHEST + 1];
    const struct fragment_pipeline *frag_pipe;
    const struct blit_shader *blitter;

    unsigned int max_ffp_textures;
    DWORD vshader_version, pshader_version;
    DWORD d3d_vshader_constantF, d3d_pshader_constantF; /* Advertised d3d caps, not GL ones */
    DWORD vs_clipping;

    WORD view_ident : 1;                /* true iff view matrix is identity */
    WORD vertexBlendUsed : 1;           /* To avoid needless setting of the blend matrices */
    WORD isRecordingState : 1;
    WORD isInDraw : 1;
    WORD bCursorVisible : 1;
    WORD d3d_initialized : 1;
    WORD inScene : 1;                   /* A flag to check for proper BeginScene / EndScene call pairs */
    WORD softwareVertexProcessing : 1;  /* process vertex shaders using software or hardware */
    WORD useDrawStridedSlow : 1;
    WORD instancedDraw : 1;
    WORD filter_messages : 1;
    WORD padding : 5;

    BYTE fixed_function_usage_map;      /* MAX_TEXTURES, 8 */

#define DDRAW_PITCH_ALIGNMENT 8
#define D3D8_PITCH_ALIGNMENT 4
    unsigned char           surface_alignment; /* Line Alignment of surfaces                      */

    /* State block related */
    struct wined3d_stateblock *stateBlock;
    struct wined3d_stateblock *updateStateBlock;

    /* Internal use fields  */
    struct wined3d_device_creation_parameters create_parms;
    HWND focus_window;

    struct wined3d_swapchain **swapchains;
    UINT swapchain_count;

    struct list             resources; /* a linked list to track resources created by the device */
    struct list             shaders;   /* a linked list to track shaders (pixel and vertex)      */

    /* Render Target Support */
    DWORD valid_rt_mask;
    struct wined3d_fb_state fb;
    struct wined3d_surface *onscreen_depth_stencil;
    struct wined3d_surface *auto_depth_stencil;

    /* For rendering to a texture using glCopyTexImage */
    GLuint                  depth_blt_texture;

    /* Cursor management */
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;
    UINT                    cursorWidth, cursorHeight;
    GLuint                  cursorTexture;
    HCURSOR                 hardwareCursor;

    /* The Wine logo surface */
    struct wined3d_surface *logo_surface;

    /* Textures for when no other textures are mapped */
    UINT dummy_texture_2d[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_rect[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_3d[MAX_COMBINED_SAMPLERS];
    UINT dummy_texture_cube[MAX_COMBINED_SAMPLERS];

    /* With register combiners we can skip junk texture stages */
    DWORD                     texUnitMap[MAX_COMBINED_SAMPLERS];
    DWORD                     rev_tex_unit_map[MAX_COMBINED_SAMPLERS];

    /* Stream source management */
    struct wined3d_stream_info strided_streams;
    const struct wined3d_strided_data *up_strided;
    struct wined3d_event_query *buffer_queries[MAX_ATTRIBS];
    unsigned int num_buffer_queries;

    /* Context management */
    struct wined3d_context **contexts;
    UINT context_count;

    /* High level patch management */
#define PATCHMAP_SIZE 43
#define PATCHMAP_HASHFUNC(x) ((x) % PATCHMAP_SIZE) /* Primitive and simple function */
    struct list             patches[PATCHMAP_SIZE];
};

HRESULT device_clear_render_targets(struct wined3d_device *device, UINT rt_count, const struct wined3d_fb_state *fb,
        UINT rect_count, const RECT *rects, const RECT *draw_rect, DWORD flags,
        const struct wined3d_color *color, float depth, DWORD stencil) DECLSPEC_HIDDEN;
BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
void device_context_remove(struct wined3d_device *device, struct wined3d_context *context) DECLSPEC_HIDDEN;
HRESULT device_init(struct wined3d_device *device, struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, HWND focus_window, DWORD flags,
        BYTE surface_alignment, struct wined3d_device_parent *device_parent) DECLSPEC_HIDDEN;
void device_preload_textures(const struct wined3d_device *device) DECLSPEC_HIDDEN;
LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc) DECLSPEC_HIDDEN;
void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource) DECLSPEC_HIDDEN;
void device_stream_info_from_declaration(struct wined3d_device *device,
        struct wined3d_stream_info *stream_info, BOOL *fixup) DECLSPEC_HIDDEN;
void device_switch_onscreen_ds(struct wined3d_device *device, struct wined3d_context *context,
        struct wined3d_surface *depth_stencil) DECLSPEC_HIDDEN;
void device_update_stream_info(struct wined3d_device *device, const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
void device_update_tex_unit_map(struct wined3d_device *device) DECLSPEC_HIDDEN;
void device_invalidate_state(const struct wined3d_device *device, DWORD state) DECLSPEC_HIDDEN;

static inline BOOL isStateDirty(const struct wined3d_context *context, DWORD state)
{
    DWORD idx = state / (sizeof(*context->isStateDirty) * CHAR_BIT);
    BYTE shift = state & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
    return context->isStateDirty[idx] & (1 << shift);
}

static inline void invalidate_active_texture(const struct wined3d_device *device, struct wined3d_context *context)
{
    DWORD sampler = device->rev_tex_unit_map[context->active_texture];
    if (sampler != WINED3D_UNMAPPED_STAGE)
        context_invalidate_state(context, STATE_SAMPLER(sampler));
}

#define WINED3D_RESOURCE_ACCESS_GPU     0x1
#define WINED3D_RESOURCE_ACCESS_CPU     0x2
/* SCRATCH is mostly the same as CPU, but can't be used by the GPU at all,
 * not even for resource uploads. */
#define WINED3D_RESOURCE_ACCESS_SCRATCH 0x4

struct wined3d_resource_ops
{
    void (*resource_unload)(struct wined3d_resource *resource);
};

struct wined3d_resource
{
    LONG ref;
    struct wined3d_device *device;
    enum wined3d_resource_type type;
    const struct wined3d_format *format;
    enum wined3d_multisample_type multisample_type;
    UINT                    multisample_quality;
    DWORD                   usage;
    enum wined3d_pool pool;
    DWORD access_flags;
    UINT width;
    UINT height;
    UINT depth;
    UINT                    size;
    DWORD                   priority;
    BYTE                   *allocatedMemory; /* Pointer to the real data location */
    BYTE                   *heapMemory; /* Pointer to the HeapAlloced block of memory */
    struct list             privateData;
    struct list             resource_list_entry;

    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    const struct wined3d_resource_ops *resource_ops;
};

void resource_cleanup(struct wined3d_resource *resource) DECLSPEC_HIDDEN;
DWORD resource_get_priority(const struct wined3d_resource *resource) DECLSPEC_HIDDEN;
HRESULT resource_init(struct wined3d_resource *resource, struct wined3d_device *device,
        enum wined3d_resource_type type, const struct wined3d_format *format,
        enum wined3d_multisample_type multisample_type, UINT multisample_quality,
        DWORD usage, enum wined3d_pool pool, UINT width, UINT height, UINT depth, UINT size,
        void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops) DECLSPEC_HIDDEN;
DWORD resource_set_priority(struct wined3d_resource *resource, DWORD priority) DECLSPEC_HIDDEN;
void resource_unload(struct wined3d_resource *resource) DECLSPEC_HIDDEN;

/* Tests show that the start address of resources is 32 byte aligned */
#define RESOURCE_ALIGNMENT 16

enum wined3d_texture_state
{
    WINED3DTEXSTA_ADDRESSU       = 0,
    WINED3DTEXSTA_ADDRESSV       = 1,
    WINED3DTEXSTA_ADDRESSW       = 2,
    WINED3DTEXSTA_BORDERCOLOR    = 3,
    WINED3DTEXSTA_MAGFILTER      = 4,
    WINED3DTEXSTA_MINFILTER      = 5,
    WINED3DTEXSTA_MIPFILTER      = 6,
    WINED3DTEXSTA_MAXMIPLEVEL    = 7,
    WINED3DTEXSTA_MAXANISOTROPY  = 8,
    WINED3DTEXSTA_SRGBTEXTURE    = 9,
    WINED3DTEXSTA_SHADOW         = 10,
    MAX_WINETEXTURESTATES        = 11,
};

enum WINED3DSRGB
{
    SRGB_ANY                                = 0,    /* Uses the cached value(e.g. external calls) */
    SRGB_RGB                                = 1,    /* Loads the rgb texture */
    SRGB_SRGB                               = 2,    /* Loads the srgb texture */
};

struct gl_texture
{
    DWORD                   states[MAX_WINETEXTURESTATES];
    BOOL                    dirty;
    GLuint                  name;
};

struct wined3d_texture_ops
{
    HRESULT (*texture_bind)(struct wined3d_texture *texture,
            struct wined3d_context *context, BOOL srgb);
    void (*texture_preload)(struct wined3d_texture *texture, enum WINED3DSRGB srgb);
    void (*texture_sub_resource_add_dirty_region)(struct wined3d_resource *sub_resource,
            const struct wined3d_box *dirty_region);
    void (*texture_sub_resource_cleanup)(struct wined3d_resource *sub_resource);
};

#define WINED3D_TEXTURE_COND_NP2            0x1
#define WINED3D_TEXTURE_POW2_MAT_IDENT      0x2
#define WINED3D_TEXTURE_IS_SRGB             0x4

struct wined3d_texture
{
    struct wined3d_resource resource;
    const struct wined3d_texture_ops *texture_ops;
    struct gl_texture texture_rgb, texture_srgb;
    struct wined3d_resource **sub_resources;
    UINT layer_count;
    UINT level_count;
    float pow2_matrix[16];
    UINT lod;
    enum wined3d_texture_filter_type filter_type;
    LONG bind_count;
    DWORD sampler;
    DWORD flags;
    const struct min_lookup *min_mip_lookup;
    const GLenum *mag_lookup;
    GLenum target;
};

static inline struct wined3d_texture *wined3d_texture_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_texture, resource);
}

static inline struct gl_texture *wined3d_texture_get_gl_texture(struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    return srgb && !gl_info->supported[EXT_TEXTURE_SRGB_DECODE]
            ? &texture->texture_srgb : &texture->texture_rgb;
}

void wined3d_texture_apply_state_changes(struct wined3d_texture *texture,
        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1],
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
void wined3d_texture_set_dirty(struct wined3d_texture *texture, BOOL dirty) DECLSPEC_HIDDEN;

struct wined3d_volume
{
    struct wined3d_resource resource;
    struct wined3d_texture *container;
    BOOL                    lockable;
    BOOL                    locked;
    struct wined3d_box lockedBox;
    struct wined3d_box dirtyBox;
    BOOL                    dirty;
};

static inline struct wined3d_volume *volume_from_resource(struct wined3d_resource *resource)
{
    return CONTAINING_RECORD(resource, struct wined3d_volume, resource);
}

void volume_add_dirty_box(struct wined3d_volume *volume, const struct wined3d_box *dirty_box) DECLSPEC_HIDDEN;
void volume_load(const struct wined3d_volume *volume, struct wined3d_context *context, UINT level, BOOL srgb_mode) DECLSPEC_HIDDEN;
void volume_set_container(struct wined3d_volume *volume, struct wined3d_texture *container) DECLSPEC_HIDDEN;

struct wined3d_surface_dib
{
    HBITMAP DIBsection;
    void *bitmap_data;
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
    DWORD location;
    DWORD rt_mask;
    BOOL attached;
    GLuint id;
};

enum wined3d_container_type
{
    WINED3D_CONTAINER_NONE = 0,
    WINED3D_CONTAINER_SWAPCHAIN,
    WINED3D_CONTAINER_TEXTURE,
};

struct wined3d_subresource_container
{
    enum wined3d_container_type type;
    union
    {
        struct wined3d_swapchain *swapchain;
        struct wined3d_texture *texture;
        void *base;
    } u;
};

struct wined3d_surface_ops
{
    HRESULT (*surface_private_setup)(struct wined3d_surface *surface);
    void (*surface_realize_palette)(struct wined3d_surface *surface);
    void (*surface_map)(struct wined3d_surface *surface, const RECT *rect, DWORD flags);
    void (*surface_unmap)(struct wined3d_surface *surface);
};

struct wined3d_surface
{
    struct wined3d_resource resource;
    const struct wined3d_surface_ops *surface_ops;
    struct wined3d_subresource_container container;
    struct wined3d_palette *palette; /* D3D7 style palette handling */
    DWORD draw_binding;

    DWORD flags;

    WINED3DSURFTYPE surface_type;
    UINT                      pow2Width;
    UINT                      pow2Height;

    /* A method to retrieve the drawable size. Not in the Vtable to make it changeable */
    void (*get_drawable_size)(const struct wined3d_context *context, UINT *width, UINT *height);

    /* PBO */
    GLuint                    pbo;
    GLuint rb_multisample;
    GLuint rb_resolved;
    GLuint texture_name;
    GLuint texture_name_srgb;
    GLint texture_level;
    GLenum texture_target;

    RECT                      lockedRect;
    RECT                      dirtyRect;
    int                       lockCount;
#define MAXLOCKCOUNT          50 /* After this amount of locks do not free the sysmem copy */

    /* For GetDC */
    struct wined3d_surface_dib dib;
    HDC                       hDC;

    /* Color keys for DDraw */
    struct wined3d_color_key dst_blt_color_key;
    struct wined3d_color_key src_blt_color_key;
    struct wined3d_color_key dst_overlay_color_key;
    struct wined3d_color_key src_overlay_color_key;
    DWORD                     CKeyFlags;

    struct wined3d_color_key gl_color_key;

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
            ? surface->texture_name_srgb : surface->texture_name;
}

void surface_add_dirty_rect(struct wined3d_surface *surface, const struct wined3d_box *dirty_rect) DECLSPEC_HIDDEN;
void surface_bind(struct wined3d_surface *surface, struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
HRESULT surface_color_fill(struct wined3d_surface *s,
        const RECT *rect, const struct wined3d_color *color) DECLSPEC_HIDDEN;
GLenum surface_get_gl_buffer(const struct wined3d_surface *surface) DECLSPEC_HIDDEN;
BOOL surface_init_sysmem(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
void surface_internal_preload(struct wined3d_surface *surface, enum WINED3DSRGB srgb) DECLSPEC_HIDDEN;
BOOL surface_is_offscreen(const struct wined3d_surface *surface) DECLSPEC_HIDDEN;
HRESULT surface_load(struct wined3d_surface *surface, BOOL srgb) DECLSPEC_HIDDEN;
void surface_load_ds_location(struct wined3d_surface *surface,
        struct wined3d_context *context, DWORD location) DECLSPEC_HIDDEN;
void surface_load_fb_texture(struct wined3d_surface *surface, BOOL srgb) DECLSPEC_HIDDEN;
HRESULT surface_load_location(struct wined3d_surface *surface, DWORD location, const RECT *rect) DECLSPEC_HIDDEN;
void surface_modify_ds_location(struct wined3d_surface *surface, DWORD location, UINT w, UINT h) DECLSPEC_HIDDEN;
void surface_modify_location(struct wined3d_surface *surface, DWORD location, BOOL persistent) DECLSPEC_HIDDEN;
void surface_prepare_rb(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL multisample) DECLSPEC_HIDDEN;
void surface_prepare_texture(struct wined3d_surface *surface,
        struct wined3d_context *context, BOOL srgb) DECLSPEC_HIDDEN;
void surface_set_compatible_renderbuffer(struct wined3d_surface *surface,
        const struct wined3d_surface *rt) DECLSPEC_HIDDEN;
void surface_set_container(struct wined3d_surface *surface,
        enum wined3d_container_type type, void *container) DECLSPEC_HIDDEN;
void surface_set_texture_name(struct wined3d_surface *surface, GLuint name, BOOL srgb_name) DECLSPEC_HIDDEN;
void surface_set_texture_target(struct wined3d_surface *surface, GLenum target) DECLSPEC_HIDDEN;
void surface_translate_drawable_coords(const struct wined3d_surface *surface, HWND window, RECT *rect) DECLSPEC_HIDDEN;
void surface_update_draw_binding(struct wined3d_surface *surface) DECLSPEC_HIDDEN;
HRESULT surface_upload_from_surface(struct wined3d_surface *dst_surface, const POINT *dst_point,
        struct wined3d_surface *src_surface, const RECT *src_rect) DECLSPEC_HIDDEN;

void get_drawable_size_swapchain(const struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;
void get_drawable_size_backbuffer(const struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;
void get_drawable_size_fbo(const struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;

void draw_textured_quad(const struct wined3d_surface *src_surface, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect, enum wined3d_texture_filter_type filter) DECLSPEC_HIDDEN;
void flip_surface(struct wined3d_surface *front, struct wined3d_surface *back) DECLSPEC_HIDDEN;

/* Surface flags: */
#define SFLAG_CONVERTED         0x00000001 /* Converted for color keying or palettized. */
#define SFLAG_DISCARD           0x00000002 /* ??? */
#define SFLAG_NONPOW2           0x00000004 /* Surface sizes are not a power of 2 */
#define SFLAG_NORMCOORD         0x00000008 /* Set if GL texture coordinates are normalized (non-texture rectangle). */
#define SFLAG_LOCKABLE          0x00000010 /* Surface can be locked. */
#define SFLAG_DYNLOCK           0x00000020 /* Surface is often locked by the application. */
#define SFLAG_LOCKED            0x00000040 /* Surface is currently locked. */
#define SFLAG_DCINUSE           0x00000080 /* Set between GetDC and ReleaseDC calls. */
#define SFLAG_LOST              0x00000100 /* Surface lost flag for ddraw. */
#define SFLAG_GLCKEY            0x00000200 /* The GL texture was created with a color key. */
#define SFLAG_CLIENT            0x00000400 /* GL_APPLE_client_storage is used with this surface. */
#define SFLAG_INOVERLAYDRAW     0x00000800 /* Overlay drawing is in progress. Recursion prevention. */
#define SFLAG_DIBSECTION        0x00001000 /* Has a DIB section attached for GetDC. */
#define SFLAG_USERPTR           0x00002000 /* The application allocated the memory for this surface. */
#define SFLAG_ALLOCATED         0x00004000 /* A GL texture is allocated for this surface. */
#define SFLAG_SRGBALLOCATED     0x00008000 /* A sRGB GL texture is allocated for this surface. */
#define SFLAG_PBO               0x00010000 /* The surface has a PBO. */
#define SFLAG_INSYSMEM          0x00020000 /* The system memory copy is current. */
#define SFLAG_INTEXTURE         0x00040000 /* The GL texture is current. */
#define SFLAG_INSRGBTEX         0x00080000 /* The GL sRGB texture is current. */
#define SFLAG_INDRAWABLE        0x00100000 /* The GL drawable is current. */
#define SFLAG_INRB_MULTISAMPLE  0x00200000 /* The multisample renderbuffer is current. */
#define SFLAG_INRB_RESOLVED     0x00400000 /* The resolved renderbuffer is current. */
#define SFLAG_PIN_SYSMEM        0x02000000 /* Keep the surface in sysmem, at the same address. */

/* In some conditions the surface memory must not be freed:
 * SFLAG_CONVERTED: Converting the data back would take too long
 * SFLAG_DIBSECTION: The dib code manages the memory
 * SFLAG_LOCKED: The app requires access to the surface data
 * SFLAG_DYNLOCK: Avoid freeing the data for performance
 * SFLAG_PBO: PBOs don't use 'normal' memory. It is either allocated by the driver or must be NULL.
 * SFLAG_CLIENT: OpenGL uses our memory as backup
 */
#define SFLAG_DONOTFREE     (SFLAG_CONVERTED        | \
                             SFLAG_DYNLOCK          | \
                             SFLAG_LOCKED           | \
                             SFLAG_CLIENT           | \
                             SFLAG_DIBSECTION       | \
                             SFLAG_USERPTR          | \
                             SFLAG_PBO              | \
                             SFLAG_PIN_SYSMEM)

#define SFLAG_LOCATIONS     (SFLAG_INSYSMEM         | \
                             SFLAG_INTEXTURE        | \
                             SFLAG_INSRGBTEX        | \
                             SFLAG_INDRAWABLE       | \
                             SFLAG_INRB_MULTISAMPLE | \
                             SFLAG_INRB_RESOLVED)

typedef enum {
    NO_CONVERSION,
    CONVERT_PALETTED,
    CONVERT_PALETTED_CK,
    CONVERT_CK_565,
    CONVERT_CK_5551,
    CONVERT_CK_RGB24,
    CONVERT_RGB32_888
} CONVERT_TYPES;

HRESULT d3dfmt_get_conv(const struct wined3d_surface *surface, BOOL need_alpha_ck, BOOL use_texturing,
        struct wined3d_format *format, CONVERT_TYPES *convert) DECLSPEC_HIDDEN;
void d3dfmt_p8_init_palette(const struct wined3d_surface *surface, BYTE table[256][4], BOOL colorkey) DECLSPEC_HIDDEN;

struct wined3d_vertex_declaration_element
{
    const struct wined3d_format *format;
    BOOL ffp_valid;
    WORD input_slot;
    WORD offset;
    UINT output_slot;
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

    DWORD                   streams[MAX_STREAMS];
    UINT                    num_streams;
    BOOL                    position_transformed;
    BOOL                    half_float_conv_needed;
};

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

struct wined3d_stream_state
{
    struct wined3d_buffer *buffer;
    UINT offset;
    UINT stride;
    UINT frequency;
    UINT flags;
};

struct wined3d_state
{
    const struct wined3d_fb_state *fb;

    struct wined3d_vertex_declaration *vertex_declaration;
    struct wined3d_stream_state streams[MAX_STREAMS + 1 /* tesselated pseudo-stream */];
    BOOL user_stream;
    struct wined3d_buffer *index_buffer;
    enum wined3d_format_id index_format;
    INT base_vertex_index;
    INT load_base_vertex_index; /* Non-indexed drawing needs 0 here, indexed needs base_vertex_index. */
    GLenum gl_primitive_type;

    struct wined3d_shader *vertex_shader;
    BOOL vs_consts_b[MAX_CONST_B];
    INT vs_consts_i[MAX_CONST_I * 4];
    float *vs_consts_f;

    struct wined3d_shader *pixel_shader;
    BOOL ps_consts_b[MAX_CONST_B];
    INT ps_consts_i[MAX_CONST_I * 4];
    float *ps_consts_f;

    struct wined3d_texture *textures[MAX_COMBINED_SAMPLERS];
    DWORD sampler_states[MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];
    DWORD texture_states[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
    DWORD lowest_disabled_stage;

    struct wined3d_matrix transforms[HIGHEST_TRANSFORMSTATE + 1];
    double clip_planes[MAX_CLIPPLANES][4];
    struct wined3d_material material;
    struct wined3d_viewport viewport;
    RECT scissor_rect;

    /* Light hashmap . Collisions are handled using standard wine double linked lists */
#define LIGHTMAP_SIZE 43 /* Use of a prime number recommended. Set to 1 for a linked list! */
#define LIGHTMAP_HASHFUNC(x) ((x) % LIGHTMAP_SIZE) /* Primitive and simple function */
    struct list light_map[LIGHTMAP_SIZE]; /* Hash map containing the lights */
    const struct wined3d_light_info *lights[MAX_ACTIVE_LIGHTS]; /* Map of opengl lights to d3d lights */

    DWORD render_states[WINEHIGHEST_RENDER_STATE + 1];
};

struct wined3d_stateblock
{
    LONG                      ref;     /* Note: Ref counting not required */
    struct wined3d_device *device;
    enum wined3d_stateblock_type blockType;

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
void stateblock_init_default_state(struct wined3d_stateblock *stateblock) DECLSPEC_HIDDEN;
void stateblock_unbind_resources(struct wined3d_stateblock *stateblock) DECLSPEC_HIDDEN;

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
    HRESULT (*query_issue)(struct wined3d_query *query, DWORD flags);
};

struct wined3d_query
{
    LONG ref;
    const struct wined3d_query_ops *query_ops;
    struct wined3d_device *device;
    enum query_state         state;
    enum wined3d_query_type type;
    DWORD data_size;
    void                     *extendedData;
};

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

#define WINED3D_BUFFER_OPTIMIZED    0x01    /* Optimize has been called for the buffer */
#define WINED3D_BUFFER_HASDESC      0x02    /* A vertex description has been found */
#define WINED3D_BUFFER_CREATEBO     0x04    /* Attempt to create a buffer object next PreLoad */
#define WINED3D_BUFFER_DOUBLEBUFFER 0x08    /* Use a vbo and local allocated memory */
#define WINED3D_BUFFER_FLUSH        0x10    /* Manual unmap flushing */
#define WINED3D_BUFFER_DISCARD      0x20    /* A DISCARD lock has occurred since the last PreLoad */
#define WINED3D_BUFFER_NOSYNC       0x40    /* All locks since the last PreLoad had NOOVERWRITE set */
#define WINED3D_BUFFER_APPLESYNC    0x80    /* Using sync as in GL_APPLE_flush_buffer_range */

struct wined3d_buffer
{
    struct wined3d_resource resource;

    struct wined3d_buffer_desc desc;

    GLuint buffer_object;
    GLenum buffer_object_usage;
    GLenum buffer_type_hint;
    UINT buffer_object_size;
    LONG bind_count;
    DWORD flags;

    LONG lock_count;
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

void buffer_get_memory(struct wined3d_buffer *buffer, const struct wined3d_gl_info *gl_info,
        struct wined3d_bo_address *data) DECLSPEC_HIDDEN;
BYTE *buffer_get_sysmem(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

struct wined3d_rendertarget_view
{
    LONG refcount;

    struct wined3d_resource *resource;
    void *parent;
};

struct wined3d_swapchain_ops
{
    HRESULT (*swapchain_present)(struct wined3d_swapchain *swapchain, const RECT *src_rect,
            const RECT *dst_rect, const RGNDATA *dirty_region, DWORD flags);
};

struct wined3d_swapchain
{
    LONG ref;
    void *parent;
    const struct wined3d_parent_ops *parent_ops;
    const struct wined3d_swapchain_ops *swapchain_ops;
    struct wined3d_device *device;

    struct wined3d_surface **back_buffers;
    struct wined3d_surface *front_buffer;
    struct wined3d_swapchain_desc desc;
    DWORD orig_width, orig_height;
    enum wined3d_format_id orig_fmt;
    struct wined3d_gamma_ramp orig_gamma;
    BOOL render_to_fbo;
    const struct wined3d_format *ds_format;

    LONG prev_time, frames;   /* Performance tracking */

    struct wined3d_context **context;
    unsigned int num_contexts;

    HWND win_handle;
    HWND device_window;

    HDC backup_dc;
    HWND backup_wnd;
};

void x11_copy_to_screen(const struct wined3d_swapchain *swapchain, const RECT *rect) DECLSPEC_HIDDEN;

struct wined3d_context *swapchain_get_context(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_destroy_contexts(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
HDC swapchain_get_backup_dc(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_update_draw_bindings(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;
void swapchain_update_render_to_fbo(struct wined3d_swapchain *swapchain) DECLSPEC_HIDDEN;

#define DEFAULT_REFRESH_RATE 0

/*****************************************************************************
 * Utility function prototypes
 */

/* Trace routines */
const char *debug_d3dformat(enum wined3d_format_id format_id) DECLSPEC_HIDDEN;
const char *debug_d3ddevicetype(enum wined3d_device_type device_type) DECLSPEC_HIDDEN;
const char *debug_d3dresourcetype(enum wined3d_resource_type resource_type) DECLSPEC_HIDDEN;
const char *debug_d3dusage(DWORD usage) DECLSPEC_HIDDEN;
const char *debug_d3dusagequery(DWORD usagequery) DECLSPEC_HIDDEN;
const char *debug_d3ddeclmethod(WINED3DDECLMETHOD method) DECLSPEC_HIDDEN;
const char *debug_d3ddeclusage(BYTE usage) DECLSPEC_HIDDEN;
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
const char *debug_d3dbasis(enum wined3d_basis_type basis) DECLSPEC_HIDDEN;
const char *debug_d3ddegree(enum wined3d_degree_type order) DECLSPEC_HIDDEN;
const char *debug_d3dtop(enum wined3d_texture_op d3dtop) DECLSPEC_HIDDEN;
void dump_color_fixup_desc(struct color_fixup_desc fixup) DECLSPEC_HIDDEN;
const char *debug_surflocation(DWORD flag) DECLSPEC_HIDDEN;

BOOL is_invalid_op(const struct wined3d_state *state, int stage,
        enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3) DECLSPEC_HIDDEN;
void set_tex_op_nvrc(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state,
        BOOL is_alpha, int stage, enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3,
        INT texture_idx, DWORD dst) DECLSPEC_HIDDEN;
void set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords,
        BOOL transformed, enum wined3d_format_id coordtype, BOOL ffp_can_disable_proj) DECLSPEC_HIDDEN;
void texture_activate_dimensions(const struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
void sampler_texdim(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void tex_alphaop(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void apply_pixelshader(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogcolor(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogdensity(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fogstartend(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;
void state_fog_fragpart(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id) DECLSPEC_HIDDEN;

BOOL getColorBits(const struct wined3d_format *format,
        BYTE *redSize, BYTE *greenSize, BYTE *blueSize, BYTE *alphaSize, BYTE *totalSize) DECLSPEC_HIDDEN;
BOOL getDepthStencilBits(const struct wined3d_format *format,
        BYTE *depthSize, BYTE *stencilSize) DECLSPEC_HIDDEN;

/* Math utils */
void multiply_matrix(struct wined3d_matrix *dest, const struct wined3d_matrix *src1,
        const struct wined3d_matrix *src2) DECLSPEC_HIDDEN;
UINT wined3d_log2i(UINT32 x) DECLSPEC_HIDDEN;
unsigned int count_bits(unsigned int mask) DECLSPEC_HIDDEN;

void select_shader_mode(const struct wined3d_gl_info *gl_info, int *ps_selected, int *vs_selected) DECLSPEC_HIDDEN;

struct wined3d_shader_lconst
{
    struct list entry;
    unsigned int idx;
    DWORD value[4];
};

struct wined3d_shader_limits
{
    unsigned int temporary;
    unsigned int texcoord;
    unsigned int sampler;
    unsigned int constant_int;
    unsigned int constant_float;
    unsigned int constant_bool;
    unsigned int address;
    unsigned int packed_output;
    unsigned int packed_input;
    unsigned int attributes;
    unsigned int label;
};

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

/* Base Shader utility functions. */
int shader_addline(struct wined3d_shader_buffer *buffer, const char *fmt, ...) PRINTF_ATTR(2,3) DECLSPEC_HIDDEN;
int shader_vaddline(struct wined3d_shader_buffer *buffer, const char *fmt, va_list args) DECLSPEC_HIDDEN;

/* Vertex shader utility functions */
BOOL vshader_get_input(const struct wined3d_shader *shader,
        BYTE usage_req, BYTE usage_idx_req, unsigned int *regnum) DECLSPEC_HIDDEN;

struct wined3d_vertex_shader
{
    struct wined3d_shader_attribute attributes[MAX_ATTRIBS];
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
    struct wined3d_shader_limits limits;
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

    struct wined3d_shader_signature_element input_signature[max(MAX_ATTRIBS, MAX_REG_INPUT)];
    struct wined3d_shader_signature_element output_signature[MAX_REG_OUTPUT];

    /* Pointer to the parent device */
    struct wined3d_device *device;
    struct list shader_list_entry;

    union
    {
        struct wined3d_vertex_shader vs;
        struct wined3d_pixel_shader ps;
    } u;
};

void pixelshader_update_samplers(struct wined3d_shader_reg_maps *reg_maps,
        struct wined3d_texture * const *textures) DECLSPEC_HIDDEN;
void find_ps_compile_args(const struct wined3d_state *state,
        const struct wined3d_shader *shader, struct ps_compile_args *args) DECLSPEC_HIDDEN;

void find_vs_compile_args(const struct wined3d_state *state,
        const struct wined3d_shader *shader, struct vs_compile_args *args) DECLSPEC_HIDDEN;

void shader_buffer_clear(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
BOOL shader_buffer_init(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
void shader_buffer_free(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
void shader_dump_src_param(const struct wined3d_shader_src_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
void shader_dump_dst_param(const struct wined3d_shader_dst_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps,
        unsigned int max) DECLSPEC_HIDDEN;
void shader_generate_main(const struct wined3d_shader *shader, struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, const DWORD *byte_code, void *backend_ctx) DECLSPEC_HIDDEN;
BOOL shader_match_semantic(const char *semantic_name, WINED3DDECLUSAGE usage) DECLSPEC_HIDDEN;

static inline BOOL shader_is_pshader_version(enum wined3d_shader_type type)
{
    return type == WINED3D_SHADER_TYPE_PIXEL;
}

static inline BOOL shader_is_vshader_version(enum wined3d_shader_type type)
{
    return type == WINED3D_SHADER_TYPE_VERTEX;
}

static inline BOOL shader_is_scalar(const struct wined3d_shader_register *reg)
{
    switch (reg->type)
    {
        case WINED3DSPR_RASTOUT:
            /* oFog & oPts */
            if (reg->idx) return TRUE;
            /* oPos */
            return FALSE;

        case WINED3DSPR_DEPTHOUT:   /* oDepth */
        case WINED3DSPR_CONSTBOOL:  /* b# */
        case WINED3DSPR_LOOP:       /* aL */
        case WINED3DSPR_PREDICATE:  /* p0 */
            return TRUE;

        case WINED3DSPR_MISCTYPE:
            switch(reg->idx)
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
    position_fixup[0] = 1.0f;
    position_fixup[1] = 1.0f;
    position_fixup[2] = (63.0f / 64.0f) / state->viewport.width;
    position_fixup[3] = -(63.0f / 64.0f) / state->viewport.height;

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

/* sRGB correction constants */
static const float srgb_cmp = 0.0031308f;
static const float srgb_mul_low = 12.92f;
static const float srgb_pow = 0.41666f;
static const float srgb_mul_high = 1.055f;
static const float srgb_sub_high = 0.055f;

struct wined3d_palette
{
    LONG ref;
    void *parent;
    struct wined3d_device *device;

    HPALETTE                   hpal;
    WORD                       palVersion;     /*|               */
    WORD                       palNumEntries;  /*|  LOGPALETTE   */
    PALETTEENTRY               palents[256];   /*|               */
    /* This is to store the palette in 'screen format' */
    int                        screen_palents[256];
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
#define WINED3DFMT_FLAG_FOURCC                      0x00000020
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

struct wined3d_format
{
    enum wined3d_format_id id;

    DWORD red_mask;
    DWORD green_mask;
    DWORD blue_mask;
    DWORD alpha_mask;
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
    unsigned int flags;
    float heightscale;
    struct color_fixup_desc color_fixup;
    void (*convert)(const BYTE *src, BYTE *dst, UINT pitch, UINT width, UINT height);
};

const struct wined3d_format *wined3d_get_format(const struct wined3d_gl_info *gl_info,
        enum wined3d_format_id format_id) DECLSPEC_HIDDEN;
UINT wined3d_format_calculate_size(const struct wined3d_format *format,
        UINT alignment, UINT width, UINT height) DECLSPEC_HIDDEN;
DWORD wined3d_format_convert_from_float(const struct wined3d_surface *surface,
        const struct wined3d_color *color) DECLSPEC_HIDDEN;

static inline BOOL use_vs(const struct wined3d_state *state)
{
    /* Check stateblock->vertexDecl to allow this to be used from
     * IWineD3DDeviceImpl_FindTexUnitMap(). This is safe because
     * stateblock->vertexShader implies a vertex declaration instead of ddraw
     * style strided data. */
    return state->vertex_shader && !state->vertex_declaration->position_transformed;
}

static inline BOOL use_ps(const struct wined3d_state *state)
{
    return !!state->pixel_shader;
}

static inline void context_apply_state(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct StateEntry *state_table = context->state_table;
    DWORD rep = state_table[state_id].representative;
    state_table[rep].apply(context, state, rep);
}

/* The WNDCLASS-Name for the fake window which we use to retrieve the GL capabilities */
#define WINED3D_OPENGL_WINDOW_CLASS_NAME "WineD3D_OpenGL"

#define MAKEDWORD_VERSION(maj, min) (((maj & 0xffff) << 16) | (min & 0xffff))

#endif
