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

#define GLINFO_LOCATION ((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info

/* TODO: Vertex and Pixel shaders are almost identical, the only exception being the way that some of the data is looked up or the availability of some of the data i.e. some instructions are only valid for pshaders and some for vshaders
because of this the bulk of the software pipeline can be shared between pixel and vertex shaders... and it wouldn't surprise me if the program can be cross compiled using a large body of shared code */

#define GLNAME_REQUIRE_GLSL  ((const char *)1)

CONST SHADER_OPCODE IWineD3DVertexShaderImpl_shader_ins[] = {
    /* This table is not order or position dependent. */

    /* Arithmetic */
    {WINED3DSIO_NOP,     "nop",     0, 0, WINED3DSIH_NOP,     0,                      0                     },
    {WINED3DSIO_MOV,     "mov",     1, 2, WINED3DSIH_MOV,     0,                      0                     },
    {WINED3DSIO_MOVA,    "mova",    1, 2, WINED3DSIH_MOVA,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ADD,     "add",     1, 3, WINED3DSIH_ADD,     0,                      0                     },
    {WINED3DSIO_SUB,     "sub",     1, 3, WINED3DSIH_SUB,     0,                      0                     },
    {WINED3DSIO_MAD,     "mad",     1, 4, WINED3DSIH_MAD,     0,                      0                     },
    {WINED3DSIO_MUL,     "mul",     1, 3, WINED3DSIH_MUL,     0,                      0                     },
    {WINED3DSIO_RCP,     "rcp",     1, 2, WINED3DSIH_RCP,     0,                      0                     },
    {WINED3DSIO_RSQ,     "rsq",     1, 2, WINED3DSIH_RSQ,     0,                      0                     },
    {WINED3DSIO_DP3,     "dp3",     1, 3, WINED3DSIH_DP3,     0,                      0                     },
    {WINED3DSIO_DP4,     "dp4",     1, 3, WINED3DSIH_DP4,     0,                      0                     },
    {WINED3DSIO_MIN,     "min",     1, 3, WINED3DSIH_MIN,     0,                      0                     },
    {WINED3DSIO_MAX,     "max",     1, 3, WINED3DSIH_MAX,     0,                      0                     },
    {WINED3DSIO_SLT,     "slt",     1, 3, WINED3DSIH_SLT,     0,                      0                     },
    {WINED3DSIO_SGE,     "sge",     1, 3, WINED3DSIH_SGE,     0,                      0                     },
    {WINED3DSIO_ABS,     "abs",     1, 2, WINED3DSIH_ABS,     0,                      0                     },
    {WINED3DSIO_EXP,     "exp",     1, 2, WINED3DSIH_EXP,     0,                      0                     },
    {WINED3DSIO_LOG,     "log",     1, 2, WINED3DSIH_LOG,     0,                      0                     },
    {WINED3DSIO_EXPP,    "expp",    1, 2, WINED3DSIH_EXPP,    0,                      0                     },
    {WINED3DSIO_LOGP,    "logp",    1, 2, WINED3DSIH_LOGP,    0,                      0                     },
    {WINED3DSIO_LIT,     "lit",     1, 2, WINED3DSIH_LIT,     0,                      0                     },
    {WINED3DSIO_DST,     "dst",     1, 3, WINED3DSIH_DST,     0,                      0                     },
    {WINED3DSIO_LRP,     "lrp",     1, 4, WINED3DSIH_LRP,     0,                      0                     },
    {WINED3DSIO_FRC,     "frc",     1, 2, WINED3DSIH_FRC,     0,                      0                     },
    {WINED3DSIO_POW,     "pow",     1, 3, WINED3DSIH_POW,     0,                      0                     },
    {WINED3DSIO_CRS,     "crs",     1, 3, WINED3DSIH_CRS,     0,                      0                     },
    /* TODO: sng can possibly be performed as
        RCP tmp, vec
        MUL out, tmp, vec*/
    {WINED3DSIO_SGN,     "sgn",     1, 2, WINED3DSIH_SGN,     0,                      0                     },
    {WINED3DSIO_NRM,     "nrm",     1, 2, WINED3DSIH_NRM,     0,                      0                     },
    {WINED3DSIO_SINCOS,  "sincos",  1, 4, WINED3DSIH_SINCOS,  WINED3DVS_VERSION(2,0), WINED3DVS_VERSION(2,1)},
    {WINED3DSIO_SINCOS,  "sincos",  1, 2, WINED3DSIH_SINCOS,  WINED3DVS_VERSION(3,0), -1                    },
    /* Matrix */
    {WINED3DSIO_M4x4,    "m4x4",    1, 3, WINED3DSIH_M4x4,    0,                      0                     },
    {WINED3DSIO_M4x3,    "m4x3",    1, 3, WINED3DSIH_M4x3,    0,                      0                     },
    {WINED3DSIO_M3x4,    "m3x4",    1, 3, WINED3DSIH_M3x4,    0,                      0                     },
    {WINED3DSIO_M3x3,    "m3x3",    1, 3, WINED3DSIH_M3x3,    0,                      0                     },
    {WINED3DSIO_M3x2,    "m3x2",    1, 3, WINED3DSIH_M3x2,    0,                      0                     },
    /* Declare registers */
    {WINED3DSIO_DCL,     "dcl",     0, 2, WINED3DSIH_DCL,     0,                      0                     },
    /* Constant definitions */
    {WINED3DSIO_DEF,     "def",     1, 5, WINED3DSIH_DEF,     0,                      0                     },
    {WINED3DSIO_DEFB,    "defb",    1, 2, WINED3DSIH_DEFB,    0,                      0                     },
    {WINED3DSIO_DEFI,    "defi",    1, 5, WINED3DSIH_DEFI,    0,                      0                     },
    /* Flow control - requires GLSL or software shaders */
    {WINED3DSIO_REP ,    "rep",     0, 1, WINED3DSIH_REP,     WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDREP,  "endrep",  0, 0, WINED3DSIH_ENDREP,  WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_IF,      "if",      0, 1, WINED3DSIH_IF,      WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_IFC,     "ifc",     0, 2, WINED3DSIH_IFC,     WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_ELSE,    "else",    0, 0, WINED3DSIH_ELSE,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDIF,   "endif",   0, 0, WINED3DSIH_ENDIF,   WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_BREAK,   "break",   0, 0, WINED3DSIH_BREAK,   WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKC,  "breakc",  0, 2, WINED3DSIH_BREAKC,  WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKP,  "breakp",  0, 1, WINED3DSIH_BREAKP,  0,                      0                     },
    {WINED3DSIO_CALL,    "call",    0, 1, WINED3DSIH_CALL,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_CALLNZ,  "callnz",  0, 2, WINED3DSIH_CALLNZ,  WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_LOOP,    "loop",    0, 2, WINED3DSIH_LOOP,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_RET,     "ret",     0, 0, WINED3DSIH_RET,     WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDLOOP, "endloop", 0, 0, WINED3DSIH_ENDLOOP, WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_LABEL,   "label",   0, 1, WINED3DSIH_LABEL,   WINED3DVS_VERSION(2,0), -1                    },

    {WINED3DSIO_SETP,    "setp",    1, 3, WINED3DSIH_SETP,    0,                      0                     },
    {WINED3DSIO_TEXLDL,  "texldl",  1, 3, WINED3DSIH_TEXLDL,  WINED3DVS_VERSION(3,0), -1                    },
    {0,                  NULL,      0, 0, 0,                  0,                      0                     }
};

