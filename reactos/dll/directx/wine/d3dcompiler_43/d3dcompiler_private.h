/*
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
 * Copyright 2010 Rico Schüller
 * Copyright 2012 Matteo Bruni for CodeWeavers
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

#ifndef __WINE_D3DCOMPILER_PRIVATE_H
#define __WINE_D3DCOMPILER_PRIVATE_H

#include <config.h>
#include <wine/port.h>

#include <assert.h>
#include <stdio.h>

#define COBJMACROS
#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <d3dcompiler.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/rbtree.h>

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

/* TRACE helper functions */
const char *debug_d3dcompiler_d3d_blob_part(D3D_BLOB_PART part) DECLSPEC_HIDDEN;
const char *debug_d3dcompiler_shader_variable_class(D3D_SHADER_VARIABLE_CLASS c) DECLSPEC_HIDDEN;
const char *debug_d3dcompiler_shader_variable_type(D3D_SHADER_VARIABLE_TYPE t) DECLSPEC_HIDDEN;

enum shader_type
{
    ST_UNKNOWN,
    ST_VERTEX,
    ST_PIXEL
};

enum bwriter_comparison_type
{
    BWRITER_COMPARISON_NONE,
    BWRITER_COMPARISON_GT,
    BWRITER_COMPARISON_EQ,
    BWRITER_COMPARISON_GE,
    BWRITER_COMPARISON_LT,
    BWRITER_COMPARISON_NE,
    BWRITER_COMPARISON_LE
};

struct constant {
    DWORD                   regnum;
    union {
        float               f;
        INT                 i;
        BOOL                b;
        DWORD               d;
    }                       value[4];
};

struct shader_reg {
    DWORD                   type;
    DWORD                   regnum;
    struct shader_reg       *rel_reg;
    DWORD                   srcmod;
    union {
        DWORD               swizzle;
        DWORD               writemask;
    } u;
};

struct instruction {
    DWORD                   opcode;
    DWORD                   dstmod;
    DWORD                   shift;
    enum bwriter_comparison_type comptype;
    BOOL                    has_dst;
    struct shader_reg       dst;
    struct shader_reg       *src;
    unsigned int            num_srcs; /* For freeing the rel_regs */
    BOOL                    has_predicate;
    struct shader_reg       predicate;
    BOOL                    coissue;
};

struct declaration {
    DWORD                   usage, usage_idx;
    DWORD                   regnum;
    DWORD                   mod;
    DWORD                   writemask;
    BOOL                    builtin;
};

struct samplerdecl {
    DWORD                   type;
    DWORD                   regnum;
    DWORD                   mod;
};

#define INSTRARRAY_INITIAL_SIZE 8
struct bwriter_shader {
    enum shader_type        type;

    /* Shader version selected */
    DWORD                   version;

    /* Local constants. Every constant that is not defined below is loaded from
     * the global constant set at shader runtime
     */
    struct constant         **constF;
    struct constant         **constI;
    struct constant         **constB;
    unsigned int            num_cf, num_ci, num_cb;

    /* Declared input and output varyings */
    struct declaration      *inputs, *outputs;
    unsigned int            num_inputs, num_outputs;
    struct samplerdecl      *samplers;
    unsigned int            num_samplers;

    /* Are special pixel shader 3.0 registers declared? */
    BOOL                    vPos, vFace;

    /* Array of shader instructions - The shader code itself */
    struct instruction      **instr;
    unsigned int            num_instrs, instr_alloc_size;
};

