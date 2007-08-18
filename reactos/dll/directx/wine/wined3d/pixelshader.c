/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
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

#if 0 /* Must not be 1 in cvs version */
# define PSTRACE(A) TRACE A
# define TRACE_VSVECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)
#else
# define PSTRACE(A)
# define TRACE_VSVECTOR(name)
#endif

#define GLNAME_REQUIRE_GLSL  ((const char *)1)
/* *******************************************
   IWineD3DPixelShader IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
        || IsEqualGUID(riid, &IID_IWineD3DPixelShader)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        /* SetPixelShader does not AddRef. If the bound pixel shader is destroyed, the pointer in the stateblock remains
         * unchanged. Drawing again will most likely crash, even on windows. A problem can occur if the application creates
         * a new pixel shader which resides at the same address. Then SetPixelShader will think it is a NOP change, and won't
         * dirtify the state.
         *
         * Do NOT call GetPixelShader here. This will addRef and cause a recursion. And do NOT set the pixel shader to NULL,
         * Windows does not do that(Although no test exists since they'd crash randomly)
         */
        if(iface == ((IWineD3DDeviceImpl *) This->baseShader.device)->stateBlock->pixelShader) {
            IWineD3DDeviceImpl_MarkStateDirty((IWineD3DDeviceImpl *) This->baseShader.device, STATE_PIXELSHADER);
        }

        if (This->baseShader.shader_mode == SHADER_GLSL && This->baseShader.prgId != 0) {
            struct list *linked_programs = &This->baseShader.linked_programs;

            TRACE("Deleting linked programs\n");
            if (linked_programs->next) {
                struct glsl_shader_prog_link *entry, *entry2;
                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, pshader_entry) {
                    delete_glsl_program_entry(This->baseShader.device, entry);
                }
            }

            TRACE("Deleting shader object %u\n", This->baseShader.prgId);
            GL_EXTCALL(glDeleteObjectARB(This->baseShader.prgId));
            checkGLcall("glDeleteObjectARB");
        }
        shader_delete_constant_list(&This->baseShader.constantsF);
        shader_delete_constant_list(&This->baseShader.constantsB);
        shader_delete_constant_list(&This->baseShader.constantsI);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
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
  if (NULL == This->baseShader.function) { /* no function defined */
    TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
    (*(DWORD **) pData) = NULL;
  } else {
    if (This->baseShader.functionLength == 0) {

    }
    TRACE("(%p) : GetFunction copying to %p\n", This, pData);
    memcpy(pData, This->baseShader.function, This->baseShader.functionLength);
  }
  return WINED3D_OK;
}