static void vshader_set_limits(
      IWineD3DVertexShaderImpl *This) {

      This->baseShader.limits.texcoord = 0;
      This->baseShader.limits.attributes = 16;
      This->baseShader.limits.packed_input = 0;

      /* Must match D3DCAPS9.MaxVertexShaderConst: at least 256 for vs_2_0 */
      This->baseShader.limits.constant_float = GL_LIMITS(vshader_constantsF);

      switch (This->baseShader.reg_maps.shader_version)
      {
          case WINED3DVS_VERSION(1,0):
          case WINED3DVS_VERSION(1,1):
                   This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 0;
                   This->baseShader.limits.constant_int = 0;
                   This->baseShader.limits.address = 1;
                   This->baseShader.limits.packed_output = 0;
                   This->baseShader.limits.sampler = 0;
                   This->baseShader.limits.label = 0;
                   break;
      
          case WINED3DVS_VERSION(2,0):
          case WINED3DVS_VERSION(2,1):
                   This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.address = 1;
                   This->baseShader.limits.packed_output = 0;
                   This->baseShader.limits.sampler = 0;
                   This->baseShader.limits.label = 16;
                   break;

          case WINED3DVS_VERSION(3,0):
                   This->baseShader.limits.temporary = 32;
                   This->baseShader.limits.constant_bool = 32;
                   This->baseShader.limits.constant_int = 32;
                   This->baseShader.limits.address = 1;
                   This->baseShader.limits.packed_output = 12;
                   This->baseShader.limits.sampler = 4;
                   This->baseShader.limits.label = 16; /* FIXME: 2048 */
                   break;

          default: This->baseShader.limits.temporary = 12;
                   This->baseShader.limits.constant_bool = 16;
                   This->baseShader.limits.constant_int = 16;
                   This->baseShader.limits.address = 1;
                   This->baseShader.limits.packed_output = 0;
                   This->baseShader.limits.sampler = 0;
                   This->baseShader.limits.label = 16;
                   FIXME("Unrecognized vertex shader version %#x\n",
                           This->baseShader.reg_maps.shader_version);
      }
}

