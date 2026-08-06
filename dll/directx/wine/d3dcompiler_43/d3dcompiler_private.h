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

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/rbtree.h"

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "d3dcompiler.h"
#include "utils.h"

#include <assert.h>
#include <stdint.h>

#include <vkd3d_shader.h>

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

/* TRACE helper functions */
const char *debug_d3dcompiler_d3d_blob_part(D3D_BLOB_PART part);
const char *debug_d3dcompiler_shader_variable_class(D3D_SHADER_VARIABLE_CLASS c);
const char *debug_d3dcompiler_shader_variable_type(D3D_SHADER_VARIABLE_TYPE t);

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

struct constant
{
    unsigned int regnum;
    union
    {
        float               f;
        int                 i;
        BOOL                b;
        uint32_t            d;
    } value[4];
};

struct shader_reg
{
    uint32_t type;
    unsigned int regnum;
    struct shader_reg *rel_reg;
    uint32_t srcmod;
    union
    {
        uint32_t swizzle;
        uint32_t writemask;
    };
};

struct instruction
{
    uint32_t                opcode;
    uint32_t                dstmod;
    uint32_t                shift;
    enum bwriter_comparison_type comptype;
    BOOL                    has_dst;
    struct shader_reg       dst;
    struct shader_reg       *src;
    unsigned int            num_srcs; /* For freeing the rel_regs */
    BOOL                    has_predicate;
    struct shader_reg       predicate;
    BOOL                    coissue;
};

struct declaration
{
    uint32_t                usage, usage_idx;
    uint32_t                regnum;
    uint32_t                mod;
    uint32_t                writemask;
    BOOL                    builtin;
};

struct samplerdecl
{
    uint32_t                type;
    uint32_t                regnum;
    uint32_t                mod;
};

struct bwriter_shader
{
    enum shader_type        type;
    unsigned char major_version, minor_version;

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

struct asm_parser;

/* This structure is only used in asmshader.y, but since the .l file accesses the semantic types
 * too it has to know it as well
 */
struct rel_reg
{
    BOOL            has_rel_reg;
    uint32_t        type;
    uint32_t        additional_offset;
    uint32_t        rel_regnum;
    uint32_t        swizzle;
};

#define MAX_SRC_REGS 4

struct src_regs
{
    struct shader_reg reg[MAX_SRC_REGS];
    unsigned int      count;
};

struct asmparser_backend
{
    void (*constF)(struct asm_parser *This, uint32_t reg, float x, float y, float z, float w);
    void (*constI)(struct asm_parser *This, uint32_t reg, int x, int y, int z, int w);
    void (*constB)(struct asm_parser *This, uint32_t reg, BOOL x);

    void (*dstreg)(struct asm_parser *This, struct instruction *instr,
                   const struct shader_reg *dst);
    void (*srcreg)(struct asm_parser *This, struct instruction *instr, int num,
                   const struct shader_reg *src);

    void (*predicate)(struct asm_parser *This,
                      const struct shader_reg *predicate);
    void (*coissue)(struct asm_parser *This);

    void (*dcl_output)(struct asm_parser *This, uint32_t usage, uint32_t num,
                       const struct shader_reg *reg);
    void (*dcl_input)(struct asm_parser *This, uint32_t usage, uint32_t num,
                      uint32_t mod, const struct shader_reg *reg);
    void (*dcl_sampler)(struct asm_parser *This, uint32_t samptype, uint32_t mod,
                        uint32_t regnum, unsigned int line_no);

    void (*end)(struct asm_parser *This);