static inline void *d3dcompiler_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void *d3dcompiler_realloc(void *ptr, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static inline BOOL d3dcompiler_free(void *ptr)
{
    return HeapFree(GetProcessHeap(), 0, ptr);
}

static inline char *d3dcompiler_strdup(const char *string)
{
    char *copy;
    SIZE_T len;

    if (!string)
        return NULL;

    len = strlen(string);
    copy = d3dcompiler_alloc(len + 1);
    if (copy)
        memcpy(copy, string, len + 1);
    return copy;
}

struct asm_parser;

/* This structure is only used in asmshader.y, but since the .l file accesses the semantic types
 * too it has to know it as well
 */
struct rel_reg {
    BOOL            has_rel_reg;
    DWORD           type;
    DWORD           additional_offset;
    DWORD           rel_regnum;
    DWORD           swizzle;
};

#define MAX_SRC_REGS 4

struct src_regs {
    struct shader_reg reg[MAX_SRC_REGS];
    unsigned int      count;
};

struct asmparser_backend {
    void (*constF)(struct asm_parser *This, DWORD reg, float x, float y, float z, float w);
    void (*constI)(struct asm_parser *This, DWORD reg, INT x, INT y, INT z, INT w);
    void (*constB)(struct asm_parser *This, DWORD reg, BOOL x);

    void (*dstreg)(struct asm_parser *This, struct instruction *instr,
                   const struct shader_reg *dst);
    void (*srcreg)(struct asm_parser *This, struct instruction *instr, int num,
                   const struct shader_reg *src);

    void (*predicate)(struct asm_parser *This,
                      const struct shader_reg *predicate);
    void (*coissue)(struct asm_parser *This);

    void (*dcl_output)(struct asm_parser *This, DWORD usage, DWORD num,
                       const struct shader_reg *reg);
    void (*dcl_input)(struct asm_parser *This, DWORD usage, DWORD num,
                      DWORD mod, const struct shader_reg *reg);
    void (*dcl_sampler)(struct asm_parser *This, DWORD samptype, DWORD mod,
                        DWORD regnum, unsigned int line_no);

    void (*end)(struct asm_parser *This);

    void (*instr)(struct asm_parser *parser, DWORD opcode, DWORD mod, DWORD shift,
            enum bwriter_comparison_type comp, const struct shader_reg *dst,
            const struct src_regs *srcs, int expectednsrcs);
};

struct instruction *alloc_instr(unsigned int srcs) DECLSPEC_HIDDEN;
BOOL add_instruction(struct bwriter_shader *shader, struct instruction *instr) DECLSPEC_HIDDEN;
BOOL add_constF(struct bwriter_shader *shader, DWORD reg, float x, float y, float z, float w) DECLSPEC_HIDDEN;
BOOL add_constI(struct bwriter_shader *shader, DWORD reg, INT x, INT y, INT z, INT w) DECLSPEC_HIDDEN;
BOOL add_constB(struct bwriter_shader *shader, DWORD reg, BOOL x) DECLSPEC_HIDDEN;
BOOL record_declaration(struct bwriter_shader *shader, DWORD usage, DWORD usage_idx,
        DWORD mod, BOOL output, DWORD regnum, DWORD writemask, BOOL builtin) DECLSPEC_HIDDEN;
BOOL record_sampler(struct bwriter_shader *shader, DWORD samptype, DWORD mod, DWORD regnum) DECLSPEC_HIDDEN;

#define MESSAGEBUFFER_INITIAL_SIZE 256

enum parse_status
{
    PARSE_SUCCESS = 0,
    PARSE_WARN = 1,
    PARSE_ERR = 2
};

struct compilation_messages
{
    char *string;
    unsigned int size;
    unsigned int capacity;
};

struct asm_parser
{
    /* The function table of the parser implementation */
    const struct asmparser_backend *funcs;

    /* Private data follows */
    struct bwriter_shader *shader;
    unsigned int m3x3pad_count;

    enum parse_status status;
    struct compilation_messages messages;
    unsigned int line_no;
};

extern struct asm_parser asm_ctx DECLSPEC_HIDDEN;

void create_vs10_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs11_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs20_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs2x_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs30_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps10_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps11_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps12_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps13_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps14_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps20_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps2x_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps30_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;

struct bwriter_shader *parse_asm_shader(char **messages) DECLSPEC_HIDDEN;

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

void compilation_message(struct compilation_messages *msg, const char *fmt, va_list args) DECLSPEC_HIDDEN;
void asmparser_message(struct asm_parser *ctx, const char *fmt, ...) PRINTF_ATTR(2,3) DECLSPEC_HIDDEN;
static inline void set_parse_status(enum parse_status *current, enum parse_status update)
{
    if (update == PARSE_ERR)
        *current = PARSE_ERR;
    else if (update == PARSE_WARN && *current == PARSE_SUCCESS)
        *current = PARSE_WARN;
}

/* A reasonable value as initial size */
#define BYTECODEBUFFER_INITIAL_SIZE 32
struct bytecode_buffer {
    DWORD *data;
    DWORD size;
    DWORD alloc_size;
    /* For tracking rare out of memory situations without passing
     * return values around everywhere
     */
    HRESULT state;
};

struct bc_writer; /* Predeclaration for use in vtable parameters */

typedef void (*instr_writer)(struct bc_writer *This,
                             const struct instruction *instr,
                             struct bytecode_buffer *buffer);

struct bytecode_backend {
    void (*header)(struct bc_writer *This, const struct bwriter_shader *shader,
                   struct bytecode_buffer *buffer);
    void (*end)(struct bc_writer *This, const struct bwriter_shader *shader,
                struct bytecode_buffer *buffer);
    void (*srcreg)(struct bc_writer *This, const struct shader_reg *reg,
                   struct bytecode_buffer *buffer);
    void (*dstreg)(struct bc_writer *This, const struct shader_reg *reg,
                   struct bytecode_buffer *buffer, DWORD shift, DWORD mod);
    void (*opcode)(struct bc_writer *This, const struct instruction *instr,
                   DWORD token, struct bytecode_buffer *buffer);

    const struct instr_handler_table {
        DWORD opcode;
        instr_writer func;
    } *instructions;
};

/* Bytecode writing stuff */
struct bc_writer {
    const struct bytecode_backend *funcs;

    /* Avoid result checking */
    HRESULT                       state;

    DWORD                         version;

    /* Vertex shader varying mapping */
    DWORD                         oPos_regnum;
    DWORD                         oD_regnum[2];
    DWORD                         oT_regnum[8];
    DWORD                         oFog_regnum;
    DWORD                         oFog_mask;
    DWORD                         oPts_regnum;
    DWORD                         oPts_mask;

    /* Pixel shader specific members */
    DWORD                         t_regnum[8];
    DWORD                         v_regnum[2];
};

/* Debug utility routines */
const char *debug_print_srcmod(DWORD mod) DECLSPEC_HIDDEN;
const char *debug_print_dstmod(DWORD mod) DECLSPEC_HIDDEN;
const char *debug_print_shift(DWORD shift) DECLSPEC_HIDDEN;
const char *debug_print_dstreg(const struct shader_reg *reg) DECLSPEC_HIDDEN;
const char *debug_print_srcreg(const struct shader_reg *reg) DECLSPEC_HIDDEN;
const char *debug_print_comp(DWORD comp) DECLSPEC_HIDDEN;
const char *debug_print_opcode(DWORD opcode) DECLSPEC_HIDDEN;

/* Used to signal an incorrect swizzle/writemask */
#define SWIZZLE_ERR ~0U

/* Enumerations and defines used in the bytecode writer intermediate
 * representation. */
enum bwritershader_instruction_opcode_type
{
    BWRITERSIO_NOP,
    BWRITERSIO_MOV,
    BWRITERSIO_ADD,
    BWRITERSIO_SUB,
    BWRITERSIO_MAD,
    BWRITERSIO_MUL,
    BWRITERSIO_RCP,
    BWRITERSIO_RSQ,
    BWRITERSIO_DP3,
    BWRITERSIO_DP4,
    BWRITERSIO_MIN,
    BWRITERSIO_MAX,
    BWRITERSIO_SLT,
    BWRITERSIO_SGE,
    BWRITERSIO_EXP,
    BWRITERSIO_LOG,
    BWRITERSIO_LIT,
    BWRITERSIO_DST,
    BWRITERSIO_LRP,
    BWRITERSIO_FRC,
    BWRITERSIO_M4x4,
    BWRITERSIO_M4x3,
    BWRITERSIO_M3x4,
    BWRITERSIO_M3x3,
    BWRITERSIO_M3x2,
    BWRITERSIO_CALL,
    BWRITERSIO_CALLNZ,
    BWRITERSIO_LOOP,
    BWRITERSIO_RET,
    BWRITERSIO_ENDLOOP,
    BWRITERSIO_LABEL,
    BWRITERSIO_DCL,
    BWRITERSIO_POW,
    BWRITERSIO_CRS,
    BWRITERSIO_SGN,
    BWRITERSIO_ABS,
    BWRITERSIO_NRM,
    BWRITERSIO_SINCOS,
    BWRITERSIO_REP,
    BWRITERSIO_ENDREP,
    BWRITERSIO_IF,
    BWRITERSIO_IFC,
    BWRITERSIO_ELSE,
    BWRITERSIO_ENDIF,
    BWRITERSIO_BREAK,
    BWRITERSIO_BREAKC,
    BWRITERSIO_MOVA,
    BWRITERSIO_DEFB,
    BWRITERSIO_DEFI,

    BWRITERSIO_TEXCOORD,
    BWRITERSIO_TEXKILL,
    BWRITERSIO_TEX,
    BWRITERSIO_TEXBEM,
    BWRITERSIO_TEXBEML,
    BWRITERSIO_TEXREG2AR,
    BWRITERSIO_TEXREG2GB,
    BWRITERSIO_TEXM3x2PAD,
    BWRITERSIO_TEXM3x2TEX,
    BWRITERSIO_TEXM3x3PAD,
    BWRITERSIO_TEXM3x3TEX,
    BWRITERSIO_TEXM3x3SPEC,
    BWRITERSIO_TEXM3x3VSPEC,
    BWRITERSIO_EXPP,
    BWRITERSIO_LOGP,
    BWRITERSIO_CND,
    BWRITERSIO_DEF,
    BWRITERSIO_TEXREG2RGB,
    BWRITERSIO_TEXDP3TEX,
    BWRITERSIO_TEXM3x2DEPTH,
    BWRITERSIO_TEXDP3,
    BWRITERSIO_TEXM3x3,
    BWRITERSIO_TEXDEPTH,
    BWRITERSIO_CMP,
    BWRITERSIO_BEM,
    BWRITERSIO_DP2ADD,
    BWRITERSIO_DSX,
    BWRITERSIO_DSY,
    BWRITERSIO_TEXLDD,
    BWRITERSIO_SETP,
    BWRITERSIO_TEXLDL,
    BWRITERSIO_BREAKP,
    BWRITERSIO_TEXLDP,
    BWRITERSIO_TEXLDB,

    BWRITERSIO_PHASE,
    BWRITERSIO_COMMENT,
    BWRITERSIO_END,
};

enum bwritershader_param_register_type
{
    BWRITERSPR_TEMP,
    BWRITERSPR_INPUT,
    BWRITERSPR_CONST,
    BWRITERSPR_ADDR,
    BWRITERSPR_TEXTURE,
    BWRITERSPR_RASTOUT,
    BWRITERSPR_ATTROUT,
    BWRITERSPR_TEXCRDOUT,
    BWRITERSPR_OUTPUT,
    BWRITERSPR_CONSTINT,
    BWRITERSPR_COLOROUT,
    BWRITERSPR_DEPTHOUT,
    BWRITERSPR_SAMPLER,
    BWRITERSPR_CONSTBOOL,
    BWRITERSPR_LOOP,
    BWRITERSPR_MISCTYPE,
    BWRITERSPR_LABEL,
    BWRITERSPR_PREDICATE
};

enum bwritervs_rastout_offsets
{
    BWRITERSRO_POSITION,
    BWRITERSRO_FOG,
    BWRITERSRO_POINT_SIZE
};

#define BWRITERSP_WRITEMASK_0   0x1 /* .x r */
#define BWRITERSP_WRITEMASK_1   0x2 /* .y g */
#define BWRITERSP_WRITEMASK_2   0x4 /* .z b */
#define BWRITERSP_WRITEMASK_3   0x8 /* .w a */
#define BWRITERSP_WRITEMASK_ALL 0xf /* all */

enum bwritershader_param_dstmod_type
{
    BWRITERSPDM_NONE = 0,
    BWRITERSPDM_SATURATE = 1,
    BWRITERSPDM_PARTIALPRECISION = 2,
    BWRITERSPDM_MSAMPCENTROID = 4,
};

enum bwritersampler_texture_type
{
    BWRITERSTT_UNKNOWN = 0,
    BWRITERSTT_1D = 1,
    BWRITERSTT_2D = 2,
    BWRITERSTT_CUBE = 3,
    BWRITERSTT_VOLUME = 4,
};

#define BWRITERSI_TEXLD_PROJECT 1
#define BWRITERSI_TEXLD_BIAS    2

enum bwritershader_param_srcmod_type
{
    BWRITERSPSM_NONE = 0,
    BWRITERSPSM_NEG,
    BWRITERSPSM_BIAS,
    BWRITERSPSM_BIASNEG,
    BWRITERSPSM_SIGN,
    BWRITERSPSM_SIGNNEG,
    BWRITERSPSM_COMP,
    BWRITERSPSM_X2,
    BWRITERSPSM_X2NEG,
    BWRITERSPSM_DZ,
    BWRITERSPSM_DW,
    BWRITERSPSM_ABS,
    BWRITERSPSM_ABSNEG,
    BWRITERSPSM_NOT,
};

#define BWRITER_SM1_VS  0xfffe
#define BWRITER_SM1_PS  0xffff

#define BWRITERPS_VERSION(major, minor) ((BWRITER_SM1_PS << 16) | ((major) << 8) | (minor))
#define BWRITERVS_VERSION(major, minor) ((BWRITER_SM1_VS << 16) | ((major) << 8) | (minor))

#define BWRITERVS_SWIZZLE_SHIFT      16
#define BWRITERVS_SWIZZLE_MASK       (0xFF << BWRITERVS_SWIZZLE_SHIFT)

#define BWRITERVS_X_X       (0 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_Y       (1 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_Z       (2 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_W       (3 << BWRITERVS_SWIZZLE_SHIFT)

#define BWRITERVS_Y_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 2))

#define BWRITERVS_Z_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 4))

#define BWRITERVS_W_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 6))

#define BWRITERVS_NOSWIZZLE (BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z | BWRITERVS_W_W)

#define BWRITERVS_SWIZZLE_X (BWRITERVS_X_X | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X)
#define BWRITERVS_SWIZZLE_Y (BWRITERVS_X_Y | BWRITERVS_Y_Y | BWRITERVS_Z_Y | BWRITERVS_W_Y)
#define BWRITERVS_SWIZZLE_Z (BWRITERVS_X_Z | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z)
#define BWRITERVS_SWIZZLE_W (BWRITERVS_X_W | BWRITERVS_Y_W | BWRITERVS_Z_W | BWRITERVS_W_W)

enum bwriterdeclusage
{
    BWRITERDECLUSAGE_POSITION,
    BWRITERDECLUSAGE_BLENDWEIGHT,
    BWRITERDECLUSAGE_BLENDINDICES,
    BWRITERDECLUSAGE_NORMAL,
    BWRITERDECLUSAGE_PSIZE,
    BWRITERDECLUSAGE_TEXCOORD,
    BWRITERDECLUSAGE_TANGENT,
    BWRITERDECLUSAGE_BINORMAL,
    BWRITERDECLUSAGE_TESSFACTOR,
    BWRITERDECLUSAGE_POSITIONT,
    BWRITERDECLUSAGE_COLOR,
    BWRITERDECLUSAGE_FOG,
    BWRITERDECLUSAGE_DEPTH,
    BWRITERDECLUSAGE_SAMPLE
};

/* ps 1.x texture registers mappings */
#define T0_REG          2
#define T1_REG          3
#define T2_REG          4
#define T3_REG          5

struct bwriter_shader *SlAssembleShader(const char *text, char **messages) DECLSPEC_HIDDEN;
HRESULT SlWriteBytecode(const struct bwriter_shader *shader, int dxversion, DWORD **result, DWORD *size) DECLSPEC_HIDDEN;
void SlDeleteShader(struct bwriter_shader *shader) DECLSPEC_HIDDEN;

/* The general IR structure is inspired by Mesa GLSL hir, even though the code
 * ends up being quite different in practice. Anyway, here comes the relevant
 * licensing information.
 *
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

enum hlsl_type_class
{
    HLSL_CLASS_SCALAR,
    HLSL_CLASS_VECTOR,
    HLSL_CLASS_MATRIX,
    HLSL_CLASS_LAST_NUMERIC = HLSL_CLASS_MATRIX,
    HLSL_CLASS_STRUCT,
    HLSL_CLASS_ARRAY,
    HLSL_CLASS_OBJECT,
};

enum hlsl_base_type
{
    HLSL_TYPE_FLOAT,
    HLSL_TYPE_HALF,
    HLSL_TYPE_DOUBLE,
    HLSL_TYPE_INT,
    HLSL_TYPE_UINT,
    HLSL_TYPE_BOOL,
    HLSL_TYPE_LAST_SCALAR = HLSL_TYPE_BOOL,
    HLSL_TYPE_SAMPLER,
    HLSL_TYPE_TEXTURE,
    HLSL_TYPE_PIXELSHADER,
    HLSL_TYPE_VERTEXSHADER,
    HLSL_TYPE_STRING,
    HLSL_TYPE_VOID,
};

enum hlsl_sampler_dim
{
   HLSL_SAMPLER_DIM_GENERIC,
   HLSL_SAMPLER_DIM_1D,
   HLSL_SAMPLER_DIM_2D,
   HLSL_SAMPLER_DIM_3D,
   HLSL_SAMPLER_DIM_CUBE,
};

enum hlsl_matrix_majority
{
    HLSL_COLUMN_MAJOR,
    HLSL_ROW_MAJOR
};

struct hlsl_type
{
    struct list entry;
    struct wine_rb_entry scope_entry;
    enum hlsl_type_class type;
    enum hlsl_base_type base_type;
    enum hlsl_sampler_dim sampler_dim;
    const char *name;
    unsigned int modifiers;
    unsigned int dimx;
    unsigned int dimy;
    union
    {
        struct list *elements;
        struct
        {
            struct hlsl_type *type;
            unsigned int elements_count;
        } array;
    } e;
};

struct hlsl_struct_field
{
    struct list entry;
    struct hlsl_type *type;
    const char *name;
    const char *semantic;
    DWORD modifiers;
};

struct source_location
{
    const char *file;
    unsigned int line;
    unsigned int col;
};

enum hlsl_ir_node_type
{
    HLSL_IR_VAR = 0,
    HLSL_IR_ASSIGNMENT,
    HLSL_IR_CONSTANT,
    HLSL_IR_CONSTRUCTOR,
    HLSL_IR_DEREF,
    HLSL_IR_EXPR,
    HLSL_IR_FUNCTION_DECL,
    HLSL_IR_IF,
    HLSL_IR_LOOP,
    HLSL_IR_JUMP,
    HLSL_IR_SWIZZLE,
};

struct hlsl_ir_node
{
    struct list entry;
    enum hlsl_ir_node_type type;
    struct hlsl_type *data_type;

    struct source_location loc;
};

#define HLSL_STORAGE_EXTERN          0x00000001
#define HLSL_STORAGE_NOINTERPOLATION 0x00000002
#define HLSL_MODIFIER_PRECISE        0x00000004
#define HLSL_STORAGE_SHARED          0x00000008
#define HLSL_STORAGE_GROUPSHARED     0x00000010
#define HLSL_STORAGE_STATIC          0x00000020
#define HLSL_STORAGE_UNIFORM         0x00000040
#define HLSL_STORAGE_VOLATILE        0x00000080
#define HLSL_MODIFIER_CONST          0x00000100
#define HLSL_MODIFIER_ROW_MAJOR      0x00000200
#define HLSL_MODIFIER_COLUMN_MAJOR   0x00000400
#define HLSL_MODIFIER_IN             0x00000800
#define HLSL_MODIFIER_OUT            0x00001000

#define HLSL_TYPE_MODIFIERS_MASK     (HLSL_MODIFIER_PRECISE | HLSL_STORAGE_VOLATILE | \
                                      HLSL_MODIFIER_CONST | HLSL_MODIFIER_ROW_MAJOR | \
                                      HLSL_MODIFIER_COLUMN_MAJOR)

#define HLSL_MODIFIERS_COMPARISON_MASK (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)

struct reg_reservation
{
    enum bwritershader_param_register_type type;
    DWORD regnum;
};

struct hlsl_ir_var
{
    struct hlsl_ir_node node;
    const char *name;
    const char *semantic;
    unsigned int modifiers;
    const struct reg_reservation *reg_reservation;
    struct list scope_entry;

    struct hlsl_var_allocation *allocation;
};

struct hlsl_ir_function
{
    struct wine_rb_entry entry;
    const char *name;
    struct wine_rb_tree overloads;
    BOOL intrinsic;
};

struct hlsl_ir_function_decl
{
    struct hlsl_ir_node node;
    struct wine_rb_entry entry;
    struct hlsl_ir_function *func;
    const char *semantic;
    struct list *parameters;
    struct list *body;
};

struct hlsl_ir_if
{
    struct hlsl_ir_node node;
    struct hlsl_ir_node *condition;
    struct list *then_instrs;
    struct list *else_instrs;
};

struct hlsl_ir_loop
{
    struct hlsl_ir_node node;
    /* loop condition is stored in the body (as "if (!condition) break;") */
    struct list *body;
};