/* This is an internal function,
 * used to create fake semantics for shaders
 * that don't have them - d3d8 shaders where the declaration
 * stores the register for each input
 */
static void vshader_set_input(
    IWineD3DVertexShaderImpl* This,
    unsigned int regnum,
    BYTE usage, BYTE usage_idx) {

    /* Fake usage: set reserved bit, usage, usage_idx */
    DWORD usage_token = (0x1 << 31) |
        (usage << WINED3DSP_DCL_USAGE_SHIFT) | (usage_idx << WINED3DSP_DCL_USAGEINDEX_SHIFT);

    /* Fake register; set reserved bit, regnum, type: input, wmask: all */
    DWORD reg_token = (0x1 << 31) |
        WINED3DSP_WRITEMASK_ALL | (WINED3DSPR_INPUT << WINED3DSP_REGTYPE_SHIFT) | regnum;

    This->semantics_in[regnum].usage = usage_token;
    This->semantics_in[regnum].reg = reg_token;
}

static BOOL match_usage(BYTE usage1, BYTE usage_idx1, BYTE usage2, BYTE usage_idx2) {
    if (usage_idx1 != usage_idx2) return FALSE;
    if (usage1 == usage2) return TRUE;
    if (usage1 == WINED3DDECLUSAGE_POSITION && usage2 == WINED3DDECLUSAGE_POSITIONT) return TRUE;
    if (usage2 == WINED3DDECLUSAGE_POSITION && usage1 == WINED3DDECLUSAGE_POSITIONT) return TRUE;

    return FALSE;
}

BOOL vshader_get_input(
    IWineD3DVertexShader* iface,
    BYTE usage_req, BYTE usage_idx_req,
    unsigned int* regnum) {

    IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl*) iface;
    int i;

    for (i = 0; i < MAX_ATTRIBS; i++) {
        DWORD usage_token = This->semantics_in[i].usage;
        DWORD usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
        DWORD usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

        if (usage_token && match_usage(usage, usage_idx, usage_req, usage_idx_req)) {
            *regnum = i;
            return TRUE;
        }
    }
    return FALSE;
}

/** Generate a vertex shader string using either GL_VERTEX_PROGRAM_ARB
    or GLSL and send it to the card */
static void IWineD3DVertexShaderImpl_GenerateShader(IWineD3DVertexShader *iface,
        const struct shader_reg_maps* reg_maps, const DWORD *pFunction)
{
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    SHADER_BUFFER buffer;

    This->swizzle_map = ((IWineD3DDeviceImpl *)This->baseShader.device)->strided_streams.swizzle_map;

    shader_buffer_init(&buffer);
    ((IWineD3DDeviceImpl *)This->baseShader.device)->shader_backend->shader_generate_vshader(iface, &buffer);
    shader_buffer_free(&buffer);
}