    void (*instr)(struct asm_parser *parser, uint32_t opcode, uint32_t mod, uint32_t shift,
            enum bwriter_comparison_type comp, const struct shader_reg *dst,
            const struct src_regs *srcs, int expectednsrcs);
};

struct instruction *alloc_instr(unsigned int srcs);
BOOL add_instruction(struct bwriter_shader *shader, struct instruction *instr);
BOOL add_constF(struct bwriter_shader *shader, uint32_t reg, float x, float y, float z, float w);
BOOL add_constI(struct bwriter_shader *shader, uint32_t reg, int x, int y, int z, int w);
BOOL add_constB(struct bwriter_shader *shader, uint32_t reg, BOOL x);
BOOL record_declaration(struct bwriter_shader *shader, uint32_t usage, uint32_t usage_idx,
        uint32_t mod, BOOL output, uint32_t regnum, uint32_t writemask, BOOL builtin);
BOOL record_sampler(struct bwriter_shader *shader, uint32_t samptype, uint32_t mod, uint32_t regnum);

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

extern struct asm_parser asm_ctx;

void create_vs10_parser(struct asm_parser *ret);
void create_vs11_parser(struct asm_parser *ret);
void create_vs20_parser(struct asm_parser *ret);
void create_vs2x_parser(struct asm_parser *ret);
void create_vs30_parser(struct asm_parser *ret);
void create_ps10_parser(struct asm_parser *ret);
void create_ps11_parser(struct asm_parser *ret);
void create_ps12_parser(struct asm_parser *ret);
void create_ps13_parser(struct asm_parser *ret);
void create_ps14_parser(struct asm_parser *ret);
void create_ps20_parser(struct asm_parser *ret);
void create_ps2x_parser(struct asm_parser *ret);
void create_ps30_parser(struct asm_parser *ret);

struct bwriter_shader *parse_asm_shader(char **messages);

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

void compilation_message(struct compilation_messages *msg, const char *fmt, va_list args);
void WINAPIV asmparser_message(struct asm_parser *ctx, const char *fmt, ...) PRINTF_ATTR(2,3);
static inline void set_parse_status(enum parse_status *current, enum parse_status update)
{
    if (update == PARSE_ERR)
        *current = PARSE_ERR;
    else if (update == PARSE_WARN && *current == PARSE_SUCCESS)
        *current = PARSE_WARN;
}

/* Debug utility routines */
const char *debug_print_srcmod(uint32_t mod);
const char *debug_print_dstmod(uint32_t mod);
const char *debug_print_shift(uint32_t shift);
const char *debug_print_dstreg(const struct shader_reg *reg);
const char *debug_print_srcreg(const struct shader_reg *reg);
const char *debug_print_comp(uint32_t comp);
const char *debug_print_opcode(uint32_t opcode);

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

#define BWRITER_SM1_VS  0xfffeu
#define BWRITER_SM1_PS  0xffffu

#define BWRITERPS_VERSION(major, minor) ((BWRITER_SM1_PS << 16) | ((major) << 8) | (minor))
#define BWRITERVS_VERSION(major, minor) ((BWRITER_SM1_VS << 16) | ((major) << 8) | (minor))

#define BWRITERVS_X_X       (0)
#define BWRITERVS_X_Y       (1)
#define BWRITERVS_X_Z       (2)
#define BWRITERVS_X_W       (3)

#define BWRITERVS_Y_X       (0 << 2)
#define BWRITERVS_Y_Y       (1 << 2)
#define BWRITERVS_Y_Z       (2 << 2)
#define BWRITERVS_Y_W       (3 << 2)

#define BWRITERVS_Z_X       (0 << 4)
#define BWRITERVS_Z_Y       (1 << 4)
#define BWRITERVS_Z_Z       (2 << 4)
#define BWRITERVS_Z_W       (3 << 4)

#define BWRITERVS_W_X       (0 << 6)
#define BWRITERVS_W_Y       (1 << 6)
#define BWRITERVS_W_Z       (2 << 6)
#define BWRITERVS_W_W       (3 << 6)

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

struct bwriter_shader *SlAssembleShader(const char *text, char **messages);
HRESULT shader_write_bytecode(const struct bwriter_shader *shader, uint32_t **result, uint32_t *size);
void SlDeleteShader(struct bwriter_shader *shader);

#define DXBC_HEADER_SIZE (8 * sizeof(uint32_t))

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_Aon9 MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_FX10 MAKE_TAG('F', 'X', '1', '0')
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

#endif /* __WINE_D3DCOMPILER_PRIVATE_H */
