/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
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
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* DCL usage masks */
#define WINED3DSP_DCL_USAGE_SHIFT               0
#define WINED3DSP_DCL_USAGE_MASK                (0xf << WINED3DSP_DCL_USAGE_SHIFT)
#define WINED3DSP_DCL_USAGEINDEX_SHIFT          16
#define WINED3DSP_DCL_USAGEINDEX_MASK           (0xf << WINED3DSP_DCL_USAGEINDEX_SHIFT)

/* DCL sampler type */
#define WINED3DSP_TEXTURETYPE_SHIFT             27
#define WINED3DSP_TEXTURETYPE_MASK              (0xf << WINED3DSP_TEXTURETYPE_SHIFT)

/* Opcode-related masks */
#define WINED3DSI_OPCODE_MASK                   0x0000ffff

#define WINED3D_OPCODESPECIFICCONTROL_SHIFT     16
#define WINED3D_OPCODESPECIFICCONTROL_MASK      (0xff << WINED3D_OPCODESPECIFICCONTROL_SHIFT)

#define WINED3DSI_INSTLENGTH_SHIFT              24
#define WINED3DSI_INSTLENGTH_MASK               (0xf << WINED3DSI_INSTLENGTH_SHIFT)

#define WINED3DSI_COISSUE                       (1 << 30)

#define WINED3DSI_COMMENTSIZE_SHIFT             16
#define WINED3DSI_COMMENTSIZE_MASK              (0x7fff << WINED3DSI_COMMENTSIZE_SHIFT)

#define WINED3DSHADER_INSTRUCTION_PREDICATED    (1 << 28)

/* Register number mask */
#define WINED3DSP_REGNUM_MASK                   0x000007ff

/* Register type masks  */
#define WINED3DSP_REGTYPE_SHIFT                 28
#define WINED3DSP_REGTYPE_MASK                  (0x7 << WINED3DSP_REGTYPE_SHIFT)
#define WINED3DSP_REGTYPE_SHIFT2                8
#define WINED3DSP_REGTYPE_MASK2                 (0x18 << WINED3DSP_REGTYPE_SHIFT2)

/* Relative addressing mask */
#define WINED3DSHADER_ADDRESSMODE_SHIFT         13
#define WINED3DSHADER_ADDRESSMODE_MASK          (1 << WINED3DSHADER_ADDRESSMODE_SHIFT)

/* Destination modifier mask */
#define WINED3DSP_DSTMOD_SHIFT                  20
#define WINED3DSP_DSTMOD_MASK                   (0xf << WINED3DSP_DSTMOD_SHIFT)

/* Destination shift mask */
#define WINED3DSP_DSTSHIFT_SHIFT                24
#define WINED3DSP_DSTSHIFT_MASK                 (0xf << WINED3DSP_DSTSHIFT_SHIFT)

/* Write mask */
#define WINED3D_SM1_WRITEMASK_SHIFT             16
#define WINED3D_SM1_WRITEMASK_MASK              (0xf << WINED3D_SM1_WRITEMASK_SHIFT)

/* Swizzle mask */
#define WINED3DSP_SWIZZLE_SHIFT                 16
#define WINED3DSP_SWIZZLE_MASK                  (0xff << WINED3DSP_SWIZZLE_SHIFT)

/* Source modifier mask */
#define WINED3DSP_SRCMOD_SHIFT                  24
#define WINED3DSP_SRCMOD_MASK                   (0xf << WINED3DSP_SRCMOD_SHIFT)

#define WINED3DSP_END                           0x0000ffff

#define WINED3D_SM1_VERSION_MAJOR(version)      (((version) >> 8) & 0xff)
#define WINED3D_SM1_VERSION_MINOR(version)      (((version) >> 0) & 0xff)

enum WINED3DSHADER_ADDRESSMODE_TYPE
{
    WINED3DSHADER_ADDRMODE_ABSOLUTE = 0 << WINED3DSHADER_ADDRESSMODE_SHIFT,
    WINED3DSHADER_ADDRMODE_RELATIVE = 1 << WINED3DSHADER_ADDRESSMODE_SHIFT,
};