struct hlsl_ir_assignment
{
    struct hlsl_ir_node node;
    struct hlsl_ir_node *lhs;
    struct hlsl_ir_node *rhs;
    unsigned char writemask;
};

enum hlsl_ir_expr_op {
    HLSL_IR_UNOP_BIT_NOT = 0,
    HLSL_IR_UNOP_LOGIC_NOT,
    HLSL_IR_UNOP_NEG,
    HLSL_IR_UNOP_ABS,
    HLSL_IR_UNOP_SIGN,
    HLSL_IR_UNOP_RCP,
    HLSL_IR_UNOP_RSQ,
    HLSL_IR_UNOP_SQRT,
    HLSL_IR_UNOP_NRM,
    HLSL_IR_UNOP_EXP2,
    HLSL_IR_UNOP_LOG2,

    HLSL_IR_UNOP_CAST,

    HLSL_IR_UNOP_FRACT,

    HLSL_IR_UNOP_SIN,
    HLSL_IR_UNOP_COS,
    HLSL_IR_UNOP_SIN_REDUCED,    /* Reduced range [-pi, pi] */
    HLSL_IR_UNOP_COS_REDUCED,    /* Reduced range [-pi, pi] */

    HLSL_IR_UNOP_DSX,
    HLSL_IR_UNOP_DSY,

