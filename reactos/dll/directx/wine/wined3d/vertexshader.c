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

#define GLINFO_LOCATION ((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info

/* Shader debugging - Change the following line to enable debugging of software
      vertex shaders                                                             */
#if 0 /* Musxt not be 1 in cvs version */
# define VSTRACE(A) TRACE A
# define TRACE_VSVECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w)
#else
# define VSTRACE(A)
# define TRACE_VSVECTOR(name)
#endif

/**
 * DirectX9 SDK download
 *  http://msdn.microsoft.com/library/default.asp?url=/downloads/list/directx.asp
 *
 * Exploring D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx07162002.asp
 *
 * Using Vertex Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx02192001.asp
 *
 * Dx9 New
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/whatsnew.asp
 *
 * Dx9 Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/VertexShader2_0.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/Instructions/Instructions.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexDeclaration/VertexDeclaration.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader3_0/VertexShader3_0.asp
 *
 * Dx9 D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/VertexPipe/matrixstack/matrixstack.asp
 *
 * FVF
 *  http://msdn.microsoft.com/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management
 */

/* TODO: Vertex and Pixel shaders are almost identicle, the only exception being the way that some of the data is looked up or the availablity of some of the data i.e. some instructions are only valid for pshaders and some for vshaders
because of this the bulk of the software pipeline can be shared between pixel and vertex shaders... and it wouldn't supprise me if the programes can be cross compiled using a large body body shared code */

#define GLNAME_REQUIRE_GLSL  ((const char *)1)