/* *******************************************
   IWineD3DVertexShader IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DVertexShaderImpl_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, LPVOID *ppobj) {
    TRACE("iface %p, riid %s, ppobj %p\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IWineD3DVertexShader)
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

static ULONG  WINAPI IWineD3DVertexShaderImpl_AddRef(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&This->baseShader.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG WINAPI IWineD3DVertexShaderImpl_Release(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
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
   IWineD3DVertexShader IWineD3DVertexShader parts follow
   ******************************************* */

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetParent(IWineD3DVertexShader *iface, IUnknown** parent){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    
    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetDevice(IWineD3DVertexShader* iface, IWineD3DDevice **pDevice){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    IWineD3DDevice_AddRef(This->baseShader.device);
    *pDevice = This->baseShader.device;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetFunction(IWineD3DVertexShader* impl, VOID* pData, UINT* pSizeOfData) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)impl;
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

/* Note that for vertex shaders CompileShader isn't called until the
 * shader is first used. The reason for this is that we need the vertex
 * declaration the shader will be used with in order to determine if
 * the data in a register is of type D3DCOLOR, and needs swizzling. */
static HRESULT WINAPI IWineD3DVertexShaderImpl_SetFunction(IWineD3DVertexShader *iface, CONST DWORD *pFunction) {

    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;
    HRESULT hr;
    shader_reg_maps *reg_maps = &This->baseShader.reg_maps;

    TRACE("(%p) : pFunction %p\n", iface, pFunction);

    /* First pass: trace shader */
    if (TRACE_ON(d3d_shader)) shader_trace_init(pFunction, This->baseShader.shader_ins);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    /* Second pass: figure out registers used, semantics, etc.. */
    This->min_rel_offset = GL_LIMITS(vshader_constantsF);
    This->max_rel_offset = 0;
    memset(reg_maps, 0, sizeof(shader_reg_maps));
    hr = shader_get_registers_used((IWineD3DBaseShader*) This, reg_maps,
            This->semantics_in, This->semantics_out, pFunction);
    if (hr != WINED3D_OK) return hr;

    vshader_set_limits(This);

    This->baseShader.shader_mode = deviceImpl->vs_selected_mode;

    if(deviceImpl->vs_selected_mode == SHADER_ARB &&
       (GLINFO_LOCATION).arb_vs_offset_limit      &&
       This->min_rel_offset <= This->max_rel_offset) {

        if(This->max_rel_offset - This->min_rel_offset > 127) {
            FIXME("The difference between the minimum and maximum relative offset is > 127\n");
            FIXME("Which this OpenGL implementation does not support. Try using GLSL\n");
            FIXME("Min: %d, Max: %d\n", This->min_rel_offset, This->max_rel_offset);
        } else if(This->max_rel_offset - This->min_rel_offset > 63) {
            This->rel_offset = This->min_rel_offset + 63;
        } else if(This->max_rel_offset > 63) {
            This->rel_offset = This->min_rel_offset;
        } else {
            This->rel_offset = 0;
        }
    }
    This->baseShader.load_local_constsF = This->baseShader.reg_maps.usesrelconstF && !list_empty(&This->baseShader.constantsF);

    /* copy the function ... because it will certainly be released by application */
    This->baseShader.function = HeapAlloc(GetProcessHeap(), 0, This->baseShader.functionLength);
    if (!This->baseShader.function) return E_OUTOFMEMORY;
    memcpy(This->baseShader.function, pFunction, This->baseShader.functionLength);

    return WINED3D_OK;
}

/* Preload semantics for d3d8 shaders */
static void WINAPI IWineD3DVertexShaderImpl_FakeSemantics(IWineD3DVertexShader *iface, IWineD3DVertexDeclaration *vertex_declaration) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DVertexDeclarationImpl* vdecl = (IWineD3DVertexDeclarationImpl*)vertex_declaration;

    int i;
    for (i = 0; i < vdecl->declarationWNumElements - 1; ++i) {
        const WINED3DVERTEXELEMENT *element = vdecl->pDeclarationWine + i;
        vshader_set_input(This, element->Reg, element->Usage, element->UsageIndex);
    }
}

/* Set local constants for d3d8 shaders */
static HRESULT WINAPI IWIneD3DVertexShaderImpl_SetLocalConstantsF(IWineD3DVertexShader *iface,
        UINT start_idx, const float *src_data, UINT count) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    UINT i, end_idx;

    TRACE("(%p) : start_idx %u, src_data %p, count %u\n", This, start_idx, src_data, count);

    end_idx = start_idx + count;
    if (end_idx > GL_LIMITS(vshader_constantsF)) {
        WARN("end_idx %u > float constants limit %u\n", end_idx, GL_LIMITS(vshader_constantsF));
        end_idx = GL_LIMITS(vshader_constantsF);
    }

    for (i = start_idx; i < end_idx; ++i) {
        local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
        if (!lconst) return E_OUTOFMEMORY;

        lconst->idx = i;
        memcpy(lconst->value, src_data + (i - start_idx) * 4 /* 4 components */, 4 * sizeof(float));
        list_add_head(&This->baseShader.constantsF, &lconst->entry);
    }

    return WINED3D_OK;
}

HRESULT IWineD3DVertexShaderImpl_CompileShader(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    CONST DWORD *function = This->baseShader.function;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;

    TRACE("(%p) : function %p\n", iface, function);

    /* We're already compiled. */
    if (This->baseShader.is_compiled) {
        if ((This->swizzle_map & deviceImpl->strided_streams.use_map) != deviceImpl->strided_streams.swizzle_map)
        {
            WARN("Recompiling vertex shader %p due to D3DCOLOR input changes\n", This);
            goto recompile;
        }

        return WINED3D_OK;

        recompile:
        if(This->recompile_count < 50) {
            This->recompile_count++;
        } else {
            FIXME("Vertexshader %p recompiled more than 50 times\n", This);
        }

        deviceImpl->shader_backend->shader_destroy((IWineD3DBaseShader *) iface);
    }

    /* Generate the HW shader */
    TRACE("(%p) : Generating hardware program\n", This);
    IWineD3DVertexShaderImpl_GenerateShader(iface, &This->baseShader.reg_maps, function);

    This->baseShader.is_compiled = TRUE;

    return WINED3D_OK;
}

const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DVertexShaderImpl_QueryInterface,
    IWineD3DVertexShaderImpl_AddRef,
    IWineD3DVertexShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DVertexShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DVertexShaderImpl_SetFunction,
    /*** IWineD3DVertexShader methods ***/
    IWineD3DVertexShaderImpl_GetDevice,
    IWineD3DVertexShaderImpl_GetFunction,
    IWineD3DVertexShaderImpl_FakeSemantics,
    IWIneD3DVertexShaderImpl_SetLocalConstantsF
};
