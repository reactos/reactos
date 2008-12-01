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
    {WINED3DSIO_NOP,     "nop",     "NOP",               0, 0, WINED3DSIH_NOP,     0,                      0                     },
    {WINED3DSIO_MOV,     "mov",     "MOV",               1, 2, WINED3DSIH_MOV,     0,                      0                     },
    {WINED3DSIO_MOVA,    "mova",    NULL,                1, 2, WINED3DSIH_MOVA,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ADD,     "add",     "ADD",               1, 3, WINED3DSIH_ADD,     0,                      0                     },
    {WINED3DSIO_SUB,     "sub",     "SUB",               1, 3, WINED3DSIH_SUB,     0,                      0                     },
    {WINED3DSIO_MAD,     "mad",     "MAD",               1, 4, WINED3DSIH_MAD,     0,                      0                     },
    {WINED3DSIO_MUL,     "mul",     "MUL",               1, 3, WINED3DSIH_MUL,     0,                      0                     },
    {WINED3DSIO_RCP,     "rcp",     "RCP",               1, 2, WINED3DSIH_RCP,     0,                      0                     },
    {WINED3DSIO_RSQ,     "rsq",     "RSQ",               1, 2, WINED3DSIH_RSQ,     0,                      0                     },
    {WINED3DSIO_DP3,     "dp3",     "DP3",               1, 3, WINED3DSIH_DP3,     0,                      0                     },
    {WINED3DSIO_DP4,     "dp4",     "DP4",               1, 3, WINED3DSIH_DP4,     0,                      0                     },
    {WINED3DSIO_MIN,     "min",     "MIN",               1, 3, WINED3DSIH_MIN,     0,                      0                     },
    {WINED3DSIO_MAX,     "max",     "MAX",               1, 3, WINED3DSIH_MAX,     0,                      0                     },
    {WINED3DSIO_SLT,     "slt",     "SLT",               1, 3, WINED3DSIH_SLT,     0,                      0                     },
    {WINED3DSIO_SGE,     "sge",     "SGE",               1, 3, WINED3DSIH_SGE,     0,                      0                     },
    {WINED3DSIO_ABS,     "abs",     "ABS",               1, 2, WINED3DSIH_ABS,     0,                      0                     },
    {WINED3DSIO_EXP,     "exp",     "EX2",               1, 2, WINED3DSIH_EXP,     0,                      0                     },
    {WINED3DSIO_LOG,     "log",     "LG2",               1, 2, WINED3DSIH_LOG,     0,                      0                     },
    {WINED3DSIO_EXPP,    "expp",    "EXP",               1, 2, WINED3DSIH_EXPP,    0,                      0                     },
    {WINED3DSIO_LOGP,    "logp",    "LOG",               1, 2, WINED3DSIH_LOGP,    0,                      0                     },
    {WINED3DSIO_LIT,     "lit",     "LIT",               1, 2, WINED3DSIH_LIT,     0,                      0                     },
    {WINED3DSIO_DST,     "dst",     "DST",               1, 3, WINED3DSIH_DST,     0,                      0                     },
    {WINED3DSIO_LRP,     "lrp",     "LRP",               1, 4, WINED3DSIH_LRP,     0,                      0                     },
    {WINED3DSIO_FRC,     "frc",     "FRC",               1, 2, WINED3DSIH_FRC,     0,                      0                     },
    {WINED3DSIO_POW,     "pow",     "POW",               1, 3, WINED3DSIH_POW,     0,                      0                     },
    {WINED3DSIO_CRS,     "crs",     "XPD",               1, 3, WINED3DSIH_CRS,     0,                      0                     },
    /* TODO: sng can possibly be performed a  s
        RCP tmp, vec
        MUL out, tmp, vec*/
    {WINED3DSIO_SGN,     "sgn",     NULL,                1, 2, WINED3DSIH_SGN,     0,                      0                     },
    {WINED3DSIO_NRM,     "nrm",     NULL,                1, 2, WINED3DSIH_NRM,     0,                      0                     },
    {WINED3DSIO_SINCOS,  "sincos",  NULL,                1, 4, WINED3DSIH_SINCOS,  WINED3DVS_VERSION(2,0), WINED3DVS_VERSION(2,1)},
    {WINED3DSIO_SINCOS,  "sincos",  "SCS",               1, 2, WINED3DSIH_SINCOS,  WINED3DVS_VERSION(3,0), -1                    },
    /* Matrix */
    {WINED3DSIO_M4x4,    "m4x4",    "undefined",         1, 3, WINED3DSIH_M4x4,    0,                      0                     },
    {WINED3DSIO_M4x3,    "m4x3",    "undefined",         1, 3, WINED3DSIH_M4x3,    0,                      0                     },
    {WINED3DSIO_M3x4,    "m3x4",    "undefined",         1, 3, WINED3DSIH_M3x4,    0,                      0                     },
    {WINED3DSIO_M3x3,    "m3x3",    "undefined",         1, 3, WINED3DSIH_M3x3,    0,                      0                     },
    {WINED3DSIO_M3x2,    "m3x2",    "undefined",         1, 3, WINED3DSIH_M3x2,    0,                      0                     },
    /* Declare registers */
    {WINED3DSIO_DCL,     "dcl",     NULL,                0, 2, WINED3DSIH_DCL,     0,                      0                     },
    /* Constant definitions */
    {WINED3DSIO_DEF,     "def",     NULL,                1, 5, WINED3DSIH_DEF,     0,                      0                     },
    {WINED3DSIO_DEFB,    "defb",    GLNAME_REQUIRE_GLSL, 1, 2, WINED3DSIH_DEFB,    0,                      0                     },
    {WINED3DSIO_DEFI,    "defi",    GLNAME_REQUIRE_GLSL, 1, 5, WINED3DSIH_DEFI,    0,                      0                     },
    /* Flow control - requires GLSL or software shaders */
    {WINED3DSIO_REP ,    "rep",     NULL,                0, 1, WINED3DSIH_REP,     WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDREP,  "endrep",  NULL,                0, 0, WINED3DSIH_ENDREP,  WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_IF,      "if",      NULL,                0, 1, WINED3DSIH_IF,      WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_IFC,     "ifc",     NULL,                0, 2, WINED3DSIH_IFC,     WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_ELSE,    "else",    NULL,                0, 0, WINED3DSIH_ELSE,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDIF,   "endif",   NULL,                0, 0, WINED3DSIH_ENDIF,   WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_BREAK,   "break",   NULL,                0, 0, WINED3DSIH_BREAK,   WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKC,  "breakc",  NULL,                0, 2, WINED3DSIH_BREAKC,  WINED3DVS_VERSION(2,1), -1                    },
    {WINED3DSIO_BREAKP,  "breakp",  GLNAME_REQUIRE_GLSL, 0, 1, WINED3DSIH_BREAKP,  0,                      0                     },
    {WINED3DSIO_CALL,    "call",    NULL,                0, 1, WINED3DSIH_CALL,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_CALLNZ,  "callnz",  NULL,                0, 2, WINED3DSIH_CALLNZ,  WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_LOOP,    "loop",    NULL,                0, 2, WINED3DSIH_LOOP,    WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_RET,     "ret",     NULL,                0, 0, WINED3DSIH_RET,     WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_ENDLOOP, "endloop", NULL,                0, 0, WINED3DSIH_ENDLOOP, WINED3DVS_VERSION(2,0), -1                    },
    {WINED3DSIO_LABEL,   "label",   NULL,                0, 1, WINED3DSIH_LABEL,   WINED3DVS_VERSION(2,0), -1                    },

    {WINED3DSIO_SETP,    "setp",    GLNAME_REQUIRE_GLSL, 1, 3, WINED3DSIH_SETP,    0,                      0                     },
    {WINED3DSIO_TEXLDL,  "texldl",  NULL,                1, 3, WINED3DSIH_TEXLDL,  WINED3DVS_VERSION(3,0), -1                    },
    {0,                  NULL,      NULL,                0, 0, 0,                  0,                      0                     }
};