CONST SHADER_OPCODE IWineD3DVertexShaderImpl_shader_ins[] = {
    /* This table is not order or position dependent. */

    /* Arithmetic */
    {WINED3DSIO_NOP,    "nop",  "NOP", 0, 0, vshader_hw_map2gl,   NULL, 0, 0},
    {WINED3DSIO_MOV,    "mov",  "MOV", 1, 2, vshader_hw_map2gl,   shader_glsl_mov, 0, 0},
    {WINED3DSIO_MOVA,   "mova",  NULL, 1, 2, vshader_hw_map2gl,   shader_glsl_mov, WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_ADD,    "add",  "ADD", 1, 3, vshader_hw_map2gl,   shader_glsl_arith, 0, 0},
    {WINED3DSIO_SUB,    "sub",  "SUB", 1, 3, vshader_hw_map2gl,   shader_glsl_arith, 0, 0},
    {WINED3DSIO_MAD,    "mad",  "MAD", 1, 4, vshader_hw_map2gl,   shader_glsl_mad, 0, 0},
    {WINED3DSIO_MUL,    "mul",  "MUL", 1, 3, vshader_hw_map2gl,   shader_glsl_arith, 0, 0},
    {WINED3DSIO_RCP,    "rcp",  "RCP", 1, 2, vshader_hw_rsq_rcp,  shader_glsl_rcp, 0, 0},
    {WINED3DSIO_RSQ,    "rsq",  "RSQ", 1, 2, vshader_hw_rsq_rcp,  shader_glsl_rsq, 0, 0},
    {WINED3DSIO_DP3,    "dp3",  "DP3", 1, 3, vshader_hw_map2gl,   shader_glsl_dot, 0, 0},
    {WINED3DSIO_DP4,    "dp4",  "DP4", 1, 3, vshader_hw_map2gl,   shader_glsl_dot, 0, 0},
    {WINED3DSIO_MIN,    "min",  "MIN", 1, 3, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_MAX,    "max",  "MAX", 1, 3, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_SLT,    "slt",  "SLT", 1, 3, vshader_hw_map2gl,   shader_glsl_compare, 0, 0},
    {WINED3DSIO_SGE,    "sge",  "SGE", 1, 3, vshader_hw_map2gl,   shader_glsl_compare, 0, 0},
    {WINED3DSIO_ABS,    "abs",  "ABS", 1, 2, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_EXP,    "exp",  "EX2", 1, 2, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_LOG,    "log",  "LG2", 1, 2, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_EXPP,   "expp", "EXP", 1, 2, vshader_hw_map2gl,   shader_glsl_expp, 0, 0},
    {WINED3DSIO_LOGP,   "logp", "LOG", 1, 2, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_LIT,    "lit",  "LIT", 1, 2, vshader_hw_map2gl,   shader_glsl_lit, 0, 0},
    {WINED3DSIO_DST,    "dst",  "DST", 1, 3, vshader_hw_map2gl,   shader_glsl_dst, 0, 0},
    {WINED3DSIO_LRP,    "lrp",  "LRP", 1, 4, NULL,                shader_glsl_lrp, 0, 0},
    {WINED3DSIO_FRC,    "frc",  "FRC", 1, 2, vshader_hw_map2gl,   shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_POW,    "pow",  "POW", 1, 3, NULL,                shader_glsl_pow, 0, 0},
    {WINED3DSIO_CRS,    "crs",  "XPS", 1, 3, NULL,                shader_glsl_cross, 0, 0},
    /* TODO: sng can possibly be performed a  s
        RCP tmp, vec
        MUL out, tmp, vec*/
    {WINED3DSIO_SGN,  "sgn",  NULL,  1, 2, NULL,                shader_glsl_map2gl, 0, 0},
    /* TODO: xyz normalise can be performed as VS_ARB using one temporary register,
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec.xyz, vec, tmp;
    but I think this is better because it accounts for w properly.
        DP3 tmp , vec, vec;
        RSQ tmp, tmp.x;
        MUL vec, vec, tmp;
    */
    {WINED3DSIO_NRM,    "nrm",      NULL, 1, 2, NULL, shader_glsl_map2gl, 0, 0},
    {WINED3DSIO_SINCOS, "sincos",   NULL, 1, 4, NULL, shader_glsl_sincos, WINED3DVS_VERSION(2,0), WINED3DVS_VERSION(2,1)},
    {WINED3DSIO_SINCOS, "sincos",   NULL, 1, 2, NULL, shader_glsl_sincos, WINED3DVS_VERSION(3,0), -1},
    /* Matrix */
    {WINED3DSIO_M4x4,   "m4x4", "undefined", 1, 3, vshader_hw_mnxn, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M4x3,   "m4x3", "undefined", 1, 3, vshader_hw_mnxn, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x4,   "m3x4", "undefined", 1, 3, vshader_hw_mnxn, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x3,   "m3x3", "undefined", 1, 3, vshader_hw_mnxn, shader_glsl_mnxn, 0, 0},
    {WINED3DSIO_M3x2,   "m3x2", "undefined", 1, 3, vshader_hw_mnxn, shader_glsl_mnxn, 0, 0},
    /* Declare registers */
    {WINED3DSIO_DCL,    "dcl",      NULL,                0, 2, NULL, NULL, 0, 0},
    /* Constant definitions */
    {WINED3DSIO_DEF,    "def",      NULL,                1, 5, NULL, NULL, 0, 0},
    {WINED3DSIO_DEFB,   "defb",     GLNAME_REQUIRE_GLSL, 1, 2, NULL, NULL, 0, 0},
    {WINED3DSIO_DEFI,   "defi",     GLNAME_REQUIRE_GLSL, 1, 5, NULL, NULL, 0, 0},
    /* Flow control - requires GLSL or software shaders */
    {WINED3DSIO_REP ,   "rep",      NULL, 0, 1, NULL, shader_glsl_rep,    WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_ENDREP, "endrep",   NULL, 0, 0, NULL, shader_glsl_end,    WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_IF,     "if",       NULL, 0, 1, NULL, shader_glsl_if,     WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_IFC,    "ifc",      NULL, 0, 2, NULL, shader_glsl_ifc,    WINED3DVS_VERSION(2,1), -1},
    {WINED3DSIO_ELSE,   "else",     NULL, 0, 0, NULL, shader_glsl_else,   WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_ENDIF,  "endif",    NULL, 0, 0, NULL, shader_glsl_end,    WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_BREAK,  "break",    NULL, 0, 0, NULL, shader_glsl_break,  WINED3DVS_VERSION(2,1), -1},
    {WINED3DSIO_BREAKC, "breakc",   NULL, 0, 2, NULL, shader_glsl_breakc, WINED3DVS_VERSION(2,1), -1},
    {WINED3DSIO_BREAKP, "breakp",   GLNAME_REQUIRE_GLSL, 0, 1, NULL, NULL, 0, 0},
    {WINED3DSIO_CALL,   "call",     NULL, 0, 1, NULL, shader_glsl_call,   WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_CALLNZ, "callnz",   NULL, 0, 2, NULL, shader_glsl_callnz, WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_LOOP,   "loop",     NULL, 0, 2, NULL, shader_glsl_loop,   WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_RET,    "ret",      NULL, 0, 0, NULL, NULL,               WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_ENDLOOP,"endloop",  NULL, 0, 0, NULL, shader_glsl_end,    WINED3DVS_VERSION(2,0), -1},
    {WINED3DSIO_LABEL,  "label",    NULL, 0, 1, NULL, shader_glsl_label,  WINED3DVS_VERSION(2,0), -1},

    {WINED3DSIO_SETP,   "setp",     GLNAME_REQUIRE_GLSL, 1, 3, NULL, NULL, 0, 0},
    {WINED3DSIO_TEXLDL, "texldl",   NULL, 1, 3, NULL, shader_glsl_texldl, WINED3DVS_VERSION(3,0), -1},
    {0,                 NULL,       NULL,                0, 0, NULL, NULL, 0, 0}
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
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    IWineD3DVertexDeclarationImpl *vertexDeclaration = (IWineD3DVertexDeclarationImpl *)deviceImpl->stateBlock->vertexDecl;

    DWORD usage_token = This->semantics_in[regnum].usage;
    DWORD usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
    DWORD usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

    if (vertexDeclaration) {
        int i;
        /* Find the declaration element that matches our register, then check
         * if it has D3DCOLOR as it's type. This works for both d3d8 and d3d9. */
        for (i = 0; i < vertexDeclaration->declarationWNumElements-1; ++i) {
            WINED3DVERTEXELEMENT *element = vertexDeclaration->pDeclarationWine + i;
            if (match_usage(element->Usage, element->UsageIndex, usage, usage_idx)) {
                return element->Type == WINED3DDECLTYPE_D3DCOLOR;
            }
        }
    }

    ERR("Either no vertexdeclaration present, or register not matched. This should never happen.\n");
    return FALSE;
}