    HLSL_IR_UNOP_SAT,

    HLSL_IR_UNOP_PREINC,
    HLSL_IR_UNOP_PREDEC,
    HLSL_IR_UNOP_POSTINC,
    HLSL_IR_UNOP_POSTDEC,

    HLSL_IR_BINOP_ADD,
    HLSL_IR_BINOP_SUB,
    HLSL_IR_BINOP_MUL,
    HLSL_IR_BINOP_DIV,

    HLSL_IR_BINOP_MOD,

    HLSL_IR_BINOP_LESS,
    HLSL_IR_BINOP_GREATER,
    HLSL_IR_BINOP_LEQUAL,
    HLSL_IR_BINOP_GEQUAL,
    HLSL_IR_BINOP_EQUAL,
    HLSL_IR_BINOP_NEQUAL,

    HLSL_IR_BINOP_LOGIC_AND,
    HLSL_IR_BINOP_LOGIC_OR,

    HLSL_IR_BINOP_LSHIFT,
    HLSL_IR_BINOP_RSHIFT,
    HLSL_IR_BINOP_BIT_AND,
    HLSL_IR_BINOP_BIT_OR,
    HLSL_IR_BINOP_BIT_XOR,

    HLSL_IR_BINOP_DOT,
    HLSL_IR_BINOP_CRS,
    HLSL_IR_BINOP_MIN,
    HLSL_IR_BINOP_MAX,