CONST SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[] = {
    /* Arithmethic */
    {WINED3DSIO_NOP,  "nop", "NOP", 0, 0, pshader_hw_map2gl, NULL, 0, 0},
    {WINED3DSIO_MOV,  "mov", "MOV", 1, 2, pshader_hw_map2gl, shader_glsl_mov, 0, 0},
    {WINED3DSIO_ADD,  "add", "ADD", 1, 3, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {WINED3DSIO_SUB,  "sub", "SUB", 1, 3, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {WINED3DSIO_MAD,  "mad", "MAD", 1, 4, pshader_hw_map2gl, shader_glsl_mad, 0, 0},
    {WINED3DSIO_MUL,  "mul", "MUL", 1, 3, pshader_hw_map2gl, shader_glsl_arith, 0, 0},
    {WINED3DSIO_RCP,  "rcp", "RCP",  1, 2, pshader_hw_map2gl, shader_glsl_rcp, 0, 0},
    {WINED3DSIO_RSQ,  "rsq",  "RSQ", 1, 2, pshader_hw_map2gl, shader_glsl_rsq, 0, 0},
    {WINED3DSIO_DP3,  "dp3",  "DP3", 1, 3, pshader_hw_map2gl, shader_glsl_dot, 0, 0},
    {WINED3DSIO_DP4,  "dp4",  "DP4", 1, 3, pshader_hw_map2gl, shader_glsl_dot, 0, 0},
    {WINED3DSIO_MIN,  "min",  "MIN", 1, 3, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_MAX,  "max",  "MAX", 1, 3, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_SLT,  "slt",  "SLT", 1, 3, pshader_hw_map2gl, shader_glsl_compare, 0, 0},
    {WINED3DSIO_SGE,  "sge",  "SGE", 1, 3, pshader_hw_map2gl, shader_glsl_compare, 0, 0},
    {WINED3DSIO_ABS,  "abs",  "ABS", 1, 2, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_EXP,  "exp",  "EX2", 1, 2, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_LOG,  "log",  "LG2", 1, 2, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_EXPP, "expp", "EXP", 1, 2, pshader_hw_map2gl, shader_glsl_expp, 0, 0},
    {WINED3DSIO_LOGP, "logp", "LOG", 1, 2, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_DST,  "dst",  "DST", 1, 3, pshader_hw_map2gl, shader_glsl_dst, 0, 0},
    {WINED3DSIO_LRP,  "lrp",  "LRP", 1, 4, pshader_hw_map2gl, shader_glsl_lrp, 0, 0},
    {WINED3DSIO_FRC,  "frc",  "FRC", 1, 2, pshader_hw_map2gl, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_CND,  "cnd",  NULL, 1, 4, pshader_hw_cnd, shader_glsl_cnd, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_CMP,  "cmp",  NULL, 1, 4, pshader_hw_cmp, shader_glsl_cmp, WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(3,0)},
    {WINED3DSIO_POW,  "pow",  "POW", 1, 3, NULL, shader_glsl_pow, 0, 0},
    {WINED3DSIO_CRS,  "crs",  "XPS", 1, 3, NULL, shader_glsl_cross, 0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;
    */
    {WINED3DSIO_NRM,      "nrm",      NULL, 1, 2, NULL, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_SINCOS,   "sincos",   NULL, 1, 4, NULL, shader_glsl_sincos, WINED3DPS_VERSION(2,0), WINED3DPS_VERSION(2,1)},
    {WINED3DSIO_SINCOS,   "sincos",   NULL, 1, 2, NULL, shader_glsl_sincos, WINED3DPS_VERSION(3,0), -1},
    /* TODO: dp2add can be made out of multiple instuctions */
    {WINED3DSIO_DP2ADD,   "dp2add",   GLNAME_REQUIRE_GLSL,  1, 4, NULL, pshader_glsl_dp2add, WINED3DPS_VERSION(2,0), -1},
    /* Matrix */
    {WINED3DSIO_M4x4, "m4x4", "undefined", 1, 3, NULL, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M4x3, "m4x3", "undefined", 1, 3, NULL, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x4, "m3x4", "undefined", 1, 3, NULL, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x3, "m3x3", "undefined", 1, 3, NULL, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x2, "m3x2", "undefined", 1, 3, NULL, shader_glsl_mnxn, 0, 0},
    /* Register declarations */
    {WINED3DSIO_DCL,      "dcl",      NULL, 0, 2, NULL, NULL, 0, 0},
    /* Flow control - requires GLSL or software shaders */
    {WINED3DSIO_REP ,     "rep",      NULL, 0, 1, NULL, shader_glsl_rep,    WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_ENDREP,   "endrep",   NULL, 0, 0, NULL, shader_glsl_end,    WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_IF,       "if",       NULL, 0, 1, NULL, shader_glsl_if,     WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_IFC,      "ifc",      NULL, 0, 2, NULL, shader_glsl_ifc,    WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_ELSE,     "else",     NULL, 0, 0, NULL, shader_glsl_else,   WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_ENDIF,    "endif",    NULL, 0, 0, NULL, shader_glsl_end,    WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_BREAK,    "break",    NULL, 0, 0, NULL, shader_glsl_break,  WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_BREAKC,   "breakc",   NULL, 0, 2, NULL, shader_glsl_breakc, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_BREAKP,   "breakp",   GLNAME_REQUIRE_GLSL, 0, 1, NULL, NULL, 0, 0},
    {WINED3DSIO_CALL,     "call",     NULL, 0, 1, NULL, shader_glsl_call, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_CALLNZ,   "callnz",   NULL, 0, 2, NULL, shader_glsl_callnz, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_LOOP,     "loop",     NULL, 0, 2, NULL, shader_glsl_loop,   WINED3DPS_VERSION(3,0), -1},
    {WINED3DSIO_RET,      "ret",      NULL, 0, 0, NULL, NULL,               WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_ENDLOOP,  "endloop",  NULL, 0, 0, NULL, shader_glsl_end,    WINED3DPS_VERSION(3,0), -1},
    {WINED3DSIO_LABEL,    "label",    NULL, 0, 1, NULL, shader_glsl_label,  WINED3DPS_VERSION(2,1), -1},
    /* Constant definitions */
    {WINED3DSIO_DEF,      "def",      "undefined",         1, 5, NULL, NULL, 0, 0},
    {WINED3DSIO_DEFB,     "defb",     GLNAME_REQUIRE_GLSL, 1, 2, NULL, NULL, 0, 0},
    {WINED3DSIO_DEFI,     "defi",     GLNAME_REQUIRE_GLSL, 1, 5, NULL, NULL, 0, 0},
    /* Texture */
    {WINED3DSIO_TEXCOORD, "texcoord", "undefined", 1, 1, pshader_hw_texcoord, pshader_glsl_texcoord, 0, WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXCOORD, "texcrd",   "undefined", 1, 2, pshader_hw_texcoord, pshader_glsl_texcoord, WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_TEXKILL,  "texkill",  "KIL",       1, 1, pshader_hw_map2gl, pshader_glsl_texkill, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(3,0)},
    {WINED3DSIO_TEX,      "tex",      "undefined", 1, 1, pshader_hw_tex, pshader_glsl_tex, 0, WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEX,      "texld",    "undefined", 1, 2, pshader_hw_tex, pshader_glsl_tex, WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_TEX,      "texld",    "undefined", 1, 3, pshader_hw_tex, pshader_glsl_tex, WINED3DPS_VERSION(2,0), -1},
    {WINED3DSIO_TEXBEM,   "texbem",   "undefined", 1, 2, pshader_hw_texbem, pshader_glsl_texbem, 0, WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXBEML,  "texbeml",  GLNAME_REQUIRE_GLSL, 1, 2, NULL, NULL, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2AR,"texreg2ar","undefined", 1, 2, pshader_hw_texreg2ar, pshader_glsl_texreg2ar, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2GB,"texreg2gb","undefined", 1, 2, pshader_hw_texreg2gb, pshader_glsl_texreg2gb, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXREG2RGB,   "texreg2rgb",   GLNAME_REQUIRE_GLSL, 1, 2, NULL, pshader_glsl_texreg2rgb, WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2PAD,   "texm3x2pad",   "undefined", 1, 2, pshader_hw_texm3x2pad, pshader_glsl_texm3x2pad, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2TEX,   "texm3x2tex",   "undefined", 1, 2, pshader_hw_texm3x2tex, pshader_glsl_texm3x2tex, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3PAD,   "texm3x3pad",   "undefined", 1, 2, pshader_hw_texm3x3pad, pshader_glsl_texm3x3pad, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3DIFF,  "texm3x3diff",  GLNAME_REQUIRE_GLSL, 1, 2, NULL, NULL, WINED3DPS_VERSION(0,0), WINED3DPS_VERSION(0,0)},
    {WINED3DSIO_TEXM3x3SPEC,  "texm3x3spec",  "undefined", 1, 3, pshader_hw_texm3x3spec, pshader_glsl_texm3x3spec, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3VSPEC, "texm3x3vspec",  "undefined", 1, 2, pshader_hw_texm3x3vspec, pshader_glsl_texm3x3vspec, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3TEX,   "texm3x3tex",   "undefined", 1, 2, pshader_hw_texm3x3tex, pshader_glsl_texm3x3tex, WINED3DPS_VERSION(1,0), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDP3TEX,    "texdp3tex",    GLNAME_REQUIRE_GLSL, 1, 2, NULL, pshader_glsl_texdp3tex, WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x2DEPTH, "texm3x2depth", GLNAME_REQUIRE_GLSL, 1, 2, NULL, pshader_glsl_texm3x2depth, WINED3DPS_VERSION(1,3), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDP3,   "texdp3",   GLNAME_REQUIRE_GLSL, 1, 2, NULL, pshader_glsl_texdp3, WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXM3x3,  "texm3x3",  GLNAME_REQUIRE_GLSL, 1, 2, NULL, pshader_glsl_texm3x3, WINED3DPS_VERSION(1,2), WINED3DPS_VERSION(1,3)},
    {WINED3DSIO_TEXDEPTH, "texdepth", GLNAME_REQUIRE_GLSL, 1, 1, NULL, pshader_glsl_texdepth, WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_BEM,      "bem",      "undefined",         1, 3, pshader_hw_bem, pshader_glsl_bem, WINED3DPS_VERSION(1,4), WINED3DPS_VERSION(1,4)},
    {WINED3DSIO_DSX,      "dsx",      NULL, 1, 2, NULL, shader_glsl_map2gl, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_DSY,      "dsy",      NULL, 1, 2, NULL, shader_glsl_map2gl, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_TEXLDD,   "texldd",   GLNAME_REQUIRE_GLSL, 1, 5, NULL, NULL, WINED3DPS_VERSION(2,1), -1},
    {WINED3DSIO_SETP,     "setp",     GLNAME_REQUIRE_GLSL, 1, 3, NULL, NULL, 0, 0},
    {WINED3DSIO_TEXLDL,   "texldl",   NULL, 1, 3, NULL, shader_glsl_texldl, WINED3DPS_VERSION(3,0), -1},
    {WINED3DSIO_PHASE,    "phase",    GLNAME_REQUIRE_GLSL, 0, 0, NULL, NULL, 0, 0},
    {0,               NULL,       NULL,   0, 0, NULL,            NULL, 0, 0}
};

static void pshader_set_limits(
      IWineD3DPixelShaderImpl *This) { 

      This->baseShader.limits.attributes = 0;
      This->baseShader.limits.address = 0;
      This->baseShader.limits.packed_output = 0;

      switch (This->baseShader.hex_version) {
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
                       This->baseShader.hex_version);
      }
}

/** Generate a pixel shader string using either GL_FRAGMENT_PROGRAM_ARB
    or GLSL and send it to the card */
static inline VOID IWineD3DPixelShaderImpl_GenerateShader(
    IWineD3DPixelShader *iface,
    shader_reg_maps* reg_maps,
    CONST DWORD *pFunction) {

    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    SHADER_BUFFER buffer;

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

    if (This->baseShader.shader_mode == SHADER_GLSL) {

        /* Create the hw GLSL shader object and assign it as the baseShader.prgId */
        GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));

        if (GL_SUPPORT(ARB_DRAW_BUFFERS)) {
            shader_addline(&buffer, "#extension GL_ARB_draw_buffers : enable\n");
        }

        /* Base Declarations */
        shader_generate_glsl_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer, &GLINFO_LOCATION);

        /* Pack 3.0 inputs */
        if (This->baseShader.hex_version >= WINED3DPS_VERSION(3,0))
            pshader_glsl_input_pack(&buffer, This->semantics_in);

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        /* Pixel shaders < 2.0 place the resulting color in R0 implicitly */
        if (This->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
            /* Some older cards like GeforceFX ones don't support multiple buffers, so also not gl_FragData */
            if(GL_SUPPORT(ARB_DRAW_BUFFERS))
                shader_addline(&buffer, "gl_FragData[0] = R0;\n");
            else
                shader_addline(&buffer, "gl_FragColor = R0;\n");
        }

        /* Pixel shader < 3.0 do not replace the fog stage.
         * This implements linear fog computation and blending.
         * TODO: non linear fog
         * NOTE: gl_Fog.start and gl_Fog.end don't hold fog start s and end e but
         * -1/(e-s) and e/(e-s) respectively.
         */
        if(This->baseShader.hex_version < WINED3DPS_VERSION(3,0)) {
            shader_addline(&buffer, "float Fog = clamp(gl_FogFragCoord * gl_Fog.start + gl_Fog.end, 0.0, 1.0);\n");
            if(GL_SUPPORT(ARB_DRAW_BUFFERS))
                shader_addline(&buffer, "gl_FragData[0].xyz = mix(gl_Fog.color.xyz, gl_FragData[0].xyz, Fog);\n");
            else
                shader_addline(&buffer, "gl_FragColor.xyz = mix(gl_Fog.color.xyz, gl_FragColor.xyz, Fog);\n");
        }

        shader_addline(&buffer, "}\n");

        TRACE("Compiling shader object %u\n", shader_obj);
        GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer.buffer, NULL));
        GL_EXTCALL(glCompileShaderARB(shader_obj));
        print_glsl_info_log(&GLINFO_LOCATION, shader_obj);

        /* Store the shader object */
        This->baseShader.prgId = shader_obj;

    } else if (This->baseShader.shader_mode == SHADER_ARB) {
        /*  Create the hw ARB shader */
        shader_addline(&buffer, "!!ARBfp1.0\n");

        shader_addline(&buffer, "TEMP TMP;\n");     /* Used in matrix ops */
        shader_addline(&buffer, "TEMP TMP2;\n");    /* Used in matrix ops */
        shader_addline(&buffer, "TEMP TA;\n");      /* Used for modifiers */
        shader_addline(&buffer, "TEMP TB;\n");      /* Used for modifiers */
        shader_addline(&buffer, "TEMP TC;\n");      /* Used for modifiers */
        shader_addline(&buffer, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
        shader_addline(&buffer, "PARAM coefmul = { 2, 4, 8, 16 };\n");
        shader_addline(&buffer, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");

        /* Base Declarations */
        shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer, &GLINFO_LOCATION);

        /* We need two variables for fog blending */
        shader_addline(&buffer, "TEMP TMP_FOG;\n");
        if (This->baseShader.hex_version >= WINED3DPS_VERSION(2,0)) {
            shader_addline(&buffer, "TEMP TMP_COLOR;\n");
        }

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        /* calculate fog and blend it
         * NOTE: state.fog.params.y and state.fog.params.z don't hold fog start s and end e but
         * -1/(e-s) and e/(e-s) respectively.
         */
        shader_addline(&buffer, "MAD_SAT TMP_FOG, fragment.fogcoord, state.fog.params.y, state.fog.params.z;\n");
        if (This->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
            shader_addline(&buffer, "LRP result.color.rgb, TMP_FOG.x, R0, state.fog.color;\n");
            shader_addline(&buffer, "MOV result.color.a, R0.a;\n");
        } else {
            shader_addline(&buffer, "LRP result.color.rgb, TMP_FOG.x, TMP_COLOR, state.fog.color;\n");
            shader_addline(&buffer, "MOV result.color.a, TMP_COLOR.a;\n");
        }

        shader_addline(&buffer, "END\n"); 

        /* TODO: change to resource.glObjectHandle or something like that */
        GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

        TRACE("Creating a hw pixel shader, prg=%d\n", This->baseShader.prgId);
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, This->baseShader.prgId));

        TRACE("Created hw pixel shader, prg=%d\n", This->baseShader.prgId);
        /* Create the program and check for errors */
        GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            buffer.bsize, buffer.buffer));

        if (glGetError() == GL_INVALID_OPERATION) {
            GLint errPos;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
            FIXME("HW PixelShader Error at position %d: %s\n",
                  errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
            This->baseShader.prgId = -1;
        }
    }

    This->needsbumpmat = reg_maps->bumpmat;