struct wined3d_sm1_opcode_info
{
    unsigned int opcode;
    UINT dst_count;
    UINT param_count;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    DWORD min_version;
    DWORD max_version;
};

struct wined3d_sm1_data
{
    struct wined3d_shader_version shader_version;
    const struct wined3d_sm1_opcode_info *opcode_table;
};

/* This table is not order or position dependent. */
static const struct wined3d_sm1_opcode_info vs_opcode_table[] =
{
    /* Arithmetic */
    {WINED3DSIO_NOP,          0, 0, WINED3DSIH_NOP,          0,                           0                          },
    {WINED3DSIO_MOV,          1, 2, WINED3DSIH_MOV,          0,                           0                          },
    {WINED3DSIO_MOVA,         1, 2, WINED3DSIH_MOVA,         WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_ADD,          1, 3, WINED3DSIH_ADD,          0,                           0                          },
    {WINED3DSIO_SUB,          1, 3, WINED3DSIH_SUB,          0,                           0                          },
    {WINED3DSIO_MAD,          1, 4, WINED3DSIH_MAD,          0,                           0                          },
    {WINED3DSIO_MUL,          1, 3, WINED3DSIH_MUL,          0,                           0                          },
    {WINED3DSIO_RCP,          1, 2, WINED3DSIH_RCP,          0,                           0                          },
    {WINED3DSIO_RSQ,          1, 2, WINED3DSIH_RSQ,          0,                           0                          },
    {WINED3DSIO_DP3,          1, 3, WINED3DSIH_DP3,          0,                           0                          },
    {WINED3DSIO_DP4,          1, 3, WINED3DSIH_DP4,          0,                           0                          },
    {WINED3DSIO_MIN,          1, 3, WINED3DSIH_MIN,          0,                           0                          },
    {WINED3DSIO_MAX,          1, 3, WINED3DSIH_MAX,          0,                           0                          },
    {WINED3DSIO_SLT,          1, 3, WINED3DSIH_SLT,          0,                           0                          },
    {WINED3DSIO_SGE,          1, 3, WINED3DSIH_SGE,          0,                           0                          },
    {WINED3DSIO_ABS,          1, 2, WINED3DSIH_ABS,          0,                           0                          },
    {WINED3DSIO_EXP,          1, 2, WINED3DSIH_EXP,          0,                           0                          },
    {WINED3DSIO_LOG,          1, 2, WINED3DSIH_LOG,          0,                           0                          },
    {WINED3DSIO_EXPP,         1, 2, WINED3DSIH_EXPP,         0,                           0                          },
    {WINED3DSIO_LOGP,         1, 2, WINED3DSIH_LOGP,         0,                           0                          },
    {WINED3DSIO_LIT,          1, 2, WINED3DSIH_LIT,          0,                           0                          },
    {WINED3DSIO_DST,          1, 3, WINED3DSIH_DST,          0,                           0                          },
    {WINED3DSIO_LRP,          1, 4, WINED3DSIH_LRP,          0,                           0                          },
    {WINED3DSIO_FRC,          1, 2, WINED3DSIH_FRC,          0,                           0                          },
    {WINED3DSIO_POW,          1, 3, WINED3DSIH_POW,          0,                           0                          },
    {WINED3DSIO_CRS,          1, 3, WINED3DSIH_CRS,          0,                           0                          },
    {WINED3DSIO_SGN,          1, 2, WINED3DSIH_SGN,          0,                           0                          },
    {WINED3DSIO_NRM,          1, 2, WINED3DSIH_NRM,          0,                           0                          },
    {WINED3DSIO_SINCOS,       1, 4, WINED3DSIH_SINCOS,       WINED3D_SHADER_VERSION(2,0), WINED3D_SHADER_VERSION(2,1)},
    {WINED3DSIO_SINCOS,       1, 2, WINED3DSIH_SINCOS,       WINED3D_SHADER_VERSION(3,0), -1                         },
    /* Matrix */
    {WINED3DSIO_M4x4,         1, 3, WINED3DSIH_M4x4,         0,                           0                          },
    {WINED3DSIO_M4x3,         1, 3, WINED3DSIH_M4x3,         0,                           0                          },
    {WINED3DSIO_M3x4,         1, 3, WINED3DSIH_M3x4,         0,                           0                          },
    {WINED3DSIO_M3x3,         1, 3, WINED3DSIH_M3x3,         0,                           0                          },
    {WINED3DSIO_M3x2,         1, 3, WINED3DSIH_M3x2,         0,                           0                          },
    /* Declare registers */
    {WINED3DSIO_DCL,          0, 2, WINED3DSIH_DCL,          0,                           0                          },
    /* Constant definitions */
    {WINED3DSIO_DEF,          1, 5, WINED3DSIH_DEF,          0,                           0                          },
    {WINED3DSIO_DEFB,         1, 2, WINED3DSIH_DEFB,         0,                           0                          },
    {WINED3DSIO_DEFI,         1, 5, WINED3DSIH_DEFI,         0,                           0                          },
    /* Flow control */
    {WINED3DSIO_REP,          0, 1, WINED3DSIH_REP,          WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_ENDREP,       0, 0, WINED3DSIH_ENDREP,       WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_IF,           0, 1, WINED3DSIH_IF,           WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_IFC,          0, 2, WINED3DSIH_IFC,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_ELSE,         0, 0, WINED3DSIH_ELSE,         WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_ENDIF,        0, 0, WINED3DSIH_ENDIF,        WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_BREAK,        0, 0, WINED3DSIH_BREAK,        WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_BREAKC,       0, 2, WINED3DSIH_BREAKC,       WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_BREAKP,       0, 1, WINED3DSIH_BREAKP,       0,                           0                          },
    {WINED3DSIO_CALL,         0, 1, WINED3DSIH_CALL,         WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_CALLNZ,       0, 2, WINED3DSIH_CALLNZ,       WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_LOOP,         0, 2, WINED3DSIH_LOOP,         WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_RET,          0, 0, WINED3DSIH_RET,          WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_ENDLOOP,      0, 0, WINED3DSIH_ENDLOOP,      WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_LABEL,        0, 1, WINED3DSIH_LABEL,        WINED3D_SHADER_VERSION(2,0), -1                         },

    {WINED3DSIO_SETP,         1, 3, WINED3DSIH_SETP,         0,                           0                          },
    {WINED3DSIO_TEXLDL,       1, 3, WINED3DSIH_TEXLDL,       WINED3D_SHADER_VERSION(3,0), -1                         },
    {0,                       0, 0, WINED3DSIH_TABLE_SIZE,   0,                           0                          },
};

