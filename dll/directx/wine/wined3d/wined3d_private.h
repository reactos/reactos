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
    COMPLEX_FIXUP_YUY2 = 0,
    COMPLEX_FIXUP_UYVY = 1,
    COMPLEX_FIXUP_YV12 = 2,
    COMPLEX_FIXUP_P8   = 3,
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
    GLenum mip[WINED3DTEXF_LINEAR + 1];
};

const struct min_lookup minMipLookup[WINED3DTEXF_LINEAR + 1] DECLSPEC_HIDDEN;
const struct min_lookup minMipLookup_noFilter[WINED3DTEXF_LINEAR + 1] DECLSPEC_HIDDEN;
const struct min_lookup minMipLookup_noMip[WINED3DTEXF_LINEAR + 1] DECLSPEC_HIDDEN;
const GLenum magLookup[WINED3DTEXF_LINEAR + 1] DECLSPEC_HIDDEN;
const GLenum magLookup_noFilter[WINED3DTEXF_LINEAR + 1] DECLSPEC_HIDDEN;

static inline GLenum wined3d_gl_mag_filter(const GLenum mag_lookup[], WINED3DTEXTUREFILTERTYPE mag_filter)
{
    return mag_lookup[mag_filter];
}

static inline GLenum wined3d_gl_min_mip_filter(const struct min_lookup min_mip_lookup[],
        WINED3DTEXTUREFILTERTYPE min_filter, WINED3DTEXTUREFILTERTYPE mip_filter)
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
        else return sgn * pow(2, -14.0f) * ((float)m / 1024.0f);
    } else if(e < 31) {
        return sgn * pow(2, (float)e - 15.0f) * (1.0f + ((float)m / 1024.0f));
    } else {
        if(m == 0) return sgn / 0.0f; /* +INF / -INF */
        else return 0.0f / 0.0f; /* NAN */
    }
}

static inline float float_24_to_32(DWORD in)
{
    const float sgn = in & 0x800000 ? -1.0f : 1.0f;
    const unsigned short e = (in & 0x780000) >> 19;
    const unsigned short m = in & 0x7ffff;

    if (e == 0)
    {
        if (m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * pow(2, -6.0f) * ((float)m / 524288.0f);
    }
    else if (e < 15)
    {
        return sgn * pow(2, (float)e - 7.0f) * (1.0f + ((float)m / 524288.0f));
    }
    else
    {
        if (m == 0) return sgn / 0.0f; /* +INF / -INF */
        else return 0.0f / 0.0f; /* NAN */
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
#define ORM_PBUFFER     1
#define ORM_FBO         2

#define SHADER_ARB  1
#define SHADER_GLSL 2
#define SHADER_ATI  3
#define SHADER_NONE 4

#define RTL_DISABLE   -1
#define RTL_READDRAW   1
#define RTL_READTEX    2

#define PCI_VENDOR_NONE 0xffff /* e.g. 0x8086 for Intel and 0x10de for Nvidia */
#define PCI_DEVICE_NONE 0xffff /* e.g. 0x14f for a Geforce6200 */

/* NOTE: When adding fields to this structure, make sure to update the default
 * values in wined3d_main.c as well. */
typedef struct wined3d_settings_s {
/* vertex and pixel shader modes */
  int vs_mode;
  int ps_mode;
/* Ideally, we don't want the user to have to request GLSL.  If the hardware supports GLSL,
    we should use it.  However, until it's fully implemented, we'll leave it as a registry
    setting for developers. */
  BOOL glslRequested;
  int offscreen_rendering_mode;
  int rendertargetlock_mode;
  unsigned short pci_vendor_id;
  unsigned short pci_device_id;
/* Memory tracking and object counting */
  unsigned int emulated_textureram;
  char *logo;
  int allow_multisampling;
} wined3d_settings_t;

extern wined3d_settings_t wined3d_settings DECLSPEC_HIDDEN;

typedef enum _WINED3DSAMPLER_TEXTURE_TYPE
{
    WINED3DSTT_UNKNOWN = 0,
    WINED3DSTT_1D = 1,
    WINED3DSTT_2D = 2,
    WINED3DSTT_CUBE = 3,
    WINED3DSTT_VOLUME = 4,
} WINED3DSAMPLER_TEXTURE_TYPE;

typedef enum _WINED3DSHADER_PARAM_REGISTER_TYPE
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
} WINED3DSHADER_PARAM_REGISTER_TYPE;

enum wined3d_immconst_type
{
    WINED3D_IMMCONST_FLOAT,
    WINED3D_IMMCONST_FLOAT4,
};

#define WINED3DSP_NOSWIZZLE (0 | (1 << 2) | (2 << 4) | (3 << 6))

typedef enum _WINED3DSHADER_PARAM_SRCMOD_TYPE
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
} WINED3DSHADER_PARAM_SRCMOD_TYPE;

#define WINED3DSP_WRITEMASK_0   0x1 /* .x r */
#define WINED3DSP_WRITEMASK_1   0x2 /* .y g */
#define WINED3DSP_WRITEMASK_2   0x4 /* .z b */
#define WINED3DSP_WRITEMASK_3   0x8 /* .w a */
#define WINED3DSP_WRITEMASK_ALL 0xf /* all */

typedef enum _WINED3DSHADER_PARAM_DSTMOD_TYPE
{
    WINED3DSPDM_NONE = 0,
    WINED3DSPDM_SATURATE = 1,
    WINED3DSPDM_PARTIALPRECISION = 2,
    WINED3DSPDM_MSAMPCENTROID = 4,
} WINED3DSHADER_PARAM_DSTMOD_TYPE;

/* Undocumented opcode control to identify projective texture lookups in ps 2.0 and later */
#define WINED3DSI_TEXLD_PROJECT 1
#define WINED3DSI_TEXLD_BIAS    2

typedef enum COMPARISON_TYPE
{
    COMPARISON_GT = 1,
    COMPARISON_EQ = 2,
    COMPARISON_GE = 3,
    COMPARISON_LT = 4,
    COMPARISON_NE = 5,
    COMPARISON_LE = 6,
} COMPARISON_TYPE;

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
    WINED3DSIH_EXP,
    WINED3DSIH_EXPP,
    WINED3DSIH_FRC,
    WINED3DSIH_IADD,
    WINED3DSIH_IF,
    WINED3DSIH_IFC,
    WINED3DSIH_IGE,
    WINED3DSIH_LABEL,
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
    WINED3DSIH_MUL,
    WINED3DSIH_NOP,
    WINED3DSIH_NRM,
    WINED3DSIH_PHASE,
    WINED3DSIH_POW,
    WINED3DSIH_RCP,
    WINED3DSIH_REP,
    WINED3DSIH_RET,
    WINED3DSIH_RSQ,
    WINED3DSIH_SETP,
    WINED3DSIH_SGE,
    WINED3DSIH_SGN,
    WINED3DSIH_SINCOS,
    WINED3DSIH_SLT,
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

typedef struct shader_reg_maps
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

    WINED3DSAMPLER_TEXTURE_TYPE sampler_type[max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS)];
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
    WORD padding        : 4;

    /* Whether or not loops are used in this shader, and nesting depth */
    unsigned loop_depth;
    unsigned highest_render_target;

} shader_reg_maps;

struct wined3d_shader_context
{
    IWineD3DBaseShader *shader;
    const struct wined3d_gl_info *gl_info;
    const struct shader_reg_maps *reg_maps;
    struct wined3d_shader_buffer *buffer;
    void *backend_data;
};

struct wined3d_shader_register
{
    WINED3DSHADER_PARAM_REGISTER_TYPE type;
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
    DWORD modifiers;
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
    WINED3DSAMPLER_TEXTURE_TYPE sampler_type;
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

    WINED3DVSHADERCAPS2_0   VS20Caps;
    WINED3DPSHADERCAPS2_0   PS20Caps;