    HLSL_IR_BINOP_POW,

    HLSL_IR_TEROP_LERP,

    HLSL_IR_SEQUENCE,
};

struct hlsl_ir_expr
{
    struct hlsl_ir_node node;
    enum hlsl_ir_expr_op op;
    struct hlsl_ir_node *operands[3];
    struct list *subexpressions;
};

enum hlsl_ir_jump_type
{
    HLSL_IR_JUMP_BREAK,
    HLSL_IR_JUMP_CONTINUE,
    HLSL_IR_JUMP_DISCARD,
    HLSL_IR_JUMP_RETURN,
};

struct hlsl_ir_jump
{
    struct hlsl_ir_node node;
    enum hlsl_ir_jump_type type;
    struct hlsl_ir_node *return_value;
};

struct hlsl_ir_swizzle
{
    struct hlsl_ir_node node;
    struct hlsl_ir_node *val;
    DWORD swizzle;
};

enum hlsl_ir_deref_type
{
    HLSL_IR_DEREF_VAR,
    HLSL_IR_DEREF_ARRAY,
    HLSL_IR_DEREF_RECORD,
};

struct hlsl_ir_deref
{
    struct hlsl_ir_node node;
    enum hlsl_ir_deref_type type;
    union
    {
        struct hlsl_ir_var *var;
        struct
        {
            struct hlsl_ir_node *array;
            struct hlsl_ir_node *index;
        } array;
        struct
        {
            struct hlsl_ir_node *record;
            struct hlsl_struct_field *field;
        } record;
    } v;
};