static void vshader_set_limits(
      IWineD3DVertexShaderImpl *This) {

      This->baseShader.limits.texcoord = 0;
      This->baseShader.limits.attributes = 16;
      This->baseShader.limits.packed_input = 0;

      /* Must match D3DCAPS9.MaxVertexShaderConst: at least 256 for vs_2_0 */
      This->baseShader.limits.constant_float = GL_LIMITS(vshader_constantsF);

      switch (This->baseShader.hex_version) {
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
                       This->baseShader.hex_version);
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

BOOL vshader_input_is_color(
    IWineD3DVertexShader* iface,
    unsigned int regnum) {

    IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl*) iface;

    DWORD usage_token = This->semantics_in[regnum].usage;
    DWORD usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
    DWORD usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

    int i;

    for(i = 0; i < This->num_swizzled_attribs; i++) {
        if(This->swizzled_attribs[i].usage == usage &&
           This->swizzled_attribs[i].idx == usage_idx) {
            return TRUE;
        }
    }
    return FALSE;
}

static inline void find_swizzled_attribs(IWineD3DVertexDeclaration *declaration, IWineD3DVertexShaderImpl *This) {
    UINT num = 0, i, j;
    UINT numoldswizzles = This->num_swizzled_attribs;
    IWineD3DVertexDeclarationImpl *decl = (IWineD3DVertexDeclarationImpl *) declaration;

    DWORD usage_token, usage, usage_idx;
    BOOL found;

    attrib_declaration oldswizzles[sizeof(This->swizzled_attribs) / sizeof(This->swizzled_attribs[0])];

    /* Back up the old swizzles to keep attributes that are undefined in the current declaration */
    memcpy(oldswizzles, This->swizzled_attribs, sizeof(oldswizzles));

    memset(This->swizzled_attribs, 0, sizeof(This->swizzled_attribs[0]) * MAX_ATTRIBS);

    for(i = 0; i < decl->num_swizzled_attribs; i++) {
        for(j = 0; j < MAX_ATTRIBS; j++) {

            if(!This->baseShader.reg_maps.attributes[j]) continue;

            usage_token = This->semantics_in[j].usage;
            usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
            usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

            if(decl->swizzled_attribs[i].usage == usage &&
               decl->swizzled_attribs[i].idx == usage_idx) {
                This->swizzled_attribs[num].usage = usage;
                This->swizzled_attribs[num].idx = usage_idx;
                num++;
            }
        }
    }

    /* Add previously converted attributes back in if they are not defined in the current declaration */
    for(i = 0; i < numoldswizzles; i++) {

        found = FALSE;
        for(j = 0; j < decl->declarationWNumElements; j++) {
            if(oldswizzles[i].usage == decl->pDeclarationWine[j].Usage &&
               oldswizzles[i].idx == decl->pDeclarationWine[j].UsageIndex) {
                found = TRUE;
            }
        }
        if(found) {
            /* This previously converted attribute is declared in the current declaration. Either it is
             * already in the new array, or it should not be there. Skip it
             */
            continue;
        }
        /* We have a previously swizzled attribute that is not defined by the current vertex declaration.
         * Insert it into the new conversion array to keep it in the old defined state. Otherwise we end up
         * recompiling if the old decl is used again because undefined attributes are reset to no swizzling.
         * In the reverse way(attribute was not swizzled and is not declared in new declaration) the attrib
         * stays unswizzled as well because it isn't found in the oldswizzles array
         */
        for(j = 0; j < num; j++) {
            if(oldswizzles[i].usage > This->swizzled_attribs[j].usage || (
               oldswizzles[i].usage == This->swizzled_attribs[j].usage &&
               oldswizzles[i].idx > This->swizzled_attribs[j].idx)) {
                memmove(&This->swizzled_attribs[j + 1], &This->swizzled_attribs[j],
                         sizeof(This->swizzled_attribs) - (sizeof(This->swizzled_attribs[0]) * (j + 1)));
                break;
            }
        }
        This->swizzled_attribs[j].usage = oldswizzles[i].usage;
        This->swizzled_attribs[j].idx = oldswizzles[i].idx;
        num++;
    }

    TRACE("New swizzled attributes array\n");
    for(i = 0; i < num; i++) {
        TRACE("%d: %s(%d), %d\n", i, debug_d3ddeclusage(This->swizzled_attribs[i].usage),
              This->swizzled_attribs[i].usage, This->swizzled_attribs[i].idx);
    }
    This->num_swizzled_attribs = num;
}
/** Generate a vertex shader string using either GL_VERTEX_PROGRAM_ARB
    or GLSL and send it to the card */