#if 1 /* if were using the data buffer of device then we don't need to free it */
  HeapFree(GetProcessHeap(), 0, buffer.buffer);
#endif
}

static HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface, CONST DWORD *pFunction) {

    IWineD3DPixelShaderImpl *This =(IWineD3DPixelShaderImpl *)iface;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;

    TRACE("(%p) : pFunction %p\n", iface, pFunction);

    /* First pass: trace shader */
    shader_trace_init((IWineD3DBaseShader*) This, pFunction);
    pshader_set_limits(This);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) > 1) {
        shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
        HRESULT hr;

        /* Second pass: figure out which registers are used, what the semantics are, etc.. */
        memset(reg_maps, 0, sizeof(shader_reg_maps));
        hr = shader_get_registers_used((IWineD3DBaseShader*) This, reg_maps,
            This->semantics_in, NULL, pFunction, NULL);
        if (FAILED(hr)) return hr;
        /* FIXME: validate reg_maps against OpenGL */
    }

    This->baseShader.shader_mode = deviceImpl->ps_selected_mode;

    TRACE("(%p) : Copying the function\n", This);
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

static HRESULT WINAPI IWineD3DPixelShaderImpl_CompileShader(IWineD3DPixelShader *iface) {

    IWineD3DPixelShaderImpl *This =(IWineD3DPixelShaderImpl *)iface;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    CONST DWORD *function = This->baseShader.function;

    TRACE("(%p) : function %p\n", iface, function);

    /* We're already compiled. */
    if (This->baseShader.is_compiled) return WINED3D_OK;

    /* We don't need to compile */
    if (!function) {
        This->baseShader.is_compiled = TRUE;
        return WINED3D_OK;
    }

    if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1) {
        shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
        HRESULT hr;

        /* Second pass: figure out which registers are used, what the semantics are, etc.. */
        memset(reg_maps, 0, sizeof(shader_reg_maps));
        hr = shader_get_registers_used((IWineD3DBaseShader*)This, reg_maps,
            This->semantics_in, NULL, This->baseShader.function, deviceImpl->stateBlock);
        if (FAILED(hr)) return hr;
        /* FIXME: validate reg_maps against OpenGL */
    }

    /* Generate the HW shader */
    TRACE("(%p) : Generating hardware program\n", This);
    IWineD3DPixelShaderImpl_GenerateShader(iface, &This->baseShader.reg_maps, function);

    This->baseShader.is_compiled = TRUE;

    return WINED3D_OK;
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
    IWineD3DPixelShaderImpl_CompileShader,
    /*** IWineD3DPixelShader methods ***/
    IWineD3DPixelShaderImpl_GetDevice,
    IWineD3DPixelShaderImpl_GetFunction
};