struct hlsl_ir_constant
{
    struct hlsl_ir_node node;
    union
    {
        union
        {
            unsigned u[16];
            int i[16];
            float f[16];
            double d[16];
            BOOL b[16];
        } value;
        struct hlsl_ir_constant *array_elements;
        struct list *struct_elements;
    } v;
};

struct hlsl_ir_constructor
{
    struct hlsl_ir_node node;
    struct list *arguments;
};

struct hlsl_scope
{
    struct list entry;
    struct list vars;
    struct wine_rb_tree types;
    struct hlsl_scope *upper;
};

/* Structures used only during parsing */
struct parse_parameter
{
    struct hlsl_type *type;
    const char *name;
    const char *semantic;
    const struct reg_reservation *reg_reservation;
    unsigned int modifiers;
};

struct parse_colon_attribute
{
    const char *semantic;
    struct reg_reservation *reg_reservation;
};

struct parse_variable_def
{
    struct list entry;
    struct source_location loc;

    char *name;
    unsigned int array_size;
    const char *semantic;
    struct reg_reservation *reg_reservation;
    struct list *initializer;
};

struct parse_function
{
    char *name;
    struct hlsl_ir_function_decl *decl;
};

struct parse_if_body
{
    struct list *then_instrs;
    struct list *else_instrs;
};