static VOID IWineD3DVertexShaderImpl_GenerateShader(
    IWineD3DVertexShader *iface,
    shader_reg_maps* reg_maps,
    CONST DWORD *pFunction) {

    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    IWineD3DVertexDeclaration *decl = ((IWineD3DDeviceImpl *) This->baseShader.device)->stateBlock->vertexDecl;
    SHADER_BUFFER buffer;

    find_swizzled_attribs(decl, This);

#if 0 /* FIXME: Use the buffer that is held by the device, this is ok since fixups will be skipped for software shaders
        it also requires entering a critical section but cuts down the runtime footprint of wined3d and any memory fragmentation that may occur... */
    if (This->device->fixupVertexBufferSize < SHADER_PGMSIZE) {
        HeapFree(GetProcessHeap(), 0, This->fixupVertexBuffer);
        This->fixupVertexBuffer = HeapAlloc(GetProcessHeap() , 0, SHADER_PGMSIZE);
        This->fixupVertexBufferSize = PGMSIZE;
        This->fixupVertexBuffer[0] = 0;
    }
    buffer.buffer = This->device->fixupVertexBuffer;
#else
    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE); 
#endif
    buffer.bsize = 0;
    buffer.lineNo = 0;
    buffer.newline = TRUE;

    ((IWineD3DDeviceImpl *)This->baseShader.device)->shader_backend->shader_generate_vshader(iface, &buffer);