static const struct wined3d_sm1_opcode_info ps_opcode_table[] =
{
    /* Arithmetic */
    {WINED3DSIO_NOP,          0, 0, WINED3DSIH_NOP,          0,                           0                          },
    {WINED3DSIO_MOV,          1, 2, WINED3DSIH_MOV,          0,                           0                          },
    {WINED3DSIO_ADD,          1, 3, WINED3DSIH_ADD,          0,                           0                          },
    {WINED3DSIO_SUB,          1, 3, WINED3DSIH_SUB,          0,                           0                          },
    {WINED3DSIO_MAD,          1, 4, WINED3DSIH_MAD,          0,                           0                          },
    {WINED3DSIO_MUL,          1, 3, WINED3DSIH_MUL,          0,                           0                          },
    {WINED3DSIO_RCP,          1, 2, WINED3DSIH_RCP,          0,                           0                          },
    {WINED3DSIO_RSQ,          1, 2, WINED3DSIH_RSQ,          0,                           0                          },
    {WINED3DSIO_DP3,          1, 3, WINED3DSIH_DP3,          0,                           0                          },
    {WINED3DSIO_DP4,          1, 3, WINED3DSIH_DP4,          0,                           0                          },
    {WINED3DSIO_MIN,          1, 3, WINED3DSIH_MIN,          0,                           0                          },
    {WINED3DSIO_MAX,          1, 3, WINED3DSIH_MAX,          0,                           0                          },
    {WINED3DSIO_SLT,          1, 3, WINED3DSIH_SLT,          0,                           0                          },
    {WINED3DSIO_SGE,          1, 3, WINED3DSIH_SGE,          0,                           0                          },
    {WINED3DSIO_ABS,          1, 2, WINED3DSIH_ABS,          0,                           0                          },
    {WINED3DSIO_EXP,          1, 2, WINED3DSIH_EXP,          0,                           0                          },
    {WINED3DSIO_LOG,          1, 2, WINED3DSIH_LOG,          0,                           0                          },
    {WINED3DSIO_EXPP,         1, 2, WINED3DSIH_EXPP,         0,                           0                          },
    {WINED3DSIO_LOGP,         1, 2, WINED3DSIH_LOGP,         0,                           0                          },
    {WINED3DSIO_DST,          1, 3, WINED3DSIH_DST,          0,                           0                          },
    {WINED3DSIO_LRP,          1, 4, WINED3DSIH_LRP,          0,                           0                          },
    {WINED3DSIO_FRC,          1, 2, WINED3DSIH_FRC,          0,                           0                          },
    {WINED3DSIO_CND,          1, 4, WINED3DSIH_CND,          WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,4)},
    {WINED3DSIO_CMP,          1, 4, WINED3DSIH_CMP,          WINED3D_SHADER_VERSION(1,2), WINED3D_SHADER_VERSION(3,0)},
    {WINED3DSIO_POW,          1, 3, WINED3DSIH_POW,          0,                           0                          },
    {WINED3DSIO_CRS,          1, 3, WINED3DSIH_CRS,          0,                           0                          },
    {WINED3DSIO_NRM,          1, 2, WINED3DSIH_NRM,          0,                           0                          },
    {WINED3DSIO_SINCOS,       1, 4, WINED3DSIH_SINCOS,       WINED3D_SHADER_VERSION(2,0), WINED3D_SHADER_VERSION(2,1)},
    {WINED3DSIO_SINCOS,       1, 2, WINED3DSIH_SINCOS,       WINED3D_SHADER_VERSION(3,0), -1                         },
    {WINED3DSIO_DP2ADD,       1, 4, WINED3DSIH_DP2ADD,       WINED3D_SHADER_VERSION(2,0), -1                         },
    /* Matrix */
    {WINED3DSIO_M4x4,         1, 3, WINED3DSIH_M4x4,         0,                           0                          },
    {WINED3DSIO_M4x3,         1, 3, WINED3DSIH_M4x3,         0,                           0                          },
    {WINED3DSIO_M3x4,         1, 3, WINED3DSIH_M3x4,         0,                           0                          },
    {WINED3DSIO_M3x3,         1, 3, WINED3DSIH_M3x3,         0,                           0                          },
    {WINED3DSIO_M3x2,         1, 3, WINED3DSIH_M3x2,         0,                           0                          },
    /* Register declarations */
    {WINED3DSIO_DCL,          0, 2, WINED3DSIH_DCL,          0,                           0                          },
    /* Flow control */
    {WINED3DSIO_REP,          0, 1, WINED3DSIH_REP,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_ENDREP,       0, 0, WINED3DSIH_ENDREP,       WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_IF,           0, 1, WINED3DSIH_IF,           WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_IFC,          0, 2, WINED3DSIH_IFC,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_ELSE,         0, 0, WINED3DSIH_ELSE,         WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_ENDIF,        0, 0, WINED3DSIH_ENDIF,        WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_BREAK,        0, 0, WINED3DSIH_BREAK,        WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_BREAKC,       0, 2, WINED3DSIH_BREAKC,       WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_BREAKP,       0, 1, WINED3DSIH_BREAKP,       0,                           0                          },
    {WINED3DSIO_CALL,         0, 1, WINED3DSIH_CALL,         WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_CALLNZ,       0, 2, WINED3DSIH_CALLNZ,       WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_LOOP,         0, 2, WINED3DSIH_LOOP,         WINED3D_SHADER_VERSION(3,0), -1                         },
    {WINED3DSIO_RET,          0, 0, WINED3DSIH_RET,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_ENDLOOP,      0, 0, WINED3DSIH_ENDLOOP,      WINED3D_SHADER_VERSION(3,0), -1                         },
    {WINED3DSIO_LABEL,        0, 1, WINED3DSIH_LABEL,        WINED3D_SHADER_VERSION(2,1), -1                         },
    /* Constant definitions */
    {WINED3DSIO_DEF,          1, 5, WINED3DSIH_DEF,          0,                           0                          },
    {WINED3DSIO_DEFB,         1, 2, WINED3DSIH_DEFB,         0,                           0                          },
    {WINED3DSIO_DEFI,         1, 5, WINED3DSIH_DEFI,         0,                           0                          },
    /* Texture */
    {WINED3DSIO_TEXCOORD,     1, 1, WINED3DSIH_TEXCOORD,     0,                           WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXCOORD,     1, 2, WINED3DSIH_TEXCOORD,     WINED3D_SHADER_VERSION(1,4), WINED3D_SHADER_VERSION(1,4)},
    {WINED3DSIO_TEXKILL,      1, 1, WINED3DSIH_TEXKILL,      WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(3,0)},
    {WINED3DSIO_TEX,          1, 1, WINED3DSIH_TEX,          0,                           WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEX,          1, 2, WINED3DSIH_TEX,          WINED3D_SHADER_VERSION(1,4), WINED3D_SHADER_VERSION(1,4)},
    {WINED3DSIO_TEX,          1, 3, WINED3DSIH_TEX,          WINED3D_SHADER_VERSION(2,0), -1                         },
    {WINED3DSIO_TEXBEM,       1, 2, WINED3DSIH_TEXBEM,       0,                           WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXBEML,      1, 2, WINED3DSIH_TEXBEML,      WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXREG2AR,    1, 2, WINED3DSIH_TEXREG2AR,    WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXREG2GB,    1, 2, WINED3DSIH_TEXREG2GB,    WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXREG2RGB,   1, 2, WINED3DSIH_TEXREG2RGB,   WINED3D_SHADER_VERSION(1,2), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2PAD,   1, 2, WINED3DSIH_TEXM3x2PAD,   WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2TEX,   1, 2, WINED3DSIH_TEXM3x2TEX,   WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3PAD,   1, 2, WINED3DSIH_TEXM3x3PAD,   WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3DIFF,  1, 2, WINED3DSIH_TEXM3x3DIFF,  WINED3D_SHADER_VERSION(0,0), WINED3D_SHADER_VERSION(0,0)},
    {WINED3DSIO_TEXM3x3SPEC,  1, 3, WINED3DSIH_TEXM3x3SPEC,  WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3VSPEC, 1, 2, WINED3DSIH_TEXM3x3VSPEC, WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3TEX,   1, 2, WINED3DSIH_TEXM3x3TEX,   WINED3D_SHADER_VERSION(1,0), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXDP3TEX,    1, 2, WINED3DSIH_TEXDP3TEX,    WINED3D_SHADER_VERSION(1,2), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2DEPTH, 1, 2, WINED3DSIH_TEXM3x2DEPTH, WINED3D_SHADER_VERSION(1,3), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXDP3,       1, 2, WINED3DSIH_TEXDP3,       WINED3D_SHADER_VERSION(1,2), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3,      1, 2, WINED3DSIH_TEXM3x3,      WINED3D_SHADER_VERSION(1,2), WINED3D_SHADER_VERSION(1,3)},
    {WINED3DSIO_TEXDEPTH,     1, 1, WINED3DSIH_TEXDEPTH,     WINED3D_SHADER_VERSION(1,4), WINED3D_SHADER_VERSION(1,4)},
    {WINED3DSIO_BEM,          1, 3, WINED3DSIH_BEM,          WINED3D_SHADER_VERSION(1,4), WINED3D_SHADER_VERSION(1,4)},
    {WINED3DSIO_DSX,          1, 2, WINED3DSIH_DSX,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_DSY,          1, 2, WINED3DSIH_DSY,          WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_TEXLDD,       1, 5, WINED3DSIH_TEXLDD,       WINED3D_SHADER_VERSION(2,1), -1                         },
    {WINED3DSIO_SETP,         1, 3, WINED3DSIH_SETP,         0,                           0                          },
    {WINED3DSIO_TEXLDL,       1, 3, WINED3DSIH_TEXLDL,       WINED3D_SHADER_VERSION(3,0), -1                         },
    {WINED3DSIO_PHASE,        0, 0, WINED3DSIH_PHASE,        0,                           0                          },
    {0,                       0, 0, WINED3DSIH_TABLE_SIZE,   0,                           0                          },
};

