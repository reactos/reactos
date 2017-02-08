/*
 * Direct3D bytecode output functions
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
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
 *
 */

#include "d3dcompiler_private.h"
#include "d3d9types.h"

WINE_DEFAULT_DEBUG_CHANNEL(bytecodewriter);

/****************************************************************
 * General assembler shader construction helper routines follow *
 ****************************************************************/
/* struct instruction *alloc_instr
 *
 * Allocates a new instruction structure with srcs registers
 *
 * Parameters:
 *  srcs: Number of source registers to allocate
 *
 * Returns:
 *  A pointer to the allocated instruction structure
 *  NULL in case of an allocation failure
 */
struct instruction *alloc_instr(unsigned int srcs) {
    struct instruction *ret = d3dcompiler_alloc(sizeof(*ret));
    if(!ret) {
        ERR("Failed to allocate memory for an instruction structure\n");
        return NULL;
    }

    if(srcs) {
        ret->src = d3dcompiler_alloc(srcs * sizeof(*ret->src));
        if(!ret->src) {
            ERR("Failed to allocate memory for instruction registers\n");
            d3dcompiler_free(ret);
            return NULL;
        }
        ret->num_srcs = srcs;
    }
    return ret;
}

/* void add_instruction
 *
 * Adds a new instruction to the shader's instructions array and grows the instruction array
 * if needed.
 *
 * The function does NOT copy the instruction structure. Make sure not to release the
 * instruction or any of its substructures like registers.
 *
 * Parameters:
 *  shader: Shader to add the instruction to
 *  instr: Instruction to add to the shader
 */
BOOL add_instruction(struct bwriter_shader *shader, struct instruction *instr) {
    struct instruction      **new_instructions;

    if(!shader) return FALSE;

    if(shader->instr_alloc_size == 0) {
        shader->instr = d3dcompiler_alloc(sizeof(*shader->instr) * INSTRARRAY_INITIAL_SIZE);
        if(!shader->instr) {
            ERR("Failed to allocate the shader instruction array\n");
            return FALSE;
        }
        shader->instr_alloc_size = INSTRARRAY_INITIAL_SIZE;
    } else if(shader->instr_alloc_size == shader->num_instrs) {
        new_instructions = d3dcompiler_realloc(shader->instr,
                                       sizeof(*shader->instr) * (shader->instr_alloc_size) * 2);
        if(!new_instructions) {
            ERR("Failed to grow the shader instruction array\n");
            return FALSE;
        }
        shader->instr = new_instructions;
        shader->instr_alloc_size = shader->instr_alloc_size * 2;
    } else if(shader->num_instrs > shader->instr_alloc_size) {
        ERR("More instructions than allocated. This should not happen\n");
        return FALSE;
    }

    shader->instr[shader->num_instrs] = instr;
    shader->num_instrs++;
    return TRUE;
}