/** Generate a vertex shader string using either GL_VERTEX_PROGRAM_ARB
    or GLSL and send it to the card */
static VOID IWineD3DVertexShaderImpl_GenerateShader(
    IWineD3DVertexShader *iface,
    shader_reg_maps* reg_maps,
    CONST DWORD *pFunction) {

    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
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

        /* Create the hw GLSL shader program and assign it as the baseShader.prgId */
        GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));

        /* Base Declarations */
        shader_generate_glsl_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer, &GLINFO_LOCATION);

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        /* Unpack 3.0 outputs */
        if (This->baseShader.hex_version >= WINED3DVS_VERSION(3,0))
            vshader_glsl_output_unpack(&buffer, This->semantics_out);

        /* If this shader doesn't use fog copy the z coord to the fog coord so that we can use table fog */
        if (!reg_maps->fog)
            shader_addline(&buffer, "gl_FogFragCoord = gl_Position.z;\n");
        
        /* Write the final position.
         *
         * OpenGL coordinates specify the center of the pixel while d3d coords specify
         * the corner. The offsets are stored in z and w in the 2nd row of the projection
         * matrix to avoid wasting a free shader constant. Add them to the w and z coord
         * of the 2nd row
         */
        shader_addline(&buffer, "gl_Position.x = gl_Position.x + posFixup[2];\n");
        shader_addline(&buffer, "gl_Position.y = gl_Position.y + posFixup[3];\n");
        /* Account for any inverted textures (render to texture case) by reversing the y coordinate
         *  (this is handled in drawPrim() when it sets the MODELVIEW and PROJECTION matrices)
         */
        shader_addline(&buffer, "gl_Position.y = gl_Position.y * posFixup[1];\n");

        shader_addline(&buffer, "}\n");

        TRACE("Compiling shader object %u\n", shader_obj);
        GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer.buffer, NULL));
        GL_EXTCALL(glCompileShaderARB(shader_obj));
        print_glsl_info_log(&GLINFO_LOCATION, shader_obj);

        /* Store the shader object */
        This->baseShader.prgId = shader_obj;

    } else if (This->baseShader.shader_mode == SHADER_ARB) {

        /*  Create the hw ARB shader */
        shader_addline(&buffer, "!!ARBvp1.0\n");

        /* Mesa supports only 95 constants */
        if (GL_VEND(MESA) || GL_VEND(WINE))
            This->baseShader.limits.constant_float = 
                min(95, This->baseShader.limits.constant_float);

        /* Base Declarations */
        shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, &buffer, &GLINFO_LOCATION);

        /* We need a constant to fixup the final position */
        shader_addline(&buffer, "PARAM posFixup = program.env[%d];\n", ARB_SHADER_PRIVCONST_POS);

        /* Base Shader Body */
        shader_generate_main( (IWineD3DBaseShader*) This, &buffer, reg_maps, pFunction);

        /* If this shader doesn't use fog copy the z coord to the fog coord so that we can use table fog */
        if (!reg_maps->fog)
            shader_addline(&buffer, "MOV result.fogcoord, TMP_OUT.z;\n");

        /* Write the final position.
         *
         * OpenGL coordinates specify the center of the pixel while d3d coords specify
         * the corner. The offsets are stored in the 2nd row of the projection matrix,
         * the x offset in z and the y offset in w. Add them to the resulting position
         */
        shader_addline(&buffer, "ADD TMP_OUT.x, TMP_OUT.x, posFixup.z;\n");
        shader_addline(&buffer, "ADD TMP_OUT.y, TMP_OUT.y, posFixup.w;\n");
        /* Account for any inverted textures (render to texture case) by reversing the y coordinate
         *  (this is handled in drawPrim() when it sets the MODELVIEW and PROJECTION matrices)
         */
        shader_addline(&buffer, "MUL TMP_OUT.y, TMP_OUT.y, posFixup.y;\n");

        shader_addline(&buffer, "MOV result.position, TMP_OUT;\n");
        
        shader_addline(&buffer, "END\n"); 

        /* TODO: change to resource.glObjectHandle or something like that */
        GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

        TRACE("Creating a hw vertex shader, prg=%d\n", This->baseShader.prgId);
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, This->baseShader.prgId));

        TRACE("Created hw vertex shader, prg=%d\n", This->baseShader.prgId);
        /* Create the program and check for errors */
        GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            buffer.bsize, buffer.buffer));

        if (glGetError() == GL_INVALID_OPERATION) {
            GLint errPos;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
            FIXME("HW VertexShader Error at position %d: %s\n",
                  errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
            This->baseShader.prgId = -1;
        }
    }

