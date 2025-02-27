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

#include "wine/debug.h"

#include "d3d9types.h"
#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bytecodewriter);

static BOOL array_reserve(void **elements, unsigned int *capacity, unsigned int count, unsigned int size)
{
    unsigned int max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~0u / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(8, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
    {
        ERR("Failed to allocate memory.\n");
        return FALSE;
    }

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

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
    struct instruction *ret = calloc(1, sizeof(*ret));
    if(!ret) {
        ERR("Failed to allocate memory for an instruction structure\n");
        return NULL;
    }

    if(srcs) {
        ret->src = calloc(1, srcs * sizeof(*ret->src));
        if(!ret->src) {
            ERR("Failed to allocate memory for instruction registers\n");
            free(ret);
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
    if(!shader) return FALSE;

    if (!array_reserve((void **)&shader->instr, &shader->instr_alloc_size,
            shader->num_instrs + 1, sizeof(*shader->instr)))
        return FALSE;

    shader->instr[shader->num_instrs] = instr;
    shader->num_instrs++;
    return TRUE;
}

BOOL add_constF(struct bwriter_shader *shader, uint32_t reg, float x, float y, float z, float w)
{
    struct constant *newconst;

    if (shader->num_cf)
    {
        struct constant **newarray;
        newarray = realloc(shader->constF, sizeof(*shader->constF) * (shader->num_cf + 1));
        if (!newarray)
        {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constF = newarray;
    }
    else
    {
        shader->constF = calloc(1, sizeof(*shader->constF));
        if (!shader->constF)
        {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = calloc(1, sizeof(*newconst));
    if (!newconst)
    {
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

BOOL add_constI(struct bwriter_shader *shader, uint32_t reg, int x, int y, int z, int w)
{
    struct constant *newconst;

    if (shader->num_ci)
    {
        struct constant **newarray;
        newarray = realloc(shader->constI, sizeof(*shader->constI) * (shader->num_ci + 1));
        if (!newarray)
        {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constI = newarray;
    }
    else
    {
        shader->constI = calloc(1, sizeof(*shader->constI));
        if (!shader->constI)
        {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = calloc(1, sizeof(*newconst));
    if (!newconst)
    {
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

BOOL add_constB(struct bwriter_shader *shader, uint32_t reg, BOOL x)
{
    struct constant *newconst;

    if (shader->num_cb)
    {
        struct constant **newarray;
        newarray = realloc(shader->constB, sizeof(*shader->constB) * (shader->num_cb + 1));
        if (!newarray)
        {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constB = newarray;
    }
    else
    {
        shader->constB = calloc(1, sizeof(*shader->constB));
        if (!shader->constB)
        {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = calloc(1, sizeof(*newconst));
    if (!newconst)
    {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].b = x;
    shader->constB[shader->num_cb] = newconst;

    shader->num_cb++;
    return TRUE;
}

BOOL record_declaration(struct bwriter_shader *shader, uint32_t usage, uint32_t usage_idx,
        uint32_t mod, BOOL output, uint32_t regnum, uint32_t writemask, BOOL builtin)
{
    struct declaration **decl;
    unsigned int i, *num;

    if (!shader)
        return FALSE;

    if (output)
    {
        num = &shader->num_outputs;
        decl = &shader->outputs;
    }
    else
    {
        num = &shader->num_inputs;
        decl = &shader->inputs;
    }

    if (*num == 0)
    {
        *decl = calloc(1, sizeof(**decl));
        if (!*decl)
        {
            ERR("Error allocating declarations array\n");
            return FALSE;
        }
    }
    else
    {
        struct declaration *newdecl;

        for (i = 0; i < *num; i++)
        {
            if ((*decl)[i].regnum == regnum && ((*decl)[i].writemask & writemask))
                WARN("Declaration of register %u already exists, writemask match 0x%x\n",
                      regnum, (*decl)[i].writemask & writemask);
        }

        newdecl = realloc(*decl, sizeof(**decl) * ((*num) + 1));
        if (!newdecl)
        {
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

BOOL record_sampler(struct bwriter_shader *shader, uint32_t samptype, uint32_t mod, uint32_t regnum) {
    unsigned int i;

    if (!shader)
        return FALSE;

    if (shader->num_samplers == 0)
    {
        shader->samplers = calloc(1, sizeof(*shader->samplers));
        if (!shader->samplers)
        {
            ERR("Error allocating samplers array\n");
            return FALSE;
        }
    }
    else
    {
        struct samplerdecl *newarray;

        for (i = 0; i < shader->num_samplers; i++)
        {
            if (shader->samplers[i].regnum == regnum)
            {
                WARN("Sampler %u already declared\n", regnum);
                /* This is not an error as far as the assembler is concerned.
                 * Direct3D might refuse to load the compiled shader though */
            }
        }

        newarray = realloc(shader->samplers, sizeof(*shader->samplers) * (shader->num_samplers + 1));
        if (!newarray)
        {
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

struct bytecode_buffer
{
    uint32_t *data;
    unsigned int size, alloc_size;
    HRESULT state;
};

struct bc_writer;

typedef void (*instr_writer)(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer);

struct bytecode_backend
{
    void (*header)(struct bc_writer *writer, const struct bwriter_shader *shader,
            struct bytecode_buffer *buffer);
    void (*end)(struct bc_writer *writer, const struct bwriter_shader *shader,
            struct bytecode_buffer *buffer);
    void (*srcreg)(struct bc_writer *writer, const struct shader_reg *reg,
            struct bytecode_buffer *buffer);
    void (*dstreg)(struct bc_writer *writer, const struct shader_reg *reg,
            struct bytecode_buffer *buffer, uint32_t shift, uint32_t mod);
    void (*opcode)(struct bc_writer *writer, const struct instruction *instr,
            uint32_t token, struct bytecode_buffer *buffer);

    const struct instr_handler_table
    {
        uint32_t opcode;
        instr_writer func;
    } *instructions;
};

struct bc_writer
{
    const struct bytecode_backend *funcs;
    const struct bwriter_shader *shader;

    HRESULT state;

    /* Vertex shader varying mapping. */
    uint32_t oPos_regnum, oD_regnum[2], oT_regnum[8], oFog_regnum, oFog_mask, oPts_regnum, oPts_mask;

    /* Pixel shader varying mapping. */
    uint32_t t_regnum[8], v_regnum[2];
};


/* shader bytecode buffer manipulation functions.
 * allocate_buffer creates a new buffer structure, put_u32 adds a new
 * uint32_t to the buffer. In the rare case of a memory allocation failure
 * when trying to grow the buffer a flag is set in the buffer to mark it
 * invalid. This avoids return value checking and passing in many places
 */
static struct bytecode_buffer *allocate_buffer(void) {
    struct bytecode_buffer *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret) return NULL;
    ret->state = S_OK;
    return ret;
}

static void put_u32(struct bytecode_buffer *buffer, uint32_t value)
{
    if (FAILED(buffer->state))
        return;

    if (!array_reserve((void **)&buffer->data, &buffer->alloc_size, buffer->size + 1, sizeof(*buffer->data)))
    {
        buffer->state = E_OUTOFMEMORY;
        return;
    }

    buffer->data[buffer->size++] = value;
}

/* bwriter -> d3d9 conversion functions. */

static uint32_t sm1_version(const struct bwriter_shader *shader)
{
    switch (shader->type)
    {
    case ST_VERTEX:
        return D3DVS_VERSION(shader->major_version, shader->minor_version);
    case ST_PIXEL:
        return D3DPS_VERSION(shader->major_version, shader->minor_version);
    default:
        ERR("Invalid shader type %#x.\n", shader->type);
        return 0;
    }
}

static uint32_t d3d9_swizzle(uint32_t bwriter_swizzle)
{
    uint32_t ret = 0;

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

static uint32_t d3d9_writemask(uint32_t bwriter_writemask)
{
    uint32_t ret = 0;

    if (bwriter_writemask & BWRITERSP_WRITEMASK_0) ret |= D3DSP_WRITEMASK_0;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_1) ret |= D3DSP_WRITEMASK_1;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_2) ret |= D3DSP_WRITEMASK_2;
    if (bwriter_writemask & BWRITERSP_WRITEMASK_3) ret |= D3DSP_WRITEMASK_3;

    return ret;
}

static uint32_t d3d9_srcmod(uint32_t bwriter_srcmod)
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

static uint32_t d3d9_dstmod(uint32_t bwriter_mod)
{
    uint32_t ret = 0;

    if (bwriter_mod & BWRITERSPDM_SATURATE)         ret |= D3DSPDM_SATURATE;
    if (bwriter_mod & BWRITERSPDM_PARTIALPRECISION) ret |= D3DSPDM_PARTIALPRECISION;
    if (bwriter_mod & BWRITERSPDM_MSAMPCENTROID)    ret |= D3DSPDM_MSAMPCENTROID;

    return ret;
}

static uint32_t d3d9_comparetype(uint32_t asmshader_comparetype)
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

static uint32_t d3d9_sampler(uint32_t bwriter_sampler)
{
    if (bwriter_sampler == BWRITERSTT_UNKNOWN)  return D3DSTT_UNKNOWN;
    if (bwriter_sampler == BWRITERSTT_1D)       return D3DSTT_1D;
    if (bwriter_sampler == BWRITERSTT_2D)       return D3DSTT_2D;
    if (bwriter_sampler == BWRITERSTT_CUBE)     return D3DSTT_CUBE;
    if (bwriter_sampler == BWRITERSTT_VOLUME)   return D3DSTT_VOLUME;
    FIXME("Unexpected BWRITERSAMPLER_TEXTURE_TYPE type %#x.\n", bwriter_sampler);

    return 0;
}

static uint32_t d3d9_register(uint32_t bwriter_register)
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

static uint32_t d3d9_opcode(uint32_t bwriter_opcode)
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

static uint32_t d3dsp_register(D3DSHADER_PARAM_REGISTER_TYPE type, uint32_t num)
{
    return ((type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) |
           ((type << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2) |
           (num & D3DSP_REGNUM_MASK); /* No shift */
}

/******************************************************
 * Implementation of the writer functions starts here *
 ******************************************************/
static void write_declarations(struct bc_writer *writer, struct bytecode_buffer *buffer,
        const struct declaration *decls, unsigned int num, uint32_t type)
{
    uint32_t instr_dcl = D3DSIO_DCL;
    uint32_t token;
    unsigned int i;
    struct shader_reg reg;

    ZeroMemory(&reg, sizeof(reg));

    if (writer->shader->major_version > 1)
        instr_dcl |= 2 << D3DSI_INSTLENGTH_SHIFT;

    for(i = 0; i < num; i++) {
        if(decls[i].builtin) continue;

        /* Write the DCL instruction */
        put_u32(buffer, instr_dcl);

        /* Write the usage and index */
        token = (1u << 31); /* Bit 31 of non-instruction opcodes is 1 */
        token |= (decls[i].usage << D3DSP_DCL_USAGE_SHIFT) & D3DSP_DCL_USAGE_MASK;
        token |= (decls[i].usage_idx << D3DSP_DCL_USAGEINDEX_SHIFT) & D3DSP_DCL_USAGEINDEX_MASK;
        put_u32(buffer, token);

        /* Write the dest register */
        reg.type = type;
        reg.regnum = decls[i].regnum;
        reg.writemask = decls[i].writemask;
        writer->funcs->dstreg(writer, &reg, buffer, 0, decls[i].mod);
    }
}

static void write_const(struct constant **consts, int num, uint32_t opcode, uint32_t reg_type, struct bytecode_buffer *buffer, BOOL len)
{
    const uint32_t reg = (1u << 31) | d3dsp_register( reg_type, 0 ) | D3DSP_WRITEMASK_ALL;
    uint32_t instr_def = opcode;
    unsigned int i;

    if(len) {
        if(opcode == D3DSIO_DEFB)
            instr_def |= 2 << D3DSI_INSTLENGTH_SHIFT;
        else
            instr_def |= 5 << D3DSI_INSTLENGTH_SHIFT;
    }

    for(i = 0; i < num; i++) {
        /* Write the DEF instruction */
        put_u32(buffer, instr_def);

        put_u32(buffer, reg | (consts[i]->regnum & D3DSP_REGNUM_MASK));
        put_u32(buffer, consts[i]->value[0].d);
        if(opcode != D3DSIO_DEFB) {
            put_u32(buffer, consts[i]->value[1].d);
            put_u32(buffer, consts[i]->value[2].d);
            put_u32(buffer, consts[i]->value[3].d);
        }
    }
}

static void write_constF(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constF, shader->num_cf, D3DSIO_DEF, D3DSPR_CONST, buffer, len);
}

/* This function looks for VS 1/2 registers mapping to VS 3 output registers */
static HRESULT vs_find_builtin_varyings(struct bc_writer *writer, const struct bwriter_shader *shader)
{
    uint32_t usage, usage_idx, writemask, regnum;
    unsigned int i;

    for (i = 0; i < shader->num_outputs; i++)
    {
        if (!shader->outputs[i].builtin)
            continue;

        usage = shader->outputs[i].usage;
        usage_idx = shader->outputs[i].usage_idx;
        writemask = shader->outputs[i].writemask;
        regnum = shader->outputs[i].regnum;

        switch (usage)
        {
            case BWRITERDECLUSAGE_POSITION:
            case BWRITERDECLUSAGE_POSITIONT:
                if (usage_idx > 0)
                {
                    WARN("dcl_position%u not supported in sm 1/2 shaders.\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u is oPos.\n", regnum);
                writer->oPos_regnum = regnum;
                break;

            case BWRITERDECLUSAGE_COLOR:
                if (usage_idx > 1)
                {
                    WARN("dcl_color%u not supported in sm 1/2 shaders.\n", usage_idx);
                    return E_INVALIDARG;
                }
                if (writemask != BWRITERSP_WRITEMASK_ALL)
                {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1/2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oD%u.\n", regnum, usage_idx);
                writer->oD_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if (usage_idx >= 8)
                {
                    WARN("dcl_color%u not supported in sm 1/2 shaders.\n", usage_idx);
                    return E_INVALIDARG;
                }
                if (writemask != (BWRITERSP_WRITEMASK_0)
                        && writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1)
                        && writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2)
                        && writemask != (BWRITERSP_WRITEMASK_ALL))
                {
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oT%u.\n", regnum, usage_idx);
                writer->oT_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_PSIZE:
                if (usage_idx > 0)
                {
                    WARN("dcl_psize%u not supported in sm 1/2 shaders.\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oPts.\n", regnum, writemask);
                writer->oPts_regnum = regnum;
                writer->oPts_mask = writemask;
                break;

            case BWRITERDECLUSAGE_FOG:
                if (usage_idx > 0)
                {
                    WARN("dcl_fog%u not supported in sm 1 shaders.\n", usage_idx);
                    return E_INVALIDARG;
                }
                if (writemask != BWRITERSP_WRITEMASK_0 && writemask != BWRITERSP_WRITEMASK_1
                        && writemask != BWRITERSP_WRITEMASK_2 && writemask != BWRITERSP_WRITEMASK_3)
                {
                    WARN("Unsupported fog writemask\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oFog.\n", regnum, writemask);
                writer->oFog_regnum = regnum;
                writer->oFog_mask = writemask;
                break;

            default:
                WARN("Varying type %u is not supported in shader model 1.x.\n", usage);
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

    write_declarations(This, buffer, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, FALSE);
}

static HRESULT find_ps_builtin_semantics(struct bc_writer *writer, const struct bwriter_shader *shader,
        uint32_t texcoords)
{
    uint32_t usage, usage_idx, writemask, regnum;
    unsigned int i;

    writer->v_regnum[0] = -1;
    writer->v_regnum[1] = -1;
    for (i = 0; i < 8; i++)
        writer->t_regnum[i] = -1;

    for (i = 0; i < shader->num_inputs; i++)
    {
        if (!shader->inputs[i].builtin)
            continue;

        usage = shader->inputs[i].usage;
        usage_idx = shader->inputs[i].usage_idx;
        writemask = shader->inputs[i].writemask;
        regnum = shader->inputs[i].regnum;

        switch (usage)
        {
            case BWRITERDECLUSAGE_COLOR:
                if (usage_idx > 1)
                {
                    WARN("dcl_color%u not supported in sm 1 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if (writemask != BWRITERSP_WRITEMASK_ALL)
                {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1\n");
                    return E_INVALIDARG;
                }
                TRACE("v%u is v%u\n", regnum, usage_idx);
                writer->v_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if (usage_idx > texcoords)
                {
                    WARN("dcl_texcoord%u not supported in this shader version\n", usage_idx);
                    return E_INVALIDARG;
                }
                if (writemask != (BWRITERSP_WRITEMASK_0)
                        && writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1)
                        && writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2)
                        && writemask != (BWRITERSP_WRITEMASK_ALL))
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                else
                    writemask = BWRITERSP_WRITEMASK_ALL;
                TRACE("v%u is t%u\n", regnum, usage_idx);
                writer->t_regnum[usage_idx] = regnum;
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

static void end(struct bc_writer *writer, const struct bwriter_shader *shader, struct bytecode_buffer *buffer)
{
    put_u32(buffer, D3DSIO_END);
}

static uint32_t map_vs_output(struct bc_writer *writer, uint32_t regnum, uint32_t mask, BOOL *has_components)
{
    unsigned int i;

    *has_components = TRUE;
    if (regnum == writer->oPos_regnum)
    {
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_POSITION );
    }
    if (regnum == writer->oFog_regnum && mask == writer->oFog_mask)
    {
        *has_components = FALSE;
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_FOG ) | D3DSP_WRITEMASK_ALL;
    }
    if (regnum == writer->oPts_regnum && mask == writer->oPts_mask)
    {
        *has_components = FALSE;
        return d3dsp_register( D3DSPR_RASTOUT, D3DSRO_POINT_SIZE ) | D3DSP_WRITEMASK_ALL;
    }
    for (i = 0; i < 2; i++)
    {
        if (regnum == writer->oD_regnum[i])
            return d3dsp_register( D3DSPR_ATTROUT, i );
    }
    for (i = 0; i < 8; i++)
    {
        if (regnum == writer->oT_regnum[i])
            return d3dsp_register( D3DSPR_TEXCRDOUT, i );
    }

    /* The varying must be undeclared - if an unsupported varying was declared,
     * the vs_find_builtin_varyings function would have caught it and this code
     * would not run */
    WARN("Undeclared varying %u.\n", regnum);
    writer->state = E_INVALIDARG;
    return -1;
}

static void vs_12_dstreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer,
        uint32_t shift, uint32_t mod)
{
    uint32_t token = (1u << 31); /* Bit 31 of registers is 1 */
    BOOL has_wmask;

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported for destination registers\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_OUTPUT:
            token |= map_vs_output(writer, reg->regnum, reg->writemask, &has_wmask);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
            * but are unexpected. If we hit this path it might be due to an error.
            */
            FIXME("Unexpected register type %u.\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= d3dsp_register( reg->type, reg->regnum );
            has_wmask = TRUE;
            break;

        case BWRITERSPR_ADDR:
            if (reg->regnum != 0)
            {
                WARN("Only a0 exists\n");
                writer->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_ADDR, 0 );
            has_wmask = TRUE;
            break;

        case BWRITERSPR_PREDICATE:
            if (writer->shader->major_version != 2 || writer->shader->minor_version != 1)
            {
                WARN("Predicate register is allowed only in vs_2_x\n");
                writer->state = E_INVALIDARG;
                return;
            }
            if (reg->regnum != 0)
            {
                WARN("Only predicate register p0 exists\n");
                writer->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register( D3DSPR_PREDICATE, 0 );
            has_wmask = TRUE;
            break;

        default:
            WARN("Invalid register type for 1.x-2.x vertex shader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    /* strictly speaking there are no modifiers in vs_2_0 and vs_1_x, but they can be written
     * into the bytecode and since the compiler doesn't do such checks write them
     * (the checks are done by the undocumented shader validator)
     */
    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    if (has_wmask)
        token |= d3d9_writemask(reg->writemask);
    put_u32(buffer, token);
}

static void vs_1_x_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    uint32_t token = (1u << 31); /* Bit 31 of registers is 1 */
    uint32_t component;
    BOOL has_swizzle;

    switch (reg->type)
    {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected by
             * map_vs_output */
            switch (reg->swizzle)
            {
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
            token |= map_vs_output(writer, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can
             * be encoded in the bytecode, but are unexpected. If we hit this
             * path it might be due to an error. */
            FIXME("Unexpected register type %u.\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
            token |= d3dsp_register(reg->type, reg->regnum);
            if (reg->rel_reg)
            {
                if (reg->rel_reg->type != BWRITERSPR_ADDR || reg->rel_reg->regnum != 0
                        || reg->rel_reg->swizzle != BWRITERVS_SWIZZLE_X)
                {
                    WARN("Relative addressing in vs_1_x is only allowed with a0.x\n");
                    writer->state = E_INVALIDARG;
                    return;
                }
                token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
            }
            break;

        default:
            WARN("Invalid register type for 1.x vshader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_u32(buffer, token);
}

static void write_srcregs(struct bc_writer *writer, const struct instruction *instr, struct bytecode_buffer *buffer)
{
    unsigned int i;

    if (instr->has_predicate)
        writer->funcs->srcreg(writer, &instr->predicate, buffer);

    for (i = 0; i < instr->num_srcs; ++i)
        writer->funcs->srcreg(writer, &instr->src[i], buffer);
}

static uint32_t map_ps13_temp(struct bc_writer *writer, const struct shader_reg *reg)
{
    if (reg->regnum == T0_REG)
        return d3dsp_register(D3DSPR_TEXTURE, 0);
    if (reg->regnum == T1_REG)
        return d3dsp_register(D3DSPR_TEXTURE, 1);
    if(reg->regnum == T2_REG)
        return d3dsp_register(D3DSPR_TEXTURE, 2);
    if (reg->regnum == T3_REG)
        return d3dsp_register(D3DSPR_TEXTURE, 3);
    return d3dsp_register(D3DSPR_TEMP, reg->regnum);
}

static uint32_t map_ps_input(struct bc_writer *writer, const struct shader_reg *reg)
{
    unsigned int i;

    /* Map color interpolators */
    for (i = 0; i < ARRAY_SIZE(writer->v_regnum); ++i)
    {
        if (reg->regnum == writer->v_regnum[i])
            return d3dsp_register(D3DSPR_INPUT, i);
    }
    for (i = 0; i < ARRAY_SIZE(writer->t_regnum); ++i)
    {
        if(reg->regnum == writer->t_regnum[i])
            return d3dsp_register(D3DSPR_TEXTURE, i);
    }

    WARN("Invalid ps 1/2 varying\n");
    writer->state = E_INVALIDARG;
    return 0;
}

static void ps_1_0123_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    uint32_t token = 1u << 31;

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
            break;

            /* Take care about the texture temporaries. There's a problem: They aren't
             * declared anywhere, so we can only hardcode the values that are used
             * to map ps_1_3 shaders to the common shader structure
             */
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(writer, reg);
            break;

        case BWRITERSPR_CONST: /* Can be mapped 1:1 */
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

        default:
            WARN("Invalid register type for <= ps_1_3 shader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if(reg->srcmod == BWRITERSPSM_DZ || reg->srcmod == BWRITERSPSM_DW
            || reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG
            || reg->srcmod == BWRITERSPSM_NOT)
    {
        WARN("Invalid source modifier %u for <= ps_1_3\n", reg->srcmod);
        writer->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_u32(buffer, token);
}

static void ps_1_0123_dstreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer,
        uint32_t shift, uint32_t mod)
{
    uint32_t token = (1u << 31); /* Bit 31 of registers is 1 */

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported for destination registers\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(writer, reg);
            break;

        /* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->writemask);
    put_u32(buffer, token);
}

/* The length of an instruction consists of the destination register (if any),
 * the number of source registers, the number of address registers used for
 * indirect addressing, and optionally the predicate register */
static unsigned int instrlen(const struct instruction *instr, unsigned int srcs, unsigned int dsts)
{
    unsigned int ret = srcs + dsts + (instr->has_predicate ? 1 : 0);
    unsigned int i;

    if (dsts && instr->dst.rel_reg)
        ++ret;
    for (i = 0; i < srcs; ++i)
        if (instr->src[i].rel_reg)
            ++ret;
    return ret;
}

static void sm_1_x_opcode(struct bc_writer *writer, const struct instruction *instr, uint32_t token,
        struct bytecode_buffer *buffer)
{
    /* Instruction length isn't encoded in sm_1_x. */
    if (instr->coissue)
        token |= D3DSI_COISSUE;
    put_u32(buffer, token);
}

static void instr_handler(struct bc_writer *writer, const struct instruction *instr, struct bytecode_buffer *buffer)
{
    uint32_t token = d3d9_opcode(instr->opcode);

    writer->funcs->opcode(writer, instr, token, buffer);
    if (instr->has_dst)
        writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
    write_srcregs(writer, instr, buffer);
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

static void instr_ps_1_0123_texld(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer)
{
    struct shader_reg reg;
    uint32_t swizzlemask;
    uint32_t idx;

    if (instr->src[1].type != BWRITERSPR_SAMPLER || instr->src[1].regnum > 3)
    {
        WARN("Unsupported sampler type %u regnum %u.\n", instr->src[1].type, instr->src[1].regnum);
        writer->state = E_INVALIDARG;
        return;
    }
    else if (instr->dst.type != BWRITERSPR_TEMP)
    {
        WARN("Can only sample into a temp register\n");
        writer->state = E_INVALIDARG;
        return;
    }

    idx = instr->src[1].regnum;
    if ((idx == 0 && instr->dst.regnum != T0_REG) || (idx == 1 && instr->dst.regnum != T1_REG)
            || (idx == 2 && instr->dst.regnum != T2_REG) || (idx == 3 && instr->dst.regnum != T3_REG))
    {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_x\n", idx, instr->dst.regnum);
        writer->state = E_INVALIDARG;
        return;
    }
    if (instr->src[0].type == BWRITERSPR_INPUT)
    {
        /* A simple non-dependent read tex instruction */
        if (instr->src[0].regnum != writer->t_regnum[idx])
        {
            WARN("Cannot sample from s%u with texture address data from interpolator %u\n", idx, instr->src[0].regnum);
            writer->state = E_INVALIDARG;
            return;
        }
        writer->funcs->opcode(writer, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);

        /* map the temp dstreg to the ps_1_3 texture temporary register */
        writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
    }
    else if (instr->src[0].type == BWRITERSPR_TEMP)
    {
        swizzlemask = 3 | (3 << 2) | (3 << 4);
        if ((instr->src[0].swizzle & swizzlemask) == (BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z))
        {
            TRACE("writing texreg2rgb\n");
            writer->funcs->opcode(writer, instr, D3DSIO_TEXREG2RGB & D3DSI_OPCODE_MASK, buffer);
        }
        else if (instr->src[0].swizzle == (BWRITERVS_X_W | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X))
        {
            TRACE("writing texreg2ar\n");
            writer->funcs->opcode(writer, instr, D3DSIO_TEXREG2AR & D3DSI_OPCODE_MASK, buffer);
        }
        else if (instr->src[0].swizzle == (BWRITERVS_X_Y | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z))
        {
            TRACE("writing texreg2gb\n");
            writer->funcs->opcode(writer, instr, D3DSIO_TEXREG2GB & D3DSI_OPCODE_MASK, buffer);
        }
        else
        {
            WARN("Unsupported src addr swizzle in dependent texld: 0x%08x\n", instr->src[0].swizzle);
            writer->state = E_INVALIDARG;
            return;
        }

        /* Dst and src reg can be mapped normally. Both registers are
         * temporary registers in the source shader and have to be mapped to
         * the temporary form of the texture registers. However, the src reg
         * doesn't have a swizzle. */
        writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
        reg = instr->src[0];
        reg.swizzle = BWRITERVS_NOSWIZZLE;
        writer->funcs->srcreg(writer, &reg, buffer);
    }
    else
    {
        WARN("Invalid address data source register\n");
        writer->state = E_INVALIDARG;
        return;
    }
}

static void instr_ps_1_0123_mov(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer)
{
    uint32_t token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if (instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT)
    {
        if ((instr->dst.regnum == T0_REG && instr->src[0].regnum == writer->t_regnum[0])
                || (instr->dst.regnum == T1_REG && instr->src[0].regnum == writer->t_regnum[1])
                || (instr->dst.regnum == T2_REG && instr->src[0].regnum == writer->t_regnum[2])
                || (instr->dst.regnum == T3_REG && instr->src[0].regnum == writer->t_regnum[3]))
        {
            if (instr->dstmod & BWRITERSPDM_SATURATE)
            {
                writer->funcs->opcode(writer, instr, D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK, buffer);
                /* Remove the SATURATE flag, it's implicit to the instruction */
                writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod & (~BWRITERSPDM_SATURATE));
                return;
            }
            else
            {
                WARN("A varying -> temp copy is only supported with the SATURATE modifier in <=ps_1_3\n");
                writer->state = E_INVALIDARG;
                return;
            }
        }
        else if (instr->src[0].regnum == writer->v_regnum[0] || instr->src[0].regnum == writer->v_regnum[1])
        {
            /* Handled by the normal mov below. Just drop out of the if condition */
        }
        else
        {
            WARN("Unsupported varying -> temp mov in <= ps_1_3\n");
            writer->state = E_INVALIDARG;
            return;
        }
    }

    writer->funcs->opcode(writer, instr, token, buffer);
    writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
    writer->funcs->srcreg(writer, &instr->src[0], buffer);
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

static void ps_1_4_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1. */

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported in <= ps_3_0.\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
            break;

        /* Can be mapped 1:1 */
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

        default:
            WARN("Invalid register type for ps_1_4 shader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if (reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG || reg->srcmod == BWRITERSPSM_NOT)
    {
        WARN("Invalid source modifier %u for ps_1_4\n", reg->srcmod);
        writer->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_u32(buffer, token);
}

static void ps_1_4_dstreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer,
        uint32_t shift, uint32_t mod)
{
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1. */

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported for destination registers\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
            token |= d3dsp_register( reg->type, reg->regnum );
            break;

	/* For texkill */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->writemask);
    put_u32(buffer, token);
}

static void instr_ps_1_4_mov(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer)
{
    uint32_t token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if (instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT)
    {
        if (instr->src[0].regnum == writer->t_regnum[0] || instr->src[0].regnum == writer->t_regnum[1]
                || instr->src[0].regnum == writer->t_regnum[2] || instr->src[0].regnum == writer->t_regnum[3]
                || instr->src[0].regnum == writer->t_regnum[4] || instr->src[0].regnum == writer->t_regnum[5])
        {
            /* Similar to a regular mov, but a different opcode */
            token = D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK;
        }
        else if (instr->src[0].regnum == writer->v_regnum[0] || instr->src[0].regnum == writer->v_regnum[1])
        {
            /* Handled by the normal mov below. Just drop out of the if condition */
        }
        else
        {
            WARN("Unsupported varying -> temp mov in ps_1_4\n");
            writer->state = E_INVALIDARG;
            return;
        }
    }

    writer->funcs->opcode(writer, instr, token, buffer);
    writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
    writer->funcs->srcreg(writer, &instr->src[0], buffer);
}

static void instr_ps_1_4_texld(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer)
{
    if (instr->src[1].type != BWRITERSPR_SAMPLER || instr->src[1].regnum > 5)
    {
        WARN("Unsupported sampler type %u regnum %u.\n",
             instr->src[1].type, instr->src[1].regnum);
        writer->state = E_INVALIDARG;
        return;
    }
    else if (instr->dst.type != BWRITERSPR_TEMP)
    {
        WARN("Can only sample into a temp register\n");
        writer->state = E_INVALIDARG;
        return;
    }

    if (instr->src[1].regnum != instr->dst.regnum)
    {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_4.\n",
             instr->src[1].regnum, instr->dst.regnum);
        writer->state = E_INVALIDARG;
        return;
    }

    writer->funcs->opcode(writer, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);
    writer->funcs->dstreg(writer, &instr->dst, buffer, instr->shift, instr->dstmod);
    writer->funcs->srcreg(writer, &instr->src[0], buffer);
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

    write_declarations(This, buffer, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
}

static void vs_2_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1 */
    uint32_t component;
    uint32_t d3d9reg;
    BOOL has_swizzle;

    switch (reg->type)
    {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected by
             * map_vs_output. */
            switch (reg->swizzle)
            {
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
            token |= map_vs_output(writer, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
             * but are unexpected. If we hit this path it might be due to an error.
             */
            FIXME("Unexpected register type %u.\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
        case BWRITERSPR_CONSTINT:
        case BWRITERSPR_CONSTBOOL:
        case BWRITERSPR_LABEL:
            d3d9reg = d3d9_register(reg->type);
            token |= d3dsp_register(d3d9reg, reg->regnum);
            break;

        case BWRITERSPR_LOOP:
            if (reg->regnum != 0)
            {
                WARN("Only regnum 0 is supported for the loop index register in vs_2_0\n");
                writer->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register(D3DSPR_LOOP, 0);
            break;

        case BWRITERSPR_PREDICATE:
            if (writer->shader->major_version != 2 || writer->shader->minor_version != 1)
            {
                WARN("Predicate register is allowed only in vs_2_x\n");
                writer->state = E_INVALIDARG;
                return;
            }
            if (reg->regnum > 0)
            {
                WARN("Only predicate register 0 is supported\n");
                writer->state = E_INVALIDARG;
                return;
            }
            token |= d3dsp_register(D3DSPR_PREDICATE, 0);
            break;

        default:
            WARN("Invalid register type for 2.0 vshader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */
    token |= d3d9_srcmod(reg->srcmod);

    if (reg->rel_reg)
        token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;

    put_u32(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in
     * the binary code. */
    if (token & D3DVS_ADDRMODE_RELATIVE)
        vs_2_srcreg(writer, reg->rel_reg, buffer);
}

static void sm_2_opcode(struct bc_writer *writer, const struct instruction *instr, uint32_t token,
        struct bytecode_buffer *buffer)
{
    unsigned int dst_count = instr->has_dst ? 1 : 0;

    /* From SM 2 onwards instruction length is encoded in the opcode field. */
    token |= instrlen(instr, instr->num_srcs, dst_count) << D3DSI_INSTLENGTH_SHIFT;
    if (instr->comptype)
        token |= (d3d9_comparetype(instr->comptype) << 16) & (0xf << 16);
    if (instr->has_predicate)
        token |= D3DSHADER_INSTRUCTION_PREDICATED;
    put_u32(buffer,token);
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

static void write_samplers(const struct bwriter_shader *shader, struct bytecode_buffer *buffer)
{
    const uint32_t reg = (1u << 31) | d3dsp_register(D3DSPR_SAMPLER, 0) | D3DSP_WRITEMASK_ALL;
    uint32_t instr_dcl = D3DSIO_DCL | (2 << D3DSI_INSTLENGTH_SHIFT);
    unsigned int i;
    uint32_t token;

    for (i = 0; i < shader->num_samplers; ++i)
    {
        /* Write the DCL instruction */
        put_u32(buffer, instr_dcl);
        token = 1u << 31;
        /* Already shifted */
        token |= d3d9_sampler(shader->samplers[i].type) & D3DSP_TEXTURETYPE_MASK;
        put_u32(buffer, token);
        token = reg | (shader->samplers[i].regnum & D3DSP_REGNUM_MASK);
        token |= d3d9_dstmod(shader->samplers[i].mod);
        put_u32(buffer, token);
    }
}

static void ps_2_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr = find_ps_builtin_semantics(This, shader, 8);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    write_declarations(This, buffer, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_samplers(shader, buffer);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
}

static void ps_2_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1 */
    uint32_t d3d9reg;

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
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
            token |= d3dsp_register(d3d9reg, reg->regnum);
            break;

        case BWRITERSPR_PREDICATE:
            if (writer->shader->minor_version == 0)
            {
                WARN("Predicate register not supported in ps_2_0\n");
                writer->state = E_INVALIDARG;
            }
            if (reg->regnum)
            {
                WARN("Predicate register with regnum %u not supported.\n", reg->regnum);
                writer->state = E_INVALIDARG;
            }
            token |= d3dsp_register(D3DSPR_PREDICATE, 0);
            break;

        default:
            WARN("Invalid register type for ps_2_0 shader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_u32(buffer, token);
}

static void ps_2_0_dstreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer,
        uint32_t shift, uint32_t mod)
{
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1 */
    uint32_t d3d9reg;

    if (reg->rel_reg)
    {
        WARN("Relative addressing not supported for destination registers\n");
        writer->state = E_INVALIDARG;
        return;
    }

    switch (reg->type)
    {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
        case BWRITERSPR_COLOROUT:
        case BWRITERSPR_DEPTHOUT:
            d3d9reg = d3d9_register(reg->type);
            token |= d3dsp_register(d3d9reg, reg->regnum);
            break;

        case BWRITERSPR_PREDICATE:
            if (writer->shader->minor_version == 0)
            {
                WARN("Predicate register not supported in ps_2_0\n");
                writer->state = E_INVALIDARG;
            }
            token |= d3dsp_register(D3DSPR_PREDICATE, reg->regnum);
            break;

	/* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(writer, reg);
            break;

        default:
            WARN("Invalid dest register type for 2.x pshader\n");
            writer->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->writemask);
    put_u32(buffer, token);
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
    write_declarations(This, buffer, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_declarations(This, buffer, shader->outputs, shader->num_outputs, BWRITERSPR_OUTPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
    write_samplers(shader, buffer);
}

static void sm_3_srcreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer)
{
    const struct bwriter_shader *shader = writer->shader;
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1 */
    uint32_t d3d9reg;

    d3d9reg = d3d9_register(reg->type);
    token |= d3dsp_register(d3d9reg, reg->regnum);
    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK;
    token |= d3d9_srcmod(reg->srcmod);

    if (reg->rel_reg)
    {
        if (reg->type == BWRITERSPR_CONST && shader->type == ST_PIXEL)
        {
            WARN("c%u[...] is unsupported in ps_3_0.\n", reg->regnum);
            writer->state = E_INVALIDARG;
            return;
        }

        if (((reg->rel_reg->type == BWRITERSPR_ADDR && shader->type == ST_VERTEX)
                || reg->rel_reg->type == BWRITERSPR_LOOP) && reg->rel_reg->regnum == 0)
        {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        }
        else
        {
            WARN("Unsupported relative addressing register\n");
            writer->state = E_INVALIDARG;
            return;
        }
    }

    put_u32(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in
     * the binary code. */
    if (token & D3DVS_ADDRMODE_RELATIVE)
        sm_3_srcreg(writer, reg->rel_reg, buffer);
}

static void sm_3_dstreg(struct bc_writer *writer, const struct shader_reg *reg, struct bytecode_buffer *buffer,
        uint32_t shift, uint32_t mod)
{
    const struct bwriter_shader *shader = writer->shader;
    uint32_t token = 1u << 31; /* Bit 31 of registers is 1 */
    uint32_t d3d9reg;

    if (reg->rel_reg)
    {
        if (shader->type == ST_VERTEX && reg->type == BWRITERSPR_OUTPUT)
        {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        }
        else
        {
            WARN("Relative addressing not supported for this shader type or register type\n");
            writer->state = E_INVALIDARG;
            return;
        }
    }

    d3d9reg = d3d9_register(reg->type);
    token |= d3dsp_register(d3d9reg, reg->regnum);
    token |= d3d9_dstmod(mod);
    token |= d3d9_writemask(reg->writemask);
    put_u32(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in
     * the binary code. */
    if (token & D3DVS_ADDRMODE_RELATIVE)
        sm_3_srcreg(writer, reg->rel_reg, buffer);
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

static const struct
{
    enum shader_type type;
    unsigned char major, minor;
    const struct bytecode_backend *backend;
}
shader_backends[] =
{
    {ST_VERTEX, 1, 0, &vs_1_x_backend},
    {ST_VERTEX, 1, 1, &vs_1_x_backend},
    {ST_VERTEX, 2, 0, &vs_2_0_backend},
    {ST_VERTEX, 2, 1, &vs_2_x_backend},
    {ST_VERTEX, 3, 0, &vs_3_backend},

    {ST_PIXEL, 1, 0, &ps_1_0123_backend},
    {ST_PIXEL, 1, 1, &ps_1_0123_backend},
    {ST_PIXEL, 1, 2, &ps_1_0123_backend},
    {ST_PIXEL, 1, 3, &ps_1_0123_backend},
    {ST_PIXEL, 1, 4, &ps_1_4_backend},
    {ST_PIXEL, 2, 0, &ps_2_0_backend},
    {ST_PIXEL, 2, 1, &ps_2_x_backend},
    {ST_PIXEL, 3, 0, &ps_3_backend},
};

static HRESULT call_instr_handler(struct bc_writer *writer, const struct instruction *instr,
        struct bytecode_buffer *buffer)
{
    unsigned int i;

    for (i = 0; writer->funcs->instructions[i].opcode != BWRITERSIO_END; ++i)
    {
        if (instr->opcode == writer->funcs->instructions[i].opcode)
        {
            if (!writer->funcs->instructions[i].func)
            {
                WARN("Opcode %u not supported by this profile.\n", instr->opcode);
                return E_INVALIDARG;
            }
            writer->funcs->instructions[i].func(writer, instr, buffer);
            return S_OK;
        }
    }

    FIXME("Unhandled instruction %u - %s.\n", instr->opcode, debug_print_opcode(instr->opcode));
    return E_INVALIDARG;
}

HRESULT shader_write_bytecode(const struct bwriter_shader *shader, uint32_t **result, uint32_t *size)
{
    struct bc_writer *writer;
    struct bytecode_buffer *buffer = NULL;
    HRESULT hr;
    unsigned int i;

    if(!shader){
        ERR("NULL shader structure, aborting\n");
        return E_FAIL;
    }

    if (!(writer = calloc(1, sizeof(*writer))))
        return E_OUTOFMEMORY;

    for (i = 0; i < ARRAY_SIZE(shader_backends); ++i)
    {
        if (shader->type == shader_backends[i].type
                && shader->major_version == shader_backends[i].major
                && shader->minor_version == shader_backends[i].minor)
        {
            writer->funcs = shader_backends[i].backend;
            break;
        }
    }

    if (!writer->funcs)
    {
        FIXME("Unsupported shader type %#x, version %u.%u.\n",
                shader->type, shader->major_version, shader->minor_version);
        free(writer);
        return E_NOTIMPL;
    }

    writer->shader = shader;
    *result = NULL;

    buffer = allocate_buffer();
    if(!buffer) {
        WARN("Failed to allocate a buffer for the shader bytecode\n");
        hr = E_FAIL;
        goto error;
    }

    /* Write shader type and version */
    put_u32(buffer, sm1_version(shader));

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

    *size = buffer->size * sizeof(uint32_t);
    *result = buffer->data;
    buffer->data = NULL;
    hr = S_OK;

error:
    if(buffer) {
        free(buffer->data);
        free(buffer);
    }
    free(writer);
    return hr;
}

void SlDeleteShader(struct bwriter_shader *shader) {
    unsigned int i, j;

    TRACE("Deleting shader %p\n", shader);

    for(i = 0; i < shader->num_cf; i++) {
        free(shader->constF[i]);
    }
    free(shader->constF);
    for(i = 0; i < shader->num_ci; i++) {
        free(shader->constI[i]);
    }
    free(shader->constI);
    for(i = 0; i < shader->num_cb; i++) {
        free(shader->constB[i]);
    }
    free(shader->constB);

    free(shader->inputs);
    free(shader->outputs);
    free(shader->samplers);

    for(i = 0; i < shader->num_instrs; i++) {
        for(j = 0; j < shader->instr[i]->num_srcs; j++) {
            free(shader->instr[i]->src[j].rel_reg);
        }
        free(shader->instr[i]->src);
        free(shader->instr[i]->dst.rel_reg);
        free(shader->instr[i]);
    }
    free(shader->instr);

    free(shader);
}