enum parse_unary_op
{
    UNARY_OP_PLUS,
    UNARY_OP_MINUS,
    UNARY_OP_LOGICNOT,
    UNARY_OP_BITNOT,
};

enum parse_assign_op
{
    ASSIGN_OP_ASSIGN,
    ASSIGN_OP_ADD,
    ASSIGN_OP_SUB,
    ASSIGN_OP_MUL,
    ASSIGN_OP_DIV,
    ASSIGN_OP_MOD,
    ASSIGN_OP_LSHIFT,
    ASSIGN_OP_RSHIFT,
    ASSIGN_OP_AND,
    ASSIGN_OP_OR,
    ASSIGN_OP_XOR,
};

struct hlsl_parse_ctx
{
    const char **source_files;
    unsigned int source_files_count;
    const char *source_file;
    unsigned int line_no;
    unsigned int column;
    enum parse_status status;
    struct compilation_messages messages;

    struct hlsl_scope *cur_scope;
    struct hlsl_scope *globals;
    struct list scopes;

    struct list types;
    struct wine_rb_tree functions;

    enum hlsl_matrix_majority matrix_majority;
};

extern struct hlsl_parse_ctx hlsl_ctx DECLSPEC_HIDDEN;

enum hlsl_error_level
{
    HLSL_LEVEL_ERROR = 0,
    HLSL_LEVEL_WARNING,
    HLSL_LEVEL_NOTE,
};

void hlsl_message(const char *fmt, ...) PRINTF_ATTR(1,2) DECLSPEC_HIDDEN;
void hlsl_report_message(const char *filename, DWORD line, DWORD column,
        enum hlsl_error_level level, const char *fmt, ...) PRINTF_ATTR(5,6) DECLSPEC_HIDDEN;

static inline struct hlsl_ir_var *var_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_VAR);
    return CONTAINING_RECORD(node, struct hlsl_ir_var, node);
}

static inline struct hlsl_ir_expr *expr_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_EXPR);
    return CONTAINING_RECORD(node, struct hlsl_ir_expr, node);
}

static inline struct hlsl_ir_deref *deref_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_DEREF);
    return CONTAINING_RECORD(node, struct hlsl_ir_deref, node);
}

static inline struct hlsl_ir_constant *constant_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_CONSTANT);
    return CONTAINING_RECORD(node, struct hlsl_ir_constant, node);
}

static inline struct hlsl_ir_jump *jump_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_JUMP);
    return CONTAINING_RECORD(node, struct hlsl_ir_jump, node);
}

static inline struct hlsl_ir_assignment *assignment_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_ASSIGNMENT);
    return CONTAINING_RECORD(node, struct hlsl_ir_assignment, node);
}

static inline struct hlsl_ir_swizzle *swizzle_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_SWIZZLE);
    return CONTAINING_RECORD(node, struct hlsl_ir_swizzle, node);
}

static inline struct hlsl_ir_constructor *constructor_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_CONSTRUCTOR);
    return CONTAINING_RECORD(node, struct hlsl_ir_constructor, node);
}

static inline struct hlsl_ir_if *if_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_IF);
    return CONTAINING_RECORD(node, struct hlsl_ir_if, node);
}

static inline struct hlsl_ir_loop *loop_from_node(const struct hlsl_ir_node *node)
{
    assert(node->type == HLSL_IR_LOOP);
    return CONTAINING_RECORD(node, struct hlsl_ir_loop, node);
}

BOOL add_declaration(struct hlsl_scope *scope, struct hlsl_ir_var *decl, BOOL local_var) DECLSPEC_HIDDEN;
struct hlsl_ir_var *get_variable(struct hlsl_scope *scope, const char *name) DECLSPEC_HIDDEN;
void free_declaration(struct hlsl_ir_var *decl) DECLSPEC_HIDDEN;
struct hlsl_type *new_hlsl_type(const char *name, enum hlsl_type_class type_class,
        enum hlsl_base_type base_type, unsigned dimx, unsigned dimy) DECLSPEC_HIDDEN;