/* Read a parameter opcode from the input stream,
 * and possibly a relative addressing token.
 * Return the number of tokens read */
static int shader_get_param(const struct wined3d_sm1_data *priv, const DWORD *ptr, DWORD *token, DWORD *addr_token)
{
    UINT count = 1;

    *token = *ptr;

    /* PS >= 3.0 have relative addressing (with token)
     * VS >= 2.0 have relative addressing (with token)
     * VS >= 1.0 < 2.0 have relative addressing (without token)
     * The version check below should work in general */
    if (*ptr & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        if (priv->shader_version.major < 2)
        {
            *addr_token = (1 << 31)
                    | ((WINED3DSPR_ADDR << WINED3DSP_REGTYPE_SHIFT2) & WINED3DSP_REGTYPE_MASK2)
                    | ((WINED3DSPR_ADDR << WINED3DSP_REGTYPE_SHIFT) & WINED3DSP_REGTYPE_MASK)
                    | (WINED3DSP_NOSWIZZLE << WINED3DSP_SWIZZLE_SHIFT);
        }
        else
        {
            *addr_token = *(ptr + 1);
            ++count;
        }
    }

    return count;
}

static const struct wined3d_sm1_opcode_info *shader_get_opcode(const struct wined3d_sm1_data *priv, DWORD code)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(priv->shader_version.major, priv->shader_version.minor);
    const struct wined3d_sm1_opcode_info *opcode_table = priv->opcode_table;
    DWORD i = 0;

    while (opcode_table[i].handler_idx != WINED3DSIH_TABLE_SIZE)
    {
        if ((code & WINED3DSI_OPCODE_MASK) == opcode_table[i].opcode
                && shader_version >= opcode_table[i].min_version
                && (!opcode_table[i].max_version || shader_version <= opcode_table[i].max_version))
        {
            return &opcode_table[i];
        }
        ++i;
    }

    FIXME("Unsupported opcode %#x(%d) masked %#x, shader version %#x\n",
            code, code, code & WINED3DSI_OPCODE_MASK, shader_version);

    return NULL;
}