#if 1 /* if were using the data buffer of device then we don't need to free it */
  HeapFree(GetProcessHeap(), 0, buffer.buffer);
#endif
}

/* *******************************************
   IWineD3DVertexShader IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DVertexShaderImpl_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, LPVOID *ppobj) {
    return IWineD3DBaseShaderImpl_QueryInterface((IWineD3DBaseShader *) iface, riid, ppobj);
}

static ULONG  WINAPI IWineD3DVertexShaderImpl_AddRef(IWineD3DVertexShader *iface) {
    return IWineD3DBaseShaderImpl_AddRef((IWineD3DBaseShader *) iface);
}

static ULONG WINAPI IWineD3DVertexShaderImpl_Release(IWineD3DVertexShader *iface) {
    return IWineD3DBaseShaderImpl_Release((IWineD3DBaseShader *) iface);
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
    if (NULL == This->baseShader.function) { /* no function defined */
        TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
        (*(DWORD **) pData) = NULL;
    } else {
        if(This->baseShader.functionLength == 0){

        }
        TRACE("(%p) : GetFunction copying to %p\n", This, pData);
        memcpy(pData, This->baseShader.function, This->baseShader.functionLength);
    }
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
    shader_trace_init((IWineD3DBaseShader*) This, pFunction);
    vshader_set_limits(This);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    /* Second pass: figure out registers used, semantics, etc.. */
    This->min_rel_offset = GL_LIMITS(vshader_constantsF);
    This->max_rel_offset = 0;
    memset(reg_maps, 0, sizeof(shader_reg_maps));
    hr = shader_get_registers_used((IWineD3DBaseShader*) This, reg_maps,
       This->semantics_in, This->semantics_out, pFunction, NULL);
    if (hr != WINED3D_OK) return hr;

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
    if (NULL != pFunction) {
        void *function;

        function = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->baseShader.functionLength);
        if (!function) return E_OUTOFMEMORY;
        memcpy(function, pFunction, This->baseShader.functionLength);
        This->baseShader.function = function;
    } else {
        This->baseShader.function = NULL;
    }

    return WINED3D_OK;
}