struct hlsl_type *new_array_type(struct hlsl_type *basic_type, unsigned int array_size) DECLSPEC_HIDDEN;
struct hlsl_type *clone_hlsl_type(struct hlsl_type *old) DECLSPEC_HIDDEN;
struct hlsl_type *get_type(struct hlsl_scope *scope, const char *name, BOOL recursive) DECLSPEC_HIDDEN;
BOOL find_function(const char *name) DECLSPEC_HIDDEN;
unsigned int components_count_type(struct hlsl_type *type) DECLSPEC_HIDDEN;
BOOL compare_hlsl_types(const struct hlsl_type *t1, const struct hlsl_type *t2) DECLSPEC_HIDDEN;
BOOL compatible_data_types(struct hlsl_type *s1, struct hlsl_type *s2) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *new_expr(enum hlsl_ir_expr_op op, struct hlsl_ir_node **operands,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *new_cast(struct hlsl_ir_node *node, struct hlsl_type *type,
	struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_mul(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_div(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_mod(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_add(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_sub(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_lt(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_gt(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_le(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_ge(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_eq(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_expr *hlsl_ne(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc) DECLSPEC_HIDDEN;
struct hlsl_ir_deref *new_var_deref(struct hlsl_ir_var *var) DECLSPEC_HIDDEN;
struct hlsl_ir_deref *new_record_deref(struct hlsl_ir_node *record, struct hlsl_struct_field *field) DECLSPEC_HIDDEN;
struct hlsl_ir_node *make_assignment(struct hlsl_ir_node *left, enum parse_assign_op assign_op,
        DWORD writemask, struct hlsl_ir_node *right) DECLSPEC_HIDDEN;
void push_scope(struct hlsl_parse_ctx *ctx) DECLSPEC_HIDDEN;
BOOL pop_scope(struct hlsl_parse_ctx *ctx) DECLSPEC_HIDDEN;
struct hlsl_ir_function_decl *new_func_decl(struct hlsl_type *return_type, struct list *parameters) DECLSPEC_HIDDEN;
void init_functions_tree(struct wine_rb_tree *funcs) DECLSPEC_HIDDEN;
void add_function_decl(struct wine_rb_tree *funcs, char *name, struct hlsl_ir_function_decl *decl,
        BOOL intrinsic) DECLSPEC_HIDDEN;
struct bwriter_shader *parse_hlsl_shader(const char *text, enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, char **messages) DECLSPEC_HIDDEN;

const char *debug_hlsl_type(const struct hlsl_type *type) DECLSPEC_HIDDEN;
const char *debug_modifiers(DWORD modifiers) DECLSPEC_HIDDEN;
void debug_dump_ir_function_decl(const struct hlsl_ir_function_decl *func) DECLSPEC_HIDDEN;

void free_hlsl_type(struct hlsl_type *type) DECLSPEC_HIDDEN;
void free_instr(struct hlsl_ir_node *node) DECLSPEC_HIDDEN;
void free_instr_list(struct list *list) DECLSPEC_HIDDEN;
void free_function_rb(struct wine_rb_entry *entry, void *context) DECLSPEC_HIDDEN;


#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_Aon9 MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_OSG5 MAKE_TAG('O', 'S', 'G', '5')
#define TAG_PCSG MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_RDEF MAKE_TAG('R', 'D', 'E', 'F')
#define TAG_SDBG MAKE_TAG('S', 'D', 'B', 'G')
#define TAG_SHDR MAKE_TAG('S', 'H', 'D', 'R')
#define TAG_SHEX MAKE_TAG('S', 'H', 'E', 'X')
#define TAG_STAT MAKE_TAG('S', 'T', 'A', 'T')
#define TAG_XNAP MAKE_TAG('X', 'N', 'A', 'P')
#define TAG_XNAS MAKE_TAG('X', 'N', 'A', 'S')

struct dxbc_section
{
    DWORD tag;
    const char *data;
    DWORD data_size;
};

struct dxbc
{
    UINT size;
    UINT count;
    struct dxbc_section *sections;
};

HRESULT dxbc_write_blob(struct dxbc *dxbc, ID3DBlob **blob) DECLSPEC_HIDDEN;
void dxbc_destroy(struct dxbc *dxbc) DECLSPEC_HIDDEN;
HRESULT dxbc_parse(const char *data, SIZE_T data_size, struct dxbc *dxbc) DECLSPEC_HIDDEN;
HRESULT dxbc_add_section(struct dxbc *dxbc, DWORD tag, const char *data, DWORD data_size) DECLSPEC_HIDDEN;
HRESULT dxbc_init(struct dxbc *dxbc, DWORD count) DECLSPEC_HIDDEN;

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

static inline void write_dword(char **ptr, DWORD d)
{
    memcpy(*ptr, &d, sizeof(d));
    *ptr += sizeof(d);
}

void skip_dword_unknown(const char **ptr, unsigned int count) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DCOMPILER_PRIVATE_H */