/* Return the number of parameters to skip for an opcode */
static int shader_skip_opcode(const struct wined3d_sm1_data *priv,
        const struct wined3d_sm1_opcode_info *opcode_info, DWORD opcode_token)
{
   /* Shaders >= 2.0 may contain address tokens, but fortunately they
    * have a useful length mask - use it here. Shaders 1.0 contain no such tokens */
    return (priv->shader_version.major >= 2)
            ? ((opcode_token & WINED3DSI_INSTLENGTH_MASK) >> WINED3DSI_INSTLENGTH_SHIFT) : opcode_info->param_count;
}

static void shader_parse_src_param(DWORD param, const struct wined3d_shader_src_param *rel_addr,
        struct wined3d_shader_src_param *src)
{
    src->reg.type = ((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT)
            | ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2);
    src->reg.idx = param & WINED3DSP_REGNUM_MASK;
    src->swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    src->modifiers = (param & WINED3DSP_SRCMOD_MASK) >> WINED3DSP_SRCMOD_SHIFT;
    src->reg.rel_addr = rel_addr;
}

static void shader_parse_dst_param(DWORD param, const struct wined3d_shader_src_param *rel_addr,
        struct wined3d_shader_dst_param *dst)
{
    dst->reg.type = ((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT)
            | ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2);
    dst->reg.idx = param & WINED3DSP_REGNUM_MASK;
    dst->write_mask = (param & WINED3D_SM1_WRITEMASK_MASK) >> WINED3D_SM1_WRITEMASK_SHIFT;
    dst->modifiers = (param & WINED3DSP_DSTMOD_MASK) >> WINED3DSP_DSTMOD_SHIFT;
    dst->shift = (param & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    dst->reg.rel_addr = rel_addr;
}

/* Read the parameters of an unrecognized opcode from the input stream
 * Return the number of tokens read.
 *
 * Note: This function assumes source or destination token format.
 * It will not work with specially-formatted tokens like DEF or DCL,
 * but hopefully those would be recognized */
static int shader_skip_unrecognized(const struct wined3d_sm1_data *priv, const DWORD *ptr)
{
    int tokens_read = 0;
    int i = 0;

    /* TODO: Think of a good name for 0x80000000 and replace it with a constant */
    while (*ptr & 0x80000000)
    {
        DWORD token, addr_token = 0;
        struct wined3d_shader_src_param rel_addr;

        tokens_read += shader_get_param(priv, ptr, &token, &addr_token);
        ptr += tokens_read;

        FIXME("Unrecognized opcode param: token=0x%08x addr_token=0x%08x name=", token, addr_token);

        if (token & WINED3DSHADER_ADDRMODE_RELATIVE) shader_parse_src_param(addr_token, NULL, &rel_addr);

        if (!i)
        {
            struct wined3d_shader_dst_param dst;

            shader_parse_dst_param(token, token & WINED3DSHADER_ADDRMODE_RELATIVE ? &rel_addr : NULL, &dst);
            shader_dump_dst_param(&dst, &priv->shader_version);
        }
        else
        {
            struct wined3d_shader_src_param src;

            shader_parse_src_param(token, token & WINED3DSHADER_ADDRMODE_RELATIVE ? &rel_addr : NULL, &src);
            shader_dump_src_param(&src, &priv->shader_version);
        }
        FIXME("\n");
        ++i;
    }
    return tokens_read;
}

static void *shader_sm1_init(const DWORD *byte_code, const struct wined3d_shader_signature *output_signature)
{
    struct wined3d_sm1_data *priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv));
    if (!priv)
    {
        ERR("Failed to allocate private data\n");
        return NULL;
    }

    if (output_signature)
    {
        FIXME("SM 1-3 shader shouldn't have output signatures.\n");
    }

    switch (*byte_code >> 16)
    {
        case WINED3D_SM1_VS:
            priv->shader_version.type = WINED3D_SHADER_TYPE_VERTEX;
            priv->opcode_table = vs_opcode_table;
            break;

        case WINED3D_SM1_PS:
            priv->shader_version.type = WINED3D_SHADER_TYPE_PIXEL;
            priv->opcode_table = ps_opcode_table;
            break;

        default:
            FIXME("Unrecognized shader type %#x\n", *byte_code >> 16);
            HeapFree(GetProcessHeap(), 0, priv);
            return NULL;
    }

    return priv;
}

