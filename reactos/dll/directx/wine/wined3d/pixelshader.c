/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
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

#include "config.h"

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define GLINFO_LOCATION ((IWineD3DDeviceImpl *) This->baseShader.device)->adapter->gl_info

static HRESULT  WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj) {
    TRACE("iface %p, riid %s, ppobj %p\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IWineD3DPixelShader)
            || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppobj = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&This->baseShader.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedDecrement(&This->baseShader.ref);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        shader_cleanup((IWineD3DBaseShader *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* *******************************************
   IWineD3DPixelShader IWineD3DPixelShader parts follow
   ******************************************* */

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetParent(IWineD3DPixelShader *iface, IUnknown** parent){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;

    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetDevice(IWineD3DPixelShader* iface, IWineD3DDevice **pDevice){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    IWineD3DDevice_AddRef(This->baseShader.device);
    *pDevice = This->baseShader.device;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}


static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetFunction(IWineD3DPixelShader* impl, VOID* pData, UINT* pSizeOfData) {
  IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)impl;
  TRACE("(%p) : pData(%p), pSizeOfData(%p)\n", This, pData, pSizeOfData);

  if (NULL == pData) {
    *pSizeOfData = This->baseShader.functionLength;
    return WINED3D_OK;
  }
  if (*pSizeOfData < This->baseShader.functionLength) {
    /* MSDN claims (for d3d8 at least) that if *pSizeOfData is smaller
     * than the required size we should write the required size and
     * return D3DERR_MOREDATA. That's not actually true. */
    return WINED3DERR_INVALIDCALL;
  }

  TRACE("(%p) : GetFunction copying to %p\n", This, pData);
  memcpy(pData, This->baseShader.function, This->baseShader.functionLength);

  return WINED3D_OK;
}

CONST SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[] = {
    /* Arithmetic */
    {WINED3DSIO_NOP,          "nop",          0, 0, WINED3DSIH_NOP,          0,                      0                     },
    {WINED3DSIO_MOV,          "mov",          1, 2, WINED3DSIH_MOV,          0,                      0                     },
    {WINED3DSIO_ADD,          "add",          1, 3, WINED3DSIH_ADD,          0,                      0                     },
    {WINED3DSIO_SUB,          "sub",          1, 3, WINED3DSIH_SUB,          0,                      0                     },
    {WINED3DSIO_MAD,          "mad",          1, 4, WINED3DSIH_MAD,          0,                      0                     },
    {WINED3DSIO_MUL,          "mul",          1, 3, WINED3DSIH_MUL,          0,                      0                     },
    {WINED3DSIO_RCP,          "rcp",          1, 2, WINED3DSIH_RCP,          0,                      0                     },
    {WINED3DSIO_RSQ,          "rsq",          1, 2, WINED3DSIH_RSQ,          0,                      0                     },
    {WINED3DSIO_DP3,          "dp3",          1, 3, WINED3DSIH_DP3,          0,                      0                     },
    {WINED3DSIO_DP4,          "dp4",          1, 3, WINED3DSIH_DP4,          0,                      0                     },
    {WINED3DSIO_MIN,          "min",          1, 3, WINED3DSIH_MIN,          0,                      0                     },
    {WINED3DSIO_MAX,          "max",          1, 3, WINED3DSIH_MAX,          0,                      0                     },
    {WINED3DSIO_SLT,          "slt",          1, 3, WINED3DSIH_SLT,          0,                      0                     },
    {WINED3DSIO_SGE,          "sge",          1, 3, WINED3DSIH_SGE,          0,                      0                     },
    {WINED3DSIO_ABS,          "abs",          1, 2, WINED3DSIH_ABS,          0,                      0                     },
    {WINED3DSIO_EXP,          "exp",          1, 2, WINED3DSIH_EXP,          0,                      0                     },
    {WINED3DSIO_LOG,          "log",          1, 2, WINED3DSIH_LOG,          0,                      0                     },
    {WINED3DSIO_EXPP,         "expp",         1, 2, WINED3DSIH_EXPP,         0,                      0                     },
    {WINED3DSIO_LOGP,         "logp",         1, 2, WINED3DSIH_LOGP,         0,                      0                     },
    {WINED3DSIO_DST,          "dst",          1, 3, WINED3DSIH_DST,          0,                      0                     },
    {WINED3DSIO_LRP,          "lrp",          1, 4, WINED3DSIH_LRP,          0,                      0                     },
    {WINED3DSIO_FRC,          "frc",          1, 2, WINED3DSIH_FRC,          0,                      0                     },
    {WINED3DSIO_CND,          "cnd",          1, 4, WINED3DSIH_CND,          WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_CMP,          "cmp",          1, 4, WINED3DSIH_CMP,          WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(3,0)},
    {WINED3DSIO_POW,          "pow",          1, 3, WINED3DSIH_POW,          0,                      0                     },
    {WINED3DSIO_CRS,          "crs",          1, 3, WINED3DSIH_CRS,          0,                      0                     },
    {WINED3DSIO_NRM,          "nrm",          1, 2, WINED3DSIH_NRM,          0,                      0                     },
    {WINED3DSIO_SINCOS,       "sincos",       1, 4, WINED3DSIH_SINCOS,       WINED3DPS_VERSION(2,0), WINED3DPS_VERSION(2,1)},
    {WINED3DSIO_SINCOS,       "sincos",       1, 2, WINED3DSIH_SINCOS,       WINED3DPS_VERSION(3,0), -1                    },
    {WINED3DSIO_DP2ADD,       "dp2add",       1, 4, WINED3DSIH_DP2ADD,       WINED3DPS_VERSION(2,0), -1                    },
    /* Matrix */
    {WINED3DSIO_M4x4,         "m4x4",         1, 3, WINED3DSIH_M4x4,         0,                      0                     },
    {WINED3DSIO_M4x3,         "m4x3",         1, 3, WINED3DSIH_M4x3,         0,                      0                     },
    {WINED3DSIO_M3x4,         "m3x4",         1, 3, WINED3DSIH_M3x4,         0,                      0                     },
    {WINED3DSIO_M3x3,         "m3x3",         1, 3, WINED3DSIH_M3x3,         0,                      0                     },
    {WINED3DSIO_M3x2,         "m3x2",         1, 3, WINED3DSIH_M3x2,         0,                      0                     },
    /* Register declarations */
    {WINED3DSIO_DCL,          "dcl",          0, 2, WINED3DSIH_DCL,          0,                      0                     },
    /* Flow control - requires GLSL or software shaders */
    {WINED3DSIO_REP ,         "rep",          0, 1, WINED3DSIH_REP,          WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_ENDREP,       "endrep",       0, 0, WINED3DSIH_ENDREP,       WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_IF,           "if",           0, 1, WINED3DSIH_IF,           WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_IFC,          "ifc",          0, 2, WINED3DSIH_IFC,          WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_ELSE,         "else",         0, 0, WINED3DSIH_ELSE,         WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_ENDIF,        "endif",        0, 0, WINED3DSIH_ENDIF,        WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAK,        "break",        0, 0, WINED3DSIH_BREAK,        WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKC,       "breakc",       0, 2, WINED3DSIH_BREAKC,       WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKP,       "breakp",       0, 1, WINED3DSIH_BREAKP,       0,                      0                     },
    {WINED3DSIO_CALL,         "call",         0, 1, WINED3DSIH_CALL,         WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_CALLNZ,       "callnz",       0, 2, WINED3DSIH_CALLNZ,       WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_LOOP,         "loop",         0, 2, WINED3DSIH_LOOP,         WINED3DPS_VERSION(3,0), -1                    },
    {WINED3DSIO_RET,          "ret",          0, 0, WINED3DSIH_RET,          WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_ENDLOOP,      "endloop",      0, 0, WINED3DSIH_ENDLOOP,      WINED3DPS_VERSION(3,0), -1                    },
    {WINED3DSIO_LABEL,        "label",        0, 1, WINED3DSIH_LABEL,        WINED3DPS_VERSION(2,1), -1                    },
    /* Constant definitions */
    {WINED3DSIO_DEF,          "def",          1, 5, WINED3DSIH_DEF,          0,                      0                     },
    {WINED3DSIO_DEFB,         "defb",         1, 2, WINED3DSIH_DEFB,         0,                      0                     },
    {WINED3DSIO_DEFI,         "defi",         1, 5, WINED3DSIH_DEFI,         0,                      0                     },
    /* Texture */
    {WINED3DSIO_TEXCOORD,     "texcoord",     1, 1, WINED3DSIH_TEXCOORD,     0,                      WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXCOORD,     "texcrd",       1, 2, WINED3DSIH_TEXCOORD,     WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_TEXKILL,      "texkill",      1, 1, WINED3DSIH_TEXKILL,      WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(3,0)},
    {WINED3DSIO_TEX,          "tex",          1, 1, WINED3DSIH_TEX,          0,                      WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEX,          "texld",        1, 2, WINED3DSIH_TEX,          WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_TEX,          "texld",        1, 3, WINED3DSIH_TEX,          WINED3DPS_VERSION(2,0), -1                    },
    {WINED3DSIO_TEXBEM,       "texbem",       1, 2, WINED3DSIH_TEXBEM,       0,                      WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXBEML,      "texbeml",      1, 2, WINED3DSIH_TEXBEML,      WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2AR,    "texreg2ar",    1, 2, WINED3DSIH_TEXREG2AR,    WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2GB,    "texreg2gb",    1, 2, WINED3DSIH_TEXREG2GB,    WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2RGB,   "texreg2rgb",   1, 2, WINED3DSIH_TEXREG2RGB,   WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2PAD,   "texm3x2pad",   1, 2, WINED3DSIH_TEXM3x2PAD,   WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2TEX,   "texm3x2tex",   1, 2, WINED3DSIH_TEXM3x2TEX,   WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3PAD,   "texm3x3pad",   1, 2, WINED3DSIH_TEXM3x3PAD,   WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3DIFF,  "texm3x3diff",  1, 2, WINED3DSIH_TEXM3x3DIFF,  WINED3DPS_VERSION(0,0), WINED3DPS_VERSION(0,0)},
    {WINED3DSIO_TEXM3x3SPEC,  "texm3x3spec",  1, 3, WINED3DSIH_TEXM3x3SPEC,  WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3VSPEC, "texm3x3vspec", 1, 2, WINED3DSIH_TEXM3x3VSPEC, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3TEX,   "texm3x3tex",   1, 2, WINED3DSIH_TEXM3x3TEX,   WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDP3TEX,    "texdp3tex",    1, 2, WINED3DSIH_TEXDP3TEX,    WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2DEPTH, "texm3x2depth", 1, 2, WINED3DSIH_TEXM3x2DEPTH, WINED3DPS_VERSION(1,3), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDP3,       "texdp3",       1, 2, WINED3DSIH_TEXDP3,       WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3,      "texm3x3",      1, 2, WINED3DSIH_TEXM3x3,      WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDEPTH,     "texdepth",     1, 1, WINED3DSIH_TEXDEPTH,     WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_BEM,          "bem",          1, 3, WINED3DSIH_BEM,          WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_DSX,          "dsx",          1, 2, WINED3DSIH_DSX,          WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_DSY,          "dsy",          1, 2, WINED3DSIH_DSY,          WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_TEXLDD,       "texldd",       1, 5, WINED3DSIH_TEXLDD,       WINED3DPS_VERSION(2,1), -1                    },
    {WINED3DSIO_SETP,         "setp",         1, 3, WINED3DSIH_SETP,         0,                      0                     },
    {WINED3DSIO_TEXLDL,       "texldl",       1, 3, WINED3DSIH_TEXLDL,       WINED3DPS_VERSION(3,0), -1                    },
    {WINED3DSIO_PHASE,        "phase",        0, 0, WINED3DSIH_PHASE,        0,                      0                     },
    {0,                       NULL,           0, 0, 0,                       0,                      0                     }
};

static void pshader_set_limits(
      IWineD3DPixelShaderImpl *This) { 

      This->baseShader.limits.attributes = 0;
      This->baseShader.limits.address = 0;
      This->baseShader.limits.packed_output = 0;

      switch (This->baseShader.reg_maps.shader_version)
      {
          case WINED3DPS_VERSION(1,0):
          case WINED3DPS_VERSION(1,1):
          case WINED3DPS_VERSION(1,2):
          case WINED3DPS_VERSION(1,3): 
                   This->baseShader.limits.temporary = 2;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texcoord = 4;
                   This->baseShader.limits.sampler = 4;
                   This->baseShader.limits.packed_input = 0;
                   This->baseShader.limits.label = 0;
                   break;

          case WINED3DPS_VERSION(1,4):
                   This->baseShader.limits.temporary = 6;
                   This->baseShader.limits.constant_float = 8;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.texcoord = 6;
                   This->baseShader.limits.sampler = 6;
                   This->baseShader.limits.packed_input = 0;
                   This->baseShader.limits.label = 0;
                   break;
               
          /* FIXME: temporaries must match D3DPSHADERCAPS2_0.NumTemps */ 
          case WINED3DPS_VERSION(2,0):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 8;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 0;
                   break;

          case WINED3DPS_VERSION(2,1):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 8;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 0;
                   This->baseShader.limits.label = 16;
                   break;

          case WINED3DPS_VERSION(3,0):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 224;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 0;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 12;
                   This->baseShader.limits.label = 16; /* FIXME: 2048 */
                   break;

          default: This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_float = 32;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.texcoord = 8;
                   This->baseShader.limits.sampler = 16;
                   This->baseShader.limits.packed_input = 0;
                   This->baseShader.limits.label = 0;
                   FIXME("Unrecognized pixel shader version %#x\n",
                           This->baseShader.reg_maps.shader_version);
      }
}

static HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {

    IWineD3DPixelShaderImpl *This =(IWineD3DPixelShaderImpl *)iface;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    HRESULT hr;

    TRACE("(%p) : pFunction %p\n", iface, pFunction);

    /* First pass: trace shader */
    if (TRACE_ON(d3d_shader)) shader_trace_init(pFunction, This->baseShader.shader_ins);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    /* Second pass: figure out which registers are used, what the semantics are, etc.. */
    memset(reg_maps, 0, sizeof(shader_reg_maps));
    hr = shader_get_registers_used((IWineD3DBaseShader *)This, reg_maps, This->semantics_in, NULL, pFunction);
    if (FAILED(hr)) return hr;

    pshader_set_limits(This);

    for (i = 0; i < MAX_REG_INPUT; ++i)
    {
        if (This->input_reg_used[i])
        {
            ++num_regs_used;
            highest_reg_used = i;
        }
    }

    /* Don't do any register mapping magic if it is not needed, or if we can't
     * achieve anything anyway */
    if (highest_reg_used < (GL_LIMITS(glsl_varyings) / 4)
            || num_regs_used > (GL_LIMITS(glsl_varyings) / 4))
    {
        if (num_regs_used > (GL_LIMITS(glsl_varyings) / 4))
        {
            /* This happens with relative addressing. The input mapper function
             * warns about this if the higher registers are declared too, so
             * don't write a FIXME here */
            WARN("More varying registers used than supported\n");
        }

        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            This->input_reg_map[i] = i;
        }

        This->declared_in_count = highest_reg_used + 1;
    }
    else
    {
        This->declared_in_count = 0;
        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            if (This->input_reg_used[i]) This->input_reg_map[i] = This->declared_in_count++;
            else This->input_reg_map[i] = ~0U;
        }
    }

    This->baseShader.load_local_constsF = FALSE;

    This->baseShader.shader_mode = deviceImpl->ps_selected_mode;

    TRACE("(%p) : Copying the function\n", This);

    This->baseShader.function = HeapAlloc(GetProcessHeap(), 0, This->baseShader.functionLength);
    if (!This->baseShader.function) return E_OUTOFMEMORY;
    memcpy(This->baseShader.function, pFunction, This->baseShader.functionLength);

    return WINED3D_OK;
}

static void pixelshader_update_samplers(struct shader_reg_maps *reg_maps, IWineD3DBaseTexture * const *textures)
{
    DWORD shader_version = reg_maps->shader_version;
    DWORD *samplers = reg_maps->samplers;
    unsigned int i;

    if (WINED3DSHADER_VERSION_MAJOR(shader_version) != 1) return;

    for (i = 0; i < max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS); ++i)
    {
        /* We don't sample from this sampler */
        if (!samplers[i]) continue;

        if (!textures[i])
        {
            ERR("No texture bound to sampler %u, using 2D\n", i);
            samplers[i] = (0x1 << 31) | WINED3DSTT_2D;
            continue;
        }

        switch (IWineD3DBaseTexture_GetTextureDimensions(textures[i]))
        {
            case GL_TEXTURE_RECTANGLE_ARB:
            case GL_TEXTURE_2D:
                /* We have to select between texture rectangles and 2D textures later because 2.0 and
                 * 3.0 shaders only have WINED3DSTT_2D as well */
                samplers[i] = (1 << 31) | WINED3DSTT_2D;
                break;

            case GL_TEXTURE_3D:
                samplers[i] = (1 << 31) | WINED3DSTT_VOLUME;
                break;

            case GL_TEXTURE_CUBE_MAP_ARB:
                samplers[i] = (1 << 31) | WINED3DSTT_CUBE;
                break;

            default:
                FIXME("Unrecognized texture type %#x, using 2D\n",
                        IWineD3DBaseTexture_GetTextureDimensions(textures[i]));
                samplers[i] = (0x1 << 31) | WINED3DSTT_2D;
        }
    }
}

static GLuint pixelshader_compile(IWineD3DPixelShaderImpl *This, const struct ps_compile_args *args)
{
    CONST DWORD *function = This->baseShader.function;
    GLuint retval;
    SHADER_BUFFER buffer;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;

    TRACE("(%p) : function %p\n", This, function);

    pixelshader_update_samplers(&This->baseShader.reg_maps,
            ((IWineD3DDeviceImpl *)This->baseShader.device)->stateBlock->textures);

    /* Generate the HW shader */
    TRACE("(%p) : Generating hardware program\n", This);
    This->cur_args = args;
    shader_buffer_init(&buffer);
    retval = device->shader_backend->shader_generate_pshader((IWineD3DPixelShader *)This, &buffer, args);
    shader_buffer_free(&buffer);
    This->cur_args = NULL;

    return retval;
}

const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DPixelShaderImpl_QueryInterface,
    IWineD3DPixelShaderImpl_AddRef,
    IWineD3DPixelShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DPixelShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DPixelShaderImpl_SetFunction,
    /*** IWineD3DPixelShader methods ***/
    IWineD3DPixelShaderImpl_GetDevice,
    IWineD3DPixelShaderImpl_GetFunction
};