#if 1 /* if were using the data buffer of device then we don't need to free it */
  HeapFree(GetProcessHeap(), 0, buffer.buffer);
#endif
}

/* *******************************************
   IWineD3DVertexShader IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexShaderImpl_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown) 
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
        || IsEqualGUID(riid, &IID_IWineD3DVertexShader)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexShaderImpl_AddRef(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IWineD3DVertexShaderImpl_Release(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if(iface == ((IWineD3DDeviceImpl *) This->baseShader.device)->stateBlock->vertexShader) {
            /* See comment in PixelShader::Release */
            IWineD3DDeviceImpl_MarkStateDirty((IWineD3DDeviceImpl *) This->baseShader.device, STATE_VSHADER);
        }

        if (This->baseShader.shader_mode == SHADER_GLSL && This->baseShader.prgId != 0) {
            struct list *linked_programs = &This->baseShader.linked_programs;

            TRACE("Deleting linked programs\n");
            if (linked_programs->next) {
                struct glsl_shader_prog_link *entry, *entry2;
                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, vshader_entry) {
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
    memset(reg_maps, 0, sizeof(shader_reg_maps));
    hr = shader_get_registers_used((IWineD3DBaseShader*) This, reg_maps,
       This->semantics_in, This->semantics_out, pFunction, NULL);
    if (hr != WINED3D_OK) return hr;

    This->baseShader.shader_mode = deviceImpl->vs_selected_mode;

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

static HRESULT WINAPI IWineD3DVertexShaderImpl_CompileShader(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    CONST DWORD *function = This->baseShader.function;

    TRACE("(%p) : function %p\n", iface, function);

    /* We're already compiled. */
    if (This->baseShader.is_compiled) return WINED3D_OK;

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