static void shader_sm1_free(void *data)
{
    HeapFree(GetProcessHeap(), 0, data);
}

static void shader_sm1_read_header(void *data, const DWORD **ptr, struct wined3d_shader_version *shader_version)
{
    struct wined3d_sm1_data *priv = data;
    DWORD version_token;

    version_token = *(*ptr)++;
    TRACE("version: 0x%08x\n", version_token);

    priv->shader_version.major = WINED3D_SM1_VERSION_MAJOR(version_token);
    priv->shader_version.minor = WINED3D_SM1_VERSION_MINOR(version_token);
    *shader_version = priv->shader_version;
}

static void shader_sm1_read_opcode(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins,
        UINT *param_size)
{
    struct wined3d_sm1_data *priv = data;
    const struct wined3d_sm1_opcode_info *opcode_info;
    DWORD opcode_token;

    opcode_token = *(*ptr)++;
    opcode_info = shader_get_opcode(priv, opcode_token);
    if (!opcode_info)
    {
        FIXME("Unrecognized opcode: token=0x%08x\n", opcode_token);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        *param_size = shader_skip_unrecognized(priv, *ptr);
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = (opcode_token & WINED3D_OPCODESPECIFICCONTROL_MASK) >> WINED3D_OPCODESPECIFICCONTROL_SHIFT;
    ins->coissue = opcode_token & WINED3DSI_COISSUE;
    ins->predicate = opcode_token & WINED3DSHADER_INSTRUCTION_PREDICATED;
    ins->dst_count = opcode_info->dst_count ? 1 : 0;
    ins->src_count = opcode_info->param_count - opcode_info->dst_count;
    *param_size = shader_skip_opcode(priv, opcode_info, opcode_token);
}

static void shader_sm1_read_src_param(void *data, const DWORD **ptr, struct wined3d_shader_src_param *src_param,
        struct wined3d_shader_src_param *src_rel_addr)
{
    struct wined3d_sm1_data *priv = data;
    DWORD token, addr_token;

    *ptr += shader_get_param(priv, *ptr, &token, &addr_token);
    if (token & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        shader_parse_src_param(addr_token, NULL, src_rel_addr);
        shader_parse_src_param(token, src_rel_addr, src_param);
    }
    else
    {
        shader_parse_src_param(token, NULL, src_param);
    }
}

static void shader_sm1_read_dst_param(void *data, const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
        struct wined3d_shader_src_param *dst_rel_addr)
{
    struct wined3d_sm1_data *priv = data;
    DWORD token, addr_token;

    *ptr += shader_get_param(priv, *ptr, &token, &addr_token);
    if (token & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        shader_parse_src_param(addr_token, NULL, dst_rel_addr);
        shader_parse_dst_param(token, dst_rel_addr, dst_param);
    }
    else
    {
        shader_parse_dst_param(token, NULL, dst_param);
    }
}

static void shader_sm1_read_semantic(const DWORD **ptr, struct wined3d_shader_semantic *semantic)
{
    DWORD usage_token = *(*ptr)++;
    DWORD dst_token = *(*ptr)++;

    semantic->usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
    semantic->sampler_type = (usage_token & WINED3DSP_TEXTURETYPE_MASK) >> WINED3DSP_TEXTURETYPE_SHIFT;
    shader_parse_dst_param(dst_token, NULL, &semantic->reg);
}

static void shader_sm1_read_comment(const DWORD **ptr, const char **comment)
{
    DWORD token = **ptr;

    if ((token & WINED3DSI_OPCODE_MASK) != WINED3DSIO_COMMENT)
    {
        *comment = NULL;
        return;
    }

    *comment = (const char *)++(*ptr);
    *ptr += (token & WINED3DSI_COMMENTSIZE_MASK) >> WINED3DSI_COMMENTSIZE_SHIFT;
}

static BOOL shader_sm1_is_end(void *data, const DWORD **ptr)
{
    if (**ptr == WINED3DSP_END)
    {
        ++(*ptr);
        return TRUE;
    }

    return FALSE;
}

const struct wined3d_shader_frontend sm1_shader_frontend =
{
    shader_sm1_init,
    shader_sm1_free,
    shader_sm1_read_header,
    shader_sm1_read_opcode,
    shader_sm1_read_src_param,
    shader_sm1_read_dst_param,
    shader_sm1_read_semantic,
    shader_sm1_read_comment,
    shader_sm1_is_end,
};