/* Preload semantics for d3d8 shaders */
static void WINAPI IWineD3DVertexShaderImpl_FakeSemantics(IWineD3DVertexShader *iface, IWineD3DVertexDeclaration *vertex_declaration) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DVertexDeclarationImpl* vdecl = (IWineD3DVertexDeclarationImpl*)vertex_declaration;

    int i;
    for (i = 0; i < vdecl->declarationWNumElements - 1; ++i) {
        WINED3DVERTEXELEMENT* element = vdecl->pDeclarationWine + i;
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

static inline BOOL swizzled_attribs_differ(IWineD3DVertexShaderImpl *This, IWineD3DVertexDeclarationImpl *vdecl) {
    UINT i, j, k;
    BOOL found;

    DWORD usage_token;
    DWORD usage;
    DWORD usage_idx;

    for(i = 0; i < vdecl->declarationWNumElements; i++) {
        /* Ignore tesselated streams and the termination entry(position0, stream 255, unused) */
        if(vdecl->pDeclarationWine[i].Stream >= MAX_STREAMS ||
           vdecl->pDeclarationWine[i].Type == WINED3DDECLTYPE_UNUSED) continue;

        for(j = 0; j < MAX_ATTRIBS; j++) {
            if(!This->baseShader.reg_maps.attributes[j]) continue;

            usage_token = This->semantics_in[j].usage;
            usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
            usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

            if(vdecl->pDeclarationWine[i].Usage != usage ||
               vdecl->pDeclarationWine[i].UsageIndex != usage_idx) {
                continue;
            }

            found = FALSE;
            for(k = 0; k < This->num_swizzled_attribs; k++) {
                if(This->swizzled_attribs[k].usage == usage &&
                    This->swizzled_attribs[k].idx == usage_idx) {
                    found = TRUE;
                }
            }
            if(!found && vdecl->pDeclarationWine[i].Type == WINED3DDECLTYPE_D3DCOLOR) {
                TRACE("Attribute %s%d is D3DCOLOR now but wasn't before\n",
                      debug_d3ddeclusage(usage), usage_idx);
                return TRUE;
            }
            if( found && vdecl->pDeclarationWine[i].Type != WINED3DDECLTYPE_D3DCOLOR) {
                TRACE("Attribute %s%d was D3DCOLOR before but is not any more\n",
                      debug_d3ddeclusage(usage), usage_idx);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static HRESULT WINAPI IWineD3DVertexShaderImpl_CompileShader(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    IWineD3DVertexDeclarationImpl *vdecl;
    CONST DWORD *function = This->baseShader.function;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;

    TRACE("(%p) : function %p\n", iface, function);

    /* We're already compiled. */
    if (This->baseShader.is_compiled) {
        vdecl = (IWineD3DVertexDeclarationImpl *) deviceImpl->stateBlock->vertexDecl;

        if(This->num_swizzled_attribs != vdecl->num_swizzled_attribs ||
           memcmp(This->swizzled_attribs, vdecl->swizzled_attribs, sizeof(vdecl->swizzled_attribs[0]) * This->num_swizzled_attribs) != 0) {

            /* The swizzled attributes differ between shader and declaration. This doesn't necessarily mean
             * we have to recompile, but we have to take a deeper look at see if the attribs that differ
             * are declared in the decl and used in the shader
             */
            if(swizzled_attribs_differ(This, vdecl)) {
                WARN("Recompiling vertex shader %p due to D3DCOLOR input changes\n", This);
                goto recompile;
            }
            WARN("Swizzled attribute validation required an expensive comparison\n");
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

    /* We don't need to compile */
    if (!function) {
        This->baseShader.is_compiled = TRUE;
        return WINED3D_OK;
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
    IWineD3DVertexShaderImpl_CompileShader,
    /*** IWineD3DVertexShader methods ***/
    IWineD3DVertexShaderImpl_GetDevice,
    IWineD3DVertexShaderImpl_GetFunction,
    IWineD3DVertexShaderImpl_FakeSemantics,
    IWIneD3DVertexShaderImpl_SetLocalConstantsF
};