BOOL add_constF(struct bwriter_shader *shader, DWORD reg, float x, float y, float z, float w) {
    struct constant *newconst;

    if(shader->num_cf) {
        struct constant **newarray;
        newarray = d3dcompiler_realloc(shader->constF,
                               sizeof(*shader->constF) * (shader->num_cf + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constF = newarray;
    } else {
        shader->constF = d3dcompiler_alloc(sizeof(*shader->constF));
        if(!shader->constF) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = d3dcompiler_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].f = x;
    newconst->value[1].f = y;
    newconst->value[2].f = z;
    newconst->value[3].f = w;
    shader->constF[shader->num_cf] = newconst;

    shader->num_cf++;
    return TRUE;
}

BOOL add_constI(struct bwriter_shader *shader, DWORD reg, INT x, INT y, INT z, INT w) {
    struct constant *newconst;

    if(shader->num_ci) {
        struct constant **newarray;
        newarray = d3dcompiler_realloc(shader->constI,
                               sizeof(*shader->constI) * (shader->num_ci + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constI = newarray;
    } else {
        shader->constI = d3dcompiler_alloc(sizeof(*shader->constI));
        if(!shader->constI) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = d3dcompiler_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].i = x;
    newconst->value[1].i = y;
    newconst->value[2].i = z;
    newconst->value[3].i = w;
    shader->constI[shader->num_ci] = newconst;

    shader->num_ci++;
    return TRUE;
}

BOOL add_constB(struct bwriter_shader *shader, DWORD reg, BOOL x) {
    struct constant *newconst;

    if(shader->num_cb) {
        struct constant **newarray;
        newarray = d3dcompiler_realloc(shader->constB,
                               sizeof(*shader->constB) * (shader->num_cb + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constB = newarray;
    } else {
        shader->constB = d3dcompiler_alloc(sizeof(*shader->constB));
        if(!shader->constB) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = d3dcompiler_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].b = x;
    shader->constB[shader->num_cb] = newconst;

    shader->num_cb++;
    return TRUE;
}

BOOL record_declaration(struct bwriter_shader *shader, DWORD usage,
                        DWORD usage_idx, DWORD mod, BOOL output,
                        DWORD regnum, DWORD writemask, BOOL builtin) {
    unsigned int *num;
    struct declaration **decl;
    unsigned int i;

    if(!shader) return FALSE;

    if(output) {
        num = &shader->num_outputs;
        decl = &shader->outputs;
    } else {
        num = &shader->num_inputs;
        decl = &shader->inputs;
    }

    if(*num == 0) {
        *decl = d3dcompiler_alloc(sizeof(**decl));
        if(!*decl) {
            ERR("Error allocating declarations array\n");
            return FALSE;
        }
    } else {
        struct declaration *newdecl;
        for(i = 0; i < *num; i++) {
            if((*decl)[i].regnum == regnum && ((*decl)[i].writemask & writemask)) {
                WARN("Declaration of register %u already exists, writemask match 0x%x\n",
                      regnum, (*decl)[i].writemask & writemask);
            }
        }

        newdecl = d3dcompiler_realloc(*decl,
                              sizeof(**decl) * ((*num) + 1));
        if(!newdecl) {
            ERR("Error reallocating declarations array\n");
            return FALSE;
        }
        *decl = newdecl;
    }
    (*decl)[*num].usage = usage;
    (*decl)[*num].usage_idx = usage_idx;
    (*decl)[*num].regnum = regnum;
    (*decl)[*num].mod = mod;
    (*decl)[*num].writemask = writemask;
    (*decl)[*num].builtin = builtin;
    (*num)++;

    return TRUE;
}

BOOL record_sampler(struct bwriter_shader *shader, DWORD samptype, DWORD mod, DWORD regnum) {
    unsigned int i;

    if(!shader) return FALSE;

    if(shader->num_samplers == 0) {
        shader->samplers = d3dcompiler_alloc(sizeof(*shader->samplers));
        if(!shader->samplers) {
            ERR("Error allocating samplers array\n");
            return FALSE;
        }
    } else {
        struct samplerdecl *newarray;

        for(i = 0; i < shader->num_samplers; i++) {
            if(shader->samplers[i].regnum == regnum) {
                WARN("Sampler %u already declared\n", regnum);
                /* This is not an error as far as the assembler is concerned.
                 * Direct3D might refuse to load the compiled shader though
                 */
            }
        }

        newarray = d3dcompiler_realloc(shader->samplers,
                               sizeof(*shader->samplers) * (shader->num_samplers + 1));
        if(!newarray) {
            ERR("Error reallocating samplers array\n");
            return FALSE;
        }
        shader->samplers = newarray;
    }

    shader->samplers[shader->num_samplers].type = samptype;
    shader->samplers[shader->num_samplers].mod = mod;
    shader->samplers[shader->num_samplers].regnum = regnum;
    shader->num_samplers++;
    return TRUE;
}


/* shader bytecode buffer manipulation functions.
 * allocate_buffer creates a new buffer structure, put_dword adds a new
 * DWORD to the buffer. In the rare case of a memory allocation failure
 * when trying to grow the buffer a flag is set in the buffer to mark it
 * invalid. This avoids return value checking and passing in many places
 */
static struct bytecode_buffer *allocate_buffer(void) {
    struct bytecode_buffer *ret;

    ret = d3dcompiler_alloc(sizeof(*ret));
    if(!ret) return NULL;

    ret->alloc_size = BYTECODEBUFFER_INITIAL_SIZE;
    ret->data = d3dcompiler_alloc(sizeof(DWORD) * ret->alloc_size);
    if(!ret->data) {
        d3dcompiler_free(ret);
        return NULL;
    }
    ret->state = S_OK;
    return ret;
}

static void put_dword(struct bytecode_buffer *buffer, DWORD value) {
    if(FAILED(buffer->state)) return;

    if(buffer->alloc_size == buffer->size) {
        DWORD *newarray;
        buffer->alloc_size *= 2;
        newarray = d3dcompiler_realloc(buffer->data,
                               sizeof(DWORD) * buffer->alloc_size);
        if(!newarray) {
            ERR("Failed to grow the buffer data memory\n");
            buffer->state = E_OUTOFMEMORY;
            return;
        }
        buffer->data = newarray;
    }
    buffer->data[buffer->size++] = value;
}

/* bwriter -> d3d9 conversion functions. */
static DWORD d3d9_swizzle(DWORD bwriter_swizzle)
{
    /* Currently a NOP, but this allows changing the internal definitions
     * without side effects. */
    DWORD ret = 0;

    if ((bwriter_swizzle & BWRITERVS_X_X) == BWRITERVS_X_X) ret |= D3DVS_X_X;
    if ((bwriter_swizzle & BWRITERVS_X_Y) == BWRITERVS_X_Y) ret |= D3DVS_X_Y;
    if ((bwriter_swizzle & BWRITERVS_X_Z) == BWRITERVS_X_Z) ret |= D3DVS_X_Z;
    if ((bwriter_swizzle & BWRITERVS_X_W) == BWRITERVS_X_W) ret |= D3DVS_X_W;

    if ((bwriter_swizzle & BWRITERVS_Y_X) == BWRITERVS_Y_X) ret |= D3DVS_Y_X;
    if ((bwriter_swizzle & BWRITERVS_Y_Y) == BWRITERVS_Y_Y) ret |= D3DVS_Y_Y;
    if ((bwriter_swizzle & BWRITERVS_Y_Z) == BWRITERVS_Y_Z) ret |= D3DVS_Y_Z;
    if ((bwriter_swizzle & BWRITERVS_Y_W) == BWRITERVS_Y_W) ret |= D3DVS_Y_W;

    if ((bwriter_swizzle & BWRITERVS_Z_X) == BWRITERVS_Z_X) ret |= D3DVS_Z_X;
    if ((bwriter_swizzle & BWRITERVS_Z_Y) == BWRITERVS_Z_Y) ret |= D3DVS_Z_Y;
    if ((bwriter_swizzle & BWRITERVS_Z_Z) == BWRITERVS_Z_Z) ret |= D3DVS_Z_Z;
    if ((bwriter_swizzle & BWRITERVS_Z_W) == BWRITERVS_Z_W) ret |= D3DVS_Z_W;

    if ((bwriter_swizzle & BWRITERVS_W_X) == BWRITERVS_W_X) ret |= D3DVS_W_X;
    if ((bwriter_swizzle & BWRITERVS_W_Y) == BWRITERVS_W_Y) ret |= D3DVS_W_Y;
    if ((bwriter_swizzle & BWRITERVS_W_Z) == BWRITERVS_W_Z) ret |= D3DVS_W_Z;
    if ((bwriter_swizzle & BWRITERVS_W_W) == BWRITERVS_W_W) ret |= D3DVS_W_W;

    return ret;
}

static DWORD d3d9_writemask(DWORD bwriter_writemask)
{
    DWORD ret = 0;

    if (bwriter_writemask & BWRITERSP_WRITEMASK_0) ret |= D3DSP_WRITEMASK_0;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_1) ret |= D3DSP_WRITEMASK_1;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_2) ret |= D3DSP_WRITEMASK_2;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_3) ret |= D3DSP_WRITEMASK_3;

    return ret;
}

static DWORD d3d9_srcmod(DWORD bwriter_srcmod)
{
    switch (bwriter_srcmod)
    {
        case BWRITERSPSM_NONE:       return D3DSPSM_NONE;
        case BWRITERSPSM_NEG:        return D3DSPSM_NEG;
        case BWRITERSPSM_BIAS:       return D3DSPSM_BIAS;
        case BWRITERSPSM_BIASNEG:    return D3DSPSM_BIASNEG;
        case BWRITERSPSM_SIGN:       return D3DSPSM_SIGN;
        case BWRITERSPSM_SIGNNEG:    return D3DSPSM_SIGNNEG;
        case BWRITERSPSM_COMP:       return D3DSPSM_COMP;
        case BWRITERSPSM_X2:         return D3DSPSM_X2;
        case BWRITERSPSM_X2NEG:      return D3DSPSM_X2NEG;
        case BWRITERSPSM_DZ:         return D3DSPSM_DZ;
        case BWRITERSPSM_DW:         return D3DSPSM_DW;
        case BWRITERSPSM_ABS:        return D3DSPSM_ABS;
        case BWRITERSPSM_ABSNEG:     return D3DSPSM_ABSNEG;
        case BWRITERSPSM_NOT:        return D3DSPSM_NOT;
        default:
            FIXME("Unhandled BWRITERSPSM token %#x.\n", bwriter_srcmod);
            return 0;
    }
}

static DWORD d3d9_dstmod(DWORD bwriter_mod)
{
    DWORD ret = 0;

    if (bwriter_mod & BWRITERSPDM_SATURATE)         ret |= D3DSPDM_SATURATE;
    if (bwriter_mod & BWRITERSPDM_PARTIALPRECISION) ret |= D3DSPDM_PARTIALPRECISION;
    if (bwriter_mod & BWRITERSPDM_MSAMPCENTROID)    ret |= D3DSPDM_MSAMPCENTROID;

    return ret;
}

static DWORD d3d9_comparetype(DWORD asmshader_comparetype)
{
    switch (asmshader_comparetype)
    {
        case BWRITER_COMPARISON_GT:     return D3DSPC_GT;
        case BWRITER_COMPARISON_EQ:     return D3DSPC_EQ;
        case BWRITER_COMPARISON_GE:     return D3DSPC_GE;
        case BWRITER_COMPARISON_LT:     return D3DSPC_LT;
        case BWRITER_COMPARISON_NE:     return D3DSPC_NE;
        case BWRITER_COMPARISON_LE:     return D3DSPC_LE;
        default:
            FIXME("Unexpected BWRITER_COMPARISON type %#x.\n", asmshader_comparetype);
            return 0;
    }
}

static DWORD d3d9_sampler(DWORD bwriter_sampler)
{
    if (bwriter_sampler == BWRITERSTT_UNKNOWN)  return D3DSTT_UNKNOWN;
    if (bwriter_sampler == BWRITERSTT_1D)       return D3DSTT_1D;
    if (bwriter_sampler == BWRITERSTT_2D)       return D3DSTT_2D;
    if (bwriter_sampler == BWRITERSTT_CUBE)     return D3DSTT_CUBE;
    if (bwriter_sampler == BWRITERSTT_VOLUME)   return D3DSTT_VOLUME;
    FIXME("Unexpected BWRITERSAMPLER_TEXTURE_TYPE type %#x.\n", bwriter_sampler);

    return 0;
}

static DWORD d3d9_register(DWORD bwriter_register)
{
    if (bwriter_register == BWRITERSPR_TEMP)        return D3DSPR_TEMP;
    if (bwriter_register == BWRITERSPR_INPUT)       return D3DSPR_INPUT;
    if (bwriter_register == BWRITERSPR_CONST)       return D3DSPR_CONST;
    if (bwriter_register == BWRITERSPR_ADDR)        return D3DSPR_ADDR;
    if (bwriter_register == BWRITERSPR_TEXTURE)     return D3DSPR_TEXTURE;
    if (bwriter_register == BWRITERSPR_RASTOUT)     return D3DSPR_RASTOUT;
    if (bwriter_register == BWRITERSPR_ATTROUT)     return D3DSPR_ATTROUT;
    if (bwriter_register == BWRITERSPR_TEXCRDOUT)   return D3DSPR_TEXCRDOUT;
    if (bwriter_register == BWRITERSPR_OUTPUT)      return D3DSPR_OUTPUT;
    if (bwriter_register == BWRITERSPR_CONSTINT)    return D3DSPR_CONSTINT;
    if (bwriter_register == BWRITERSPR_COLOROUT)    return D3DSPR_COLOROUT;
    if (bwriter_register == BWRITERSPR_DEPTHOUT)    return D3DSPR_DEPTHOUT;
    if (bwriter_register == BWRITERSPR_SAMPLER)     return D3DSPR_SAMPLER;
    if (bwriter_register == BWRITERSPR_CONSTBOOL)   return D3DSPR_CONSTBOOL;
    if (bwriter_register == BWRITERSPR_LOOP)        return D3DSPR_LOOP;
    if (bwriter_register == BWRITERSPR_MISCTYPE)    return D3DSPR_MISCTYPE;
    if (bwriter_register == BWRITERSPR_LABEL)       return D3DSPR_LABEL;
    if (bwriter_register == BWRITERSPR_PREDICATE)   return D3DSPR_PREDICATE;

    FIXME("Unexpected BWRITERSPR %#x.\n", bwriter_register);
    return ~0U;
}

static DWORD d3d9_opcode(DWORD bwriter_opcode)
{
    switch (bwriter_opcode)
    {
        case BWRITERSIO_NOP:         return D3DSIO_NOP;
        case BWRITERSIO_MOV:         return D3DSIO_MOV;
        case BWRITERSIO_ADD:         return D3DSIO_ADD;
        case BWRITERSIO_SUB:         return D3DSIO_SUB;
        case BWRITERSIO_MAD:         return D3DSIO_MAD;
        case BWRITERSIO_MUL:         return D3DSIO_MUL;
        case BWRITERSIO_RCP:         return D3DSIO_RCP;
        case BWRITERSIO_RSQ:         return D3DSIO_RSQ;
        case BWRITERSIO_DP3:         return D3DSIO_DP3;
        case BWRITERSIO_DP4:         return D3DSIO_DP4;
        case BWRITERSIO_MIN:         return D3DSIO_MIN;
        case BWRITERSIO_MAX:         return D3DSIO_MAX;
        case BWRITERSIO_SLT:         return D3DSIO_SLT;
        case BWRITERSIO_SGE:         return D3DSIO_SGE;
        case BWRITERSIO_EXP:         return D3DSIO_EXP;
        case BWRITERSIO_LOG:         return D3DSIO_LOG;
        case BWRITERSIO_LIT:         return D3DSIO_LIT;
        case BWRITERSIO_DST:         return D3DSIO_DST;
        case BWRITERSIO_LRP:         return D3DSIO_LRP;
        case BWRITERSIO_FRC:         return D3DSIO_FRC;
        case BWRITERSIO_M4x4:        return D3DSIO_M4x4;
        case BWRITERSIO_M4x3:        return D3DSIO_M4x3;
        case BWRITERSIO_M3x4:        return D3DSIO_M3x4;
        case BWRITERSIO_M3x3:        return D3DSIO_M3x3;
        case BWRITERSIO_M3x2:        return D3DSIO_M3x2;
        case BWRITERSIO_CALL:        return D3DSIO_CALL;
        case BWRITERSIO_CALLNZ:      return D3DSIO_CALLNZ;
        case BWRITERSIO_LOOP:        return D3DSIO_LOOP;
        case BWRITERSIO_RET:         return D3DSIO_RET;
        case BWRITERSIO_ENDLOOP:     return D3DSIO_ENDLOOP;
        case BWRITERSIO_LABEL:       return D3DSIO_LABEL;
        case BWRITERSIO_DCL:         return D3DSIO_DCL;
        case BWRITERSIO_POW:         return D3DSIO_POW;
        case BWRITERSIO_CRS:         return D3DSIO_CRS;
        case BWRITERSIO_SGN:         return D3DSIO_SGN;
        case BWRITERSIO_ABS:         return D3DSIO_ABS;
        case BWRITERSIO_NRM:         return D3DSIO_NRM;
        case BWRITERSIO_SINCOS:      return D3DSIO_SINCOS;
        case BWRITERSIO_REP:         return D3DSIO_REP;
        case BWRITERSIO_ENDREP:      return D3DSIO_ENDREP;
        case BWRITERSIO_IF:          return D3DSIO_IF;
        case BWRITERSIO_IFC:         return D3DSIO_IFC;
        case BWRITERSIO_ELSE:        return D3DSIO_ELSE;
        case BWRITERSIO_ENDIF:       return D3DSIO_ENDIF;
        case BWRITERSIO_BREAK:       return D3DSIO_BREAK;
        case BWRITERSIO_BREAKC:      return D3DSIO_BREAKC;
        case BWRITERSIO_MOVA:        return D3DSIO_MOVA;
        case BWRITERSIO_DEFB:        return D3DSIO_DEFB;
        case BWRITERSIO_DEFI:        return D3DSIO_DEFI;

        case BWRITERSIO_TEXCOORD:    return D3DSIO_TEXCOORD;
        case BWRITERSIO_TEXKILL:     return D3DSIO_TEXKILL;
        case BWRITERSIO_TEX:         return D3DSIO_TEX;
        case BWRITERSIO_TEXBEM:      return D3DSIO_TEXBEM;
        case BWRITERSIO_TEXBEML:     return D3DSIO_TEXBEML;
        case BWRITERSIO_TEXREG2AR:   return D3DSIO_TEXREG2AR;
        case BWRITERSIO_TEXREG2GB:   return D3DSIO_TEXREG2GB;
        case BWRITERSIO_TEXM3x2PAD:  return D3DSIO_TEXM3x2PAD;
        case BWRITERSIO_TEXM3x2TEX:  return D3DSIO_TEXM3x2TEX;
        case BWRITERSIO_TEXM3x3PAD:  return D3DSIO_TEXM3x3PAD;
        case BWRITERSIO_TEXM3x3TEX:  return D3DSIO_TEXM3x3TEX;
        case BWRITERSIO_TEXM3x3SPEC: return D3DSIO_TEXM3x3SPEC;
        case BWRITERSIO_TEXM3x3VSPEC:return D3DSIO_TEXM3x3VSPEC;
        case BWRITERSIO_EXPP:        return D3DSIO_EXPP;
        case BWRITERSIO_LOGP:        return D3DSIO_LOGP;
        case BWRITERSIO_CND:         return D3DSIO_CND;
        case BWRITERSIO_DEF:         return D3DSIO_DEF;
        case BWRITERSIO_TEXREG2RGB:  return D3DSIO_TEXREG2RGB;
        case BWRITERSIO_TEXDP3TEX:   return D3DSIO_TEXDP3TEX;
        case BWRITERSIO_TEXM3x2DEPTH:return D3DSIO_TEXM3x2DEPTH;
        case BWRITERSIO_TEXDP3:      return D3DSIO_TEXDP3;
        case BWRITERSIO_TEXM3x3:     return D3DSIO_TEXM3x3;
        case BWRITERSIO_TEXDEPTH:    return D3DSIO_TEXDEPTH;
        case BWRITERSIO_CMP:         return D3DSIO_CMP;
        case BWRITERSIO_BEM:         return D3DSIO_BEM;
        case BWRITERSIO_DP2ADD:      return D3DSIO_DP2ADD;
        case BWRITERSIO_DSX:         return D3DSIO_DSX;
        case BWRITERSIO_DSY:         return D3DSIO_DSY;
        case BWRITERSIO_TEXLDD:      return D3DSIO_TEXLDD;
        case BWRITERSIO_SETP:        return D3DSIO_SETP;
        case BWRITERSIO_TEXLDL:      return D3DSIO_TEXLDL;
        case BWRITERSIO_BREAKP:      return D3DSIO_BREAKP;

        case BWRITERSIO_PHASE:       return D3DSIO_PHASE;
        case BWRITERSIO_COMMENT:     return D3DSIO_COMMENT;
        case BWRITERSIO_END:         return D3DSIO_END;

        case BWRITERSIO_TEXLDP:      return D3DSIO_TEX | D3DSI_TEXLD_PROJECT;
        case BWRITERSIO_TEXLDB:      return D3DSIO_TEX | D3DSI_TEXLD_BIAS;

        default:
            FIXME("Unhandled BWRITERSIO token %#x.\n", bwriter_opcode);
            return ~0U;
    }
}

static DWORD d3dsp_register( D3DSHADER_PARAM_REGISTER_TYPE type, DWORD num )
{
    return ((type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) |
           ((type << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2) |
           (num & D3DSP_REGNUM_MASK); /* No shift */
}

/******************************************************
 * Implementation of the writer functions starts here *
 ******************************************************/
static void write_declarations(struct bc_writer *This,
                               struct bytecode_buffer *buffer, BOOL len,
                               const struct declaration *decls, unsigned int num, DWORD type) {
    DWORD i;
    DWORD instr_dcl = D3DSIO_DCL;
    DWORD token;
    struct shader_reg reg;

    ZeroMemory(&reg, sizeof(reg));

    if(len) {
        instr_dcl |= 2 << D3DSI_INSTLENGTH_SHIFT;
    }

    for(i = 0; i < num; i++) {
        if(decls[i].builtin) continue;

        /* Write the DCL instruction */
        put_dword(buffer, instr_dcl);

        /* Write the usage and index */
        token = (1u << 31); /* Bit 31 of non-instruction opcodes is 1 */
        token |= (decls[i].usage << D3DSP_DCL_USAGE_SHIFT) & D3DSP_DCL_USAGE_MASK;
        token |= (decls[i].usage_idx << D3DSP_DCL_USAGEINDEX_SHIFT) & D3DSP_DCL_USAGEINDEX_MASK;
        put_dword(buffer, token);

        /* Write the dest register */
        reg.type = type;
        reg.regnum = decls[i].regnum;
        reg.u.writemask = decls[i].writemask;
        This->funcs->dstreg(This, &reg, buffer, 0, decls[i].mod);
    }
}

static void write_const(struct constant **consts, int num, DWORD opcode, DWORD reg_type, struct bytecode_buffer *buffer, BOOL len) {
    int i;
    DWORD instr_def = opcode;
    const DWORD reg = (1u << 31) | d3dsp_register( reg_type, 0 ) | D3DSP_WRITEMASK_ALL;

    if(len) {
        if(opcode == D3DSIO_DEFB)
            instr_def |= 2 << D3DSI_INSTLENGTH_SHIFT;
        else
            instr_def |= 5 << D3DSI_INSTLENGTH_SHIFT;
    }

    for(i = 0; i < num; i++) {
        /* Write the DEF instruction */
        put_dword(buffer, instr_def);

        put_dword(buffer, reg | (consts[i]->regnum & D3DSP_REGNUM_MASK));
        put_dword(buffer, consts[i]->value[0].d);
        if(opcode != D3DSIO_DEFB) {
            put_dword(buffer, consts[i]->value[1].d);
            put_dword(buffer, consts[i]->value[2].d);
            put_dword(buffer, consts[i]->value[3].d);
        }
    }
}

static void write_constF(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constF, shader->num_cf, D3DSIO_DEF, D3DSPR_CONST, buffer, len);
}

/* This function looks for VS 1/2 registers mapping to VS 3 output registers */
static HRESULT vs_find_builtin_varyings(struct bc_writer *This, const struct bwriter_shader *shader) {
    DWORD i;
    DWORD usage, usage_idx, writemask, regnum;

    for(i = 0; i < shader->num_outputs; i++) {
        if(!shader->outputs[i].builtin) continue;

        usage = shader->outputs[i].usage;
        usage_idx = shader->outputs[i].usage_idx;
        writemask = shader->outputs[i].writemask;
        regnum = shader->outputs[i].regnum;

        switch(usage) {
            case BWRITERDECLUSAGE_POSITION:
            case BWRITERDECLUSAGE_POSITIONT:
                if(usage_idx > 0) {
                    WARN("dcl_position%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u is oPos\n", regnum);
                This->oPos_regnum = regnum;
                break;

            case BWRITERDECLUSAGE_COLOR:
                if(usage_idx > 1) {
                    WARN("dcl_color%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_ALL) {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1/2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oD%u\n", regnum, usage_idx);
                This->oD_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if(usage_idx >= 8) {
                    WARN("dcl_color%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != (BWRITERSP_WRITEMASK_0) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2) &&
                   writemask != (BWRITERSP_WRITEMASK_ALL)) {
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oT%u\n", regnum, usage_idx);
                This->oT_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_PSIZE:
                if(usage_idx > 0) {
                    WARN("dcl_psize%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oPts\n", regnum, writemask);
                This->oPts_regnum = regnum;
                This->oPts_mask = writemask;
                break;

            case BWRITERDECLUSAGE_FOG:
                if(usage_idx > 0) {
                    WARN("dcl_fog%u not supported in sm 1 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_0 && writemask != BWRITERSP_WRITEMASK_1 &&
                   writemask != BWRITERSP_WRITEMASK_2 && writemask != BWRITERSP_WRITEMASK_3) {
                    WARN("Unsupported fog writemask\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oFog\n", regnum, writemask);
                This->oFog_regnum = regnum;
                This->oFog_mask = writemask;
                break;

            default:
                WARN("Varying type %u is not supported in shader model 1.x\n", usage);
                return E_INVALIDARG;
        }
    }

    return S_OK;
}

static void vs_1_x_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }

    hr = vs_find_builtin_varyings(This, shader);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_declarations(This, buffer, FALSE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, FALSE);
}

static HRESULT find_ps_builtin_semantics(struct bc_writer *This,
                                         const struct bwriter_shader *shader,
                                         DWORD texcoords) {
    DWORD i;
    DWORD usage, usage_idx, writemask, regnum;

    This->v_regnum[0] = -1; This->v_regnum[1] = -1;
    for(i = 0; i < 8; i++) This->t_regnum[i] = -1;

    for(i = 0; i < shader->num_inputs; i++) {
        if(!shader->inputs[i].builtin) continue;

        usage = shader->inputs[i].usage;
        usage_idx = shader->inputs[i].usage_idx;
        writemask = shader->inputs[i].writemask;
        regnum = shader->inputs[i].regnum;

        switch(usage) {
            case BWRITERDECLUSAGE_COLOR:
                if(usage_idx > 1) {
                    WARN("dcl_color%u not supported in sm 1 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_ALL) {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1\n");
                    return E_INVALIDARG;
                }
                TRACE("v%u is v%u\n", regnum, usage_idx);
                This->v_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if(usage_idx > texcoords) {
                    WARN("dcl_texcoord%u not supported in this shader version\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != (BWRITERSP_WRITEMASK_0) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2) &&
                   writemask != (BWRITERSP_WRITEMASK_ALL)) {
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                } else {
                    writemask = BWRITERSP_WRITEMASK_ALL;
                }
                TRACE("v%u is t%u\n", regnum, usage_idx);
                This->t_regnum[usage_idx] = regnum;
                break;

            default:
                WARN("Varying type %u is not supported in shader model 1.x\n", usage);
                return E_INVALIDARG;
        }
    }

    return S_OK;
}

static void ps_1_x_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    /* First check the constants and varyings, and complain if unsupported things are used */
    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }

    hr = find_ps_builtin_semantics(This, shader, 4);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_constF(shader, buffer, FALSE);
}

static void ps_1_4_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    /* First check the constants and varyings, and complain if unsupported things are used */
    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }
    hr = find_ps_builtin_semantics(This, shader, 6);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_constF(shader, buffer, FALSE);
}

static void end(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    put_dword(buffer, D3DSIO_END);
}

static DWORD map_vs_output(struct bc_writer *This, DWORD regnum, DWORD mask, DWORD *has_components) {
    DWORD i;

    *has_components = TRUE;
    if(regnum == This->oPos_regnum) {
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_POSITION );
    }
    if(regnum == This->oFog_regnum && mask == This->oFog_mask) {
        *has_components = FALSE;
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_FOG ) | D3DSP_WRITEMASK_ALL;
    }
    if(regnum == This->oPts_regnum && mask == This->oPts_mask) {
        *has_components = FALSE;
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_POINT_SIZE ) | D3DSP_WRITEMASK_ALL;
    }
    for(i = 0; i < 2; i++) {
        if(regnum == This->oD_regnum[i]) {
            return d3dsp_register( D3DSPR_ATTROUT, i );
        }
    }
    for(i = 0; i < 8; i++) {
        if(regnum == This->oT_regnum[i]) {
            return d3dsp_register( D3DSPR_TEXCRDOUT, i );
        }
    }

    /* The varying must be undeclared - if an unsupported varying was declared,
     * the vs_find_builtin_varyings function would have caught it and this code
     * would not run */
    WARN("Undeclared varying %u\n", regnum);
    This->state = E_INVALIDARG;
    return -1;
}

static void vs_12_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                         struct bytecode_buffer *buffer,
                         DWORD shift, DWORD mod) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD has_wmask;

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            token |= map_vs_output(This, reg->regnum, reg->u.writemask, &has_wmask);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
            * but are unexpected. If we hit this path it might be due to an error.
            */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= d3dsp_register( reg->type, reg->regnum );
            has_wmask = TRUE;
            break;

        case BWRITERSPR_ADDR:
            if(reg->regnum != 0) {
                WARN("Only a0 exists\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_ADDR, 0 );
            has_wmask = TRUE;
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERVS_VERSION(2, 1)){
                WARN("Predicate register is allowed only in vs_2_x\n");
                This->state = E_INVALIDARG;
                return;
            }
            if(reg->regnum != 0) {
                WARN("Only predicate register p0 exists\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_PREDICATE, 0 );
            has_wmask = TRUE;
            break;

        default:
            WARN("Invalid register type for 1.x-2.x vertex shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    /* strictly speaking there are no modifiers in vs_2_0 and vs_1_x, but they can be written
     * into the bytecode and since the compiler doesn't do such checks write them
     * (the checks are done by the undocumented shader validator)
     */
    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    if(has_wmask) {
        token |= d3d9_writemask(reg->u.writemask);
    }
    put_dword(buffer, token);
}

static void vs_1_x_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD has_swizzle;
    DWORD component;

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected
               by map_vs_output
             */
            switch(reg->u.swizzle) {
                case BWRITERVS_SWIZZLE_X:
                    component = BWRITERSP_WRITEMASK_0;
                    break;
                case BWRITERVS_SWIZZLE_Y:
                    component = BWRITERSP_WRITEMASK_1;
                    break;
                case BWRITERVS_SWIZZLE_Z:
                    component = BWRITERSP_WRITEMASK_2;
                    break;
                case BWRITERVS_SWIZZLE_W:
                    component = BWRITERSP_WRITEMASK_3;
                    break;
                default:
                    component = 0;
            }
            token |= map_vs_output(This, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
             * but are unexpected. If we hit this path it might be due to an error.
             */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
            token |= d3dsp_register( reg->type, reg->regnum );
            if(reg->rel_reg) {
                if(reg->rel_reg->type != BWRITERSPR_ADDR ||
                   reg->rel_reg->regnum != 0 ||
                   reg->rel_reg->u.swizzle != BWRITERVS_SWIZZLE_X) {
                    WARN("Relative addressing in vs_1_x is only allowed with a0.x\n");
                    This->state = E_INVALIDARG;
                    return;
                }
                token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
            }
            break;

        default:
            WARN("Invalid register type for 1.x vshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void write_srcregs(struct bc_writer *This, const struct instruction *instr,
                          struct bytecode_buffer *buffer){
    unsigned int i;
    if(instr->has_predicate){
        This->funcs->srcreg(This, &instr->predicate, buffer);
    }
    for(i = 0; i < instr->num_srcs; i++){
        This->funcs->srcreg(This, &instr->src[i], buffer);
    }
}

static DWORD map_ps13_temp(struct bc_writer *This, const struct shader_reg *reg) {
    if(reg->regnum == T0_REG) {
        return d3dsp_register( D3DSPR_TEXTURE, 0 );
    } else if(reg->regnum == T1_REG) {
        return d3dsp_register( D3DSPR_TEXTURE, 1 );
    } else if(reg->regnum == T2_REG) {
        return d3dsp_register( D3DSPR_TEXTURE, 2 );
    } else if(reg->regnum == T3_REG) {
        return d3dsp_register( D3DSPR_TEXTURE, 3 );
    } else {
        return d3dsp_register( D3DSPR_TEMP, reg->regnum );
    }
}

static DWORD map_ps_input(struct bc_writer *This,
                          const struct shader_reg *reg) {
    DWORD i;
    /* Map color interpolators */
    for(i = 0; i < 2; i++) {
        if(reg->regnum == This->v_regnum[i]) {
            return d3dsp_register( D3DSPR_INPUT, i );
        }
    }
    for(i = 0; i < 8; i++) {
        if(reg->regnum == This->t_regnum[i]) {
            return d3dsp_register( D3DSPR_TEXTURE, i );
        }
    }

    WARN("Invalid ps 1/2 varying\n");
    This->state = E_INVALIDARG;
    return 0;
}

static void ps_1_0123_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                             struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

            /* Take care about the texture temporaries. There's a problem: They aren't
             * declared anywhere, so we can only hardcode the values that are used
             * to map ps_1_3 shaders to the common shader structure
             */
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(This, reg);
            break;

        case BWRITERSPR_CONST: /* Can be mapped 1:1 */
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

        default:
            WARN("Invalid register type for <= ps_1_3 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if(reg->srcmod == BWRITERSPSM_DZ || reg->srcmod == BWRITERSPSM_DW ||
       reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG ||
       reg->srcmod == BWRITERSPSM_NOT) {
        WARN("Invalid source modifier %u for <= ps_1_3\n", reg->srcmod);
        This->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_1_0123_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                             struct bytecode_buffer *buffer,
                             DWORD shift, DWORD mod) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(This, reg);
            break;

        /* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

/* The length of an instruction consists of the destination register (if any),
 * the number of source registers, the number of address registers used for
 * indirect addressing, and optionally the predicate register
 */
static DWORD instrlen(const struct instruction *instr, unsigned int srcs, unsigned int dsts) {
    unsigned int i;
    DWORD ret = srcs + dsts + (instr->has_predicate ? 1 : 0);

    if(dsts){
        if(instr->dst.rel_reg) ret++;
    }
    for(i = 0; i < srcs; i++) {
        if(instr->src[i].rel_reg) ret++;
    }
    return ret;
}

static void sm_1_x_opcode(struct bc_writer *This,
                          const struct instruction *instr,
                          DWORD token, struct bytecode_buffer *buffer) {
    /* In sm_1_x instruction length isn't encoded */
    if(instr->coissue){
        token |= D3DSI_COISSUE;
    }
    put_dword(buffer, token);
}

static void instr_handler(struct bc_writer *This,
                          const struct instruction *instr,
                          struct bytecode_buffer *buffer) {
    DWORD token = d3d9_opcode(instr->opcode);

    This->funcs->opcode(This, instr, token, buffer);
    if(instr->has_dst) This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    write_srcregs(This, instr, buffer);
}

static const struct instr_handler_table vs_1_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},

    {BWRITERSIO_END,            NULL}, /* Sentinel value, it signals
                                          the end of the list */
};

static const struct bytecode_backend vs_1_x_backend = {
    vs_1_x_header,
    end,
    vs_1_x_srcreg,
    vs_12_dstreg,
    sm_1_x_opcode,
    vs_1_x_handlers
};

static void instr_ps_1_0123_texld(struct bc_writer *This,
                                  const struct instruction *instr,
                                  struct bytecode_buffer *buffer) {
    DWORD idx;
    struct shader_reg reg;
    DWORD swizzlemask;

    if(instr->src[1].type != BWRITERSPR_SAMPLER ||
       instr->src[1].regnum > 3) {
        WARN("Unsupported sampler type %u regnum %u\n",
             instr->src[1].type, instr->src[1].regnum);
        This->state = E_INVALIDARG;
        return;
    } else if(instr->dst.type != BWRITERSPR_TEMP) {
        WARN("Can only sample into a temp register\n");
        This->state = E_INVALIDARG;
        return;
    }

    idx = instr->src[1].regnum;
    if((idx == 0 && instr->dst.regnum != T0_REG) ||
       (idx == 1 && instr->dst.regnum != T1_REG) ||
       (idx == 2 && instr->dst.regnum != T2_REG) ||
       (idx == 3 && instr->dst.regnum != T3_REG)) {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_x\n",
             idx, instr->dst.regnum);
        This->state = E_INVALIDARG;
        return;
    }
    if(instr->src[0].type == BWRITERSPR_INPUT) {
        /* A simple non-dependent read tex instruction */
        if(instr->src[0].regnum != This->t_regnum[idx]) {
            WARN("Cannot sample from s%u with texture address data from interpolator %u\n",
                 idx, instr->src[0].regnum);
            This->state = E_INVALIDARG;
            return;
        }
        This->funcs->opcode(This, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);

        /* map the temp dstreg to the ps_1_3 texture temporary register */
        This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    } else if(instr->src[0].type == BWRITERSPR_TEMP) {

        swizzlemask = (3 << BWRITERVS_SWIZZLE_SHIFT) |
            (3 << (BWRITERVS_SWIZZLE_SHIFT + 2)) |
            (3 << (BWRITERVS_SWIZZLE_SHIFT + 4));
        if((instr->src[0].u.swizzle & swizzlemask) == (BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z)) {
            TRACE("writing texreg2rgb\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2RGB & D3DSI_OPCODE_MASK, buffer);
        } else if(instr->src[0].u.swizzle == (BWRITERVS_X_W | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X)) {
            TRACE("writing texreg2ar\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2AR & D3DSI_OPCODE_MASK, buffer);
        } else if(instr->src[0].u.swizzle == (BWRITERVS_X_Y | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z)) {
            TRACE("writing texreg2gb\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2GB & D3DSI_OPCODE_MASK, buffer);
        } else {
            WARN("Unsupported src addr swizzle in dependent texld: 0x%08x\n", instr->src[0].u.swizzle);
            This->state = E_INVALIDARG;
            return;
        }

        /* Dst and src reg can be mapped normally. Both registers are temporary registers in the
         * source shader and have to be mapped to the temporary form of the texture registers. However,
         * the src reg doesn't have a swizzle
         */
        This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
        reg = instr->src[0];
        reg.u.swizzle = BWRITERVS_NOSWIZZLE;
        This->funcs->srcreg(This, &reg, buffer);
    } else {
        WARN("Invalid address data source register\n");
        This->state = E_INVALIDARG;
        return;
    }
}

static void instr_ps_1_0123_mov(struct bc_writer *This,
                                const struct instruction *instr,
                                struct bytecode_buffer *buffer) {
    DWORD token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if(instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT) {
        if((instr->dst.regnum == T0_REG && instr->src[0].regnum == This->t_regnum[0]) ||
           (instr->dst.regnum == T1_REG && instr->src[0].regnum == This->t_regnum[1]) ||
           (instr->dst.regnum == T2_REG && instr->src[0].regnum == This->t_regnum[2]) ||
           (instr->dst.regnum == T3_REG && instr->src[0].regnum == This->t_regnum[3])) {
            if(instr->dstmod & BWRITERSPDM_SATURATE) {
                This->funcs->opcode(This, instr, D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK, buffer);
                /* Remove the SATURATE flag, it's implicit to the instruction */
                This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod & (~BWRITERSPDM_SATURATE));
                return;
            } else {
                WARN("A varying -> temp copy is only supported with the SATURATE modifier in <=ps_1_3\n");
                This->state = E_INVALIDARG;
                return;
            }
        } else if(instr->src[0].regnum == This->v_regnum[0] ||
                  instr->src[0].regnum == This->v_regnum[1]) {
            /* Handled by the normal mov below. Just drop out of the if condition */
        } else {
            WARN("Unsupported varying -> temp mov in <= ps_1_3\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    This->funcs->opcode(This, instr, token, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static const struct instr_handler_table ps_1_0123_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_ps_1_0123_mov},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},

    /* pshader instructions */
    {BWRITERSIO_CND,            instr_handler},
    {BWRITERSIO_CMP,            instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_TEX,            instr_ps_1_0123_texld},
    {BWRITERSIO_TEXBEM,         instr_handler},
    {BWRITERSIO_TEXBEML,        instr_handler},
    {BWRITERSIO_TEXM3x2PAD,     instr_handler},
    {BWRITERSIO_TEXM3x3PAD,     instr_handler},
    {BWRITERSIO_TEXM3x3SPEC,    instr_handler},
    {BWRITERSIO_TEXM3x3VSPEC,   instr_handler},
    {BWRITERSIO_TEXM3x3TEX,     instr_handler},
    {BWRITERSIO_TEXM3x3,        instr_handler},
    {BWRITERSIO_TEXM3x2DEPTH,   instr_handler},
    {BWRITERSIO_TEXM3x2TEX,     instr_handler},
    {BWRITERSIO_TEXDP3,         instr_handler},
    {BWRITERSIO_TEXDP3TEX,      instr_handler},
    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_1_0123_backend = {
    ps_1_x_header,
    end,
    ps_1_0123_srcreg,
    ps_1_0123_dstreg,
    sm_1_x_opcode,
    ps_1_0123_handlers
};

static void ps_1_4_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        /* Can be mapped 1:1 */
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

        default:
            WARN("Invalid register type for ps_1_4 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if(reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG ||
       reg->srcmod == BWRITERSPSM_NOT) {
        WARN("Invalid source modifier %u for ps_1_4\n", reg->srcmod);
        This->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_1_4_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer,
                          DWORD shift, DWORD mod) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

	/* For texkill */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

static void instr_ps_1_4_mov(struct bc_writer *This,
                             const struct instruction *instr,
                             struct bytecode_buffer *buffer) {
    DWORD token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if(instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT) {
        if(instr->src[0].regnum == This->t_regnum[0] ||
           instr->src[0].regnum == This->t_regnum[1] ||
           instr->src[0].regnum == This->t_regnum[2] ||
           instr->src[0].regnum == This->t_regnum[3] ||
           instr->src[0].regnum == This->t_regnum[4] ||
           instr->src[0].regnum == This->t_regnum[5]) {
            /* Similar to a regular mov, but a different opcode */
            token = D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK;
        } else if(instr->src[0].regnum == This->v_regnum[0] ||
                  instr->src[0].regnum == This->v_regnum[1]) {
            /* Handled by the normal mov below. Just drop out of the if condition */
        } else {
            WARN("Unsupported varying -> temp mov in ps_1_4\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    This->funcs->opcode(This, instr, token, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static void instr_ps_1_4_texld(struct bc_writer *This,
                               const struct instruction *instr,
                               struct bytecode_buffer *buffer) {
    if(instr->src[1].type != BWRITERSPR_SAMPLER ||
       instr->src[1].regnum > 5) {
        WARN("Unsupported sampler type %u regnum %u\n",
             instr->src[1].type, instr->src[1].regnum);
        This->state = E_INVALIDARG;
        return;
    } else if(instr->dst.type != BWRITERSPR_TEMP) {
        WARN("Can only sample into a temp register\n");
        This->state = E_INVALIDARG;
        return;
    }

    if(instr->src[1].regnum != instr->dst.regnum) {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_4\n",
             instr->src[1].regnum, instr->dst.regnum);
        This->state = E_INVALIDARG;
        return;
    }

    This->funcs->opcode(This, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static const struct instr_handler_table ps_1_4_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_ps_1_4_mov},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},

    /* pshader instructions */
    {BWRITERSIO_CND,            instr_handler},
    {BWRITERSIO_CMP,            instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_TEX,            instr_ps_1_4_texld},
    {BWRITERSIO_TEXDEPTH,       instr_handler},
    {BWRITERSIO_BEM,            instr_handler},

    {BWRITERSIO_PHASE,          instr_handler},
    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_1_4_backend = {
    ps_1_4_header,
    end,
    ps_1_4_srcreg,
    ps_1_4_dstreg,
    sm_1_x_opcode,
    ps_1_4_handlers
};

static void write_constB(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constB, shader->num_cb, D3DSIO_DEFB, D3DSPR_CONSTBOOL, buffer, len);
}

static void write_constI(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constI, shader->num_ci, D3DSIO_DEFI, D3DSPR_CONSTINT, buffer, len);
}

static void vs_2_header(struct bc_writer *This,
                        const struct bwriter_shader *shader,
                        struct bytecode_buffer *buffer) {
    HRESULT hr;

    hr = vs_find_builtin_varyings(This, shader);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
}

static void vs_2_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD has_swizzle;
    DWORD component;
    DWORD d3d9reg;

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected
               by map_vs_output
             */
            switch(reg->u.swizzle) {
                case BWRITERVS_SWIZZLE_X:
                    component = BWRITERSP_WRITEMASK_0;
                    break;
                case BWRITERVS_SWIZZLE_Y:
                    component = BWRITERSP_WRITEMASK_1;
                    break;
                case BWRITERVS_SWIZZLE_Z:
                    component = BWRITERSP_WRITEMASK_2;
                    break;
                case BWRITERVS_SWIZZLE_W:
                    component = BWRITERSP_WRITEMASK_3;
                    break;
                default:
                    component = 0;
            }
            token |= map_vs_output(This, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
             * but are unexpected. If we hit this path it might be due to an error.
             */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
        case BWRITERSPR_CONSTINT:
        case BWRITERSPR_CONSTBOOL:
        case BWRITERSPR_LABEL:
            d3d9reg = d3d9_register(reg->type);
            token |= d3dsp_register( d3d9reg, reg->regnum );
            break;

        case BWRITERSPR_LOOP:
            if(reg->regnum != 0) {
                WARN("Only regnum 0 is supported for the loop index register in vs_2_0\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_LOOP, 0 );
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERVS_VERSION(2, 1)){
                WARN("Predicate register is allowed only in vs_2_x\n");
                This->state = E_INVALIDARG;
                return;
            }
            if(reg->regnum > 0) {
                WARN("Only predicate register 0 is supported\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_PREDICATE, 0 );
            break;

        default:
            WARN("Invalid register type for 2.0 vshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);

    if(reg->rel_reg)
        token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;

    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE)
        vs_2_srcreg(This, reg->rel_reg, buffer);
}

static void sm_2_opcode(struct bc_writer *This,
                        const struct instruction *instr,
                        DWORD token, struct bytecode_buffer *buffer) {
    /* From sm 2 onwards instruction length is encoded in the opcode field */
    int dsts = instr->has_dst ? 1 : 0;
    token |= instrlen(instr, instr->num_srcs, dsts) << D3DSI_INSTLENGTH_SHIFT;
    if(instr->comptype)
        token |= (d3d9_comparetype(instr->comptype) << 16) & (0xf << 16);
    if(instr->has_predicate)
        token |= D3DSHADER_INSTRUCTION_PREDICATED;
    put_dword(buffer,token);
}

static const struct instr_handler_table vs_2_0_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_2_0_backend = {
    vs_2_header,
    end,
    vs_2_srcreg,
    vs_12_dstreg,
    sm_2_opcode,
    vs_2_0_handlers
};

static const struct instr_handler_table vs_2_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_2_x_backend = {
    vs_2_header,
    end,
    vs_2_srcreg,
    vs_12_dstreg,
    sm_2_opcode,
    vs_2_x_handlers
};

static void write_samplers(const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    DWORD i;
    DWORD instr_dcl = D3DSIO_DCL | (2 << D3DSI_INSTLENGTH_SHIFT);
    DWORD token;
    const DWORD reg = (1u << 31) | d3dsp_register( D3DSPR_SAMPLER, 0 ) | D3DSP_WRITEMASK_ALL;

    for(i = 0; i < shader->num_samplers; i++) {
        /* Write the DCL instruction */
        put_dword(buffer, instr_dcl);
        token = (1u << 31);
        /* Already shifted */
        token |= (d3d9_sampler(shader->samplers[i].type)) & D3DSP_TEXTURETYPE_MASK;
        put_dword(buffer, token);
        token = reg | (shader->samplers[i].regnum & D3DSP_REGNUM_MASK);
        token |= d3d9_dstmod(shader->samplers[i].mod);
        put_dword(buffer, token);
    }
}

static void ps_2_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr = find_ps_builtin_semantics(This, shader, 8);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_samplers(shader, buffer);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
}

static void ps_2_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

            /* Can be mapped 1:1 */
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_COLOROUT:
        case BWRITERSPR_CONSTBOOL:
        case BWRITERSPR_CONSTINT:
        case BWRITERSPR_SAMPLER:
        case BWRITERSPR_LABEL:
        case BWRITERSPR_DEPTHOUT:
            d3d9reg = d3d9_register(reg->type);
            token |= d3dsp_register( d3d9reg, reg->regnum );
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERPS_VERSION(2, 1)){
                WARN("Predicate register not supported in ps_2_0\n");
                This->state = E_INVALIDARG;
            }
            if(reg->regnum) {
                WARN("Predicate register with regnum %u not supported\n",
                     reg->regnum);
                This->state = E_INVALIDARG;
            }
            token |= d3dsp_register( D3DSPR_PREDICATE, 0 );
            break;

        default:
            WARN("Invalid register type for ps_2_0 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_2_0_dstreg(struct bc_writer *This,
                          const struct shader_reg *reg,
                          struct bytecode_buffer *buffer,
                          DWORD shift, DWORD mod) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
        case BWRITERSPR_COLOROUT:
        case BWRITERSPR_DEPTHOUT:
            d3d9reg = d3d9_register(reg->type);
            token |= d3dsp_register( d3d9reg, reg->regnum );
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERPS_VERSION(2, 1)){
                WARN("Predicate register not supported in ps_2_0\n");
                This->state = E_INVALIDARG;
            }
            token |= d3dsp_register( D3DSPR_PREDICATE, reg->regnum );
            break;

	/* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 2.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

static const struct instr_handler_table ps_2_0_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_2_0_backend = {
    ps_2_header,
    end,
    ps_2_srcreg,
    ps_2_0_dstreg,
    sm_2_opcode,
    ps_2_0_handlers
};

static const struct instr_handler_table ps_2_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_RET,            instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_DSX,            instr_handler},
    {BWRITERSIO_DSY,            instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},

    {BWRITERSIO_TEXLDD,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_2_x_backend = {
    ps_2_header,
    end,
    ps_2_srcreg,
    ps_2_0_dstreg,
    sm_2_opcode,
    ps_2_x_handlers
};

static void sm_3_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_declarations(This, buffer, TRUE, shader->outputs, shader->num_outputs, BWRITERSPR_OUTPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
    write_samplers(shader, buffer);
}

static void sm_3_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    d3d9reg = d3d9_register(reg->type);
    token |= d3dsp_register( d3d9reg, reg->regnum );
    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK;
    token |= d3d9_srcmod(reg->srcmod);

    if(reg->rel_reg) {
        if(reg->type == BWRITERSPR_CONST && This->version == BWRITERPS_VERSION(3, 0)) {
            WARN("c%u[...] is unsupported in ps_3_0\n", reg->regnum);
            This->state = E_INVALIDARG;
            return;
        }
        if(((reg->rel_reg->type == BWRITERSPR_ADDR && This->version == BWRITERVS_VERSION(3, 0)) ||
           reg->rel_reg->type == BWRITERSPR_LOOP) &&
           reg->rel_reg->regnum == 0) {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        } else {
            WARN("Unsupported relative addressing register\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE) {
        sm_3_srcreg(This, reg->rel_reg, buffer);
    }
}

static void sm_3_dstreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer,
                        DWORD shift, DWORD mod) {
    DWORD token = (1u << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    if(reg->rel_reg) {
        if(This->version == BWRITERVS_VERSION(3, 0) &&
           reg->type == BWRITERSPR_OUTPUT) {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        } else {
            WARN("Relative addressing not supported for this shader type or register type\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    d3d9reg = d3d9_register(reg->type);
    token |= d3dsp_register( d3d9reg, reg->regnum );
    token |= d3d9_dstmod(mod);
    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE) {
        sm_3_srcreg(This, reg->rel_reg, buffer);
    }
}

static const struct instr_handler_table vs_3_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},
    {BWRITERSIO_TEXLDL,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_3_backend = {
    sm_3_header,
    end,
    sm_3_srcreg,
    sm_3_dstreg,
    sm_2_opcode,
    vs_3_handlers
};

static const struct instr_handler_table ps_3_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},
    {BWRITERSIO_TEXLDL,         instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_DSX,            instr_handler},
    {BWRITERSIO_DSY,            instr_handler},
    {BWRITERSIO_TEXLDD,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_3_backend = {
    sm_3_header,
    end,
    sm_3_srcreg,
    sm_3_dstreg,
    sm_2_opcode,
    ps_3_handlers
};

static void init_vs10_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 1.0 writer\n");
    writer->funcs = &vs_1_x_backend;
}

static void init_vs11_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 1.1 writer\n");
    writer->funcs = &vs_1_x_backend;
}

static void init_vs20_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 2.0 writer\n");
    writer->funcs = &vs_2_0_backend;
}

static void init_vs2x_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 2.x writer\n");
    writer->funcs = &vs_2_x_backend;
}

static void init_vs30_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 3.0 writer\n");
    writer->funcs = &vs_3_backend;
}

static void init_ps10_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.0 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps11_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.1 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps12_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.2 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps13_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.3 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps14_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.4 writer\n");
    writer->funcs = &ps_1_4_backend;
}

static void init_ps20_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 2.0 writer\n");
    writer->funcs = &ps_2_0_backend;
}

static void init_ps2x_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 2.x writer\n");
    writer->funcs = &ps_2_x_backend;
}

static void init_ps30_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 3.0 writer\n");
    writer->funcs = &ps_3_backend;
}

static struct bc_writer *create_writer(DWORD version, DWORD dxversion) {
    struct bc_writer *ret = d3dcompiler_alloc(sizeof(*ret));

    if(!ret) {
        WARN("Failed to allocate a bytecode writer instance\n");
        return NULL;
    }

    switch(version) {
        case BWRITERVS_VERSION(1, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 1.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs10_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs11_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs20_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            init_vs2x_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(3, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 3.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs30_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(1, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps10_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps11_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 2):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.2 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps12_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 3):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.3 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps13_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 4):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.4 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps14_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps20_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            init_ps2x_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(3, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 3.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps30_dx9_writer(ret);
            break;

        default:
            WARN("Unexpected shader version requested: %08x\n", version);
            goto fail;
    }
    ret->version = version;
    return ret;

fail:
    d3dcompiler_free(ret);
    return NULL;
}

static HRESULT call_instr_handler(struct bc_writer *writer,
                                  const struct instruction *instr,
                                  struct bytecode_buffer *buffer) {
    DWORD i=0;

    while(writer->funcs->instructions[i].opcode != BWRITERSIO_END) {
        if(instr->opcode == writer->funcs->instructions[i].opcode) {
            if(!writer->funcs->instructions[i].func) {
                WARN("Opcode %u not supported by this profile\n", instr->opcode);
                return E_INVALIDARG;
            }
            writer->funcs->instructions[i].func(writer, instr, buffer);
            return S_OK;
        }
        i++;
    }

    FIXME("Unhandled instruction %u - %s\n", instr->opcode,
          debug_print_opcode(instr->opcode));
    return E_INVALIDARG;
}

HRESULT SlWriteBytecode(const struct bwriter_shader *shader, int dxversion, DWORD **result, DWORD *size)
{
    struct bc_writer *writer;
    struct bytecode_buffer *buffer = NULL;
    HRESULT hr;
    unsigned int i;

    if(!shader){
        ERR("NULL shader structure, aborting\n");
        return E_FAIL;
    }
    writer = create_writer(shader->version, dxversion);
    *result = NULL;

    if(!writer) {
        WARN("Could not create a bytecode writer instance. Either unsupported version\n");
        WARN("or out of memory\n");
        hr = E_FAIL;
        goto error;
    }

    buffer = allocate_buffer();
    if(!buffer) {
        WARN("Failed to allocate a buffer for the shader bytecode\n");
        hr = E_FAIL;
        goto error;
    }

    /* Write shader type and version */
    put_dword(buffer, shader->version);

    writer->funcs->header(writer, shader, buffer);
    if(FAILED(writer->state)) {
        hr = writer->state;
        goto error;
    }

    for(i = 0; i < shader->num_instrs; i++) {
        hr = call_instr_handler(writer, shader->instr[i], buffer);
        if(FAILED(hr)) {
            goto error;
        }
    }

    if(FAILED(writer->state)) {
        hr = writer->state;
        goto error;
    }

    writer->funcs->end(writer, shader, buffer);

    if(FAILED(buffer->state)) {
        hr = buffer->state;
        goto error;
    }

    *size = buffer->size * sizeof(DWORD);
    *result = buffer->data;
    buffer->data = NULL;
    hr = S_OK;

error:
    if(buffer) {
        d3dcompiler_free(buffer->data);
        d3dcompiler_free(buffer);
    }
    d3dcompiler_free(writer);
    return hr;
}

void SlDeleteShader(struct bwriter_shader *shader) {
    unsigned int i, j;

    TRACE("Deleting shader %p\n", shader);

    for(i = 0; i < shader->num_cf; i++) {
        d3dcompiler_free(shader->constF[i]);
    }
    d3dcompiler_free(shader->constF);
    for(i = 0; i < shader->num_ci; i++) {
        d3dcompiler_free(shader->constI[i]);
    }
    d3dcompiler_free(shader->constI);
    for(i = 0; i < shader->num_cb; i++) {
        d3dcompiler_free(shader->constB[i]);
    }
    d3dcompiler_free(shader->constB);

    d3dcompiler_free(shader->inputs);
    d3dcompiler_free(shader->outputs);
    d3dcompiler_free(shader->samplers);

    for(i = 0; i < shader->num_instrs; i++) {
        for(j = 0; j < shader->instr[i]->num_srcs; j++) {
            d3dcompiler_free(shader->instr[i]->src[j].rel_reg);
        }
        d3dcompiler_free(shader->instr[i]->src);
        d3dcompiler_free(shader->instr[i]->dst.rel_reg);
        d3dcompiler_free(shader->instr[i]);
    }
    d3dcompiler_free(shader->instr);

    d3dcompiler_free(shader);
}