void find_ps_compile_args(IWineD3DPixelShaderImpl *shader, IWineD3DStateBlockImpl *stateblock, struct ps_compile_args *args) {
    UINT i;
    IWineD3DBaseTextureImpl *tex;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set */
    args->srgb_correction = stateblock->renderState[WINED3DRS_SRGBWRITEENABLE] ? 1 : 0;
    args->np2_fixup = 0;

    for(i = 0; i < MAX_FRAGMENT_SAMPLERS; i++) {
        if(shader->baseShader.reg_maps.samplers[i] == 0) continue;
        tex = (IWineD3DBaseTextureImpl *) stateblock->textures[i];
        if(!tex) {
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            continue;
        }
        args->color_fixup[i] = tex->resource.format_desc->color_fixup;

        /* Flag samplers that need NP2 texcoord fixup. */
        if(!tex->baseTexture.pow2Matrix_identity) {
            args->np2_fixup |= (1 << i);
        }
    }
    if (shader->baseShader.reg_maps.shader_version >= WINED3DPS_VERSION(3,0))
    {
        if (((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.position_transformed)
        {
            args->vp_mode = pretransformed;
        }
        else if (use_vs(stateblock))
        {
            args->vp_mode = vertexshader;
        } else {
            args->vp_mode = fixedfunction;
        }
        args->fog = FOG_OFF;
    } else {
        args->vp_mode = vertexshader;
        if(stateblock->renderState[WINED3DRS_FOGENABLE]) {
            switch(stateblock->renderState[WINED3DRS_FOGTABLEMODE]) {
                case WINED3DFOG_NONE:
                    if (((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.position_transformed
                            || use_vs(stateblock))
                    {
                        args->fog = FOG_LINEAR;
                        break;
                    }
                    switch(stateblock->renderState[WINED3DRS_FOGVERTEXMODE]) {
                        case WINED3DFOG_NONE: /* Drop through */
                        case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                        case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                        case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
                    }
                    break;

                case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
            }
        } else {
            args->fog = FOG_OFF;
        }
    }
}

GLuint find_gl_pshader(IWineD3DPixelShaderImpl *shader, const struct ps_compile_args *args)
{
    UINT i;
    DWORD new_size;
    struct ps_compiled_shader *new_array;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader->num_gl_shaders; i++) {
        if(memcmp(&shader->gl_shaders[i].args, args, sizeof(*args)) == 0) {
            return shader->gl_shaders[i].prgId;
        }
    }

    TRACE("No matching GL shader found, compiling a new shader\n");
    if(shader->shader_array_size == shader->num_gl_shaders) {
        if (shader->num_gl_shaders)
        {
            new_size = shader->shader_array_size + max(1, shader->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader->gl_shaders,
                                    new_size * sizeof(*shader->gl_shaders));
        } else {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*shader->gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader->gl_shaders = new_array;
        shader->shader_array_size = new_size;
    }

    shader->gl_shaders[shader->num_gl_shaders].args = *args;
    shader->gl_shaders[shader->num_gl_shaders].prgId = pixelshader_compile(shader, args);
    return shader->gl_shaders[shader->num_gl_shaders++].prgId;
}