    DWORD               MaxVShaderInstructionsExecuted;
    DWORD               MaxPShaderInstructionsExecuted;
    DWORD               MaxVertexShader30InstructionSlots;
    DWORD               MaxPixelShader30InstructionSlots;

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
struct ps_compile_args {
    struct color_fixup_desc     color_fixup[MAX_FRAGMENT_SAMPLERS];
    enum vertexprocessing_mode  vp_mode;
    enum fogmode                fog;
    /* Projected textures(ps 1.0-1.3) */
    /* Texture types(2D, Cube, 3D) in ps 1.x */
    BOOL                        srgb_correction;
    WORD                        np2_fixup;
    /* Bitmap for NP2 texcoord fixups (16 samplers max currently).
       D3D9 has a limit of 16 samplers and the fixup is superfluous
       in D3D10 (unconditional NP2 support mandatory). */
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

typedef struct {
    void (*shader_handle_instruction)(const struct wined3d_shader_instruction *);
    void (*shader_select)(const struct wined3d_context *context, BOOL usePS, BOOL useVS);
    void (*shader_select_depth_blt)(IWineD3DDevice *iface, enum tex_types tex_type);
    void (*shader_deselect_depth_blt)(IWineD3DDevice *iface);
    void (*shader_update_float_vertex_constants)(IWineD3DDevice *iface, UINT start, UINT count);
    void (*shader_update_float_pixel_constants)(IWineD3DDevice *iface, UINT start, UINT count);
    void (*shader_load_constants)(const struct wined3d_context *context, char usePS, char useVS);
    void (*shader_load_np2fixup_constants)(IWineD3DDevice *iface, char usePS, char useVS);
    void (*shader_destroy)(IWineD3DBaseShader *iface);
    HRESULT (*shader_alloc_private)(IWineD3DDevice *iface);
    void (*shader_free_private)(IWineD3DDevice *iface);
    BOOL (*shader_dirtifyable_constants)(IWineD3DDevice *iface);
    void (*shader_get_caps)(const struct wined3d_gl_info *gl_info, struct shader_caps *caps);
    BOOL (*shader_color_fixup_supported)(struct color_fixup_desc fixup);
    void (*shader_add_instruction_modifiers)(const struct wined3d_shader_instruction *ins);
} shader_backend_t;

extern const shader_backend_t glsl_shader_backend DECLSPEC_HIDDEN;
extern const shader_backend_t arb_program_shader_backend DECLSPEC_HIDDEN;
extern const shader_backend_t none_shader_backend DECLSPEC_HIDDEN;

/* X11 locking */

extern void (* CDECL wine_tsx11_lock_ptr)(void) DECLSPEC_HIDDEN;
extern void (* CDECL wine_tsx11_unlock_ptr)(void) DECLSPEC_HIDDEN;

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
#define GL_EXTCALL(FuncName)          (GLINFO_LOCATION.FuncName)

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

/* DirectX Device Limits */
/* --------------------- */
#define MAX_MIP_LEVELS 32  /* Maximum number of mipmap levels. */
#define HIGHEST_TRANSFORMSTATE WINED3DTS_WORLDMATRIX(255) /* Highest value in WINED3DTRANSFORMSTATETYPE */

/* Checking of API calls */
/* --------------------- */
#ifndef WINE_NO_DEBUG_MSGS
#define checkGLcall(A)                                              \
do {                                                                \
    GLint err;                                                      \
    if(!__WINE_IS_DEBUG_ON(_FIXME, __wine_dbch___default)) break;   \
    err = glGetError();                                             \
    if (err == GL_NO_ERROR) {                                       \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__);        \
                                                                    \
    } else do {                                                     \
        FIXME(">>>>>>>>>>>>>>>>> %s (%#x) from %s @ %s / %d\n",     \
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
        TRACE( #name "=(data:%p, stride:%d, format:%#x, vbo %d, stream %u)\n", \
        si->elements[name].data, si->elements[name].stride, si->elements[name].format_desc->format, \
        si->elements[name].buffer_object, si->elements[name].stream_idx); } while(0)

/* Advance declaration of structures to satisfy compiler */
typedef struct IWineD3DStateBlockImpl IWineD3DStateBlockImpl;
typedef struct IWineD3DSurfaceImpl    IWineD3DSurfaceImpl;
typedef struct IWineD3DPaletteImpl    IWineD3DPaletteImpl;
typedef struct IWineD3DDeviceImpl     IWineD3DDeviceImpl;
typedef struct IWineD3DSwapChainImpl  IWineD3DSwapChainImpl;

/* Global variables */
extern const float identity[16] DECLSPEC_HIDDEN;

/*****************************************************************************
 * Compilable extra diagnostics
 */

/* Trace information per-vertex: (extremely high amount of trace) */
#if 0 /* NOTE: Must be 0 in cvs */
# define VTRACE(A) TRACE A
#else
# define VTRACE(A)
#endif

/* TODO: Confirm each of these works when wined3d move completed */
#if 0 /* NOTE: Must be 0 in cvs */
  /* To avoid having to get gigabytes of trace, the following can be compiled in, and at the start
     of each frame, a check is made for the existence of C:\D3DTRACE, and if it exists d3d trace
     is enabled, and if it doesn't exist it is disabled. */
# define FRAME_DEBUGGING
  /*  Adding in the SINGLE_FRAME_DEBUGGING gives a trace of just what makes up a single frame, before
      the file is deleted                                                                            */
# if 1 /* NOTE: Must be 1 in cvs, as this is mostly more useful than a trace from program start */
#  define SINGLE_FRAME_DEBUGGING
# endif
  /* The following, when enabled, lets you see the makeup of the frame, by drawprimitive calls.
     It can only be enabled when FRAME_DEBUGGING is also enabled
     The contents of the back buffer are written into /tmp/backbuffer_* after each primitive
     array is drawn.                                                                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */
#  define SHOW_FRAME_MAKEUP 1
# endif
  /* The following, when enabled, lets you see the makeup of the all the textures used during each
     of the drawprimitive calls. It can only be enabled when SHOW_FRAME_MAKEUP is also enabled.
     The contents of the textures assigned to each stage are written into
     /tmp/texture_*_<Stage>.ppm after each primitive array is drawn.                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */
#  define SHOW_TEXTURE_MAKEUP 0
# endif
extern BOOL isOn;
extern BOOL isDumpingFrames;
extern LONG primCounter;
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

struct wined3d_stream_info_element
{
    const struct GlPixelFormatDesc *format_desc;
    GLsizei stride;
    const BYTE *data;
    UINT stream_idx;
    GLuint buffer_object;
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
void drawPrimitive(IWineD3DDevice *iface, UINT index_count,
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
#define STATE_IS_TRANSFORM(a) ((a) >= STATE_TRANSFORM(1) && (a) <= STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(255)))

#define STATE_STREAMSRC (STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(255)) + 1)
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

#define STATE_HIGHEST (STATE_FRONTFACE)

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

struct wined3d_context
{
    const struct wined3d_gl_info *gl_info;
    /* State dirtification
     * dirtyArray is an array that contains markers for dirty states. numDirtyEntries states are dirty, their numbers are in indices
     * 0...numDirtyEntries - 1. isStateDirty is a redundant copy of the dirtyArray. Technically only one of them would be needed,
     * but with the help of both it is easy to find out if a state is dirty(just check the array index), and for applying dirty states
     * only numDirtyEntries array elements have to be checked, not STATE_HIGHEST states.
     */
    DWORD                   dirtyArray[STATE_HIGHEST + 1]; /* Won't get bigger than that, a state is never marked dirty 2 times */
    DWORD                   numDirtyEntries;
    DWORD isStateDirty[STATE_HIGHEST / (sizeof(DWORD) * CHAR_BIT) + 1]; /* Bitmap to find out quickly if a state is dirty */

    IWineD3DSurface         *surface;
    IWineD3DSurface *current_rt;
    DWORD                   tid;    /* Thread ID which owns this context at the moment */

    /* Stores some information about the context state for optimization */
    WORD render_offscreen : 1;
    WORD draw_buffer_dirty : 1;
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
    BYTE texShaderBumpMap;              /* MAX_TEXTURES, 8 */
    BYTE lastWasPow2Texture;            /* MAX_TEXTURES, 8 */
    DWORD                   numbered_array_mask;
    GLenum                  tracking_parm;     /* Which source is tracking current colour         */
    GLenum                  untracked_materials[2];
    UINT                    blit_w, blit_h;
    enum fogsource          fog_source;

    char                    *vshader_const_dirty, *pshader_const_dirty;

    /* The actual opengl context */
    UINT level;
    HGLRC restore_ctx;
    HDC restore_dc;
    HGLRC                   glCtx;
    HWND                    win_handle;
    HDC                     hdc;
    HPBUFFERARB             pbuffer;
    GLint                   aux_buffers;

    /* FBOs */
    UINT                    fbo_entry_count;
    struct list             fbo_list;
    struct list             fbo_destroy_list;
    struct fbo_entry        *current_fbo;
    GLuint                  src_fbo;
    GLuint                  dst_fbo;
    GLuint                  fbo_read_binding;
    GLuint                  fbo_draw_binding;

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

typedef void (*APPLYSTATEFUNC)(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *ctx);

struct StateEntry
{
    DWORD representative;
    APPLYSTATEFUNC apply;
};

struct StateEntryTemplate
{
    DWORD state;
    struct StateEntry content;
    GL_SupportedExt extension;
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
    void (*enable_extension)(IWineD3DDevice *iface, BOOL enable);
    void (*get_caps)(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps);
    HRESULT (*alloc_private)(IWineD3DDevice *iface);
    void (*free_private)(IWineD3DDevice *iface);
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

/* Shaders for color conversions in blits */
struct blit_shader
{
    HRESULT (*alloc_private)(IWineD3DDevice *iface);
    void (*free_private)(IWineD3DDevice *iface);
    HRESULT (*set_shader)(IWineD3DDevice *iface, const struct GlPixelFormatDesc *format_desc,
            GLenum textype, UINT width, UINT height);
    void (*unset_shader)(IWineD3DDevice *iface);
    BOOL (*color_fixup_supported)(struct color_fixup_desc fixup);
};

extern const struct blit_shader ffp_blit DECLSPEC_HIDDEN;
extern const struct blit_shader arbfp_blit DECLSPEC_HIDDEN;

typedef enum ContextUsage {
    CTXUSAGE_RESOURCELOAD       = 1,    /* Only loads textures: No State is applied */
    CTXUSAGE_DRAWPRIM           = 2,    /* OpenGL states are set up for blitting DirectDraw surfaces */
    CTXUSAGE_BLIT               = 3,    /* OpenGL states are set up 3D drawing */
    CTXUSAGE_CLEAR              = 4,    /* Drawable and states are set up for clearing */
} ContextUsage;

struct wined3d_context *context_acquire(IWineD3DDeviceImpl *This,
        IWineD3DSurface *target, enum ContextUsage usage) DECLSPEC_HIDDEN;
void context_alloc_event_query(struct wined3d_context *context,
        struct wined3d_event_query *query) DECLSPEC_HIDDEN;
void context_alloc_occlusion_query(struct wined3d_context *context,
        struct wined3d_occlusion_query *query) DECLSPEC_HIDDEN;
void context_resource_released(IWineD3DDevice *iface,
        IWineD3DResource *resource, WINED3DRESOURCETYPE type) DECLSPEC_HIDDEN;
void context_bind_fbo(struct wined3d_context *context, GLenum target, GLuint *fbo) DECLSPEC_HIDDEN;
void context_attach_depth_stencil_fbo(struct wined3d_context *context,
        GLenum fbo_target, IWineD3DSurface *depth_stencil, BOOL use_render_buffer) DECLSPEC_HIDDEN;
void context_attach_surface_fbo(const struct wined3d_context *context,
        GLenum fbo_target, DWORD idx, IWineD3DSurface *surface) DECLSPEC_HIDDEN;
struct wined3d_context *context_create(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, HWND win,
        BOOL create_pbuffer, const WINED3DPRESENT_PARAMETERS *present_parameters) DECLSPEC_HIDDEN;
void context_destroy(IWineD3DDeviceImpl *This, struct wined3d_context *context) DECLSPEC_HIDDEN;
void context_free_event_query(struct wined3d_event_query *query) DECLSPEC_HIDDEN;
void context_free_occlusion_query(struct wined3d_occlusion_query *query) DECLSPEC_HIDDEN;
struct wined3d_context *context_get_current(void) DECLSPEC_HIDDEN;
DWORD context_get_tls_idx(void) DECLSPEC_HIDDEN;
void context_release(struct wined3d_context *context) DECLSPEC_HIDDEN;
BOOL context_set_current(struct wined3d_context *ctx) DECLSPEC_HIDDEN;
void context_set_draw_buffer(struct wined3d_context *context, GLenum buffer) DECLSPEC_HIDDEN;
void context_set_tls_idx(DWORD idx) DECLSPEC_HIDDEN;

void delete_opengl_contexts(IWineD3DDevice *iface, IWineD3DSwapChain *swapchain) DECLSPEC_HIDDEN;
HRESULT create_primary_opengl_context(IWineD3DDevice *iface, IWineD3DSwapChain *swapchain) DECLSPEC_HIDDEN;

/* Macros for doing basic GPU detection based on opengl capabilities */
#define WINE_D3D6_CAPABLE(gl_info) (gl_info->supported[ARB_MULTITEXTURE])
#define WINE_D3D7_CAPABLE(gl_info) (gl_info->supported[ARB_TEXTURE_COMPRESSION] && gl_info->supported[ARB_TEXTURE_CUBE_MAP] && gl_info->supported[ARB_TEXTURE_ENV_DOT3])
#define WINE_D3D8_CAPABLE(gl_info) WINE_D3D7_CAPABLE(gl_info) && (gl_info->supported[ARB_MULTISAMPLE] && gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
#define WINE_D3D9_CAPABLE(gl_info) WINE_D3D8_CAPABLE(gl_info) && (gl_info->supported[ARB_FRAGMENT_PROGRAM] && gl_info->supported[ARB_VERTEX_SHADER])

/*****************************************************************************
 * Internal representation of a light
 */
struct wined3d_light_info
{
    WINED3DLIGHT OriginalParms; /* Note D3D8LIGHT == D3D9LIGHT */
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
extern const WINED3DLIGHT WINED3D_default_light DECLSPEC_HIDDEN;

typedef struct WineD3D_PixelFormat
{
    int iPixelFormat; /* WGL pixel format */
    int iPixelType; /* WGL pixel type e.g. WGL_TYPE_RGBA_ARB, WGL_TYPE_RGBA_FLOAT_ARB or WGL_TYPE_COLORINDEX_ARB */
    int redSize, greenSize, blueSize, alphaSize, colorSize;
    int depthSize, stencilSize;
    BOOL windowDrawable;
    BOOL pbufferDrawable;
    BOOL doubleBuffer;
    int auxBuffers;
    int numSamples;
} WineD3D_PixelFormat;

enum wined3d_gl_vendor
{
    GL_VENDOR_WINE,
    GL_VENDOR_APPLE,
    GL_VENDOR_ATI,
    GL_VENDOR_INTEL,
    GL_VENDOR_MESA,
    GL_VENDOR_NVIDIA,
};


enum wined3d_pci_vendor
{
    HW_VENDOR_WINE                     = 0x0000,
    HW_VENDOR_ATI                      = 0x1002,
    HW_VENDOR_NVIDIA                   = 0x10de,
    HW_VENDOR_INTEL                    = 0x8086,
};

enum wined3d_pci_device
{
    CARD_WINE                       = 0x0000,

    CARD_ATI_RAGE_128PRO            = 0x5246,
    CARD_ATI_RADEON_7200            = 0x5144,
    CARD_ATI_RADEON_8500            = 0x514c,
    CARD_ATI_RADEON_9500            = 0x4144,
    CARD_ATI_RADEON_XPRESS_200M     = 0x5955,
    CARD_ATI_RADEON_X700            = 0x5e4c,
    CARD_ATI_RADEON_X1600           = 0x71c2,
    CARD_ATI_RADEON_HD2300          = 0x7210,
    CARD_ATI_RADEON_HD2600          = 0x9581,
    CARD_ATI_RADEON_HD2900          = 0x9400,
    CARD_ATI_RADEON_HD3200          = 0x9620,
    CARD_ATI_RADEON_HD4350          = 0x954f,
    CARD_ATI_RADEON_HD4550          = 0x9540,
    CARD_ATI_RADEON_HD4600          = 0x9495,
    CARD_ATI_RADEON_HD4650          = 0x9498,
    CARD_ATI_RADEON_HD4670          = 0x9490,
    CARD_ATI_RADEON_HD4700          = 0x944e,
    CARD_ATI_RADEON_HD4770          = 0x94b3,
    CARD_ATI_RADEON_HD4800          = 0x944c, /* Picked one value between 9440, 944c, 9442, 9460 */
    CARD_ATI_RADEON_HD4830          = 0x944c,
    CARD_ATI_RADEON_HD4850          = 0x9442,
    CARD_ATI_RADEON_HD4870          = 0x9440,
    CARD_ATI_RADEON_HD4890          = 0x9460,
    CARD_ATI_RADEON_HD5700          = 0x68BE, /* Picked HD5750 */
    CARD_ATI_RADEON_HD5750          = 0x68BE,
    CARD_ATI_RADEON_HD5770          = 0x68B8,
    CARD_ATI_RADEON_HD5800          = 0x6898, /* Picked HD5850 */
    CARD_ATI_RADEON_HD5850          = 0x6898,
    CARD_ATI_RADEON_HD5870          = 0x6899,

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
    CARD_NVIDIA_GEFORCE_9200        = 0x086d,
    CARD_NVIDIA_GEFORCE_9400GT      = 0x042c,
    CARD_NVIDIA_GEFORCE_9500GT      = 0x0640,
    CARD_NVIDIA_GEFORCE_9600GT      = 0x0622,
    CARD_NVIDIA_GEFORCE_9800GT      = 0x0614,
    CARD_NVIDIA_GEFORCE_GTX260      = 0x05e2,
    CARD_NVIDIA_GEFORCE_GTX275      = 0x05e6,
    CARD_NVIDIA_GEFORCE_GTX280      = 0x05e1,
    CARD_NVIDIA_GEFORCE_GT240       = 0x0ca3,

    CARD_INTEL_845G                 = 0x2562,
    CARD_INTEL_I830G                = 0x3577,
    CARD_INTEL_I855G                = 0x3582,
    CARD_INTEL_I865G                = 0x2572,
    CARD_INTEL_I915G                = 0x2582,
    CARD_INTEL_I915GM               = 0x2592,
    CARD_INTEL_I945GM               = 0x27a2, /* Same as GMA 950? */
    CARD_INTEL_X3100                = 0x2a02, /* Found in Macs. Same as GMA 965? */
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
    UINT point_sprite_units;
    UINT blends;
    UINT anisotropy;
    float shininess;

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
    UINT vidmem;
    struct wined3d_gl_limits limits;
    DWORD reserved_glsl_constants;
    DWORD quirks;
    BOOL supported[WINED3D_GL_EXT_COUNT];
    GLint wrap_lookup[WINED3DTADDRESS_MIRRORONCE - WINED3DTADDRESS_WRAP + 1];

    struct wined3d_fbo_ops fbo_ops;
#define USE_GL_FUNC(type, pfn, ext, replace) type pfn;
    /* GL function pointers */
    GL_EXT_FUNCS_GEN
    /* WGL function pointers */
    WGL_EXT_FUNCS_GEN
#undef USE_GL_FUNC

    struct GlPixelFormatDesc *gl_formats;
};

struct wined3d_driver_info
{
    enum wined3d_pci_vendor vendor;
    enum wined3d_pci_device device;
    const char *name;
    const char *description;
    DWORD version_high;
    DWORD version_low;
};

/* The adapter structure */
struct wined3d_adapter
{
    UINT ordinal;
    BOOL                    opengl;
    POINT                   monitorPoint;
    struct wined3d_gl_info  gl_info;
    struct wined3d_driver_info driver_info;
    WCHAR                   DeviceName[CCHDEVICENAME]; /* DeviceName for use with e.g. ChangeDisplaySettings */
    int                     nCfgs;
    WineD3D_PixelFormat     *cfgs;
    BOOL                    brokenStencil; /* Set on cards which only offer mixed depth+stencil */
    unsigned int            TextureRam; /* Amount of texture memory both video ram + AGP/TurboCache/HyperMemory/.. */
    unsigned int            UsedTextureRam;
    LUID luid;

    const struct fragment_pipeline *fragment_pipe;
    const shader_backend_t *shader_backend;
    const struct blit_shader *blitter;
};

BOOL initPixelFormats(struct wined3d_gl_info *gl_info, enum wined3d_pci_vendor vendor) DECLSPEC_HIDDEN;
BOOL initPixelFormatsNoGL(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
extern long WineD3DAdapterChangeGLRam(IWineD3DDeviceImpl *D3DDevice, long glram) DECLSPEC_HIDDEN;
extern void add_gl_compat_wrappers(struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

/*****************************************************************************
 * High order patch management
 */
struct WineD3DRectPatch
{
    UINT                            Handle;
    float                          *mem;
    WineDirect3DVertexStridedData   strided;
    WINED3DRECTPATCH_INFO           RectPatchInfo;
    float                           numSegs[4];
    char                            has_normals, has_texcoords;
    struct list                     entry;
};

HRESULT tesselate_rectpatch(IWineD3DDeviceImpl *This, struct WineD3DRectPatch *patch) DECLSPEC_HIDDEN;

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

void gen_ffp_frag_op(IWineD3DStateBlockImpl *stateblock, struct ffp_frag_settings *settings,
        BOOL ignore_textype) DECLSPEC_HIDDEN;
const struct ffp_frag_desc *find_ffp_frag_shader(const struct wine_rb_tree *fragment_shaders,
        const struct ffp_frag_settings *settings) DECLSPEC_HIDDEN;
void add_ffp_frag_shader(struct wine_rb_tree *shaders, struct ffp_frag_desc *desc) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3D implementation structure
 */
typedef struct IWineD3DImpl
{
    /* IUnknown fields */
    const IWineD3DVtbl     *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information */
    IUnknown               *parent;
    UINT                    dxVersion;

    UINT adapter_count;
    struct wined3d_adapter adapters[1];
} IWineD3DImpl;

extern const IWineD3DVtbl IWineD3D_Vtbl DECLSPEC_HIDDEN;

BOOL wined3d_register_window(HWND window, struct IWineD3DDeviceImpl *device) DECLSPEC_HIDDEN;
void wined3d_unregister_window(HWND window) DECLSPEC_HIDDEN;
BOOL InitAdapters(IWineD3DImpl *This) DECLSPEC_HIDDEN;

/* A helper function that dumps a resource list */
void dumpResources(struct list *list) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DDevice implementation structure
 */
#define WINED3D_UNMAPPED_STAGE ~0U

/* Multithreaded flag. Removed from the public header to signal that IWineD3D::CreateDevice ignores it */
#define WINED3DCREATE_MULTITHREADED 0x00000004

struct IWineD3DDeviceImpl
{
    /* IUnknown fields      */
    const IWineD3DDeviceVtbl *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information  */
    IUnknown               *parent;
    IWineD3DDeviceParent   *device_parent;
    IWineD3D *wined3d;
    struct wined3d_adapter *adapter;

    /* Window styles to restore when switching fullscreen mode */
    LONG                    style;
    LONG                    exStyle;

    /* X and GL Information */
    GLint                   maxConcurrentLights;
    GLenum                  offscreenBuffer;

    /* Selected capabilities */
    int vs_selected_mode;
    int ps_selected_mode;
    const shader_backend_t *shader_backend;
    void *shader_priv;
    void *fragment_priv;
    void *blit_priv;
    struct StateEntry StateTable[STATE_HIGHEST + 1];
    /* Array of functions for states which are handled by more than one pipeline part */
    APPLYSTATEFUNC *multistate_funcs[STATE_HIGHEST + 1];
    const struct fragment_pipeline *frag_pipe;
    const struct blit_shader *blitter;

    unsigned int max_ffp_textures;
    DWORD d3d_vshader_constantF, d3d_pshader_constantF; /* Advertised d3d caps, not GL ones */
    DWORD vs_clipping;

    WORD view_ident : 1;                /* true iff view matrix is identity */
    WORD untransformed : 1;
    WORD vertexBlendUsed : 1;           /* To avoid needless setting of the blend matrices */
    WORD isRecordingState : 1;
    WORD isInDraw : 1;
    WORD bCursorVisible : 1;
    WORD haveHardwareCursor : 1;
    WORD d3d_initialized : 1;
    WORD inScene : 1;                   /* A flag to check for proper BeginScene / EndScene call pairs */
    WORD softwareVertexProcessing : 1;  /* process vertex shaders using software or hardware */
    WORD useDrawStridedSlow : 1;
    WORD instancedDraw : 1;
    WORD filter_messages : 1;
    WORD padding : 3;

    BYTE fixed_function_usage_map;      /* MAX_TEXTURES, 8 */

#define DDRAW_PITCH_ALIGNMENT 8
#define D3D8_PITCH_ALIGNMENT 4
    unsigned char           surface_alignment; /* Line Alignment of surfaces                      */

    /* State block related */
    IWineD3DStateBlockImpl *stateBlock;
    IWineD3DStateBlockImpl *updateStateBlock;

    /* Internal use fields  */
    WINED3DDEVICE_CREATION_PARAMETERS createParms;
    WINED3DDEVTYPE                  devType;
    HWND focus_window;

    IWineD3DSwapChain     **swapchains;
    UINT                    NumberOfSwapChains;

    struct list             resources; /* a linked list to track resources created by the device */
    struct list             shaders;   /* a linked list to track shaders (pixel and vertex)      */
    unsigned int            highest_dirty_ps_const, highest_dirty_vs_const;

    /* Render Target Support */
    IWineD3DSurface       **render_targets;
    IWineD3DSurface        *auto_depth_stencil_buffer;
    IWineD3DSurface        *stencilBufferTarget;

    /* palettes texture management */
    UINT                    NumberOfPalettes;
    PALETTEENTRY            **palettes;
    UINT                    currentPalette;

    /* For rendering to a texture using glCopyTexImage */
    GLenum                  *draw_buffers;
    GLuint                  depth_blt_texture;
    GLuint                  depth_blt_rb;
    UINT                    depth_blt_rb_w;
    UINT                    depth_blt_rb_h;

    /* Cursor management */
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;
    UINT                    cursorWidth, cursorHeight;
    GLuint                  cursorTexture;
    HCURSOR                 hardwareCursor;

    /* The Wine logo surface */
    IWineD3DSurface        *logo_surface;

    /* Textures for when no other textures are mapped */
    UINT                          dummyTextureName[MAX_TEXTURES];

    /* DirectDraw stuff */
    DWORD ddraw_width, ddraw_height;
    WINED3DFORMAT ddraw_format;

    /* Final position fixup constant */
    float                       posFixup[4];

    /* With register combiners we can skip junk texture stages */
    DWORD                     texUnitMap[MAX_COMBINED_SAMPLERS];
    DWORD                     rev_tex_unit_map[MAX_COMBINED_SAMPLERS];

    /* Stream source management */
    struct wined3d_stream_info strided_streams;
    const WineDirect3DVertexStridedData *up_strided;

    /* Context management */
    struct wined3d_context **contexts;
    UINT                    numContexts;
    struct wined3d_context *pbufferContext;              /* The context that has a pbuffer as drawable */
    DWORD                   pbufferWidth, pbufferHeight; /* Size of the buffer drawable */

    /* High level patch management */
#define PATCHMAP_SIZE 43
#define PATCHMAP_HASHFUNC(x) ((x) % PATCHMAP_SIZE) /* Primitive and simple function */
    struct list             patches[PATCHMAP_SIZE];
    struct WineD3DRectPatch *currentPatch;
};

HRESULT device_init(IWineD3DDeviceImpl *device, IWineD3DImpl *wined3d,
        UINT adapter_idx, WINED3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        IUnknown *parent, IWineD3DDeviceParent *device_parent) DECLSPEC_HIDDEN;
void device_preload_textures(IWineD3DDeviceImpl *device) DECLSPEC_HIDDEN;
LRESULT device_process_message(IWineD3DDeviceImpl *device, HWND window,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc) DECLSPEC_HIDDEN;
void device_resource_add(IWineD3DDeviceImpl *This, IWineD3DResource *resource) DECLSPEC_HIDDEN;
void device_resource_released(IWineD3DDeviceImpl *This, IWineD3DResource *resource) DECLSPEC_HIDDEN;
void device_stream_info_from_declaration(IWineD3DDeviceImpl *This,
        BOOL use_vshader, struct wined3d_stream_info *stream_info, BOOL *fixup) DECLSPEC_HIDDEN;
void device_update_stream_info(IWineD3DDeviceImpl *device, const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
HRESULT IWineD3DDeviceImpl_ClearSurface(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, DWORD Count,
        const WINED3DRECT *pRects, DWORD Flags, WINED3DCOLOR Color, float Z, DWORD Stencil) DECLSPEC_HIDDEN;
void IWineD3DDeviceImpl_FindTexUnitMap(IWineD3DDeviceImpl *This) DECLSPEC_HIDDEN;
void IWineD3DDeviceImpl_MarkStateDirty(IWineD3DDeviceImpl *This, DWORD state) DECLSPEC_HIDDEN;

static inline BOOL isStateDirty(struct wined3d_context *context, DWORD state)
{
    DWORD idx = state / (sizeof(*context->isStateDirty) * CHAR_BIT);
    BYTE shift = state & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
    return context->isStateDirty[idx] & (1 << shift);
}

/* Support for IWineD3DResource ::Set/Get/FreePrivateData. */
typedef struct PrivateData
{
    struct list entry;

    GUID tag;
    DWORD flags; /* DDSPD_* */

    union
    {
        LPVOID data;
        LPUNKNOWN object;
    } ptr;

    DWORD size;
} PrivateData;

/*****************************************************************************
 * IWineD3DResource implementation structure
 */
typedef struct IWineD3DResourceClass
{
    /* IUnknown fields */
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3DResource Information */
    IUnknown               *parent;
    WINED3DRESOURCETYPE     resourceType;
    IWineD3DDeviceImpl *device;
    WINED3DPOOL             pool;
    UINT                    size;
    DWORD                   usage;
    const struct GlPixelFormatDesc *format_desc;
    DWORD                   priority;
    BYTE                   *allocatedMemory; /* Pointer to the real data location */
    BYTE                   *heapMemory; /* Pointer to the HeapAlloced block of memory */
    struct list             privateData;
    struct list             resource_list_entry;
    const struct wined3d_parent_ops *parent_ops;
} IWineD3DResourceClass;

typedef struct IWineD3DResourceImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DResourceVtbl *lpVtbl;
    IWineD3DResourceClass   resource;
} IWineD3DResourceImpl;

void resource_cleanup(IWineD3DResource *iface) DECLSPEC_HIDDEN;
HRESULT resource_free_private_data(IWineD3DResource *iface, REFGUID guid) DECLSPEC_HIDDEN;
HRESULT resource_get_parent(IWineD3DResource *iface, IUnknown **parent) DECLSPEC_HIDDEN;
DWORD resource_get_priority(IWineD3DResource *iface) DECLSPEC_HIDDEN;
HRESULT resource_get_private_data(IWineD3DResource *iface, REFGUID guid,
        void *data, DWORD *data_size) DECLSPEC_HIDDEN;
HRESULT resource_init(IWineD3DResource *iface, WINED3DRESOURCETYPE resource_type,
        IWineD3DDeviceImpl *device, UINT size, DWORD usage, const struct GlPixelFormatDesc *format_desc,
        WINED3DPOOL pool, IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;
WINED3DRESOURCETYPE resource_get_type(IWineD3DResource *iface) DECLSPEC_HIDDEN;
DWORD resource_set_priority(IWineD3DResource *iface, DWORD new_priority) DECLSPEC_HIDDEN;
HRESULT resource_set_private_data(IWineD3DResource *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags) DECLSPEC_HIDDEN;

/* Tests show that the start address of resources is 32 byte aligned */
#define RESOURCE_ALIGNMENT 32

/*****************************************************************************
 * IWineD3DBaseTexture D3D- > openGL state map lookups
 */

typedef enum winetexturestates {
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
    WINED3DTEXSTA_ELEMENTINDEX   = 10,
    WINED3DTEXSTA_DMAPOFFSET     = 11,
    WINED3DTEXSTA_TSSADDRESSW    = 12,
    MAX_WINETEXTURESTATES        = 13,
} winetexturestates;

enum WINED3DSRGB
{
    SRGB_ANY                                = 0,    /* Uses the cached value(e.g. external calls) */
    SRGB_RGB                                = 1,    /* Loads the rgb texture */
    SRGB_SRGB                               = 2,    /* Loads the srgb texture */
    SRGB_BOTH                               = 3,    /* Loads both textures */
};

struct gl_texture
{
    DWORD                   states[MAX_WINETEXTURESTATES];
    BOOL                    dirty;
    GLuint                  name;
};

/*****************************************************************************
 * IWineD3DBaseTexture implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DBaseTextureClass
{
    struct gl_texture       texture_rgb, texture_srgb;
    UINT                    levels;
    float                   pow2Matrix[16];
    UINT                    LOD;
    WINED3DTEXTUREFILTERTYPE filterType;
    LONG                    bindCount;
    DWORD                   sampler;
    BOOL                    is_srgb;
    BOOL                    pow2Matrix_identity;
    const struct min_lookup *minMipLookup;
    const GLenum            *magLookup;
    void                    (*internal_preload)(IWineD3DBaseTexture *iface, enum WINED3DSRGB srgb);
} IWineD3DBaseTextureClass;

void surface_internal_preload(IWineD3DSurface *iface, enum WINED3DSRGB srgb) DECLSPEC_HIDDEN;
BOOL surface_init_sysmem(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
BOOL surface_is_offscreen(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
void surface_prepare_texture(IWineD3DSurfaceImpl *surface, BOOL srgb) DECLSPEC_HIDDEN;

typedef struct IWineD3DBaseTextureImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DBaseTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

} IWineD3DBaseTextureImpl;

void basetexture_apply_state_changes(IWineD3DBaseTexture *iface,
        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1],
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;
HRESULT basetexture_bind(IWineD3DBaseTexture *iface, BOOL srgb, BOOL *set_surface_desc) DECLSPEC_HIDDEN;
void basetexture_cleanup(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
void basetexture_generate_mipmaps(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
WINED3DTEXTUREFILTERTYPE basetexture_get_autogen_filter_type(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
BOOL basetexture_get_dirty(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
DWORD basetexture_get_level_count(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
DWORD basetexture_get_lod(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;
HRESULT basetexture_init(IWineD3DBaseTextureImpl *texture, UINT levels, WINED3DRESOURCETYPE resource_type,
        IWineD3DDeviceImpl *device, UINT size, DWORD usage, const struct GlPixelFormatDesc *format_desc,
        WINED3DPOOL pool, IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;
HRESULT basetexture_set_autogen_filter_type(IWineD3DBaseTexture *iface,
        WINED3DTEXTUREFILTERTYPE filter_type) DECLSPEC_HIDDEN;
BOOL basetexture_set_dirty(IWineD3DBaseTexture *iface, BOOL dirty) DECLSPEC_HIDDEN;
DWORD basetexture_set_lod(IWineD3DBaseTexture *iface, DWORD new_lod) DECLSPEC_HIDDEN;
void basetexture_unload(IWineD3DBaseTexture *iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DTexture */
    IWineD3DSurface          *surfaces[MAX_MIP_LEVELS];
    UINT                      target;
    BOOL                      cond_np2;

} IWineD3DTextureImpl;

HRESULT texture_init(IWineD3DTextureImpl *texture, UINT width, UINT height, UINT levels,
        IWineD3DDeviceImpl *device, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DCubeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DCubeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DCubeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DCubeTexture */
    IWineD3DSurface          *surfaces[6][MAX_MIP_LEVELS];
} IWineD3DCubeTextureImpl;

HRESULT cubetexture_init(IWineD3DCubeTextureImpl *texture, UINT edge_length, UINT levels,
        IWineD3DDeviceImpl *device, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

typedef struct _WINED3DVOLUMET_DESC
{
    UINT                    Width;
    UINT                    Height;
    UINT                    Depth;
} WINED3DVOLUMET_DESC;

/*****************************************************************************
 * IWineD3DVolume implementation structure (extends IUnknown)
 */
typedef struct IWineD3DVolumeImpl
{
    /* IUnknown & WineD3DResource fields */
    const IWineD3DVolumeVtbl  *lpVtbl;
    IWineD3DResourceClass      resource;

    /* WineD3DVolume Information */
    WINED3DVOLUMET_DESC      currentDesc;
    IWineD3DBase            *container;
    BOOL                    lockable;
    BOOL                    locked;
    WINED3DBOX              lockedBox;
    WINED3DBOX              dirtyBox;
    BOOL                    dirty;
} IWineD3DVolumeImpl;

void volume_add_dirty_box(IWineD3DVolume *iface, const WINED3DBOX *dirty_box) DECLSPEC_HIDDEN;
HRESULT volume_init(IWineD3DVolumeImpl *volume, IWineD3DDeviceImpl *device, UINT width,
        UINT height, UINT depth, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DVolumeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DVolumeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DVolumeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DVolumeTexture */
    IWineD3DVolume           *volumes[MAX_MIP_LEVELS];
} IWineD3DVolumeTextureImpl;

HRESULT volumetexture_init(IWineD3DVolumeTextureImpl *texture, UINT width, UINT height,
        UINT depth, UINT levels, IWineD3DDeviceImpl *device, DWORD usage, WINED3DFORMAT format,
        WINED3DPOOL pool, IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

typedef struct _WINED3DSURFACET_DESC
{
    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD                   MultiSampleQuality;
    UINT                    Width;
    UINT                    Height;
} WINED3DSURFACET_DESC;

/*****************************************************************************
 * Structure for DIB Surfaces (GetDC and GDI surfaces)
 */
typedef struct wineD3DSurface_DIB {
    HBITMAP DIBsection;
    void* bitmap_data;
    UINT bitmap_size;
    HGDIOBJ holdbitmap;
    BOOL client_memory;
} wineD3DSurface_DIB;

typedef struct {
    struct list entry;
    GLuint id;
    UINT width;
    UINT height;
} renderbuffer_entry_t;

struct fbo_entry
{
    struct list entry;
    IWineD3DSurface **render_targets;
    IWineD3DSurface *depth_stencil;
    BOOL attached;
    GLuint id;
};

/*****************************************************************************
 * IWineD3DClipp implementation structure
 */
typedef struct IWineD3DClipperImpl
{
    const IWineD3DClipperVtbl *lpVtbl;
    LONG ref;

    IUnknown *Parent;
    HWND hWnd;
} IWineD3DClipperImpl;


/*****************************************************************************
 * IWineD3DSurface implementation structure
 */
struct IWineD3DSurfaceImpl
{
    /* IUnknown & IWineD3DResource Information     */
    const IWineD3DSurfaceVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* IWineD3DSurface fields */
    IWineD3DBase              *container;
    WINED3DSURFACET_DESC      currentDesc;
    IWineD3DPaletteImpl       *palette; /* D3D7 style palette handling */
    PALETTEENTRY              *palette9; /* D3D8/9 style palette handling */

    /* TODO: move this off into a management class(maybe!) */
    DWORD                      Flags;

    UINT                      pow2Width;
    UINT                      pow2Height;

    /* A method to retrieve the drawable size. Not in the Vtable to make it changeable */
    void (*get_drawable_size)(struct wined3d_context *context, UINT *width, UINT *height);

    /* Oversized texture */
    RECT                      glRect;

    /* PBO */
    GLuint                    pbo;
    GLuint texture_name;
    GLuint texture_name_srgb;
    GLint texture_level;
    GLenum texture_target;

    RECT                      lockedRect;
    RECT                      dirtyRect;
    int                       lockCount;
#define MAXLOCKCOUNT          50 /* After this amount of locks do not free the sysmem copy */

    /* For GetDC */
    wineD3DSurface_DIB        dib;
    HDC                       hDC;

    /* Color keys for DDraw */
    WINEDDCOLORKEY            DestBltCKey;
    WINEDDCOLORKEY            DestOverlayCKey;
    WINEDDCOLORKEY            SrcOverlayCKey;
    WINEDDCOLORKEY            SrcBltCKey;
    DWORD                     CKeyFlags;

    WINEDDCOLORKEY            glCKey;

    struct list               renderbuffers;
    renderbuffer_entry_t      *current_renderbuffer;

    /* DirectDraw clippers */
    IWineD3DClipper           *clipper;

    /* DirectDraw Overlay handling */
    RECT                      overlay_srcrect;
    RECT                      overlay_destrect;
    IWineD3DSurfaceImpl       *overlay_dest;
    struct list               overlays;
    struct list               overlay_entry;
};

extern const IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl DECLSPEC_HIDDEN;
extern const IWineD3DSurfaceVtbl IWineGDISurface_Vtbl DECLSPEC_HIDDEN;

UINT surface_calculate_size(const struct GlPixelFormatDesc *format_desc,
        UINT alignment, UINT width, UINT height) DECLSPEC_HIDDEN;
void surface_gdi_cleanup(IWineD3DSurfaceImpl *This) DECLSPEC_HIDDEN;
HRESULT surface_init(IWineD3DSurfaceImpl *surface, WINED3DSURFTYPE surface_type, UINT alignment,
        UINT width, UINT height, UINT level, BOOL lockable, BOOL discard, WINED3DMULTISAMPLE_TYPE multisample_type,
        UINT multisample_quality, IWineD3DDeviceImpl *device, DWORD usage, WINED3DFORMAT format,
        WINED3DPOOL pool, IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/* Predeclare the shared Surface functions */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_QueryInterface(IWineD3DSurface *iface,
        REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
ULONG WINAPI IWineD3DBaseSurfaceImpl_AddRef(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPrivateData(IWineD3DSurface *iface,
        REFGUID refguid, const void *pData, DWORD SizeOfData, DWORD Flags) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPrivateData(IWineD3DSurface *iface,
        REFGUID refguid, void *pData, DWORD *pSizeOfData) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid) DECLSPEC_HIDDEN;
DWORD WINAPI IWineD3DBaseSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew) DECLSPEC_HIDDEN;
DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPriority(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
WINED3DRESOURCETYPE WINAPI IWineD3DBaseSurfaceImpl_GetType(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetContainer(IWineD3DSurface* iface,
        REFIID riid, void **ppContainer) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetBltStatus(IWineD3DSurface *iface, DWORD Flags) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetFlipStatus(IWineD3DSurface *iface, DWORD Flags) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_IsLost(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_Restore(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPalette(IWineD3DSurface *iface, IWineD3DPalette **Pal) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPalette(IWineD3DSurface *iface, IWineD3DPalette *Pal) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetColorKey(IWineD3DSurface *iface,
        DWORD Flags, const WINEDDCOLORKEY *CKey) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container) DECLSPEC_HIDDEN;
DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPitch(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_RealizePalette(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetOverlayPosition(IWineD3DSurface *iface, LONG X, LONG Y) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetOverlayPosition(IWineD3DSurface *iface, LONG *X, LONG *Y) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlayZOrder(IWineD3DSurface *iface,
        DWORD Flags, IWineD3DSurface *Ref) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlay(IWineD3DSurface *iface, const RECT *SrcRect,
        IWineD3DSurface *DstSurface, const RECT *DstRect, DWORD Flags, const WINEDDOVERLAYFX *FX) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetClipper(IWineD3DSurface *iface, IWineD3DClipper *clipper) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetClipper(IWineD3DSurface *iface, IWineD3DClipper **clipper) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) DECLSPEC_HIDDEN;
HRESULT IWineD3DBaseSurfaceImpl_CreateDIBSection(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_Blt(IWineD3DSurface *iface, const RECT *DestRect, IWineD3DSurface *SrcSurface,
        const RECT *SrcRect, DWORD Flags, const WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty,
        IWineD3DSurface *Source, const RECT *rsrc, DWORD trans) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT *pLockedRect,
        const RECT *pRect, DWORD Flags) DECLSPEC_HIDDEN;
void WINAPI IWineD3DBaseSurfaceImpl_BindTexture(IWineD3DSurface *iface, BOOL srgb) DECLSPEC_HIDDEN;
const void *WINAPI IWineD3DBaseSurfaceImpl_GetData(IWineD3DSurface *iface) DECLSPEC_HIDDEN;

void get_drawable_size_swapchain(struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;
void get_drawable_size_backbuffer(struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;
void get_drawable_size_pbuffer(struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;
void get_drawable_size_fbo(struct wined3d_context *context, UINT *width, UINT *height) DECLSPEC_HIDDEN;

void flip_surface(IWineD3DSurfaceImpl *front, IWineD3DSurfaceImpl *back) DECLSPEC_HIDDEN;

/* Surface flags: */
#define SFLAG_OVERSIZE      0x00000001 /* Surface is bigger than gl size, blts only */
#define SFLAG_CONVERTED     0x00000002 /* Converted for color keying or Palettized */
#define SFLAG_DIBSECTION    0x00000004 /* Has a DIB section attached for GetDC */
#define SFLAG_LOCKABLE      0x00000008 /* Surface can be locked */
#define SFLAG_DISCARD       0x00000010 /* ??? */
#define SFLAG_LOCKED        0x00000020 /* Surface is locked atm */
#define SFLAG_INTEXTURE     0x00000040 /* The GL texture contains the newest surface content */
#define SFLAG_INSRGBTEX     0x00000080 /* The GL srgb texture contains the newest surface content */
#define SFLAG_INDRAWABLE    0x00000100 /* The gl drawable contains the most up to date data */
#define SFLAG_INSYSMEM      0x00000200 /* The system memory copy is most up to date */
#define SFLAG_NONPOW2       0x00000400 /* Surface sizes are not a power of 2 */
#define SFLAG_DYNLOCK       0x00000800 /* Surface is often locked by the app */
#define SFLAG_DCINUSE       0x00001000 /* Set between GetDC and ReleaseDC calls */
#define SFLAG_LOST          0x00002000 /* Surface lost flag for DDraw */
#define SFLAG_USERPTR       0x00004000 /* The application allocated the memory for this surface */
#define SFLAG_GLCKEY        0x00008000 /* The gl texture was created with a color key */
#define SFLAG_CLIENT        0x00010000 /* GL_APPLE_client_storage is used on that texture */
#define SFLAG_ALLOCATED     0x00020000 /* A gl texture is allocated for this surface */
#define SFLAG_SRGBALLOCATED 0x00040000 /* A srgb gl texture is allocated for this surface */
#define SFLAG_PBO           0x00080000 /* Has a PBO attached for speeding up data transfers for dynamically locked surfaces */
#define SFLAG_NORMCOORD     0x00100000 /* Set if the GL texture coords are normalized(non-texture rectangle) */
#define SFLAG_DS_ONSCREEN   0x00200000 /* Is a depth stencil, last modified onscreen */
#define SFLAG_DS_OFFSCREEN  0x00400000 /* Is a depth stencil, last modified offscreen */
#define SFLAG_INOVERLAYDRAW 0x00800000 /* Overlay drawing is in progress. Recursion prevention */
#define SFLAG_SWAPCHAIN     0x01000000 /* The surface is part of a swapchain */

/* In some conditions the surface memory must not be freed:
 * SFLAG_OVERSIZE: Not all data can be kept in GL
 * SFLAG_CONVERTED: Converting the data back would take too long
 * SFLAG_DIBSECTION: The dib code manages the memory
 * SFLAG_LOCKED: The app requires access to the surface data
 * SFLAG_DYNLOCK: Avoid freeing the data for performance
 * SFLAG_PBO: PBOs don't use 'normal' memory. It is either allocated by the driver or must be NULL.
 * SFLAG_CLIENT: OpenGL uses our memory as backup
 */
#define SFLAG_DONOTFREE     (SFLAG_OVERSIZE   | \
                             SFLAG_CONVERTED  | \
                             SFLAG_DIBSECTION | \
                             SFLAG_LOCKED     | \
                             SFLAG_DYNLOCK    | \
                             SFLAG_USERPTR    | \
                             SFLAG_PBO        | \
                             SFLAG_CLIENT)

#define SFLAG_LOCATIONS     (SFLAG_INSYSMEM   | \
                             SFLAG_INTEXTURE  | \
                             SFLAG_INDRAWABLE | \
                             SFLAG_INSRGBTEX)

#define SFLAG_DS_LOCATIONS  (SFLAG_DS_ONSCREEN | \
                             SFLAG_DS_OFFSCREEN)
#define SFLAG_DS_DISCARDED   SFLAG_DS_LOCATIONS

BOOL CalculateTexRect(IWineD3DSurfaceImpl *This, RECT *Rect, float glTexCoord[4]) DECLSPEC_HIDDEN;

typedef enum {
    NO_CONVERSION,
    CONVERT_PALETTED,
    CONVERT_PALETTED_CK,
    CONVERT_CK_565,
    CONVERT_CK_5551,
    CONVERT_CK_4444,
    CONVERT_CK_4444_ARGB,
    CONVERT_CK_1555,
    CONVERT_555,
    CONVERT_CK_RGB24,
    CONVERT_CK_8888,
    CONVERT_CK_8888_ARGB,
    CONVERT_RGB32_888,
    CONVERT_V8U8,
    CONVERT_L6V5U5,
    CONVERT_X8L8V8U8,
    CONVERT_Q8W8V8U8,
    CONVERT_V16U16,
    CONVERT_A4L4,
    CONVERT_G16R16,
    CONVERT_R16G16F,
    CONVERT_R32G32F,
    CONVERT_D15S1,
    CONVERT_D24X4S4,
    CONVERT_D24FS8,
} CONVERT_TYPES;

HRESULT d3dfmt_get_conv(IWineD3DSurfaceImpl *This, BOOL need_alpha_ck, BOOL use_texturing, GLenum *format,
        GLenum *internal, GLenum *type, CONVERT_TYPES *convert, int *target_bpp, BOOL srgb_mode) DECLSPEC_HIDDEN;

BOOL palette9_changed(IWineD3DSurfaceImpl *This) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DVertexDeclaration implementation structure
 */

struct wined3d_vertex_declaration_element
{
    const struct GlPixelFormatDesc *format_desc;
    BOOL ffp_valid;
    WORD input_slot;
    WORD offset;
    UINT output_slot;
    BYTE method;
    BYTE usage;
    BYTE usage_idx;
};

typedef struct IWineD3DVertexDeclarationImpl {
    /* IUnknown  Information */
    const IWineD3DVertexDeclarationVtbl *lpVtbl;
    LONG                    ref;

    IUnknown                *parent;
    const struct wined3d_parent_ops *parent_ops;
    IWineD3DDeviceImpl *device;

    struct wined3d_vertex_declaration_element *elements;
    UINT element_count;

    DWORD                   streams[MAX_STREAMS];
    UINT                    num_streams;
    BOOL                    position_transformed;
    BOOL                    half_float_conv_needed;
} IWineD3DVertexDeclarationImpl;

HRESULT vertexdeclaration_init(IWineD3DVertexDeclarationImpl *declaration, IWineD3DDeviceImpl *device,
        const WINED3DVERTEXELEMENT *elements, UINT element_count,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DStateBlock implementation structure
 */

/* Internal state Block for Begin/End/Capture/Create/Apply info  */
/*   Note: Very long winded but gl Lists are not flexible enough */
/*   to resolve everything we need, so doing it manually for now */
typedef struct SAVEDSTATES {
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
} SAVEDSTATES;

struct StageState {
    DWORD stage;
    DWORD state;
};

struct IWineD3DStateBlockImpl
{
    /* IUnknown fields */
    const IWineD3DStateBlockVtbl *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    /* IWineD3DStateBlock information */
    IWineD3DDeviceImpl *device;
    WINED3DSTATEBLOCKTYPE     blockType;

    /* Array indicating whether things have been set or changed */
    SAVEDSTATES               changed;

    /* Vertex Shader Declaration */
    IWineD3DVertexDeclaration *vertexDecl;

    IWineD3DVertexShader      *vertexShader;

    /* Vertex Shader Constants */
    BOOL                       vertexShaderConstantB[MAX_CONST_B];
    INT                        vertexShaderConstantI[MAX_CONST_I * 4];
    float                     *vertexShaderConstantF;

    /* primitive type */
    GLenum gl_primitive_type;

    /* Stream Source */
    BOOL                      streamIsUP;
    UINT                      streamStride[MAX_STREAMS];
    UINT                      streamOffset[MAX_STREAMS + 1 /* tesselated pseudo-stream */ ];
    IWineD3DBuffer           *streamSource[MAX_STREAMS];
    UINT                      streamFreq[MAX_STREAMS + 1];
    UINT                      streamFlags[MAX_STREAMS + 1];     /*0 | WINED3DSTREAMSOURCE_INSTANCEDATA | WINED3DSTREAMSOURCE_INDEXEDDATA  */

    /* Indices */
    IWineD3DBuffer*           pIndexData;
    WINED3DFORMAT             IndexFmt;
    INT                       baseVertexIndex;
    INT                       loadBaseVertexIndex; /* non-indexed drawing needs 0 here, indexed baseVertexIndex */

    /* Transform */
    WINED3DMATRIX             transforms[HIGHEST_TRANSFORMSTATE + 1];

    /* Light hashmap . Collisions are handled using standard wine double linked lists */
#define LIGHTMAP_SIZE 43 /* Use of a prime number recommended. Set to 1 for a linked list! */
#define LIGHTMAP_HASHFUNC(x) ((x) % LIGHTMAP_SIZE) /* Primitive and simple function */
    struct list               lightMap[LIGHTMAP_SIZE]; /* Hash map containing the lights */
    const struct wined3d_light_info *activeLights[MAX_ACTIVE_LIGHTS]; /* Map of opengl lights to d3d lights */

    /* Clipping */
    double                    clipplane[MAX_CLIPPLANES][4];
    WINED3DCLIPSTATUS         clip_status;

    /* ViewPort */
    WINED3DVIEWPORT           viewport;

    /* Material */
    WINED3DMATERIAL           material;

    /* Pixel Shader */
    IWineD3DPixelShader      *pixelShader;

    /* Pixel Shader Constants */
    BOOL                       pixelShaderConstantB[MAX_CONST_B];
    INT                        pixelShaderConstantI[MAX_CONST_I * 4];
    float                     *pixelShaderConstantF;

    /* RenderState */
    DWORD                     renderState[WINEHIGHEST_RENDER_STATE + 1];

    /* Texture */
    IWineD3DBaseTexture      *textures[MAX_COMBINED_SAMPLERS];

    /* Texture State Stage */
    DWORD                     textureState[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
    DWORD                     lowest_disabled_stage;
    /* Sampler States */
    DWORD                     samplerState[MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];

    /* Scissor test rectangle */
    RECT                      scissorRect;

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

HRESULT stateblock_init(IWineD3DStateBlockImpl *stateblock,
        IWineD3DDeviceImpl *device, WINED3DSTATEBLOCKTYPE type) DECLSPEC_HIDDEN;
void stateblock_init_contained_states(IWineD3DStateBlockImpl *object) DECLSPEC_HIDDEN;

/* Direct3D terminology with little modifications. We do not have an issued state
 * because only the driver knows about it, but we have a created state because d3d
 * allows GetData on a created issue, but opengl doesn't
 */
enum query_state {
    QUERY_CREATED,
    QUERY_SIGNALLED,
    QUERY_BUILDING
};
/*****************************************************************************
 * IWineD3DQueryImpl implementation structure (extends IUnknown)
 */
typedef struct IWineD3DQueryImpl
{
    const IWineD3DQueryVtbl  *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    IUnknown                 *parent;
    IWineD3DDeviceImpl *device;

    /* IWineD3DQuery fields */
    enum query_state         state;
    WINED3DQUERYTYPE         type;
    /* TODO: Think about using a IUnknown instead of a void* */
    void                     *extendedData;
} IWineD3DQueryImpl;

HRESULT query_init(IWineD3DQueryImpl *query, IWineD3DDeviceImpl *device,
        WINED3DQUERYTYPE type, IUnknown *parent) DECLSPEC_HIDDEN;

/* IWineD3DBuffer */

/* TODO: Add tests and support for FLOAT16_4 POSITIONT, D3DCOLOR position, other
 * fixed function semantics as D3DCOLOR or FLOAT16 */
enum wined3d_buffer_conversion_type
{
    CONV_NONE,
    CONV_D3DCOLOR,
    CONV_POSITIONT,
    CONV_FLOAT16_2, /* Also handles FLOAT16_4 */
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

struct wined3d_buffer
{
    const struct IWineD3DBufferVtbl *vtbl;
    IWineD3DResourceClass resource;

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

    /* conversion stuff */
    UINT decl_change_count, full_conversion_count;
    UINT draw_count;
    UINT stride;                                            /* 0 if no conversion */
    UINT conversion_stride;                                 /* 0 if no shifted conversion */
    enum wined3d_buffer_conversion_type *conversion_map;    /* NULL if no conversion */
    /* Extra load offsets, for FLOAT16 conversion */
    UINT *conversion_shift;                                 /* NULL if no shifted conversion */
};

const BYTE *buffer_get_memory(IWineD3DBuffer *iface, UINT offset, GLuint *buffer_object) DECLSPEC_HIDDEN;
BYTE *buffer_get_sysmem(struct wined3d_buffer *This) DECLSPEC_HIDDEN;
HRESULT buffer_init(struct wined3d_buffer *buffer, IWineD3DDeviceImpl *device,
        UINT size, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool, GLenum bind_hint,
        const char *data, IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/* IWineD3DRendertargetView */
struct wined3d_rendertarget_view
{
    const struct IWineD3DRendertargetViewVtbl *vtbl;
    LONG refcount;

    IWineD3DResource *resource;
    IUnknown *parent;
};

extern const IWineD3DRendertargetViewVtbl wined3d_rendertarget_view_vtbl DECLSPEC_HIDDEN;

/*****************************************************************************
 * IWineD3DSwapChainImpl implementation structure (extends IUnknown)
 */

struct IWineD3DSwapChainImpl
{
    /*IUnknown part*/
    const IWineD3DSwapChainVtbl *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    IUnknown                 *parent;
    IWineD3DDeviceImpl *device;

    /* IWineD3DSwapChain fields */
    IWineD3DSurface         **backBuffer;
    IWineD3DSurface          *frontBuffer;
    WINED3DPRESENT_PARAMETERS presentParms;
    DWORD                     orig_width, orig_height;
    WINED3DFORMAT             orig_fmt;
    WINED3DGAMMARAMP          orig_gamma;
    BOOL                      render_to_fbo;

    long prev_time, frames;   /* Performance tracking */
    unsigned int vSyncCounter;

    struct wined3d_context **context;
    unsigned int            num_contexts;

    HWND                    win_handle;
};

const IWineD3DSwapChainVtbl IWineGDISwapChain_Vtbl DECLSPEC_HIDDEN;
void x11_copy_to_screen(IWineD3DSwapChainImpl *This, const RECT *rc) DECLSPEC_HIDDEN;

HRESULT WINAPI IWineD3DBaseSwapChainImpl_QueryInterface(IWineD3DSwapChain *iface,
        REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
ULONG WINAPI IWineD3DBaseSwapChainImpl_AddRef(IWineD3DSwapChain *iface) DECLSPEC_HIDDEN;
ULONG WINAPI IWineD3DBaseSwapChainImpl_Release(IWineD3DSwapChain *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetParent(IWineD3DSwapChain *iface, IUnknown **ppParent) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetFrontBufferData(IWineD3DSwapChain *iface,
        IWineD3DSurface *pDestSurface) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetBackBuffer(IWineD3DSwapChain *iface, UINT iBackBuffer,
        WINED3DBACKBUFFER_TYPE Type, IWineD3DSurface **ppBackBuffer) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetRasterStatus(IWineD3DSwapChain *iface,
        WINED3DRASTER_STATUS *pRasterStatus) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetDisplayMode(IWineD3DSwapChain *iface,
        WINED3DDISPLAYMODE *pMode) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetDevice(IWineD3DSwapChain *iface,
        IWineD3DDevice **ppDevice) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetPresentParameters(IWineD3DSwapChain *iface,
        WINED3DPRESENT_PARAMETERS *pPresentationParameters) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_SetGammaRamp(IWineD3DSwapChain *iface,
        DWORD Flags, const WINED3DGAMMARAMP *pRamp) DECLSPEC_HIDDEN;
HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetGammaRamp(IWineD3DSwapChain *iface,
        WINED3DGAMMARAMP *pRamp) DECLSPEC_HIDDEN;

struct wined3d_context *swapchain_create_context_for_thread(IWineD3DSwapChain *iface) DECLSPEC_HIDDEN;
HRESULT swapchain_init(IWineD3DSwapChainImpl *swapchain, WINED3DSURFTYPE surface_type,
        IWineD3DDeviceImpl *device, WINED3DPRESENT_PARAMETERS *present_parameters, IUnknown *parent) DECLSPEC_HIDDEN;
void swapchain_restore_fullscreen_window(IWineD3DSwapChainImpl *swapchain) DECLSPEC_HIDDEN;
void swapchain_setup_fullscreen_window(IWineD3DSwapChainImpl *swapchain, UINT w, UINT h) DECLSPEC_HIDDEN;

#define DEFAULT_REFRESH_RATE 0

/*****************************************************************************
 * Utility function prototypes
 */

/* Trace routines */
const char *debug_d3dformat(WINED3DFORMAT fmt) DECLSPEC_HIDDEN;
const char *debug_d3ddevicetype(WINED3DDEVTYPE devtype) DECLSPEC_HIDDEN;
const char *debug_d3dresourcetype(WINED3DRESOURCETYPE res) DECLSPEC_HIDDEN;
const char *debug_d3dusage(DWORD usage) DECLSPEC_HIDDEN;
const char *debug_d3dusagequery(DWORD usagequery) DECLSPEC_HIDDEN;
const char *debug_d3ddeclmethod(WINED3DDECLMETHOD method) DECLSPEC_HIDDEN;
const char *debug_d3ddeclusage(BYTE usage) DECLSPEC_HIDDEN;
const char *debug_d3dprimitivetype(WINED3DPRIMITIVETYPE PrimitiveType) DECLSPEC_HIDDEN;
const char *debug_d3drenderstate(DWORD state) DECLSPEC_HIDDEN;
const char *debug_d3dsamplerstate(DWORD state) DECLSPEC_HIDDEN;
const char *debug_d3dstate(DWORD state) DECLSPEC_HIDDEN;
const char *debug_d3dtexturefiltertype(WINED3DTEXTUREFILTERTYPE filter_type) DECLSPEC_HIDDEN;
const char *debug_d3dtexturestate(DWORD state) DECLSPEC_HIDDEN;
const char *debug_d3dtstype(WINED3DTRANSFORMSTATETYPE tstype) DECLSPEC_HIDDEN;
const char *debug_d3dpool(WINED3DPOOL pool) DECLSPEC_HIDDEN;
const char *debug_fbostatus(GLenum status) DECLSPEC_HIDDEN;
const char *debug_glerror(GLenum error) DECLSPEC_HIDDEN;
const char *debug_d3dbasis(WINED3DBASISTYPE basis) DECLSPEC_HIDDEN;
const char *debug_d3ddegree(WINED3DDEGREETYPE order) DECLSPEC_HIDDEN;
const char *debug_d3dtop(WINED3DTEXTUREOP d3dtop) DECLSPEC_HIDDEN;
void dump_color_fixup_desc(struct color_fixup_desc fixup) DECLSPEC_HIDDEN;
const char *debug_surflocation(DWORD flag) DECLSPEC_HIDDEN;

/* Routines for GL <-> D3D values */
GLenum StencilOp(DWORD op) DECLSPEC_HIDDEN;
GLenum CompareFunc(DWORD func) DECLSPEC_HIDDEN;
BOOL is_invalid_op(IWineD3DDeviceImpl *This, int stage, WINED3DTEXTUREOP op,
        DWORD arg1, DWORD arg2, DWORD arg3) DECLSPEC_HIDDEN;
void set_tex_op_nvrc(IWineD3DDevice *iface, BOOL is_alpha, int stage, WINED3DTEXTUREOP op,
        DWORD arg1, DWORD arg2, DWORD arg3, INT texture_idx, DWORD dst) DECLSPEC_HIDDEN;
void set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords,
        BOOL transformed, WINED3DFORMAT coordtype, BOOL ffp_can_disable_proj) DECLSPEC_HIDDEN;
void texture_activate_dimensions(DWORD stage, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void sampler_texdim(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void tex_alphaop(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void apply_pixelshader(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void state_fogcolor(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void state_fogdensity(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void state_fogstartend(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;
void state_fog_fragpart(DWORD state, IWineD3DStateBlockImpl *stateblock,
        struct wined3d_context *context) DECLSPEC_HIDDEN;

void surface_add_dirty_rect(IWineD3DSurface *iface, const RECT *dirty_rect) DECLSPEC_HIDDEN;
GLenum surface_get_gl_buffer(IWineD3DSurface *iface) DECLSPEC_HIDDEN;
void surface_load_ds_location(IWineD3DSurface *iface, struct wined3d_context *context, DWORD location) DECLSPEC_HIDDEN;
void surface_modify_ds_location(IWineD3DSurface *iface, DWORD location) DECLSPEC_HIDDEN;
void surface_set_compatible_renderbuffer(IWineD3DSurface *iface,
        unsigned int width, unsigned int height) DECLSPEC_HIDDEN;
void surface_set_texture_name(IWineD3DSurface *iface, GLuint name, BOOL srgb_name) DECLSPEC_HIDDEN;
void surface_set_texture_target(IWineD3DSurface *iface, GLenum target) DECLSPEC_HIDDEN;

BOOL getColorBits(const struct GlPixelFormatDesc *format_desc,
        short *redSize, short *greenSize, short *blueSize, short *alphaSize, short *totalSize) DECLSPEC_HIDDEN;
BOOL getDepthStencilBits(const struct GlPixelFormatDesc *format_desc,
        short *depthSize, short *stencilSize) DECLSPEC_HIDDEN;

/* Math utils */
void multiply_matrix(WINED3DMATRIX *dest, const WINED3DMATRIX *src1, const WINED3DMATRIX *src2) DECLSPEC_HIDDEN;
UINT wined3d_log2i(UINT32 x) DECLSPEC_HIDDEN;
unsigned int count_bits(unsigned int mask) DECLSPEC_HIDDEN;

void select_shader_mode(const struct wined3d_gl_info *gl_info, int *ps_selected, int *vs_selected) DECLSPEC_HIDDEN;

typedef struct local_constant {
    struct list entry;
    unsigned int idx;
    DWORD value[4];
} local_constant;

typedef struct SHADER_LIMITS {
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
} SHADER_LIMITS;

/* Keeps track of details for TEX_M#x# shader opcodes which need to
 * maintain state information between multiple codes */
typedef struct SHADER_PARSE_STATE {
    unsigned int current_row;
    DWORD texcoord_w[2];
} SHADER_PARSE_STATE;

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

/* Base Shader utility functions. */
int shader_addline(struct wined3d_shader_buffer *buffer, const char *fmt, ...) PRINTF_ATTR(2,3) DECLSPEC_HIDDEN;
int shader_vaddline(struct wined3d_shader_buffer *buffer, const char *fmt, va_list args) DECLSPEC_HIDDEN;

/* Vertex shader utility functions */
extern BOOL vshader_get_input(IWineD3DVertexShader *iface,
        BYTE usage_req, BYTE usage_idx_req, unsigned int *regnum) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DBaseShader implementation structure
 */
typedef struct IWineD3DBaseShaderClass
{
    LONG                            ref;
    SHADER_LIMITS                   limits;
    SHADER_PARSE_STATE              parse_state;
    DWORD                          *function;
    UINT                            functionLength;
    UINT                            cur_loop_depth, cur_loop_regno;
    BOOL                            load_local_constsF;
    const struct wined3d_shader_frontend *frontend;
    void *frontend_data;
    void *backend_data;

    IUnknown *parent;
    const struct wined3d_parent_ops *parent_ops;

    /* Programs this shader is linked with */
    struct list linked_programs;

    /* Immediate constants (override global ones) */
    struct list constantsB;
    struct list constantsF;
    struct list constantsI;
    shader_reg_maps reg_maps;

    struct wined3d_shader_signature_element input_signature[max(MAX_ATTRIBS, MAX_REG_INPUT)];
    struct wined3d_shader_signature_element output_signature[MAX_REG_OUTPUT];

    /* Pointer to the parent device */
    IWineD3DDevice *device;
    struct list     shader_list_entry;

} IWineD3DBaseShaderClass;

typedef struct IWineD3DBaseShaderImpl {
    /* IUnknown */
    const IWineD3DBaseShaderVtbl    *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass         baseShader;
} IWineD3DBaseShaderImpl;

void shader_buffer_clear(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
BOOL shader_buffer_init(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
void shader_buffer_free(struct wined3d_shader_buffer *buffer) DECLSPEC_HIDDEN;
void shader_dump_src_param(const struct wined3d_shader_src_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
void shader_dump_dst_param(const struct wined3d_shader_dst_param *param,
        const struct wined3d_shader_version *shader_version) DECLSPEC_HIDDEN;
unsigned int shader_find_free_input_register(const struct shader_reg_maps *reg_maps, unsigned int max) DECLSPEC_HIDDEN;
void shader_generate_main(IWineD3DBaseShader *iface, struct wined3d_shader_buffer *buffer,
        const shader_reg_maps *reg_maps, const DWORD *pFunction, void *backend_ctx) DECLSPEC_HIDDEN;
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
            if (reg->idx != 0) return TRUE;
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
            switch(reg->immconst_type)
            {
                case WINED3D_IMMCONST_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

static inline BOOL shader_constant_is_local(IWineD3DBaseShaderImpl* This, DWORD reg) {
    local_constant* lconst;

    if(This->baseShader.load_local_constsF) return FALSE;
    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
        if(lconst->idx == reg) return TRUE;
    }
    return FALSE;

}

/*****************************************************************************
 * IDirect3DVertexShader implementation structures
 */
typedef struct IWineD3DVertexShaderImpl {
    /* IUnknown parts */
    const IWineD3DVertexShaderVtbl *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* Vertex shader attributes. */
    struct wined3d_shader_attribute attributes[MAX_ATTRIBS];

    UINT                       min_rel_offset, max_rel_offset;
    UINT                       rel_offset;
} IWineD3DVertexShaderImpl;

void find_vs_compile_args(IWineD3DVertexShaderImpl *shader, IWineD3DStateBlockImpl *stateblock,
        struct vs_compile_args *args) DECLSPEC_HIDDEN;
HRESULT vertexshader_init(IWineD3DVertexShaderImpl *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

struct wined3d_geometryshader
{
    const struct IWineD3DGeometryShaderVtbl *vtbl;
    IWineD3DBaseShaderClass base_shader;
};

HRESULT geometryshader_init(struct wined3d_geometryshader *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */

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

typedef struct IWineD3DPixelShaderImpl {
    /* IUnknown parts */
    const IWineD3DPixelShaderVtbl *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* Pixel shader input semantics */
    DWORD                 input_reg_map[MAX_REG_INPUT];
    BOOL                  input_reg_used[MAX_REG_INPUT];
    unsigned int declared_in_count;

    /* Some information about the shader behavior */
    char                        vpos_uniform;

    BOOL                        color0_mov;
    DWORD                       color0_reg;

} IWineD3DPixelShaderImpl;

HRESULT pixelshader_init(IWineD3DPixelShaderImpl *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops) DECLSPEC_HIDDEN;
void pixelshader_update_samplers(struct shader_reg_maps *reg_maps,
        IWineD3DBaseTexture * const *textures) DECLSPEC_HIDDEN;
void find_ps_compile_args(IWineD3DPixelShaderImpl *shader, IWineD3DStateBlockImpl *stateblock,
        struct ps_compile_args *args) DECLSPEC_HIDDEN;

/* sRGB correction constants */
static const float srgb_cmp = 0.0031308f;
static const float srgb_mul_low = 12.92f;
static const float srgb_pow = 0.41666f;
static const float srgb_mul_high = 1.055f;
static const float srgb_sub_high = 0.055f;

/*****************************************************************************
 * IWineD3DPalette implementation structure
 */
struct IWineD3DPaletteImpl {
    /* IUnknown parts */
    const IWineD3DPaletteVtbl  *lpVtbl;
    LONG                       ref;

    IUnknown                   *parent;
    IWineD3DDeviceImpl *device;

    /* IWineD3DPalette */
    HPALETTE                   hpal;
    WORD                       palVersion;     /*|               */
    WORD                       palNumEntries;  /*|  LOGPALETTE   */
    PALETTEENTRY               palents[256];   /*|               */
    /* This is to store the palette in 'screen format' */
    int                        screen_palents[256];
    DWORD                      Flags;
};

extern const IWineD3DPaletteVtbl IWineD3DPalette_Vtbl DECLSPEC_HIDDEN;
DWORD IWineD3DPaletteImpl_Size(DWORD dwFlags) DECLSPEC_HIDDEN;

/* DirectDraw utility functions */
extern WINED3DFORMAT pixelformat_for_depth(DWORD depth) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Pixel format management
 */

/* WineD3D pixel format flags */
#define WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING 0x1
#define WINED3DFMT_FLAG_FILTERING                0x2
#define WINED3DFMT_FLAG_DEPTH                    0x4
#define WINED3DFMT_FLAG_STENCIL                  0x8
#define WINED3DFMT_FLAG_RENDERTARGET             0x10
#define WINED3DFMT_FLAG_FOURCC                   0x20
#define WINED3DFMT_FLAG_FBO_ATTACHABLE           0x40
#define WINED3DFMT_FLAG_COMPRESSED               0x80
#define WINED3DFMT_FLAG_GETDC                    0x100

struct GlPixelFormatDesc
{
    WINED3DFORMAT format;
    DWORD red_mask;
    DWORD green_mask;
    DWORD blue_mask;
    DWORD alpha_mask;
    UINT byte_count;
    WORD depth_size;
    WORD stencil_size;

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
    unsigned int Flags;
    float heightscale;
    struct color_fixup_desc color_fixup;
};

const struct GlPixelFormatDesc *getFormatDescEntry(WINED3DFORMAT fmt,
        const struct wined3d_gl_info *gl_info) DECLSPEC_HIDDEN;

static inline BOOL use_vs(IWineD3DStateBlockImpl *stateblock)
{
    /* Check stateblock->vertexDecl to allow this to be used from
     * IWineD3DDeviceImpl_FindTexUnitMap(). This is safe because
     * stateblock->vertexShader implies a vertex declaration instead of ddraw
     * style strided data. */
    return (stateblock->vertexShader
            && !((IWineD3DVertexDeclarationImpl *)stateblock->vertexDecl)->position_transformed
            && stateblock->device->vs_selected_mode != SHADER_NONE);
}

static inline BOOL use_ps(IWineD3DStateBlockImpl *stateblock)
{
    return (stateblock->pixelShader && stateblock->device->ps_selected_mode != SHADER_NONE);
}

void stretch_rect_fbo(IWineD3DDevice *iface, IWineD3DSurface *src_surface,
        WINED3DRECT *src_rect, IWineD3DSurface *dst_surface, WINED3DRECT *dst_rect,
        const WINED3DTEXTUREFILTERTYPE filter, BOOL flip) DECLSPEC_HIDDEN;

/* The WNDCLASS-Name for the fake window which we use to retrieve the GL capabilities */
#define WINED3D_OPENGL_WINDOW_CLASS_NAME "WineD3D_OpenGL"

#define WINEMAKEFOURCC(ch0, ch1, ch2, ch3) \
        ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
        ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#endif
