%{
/*
 * Copyright © 2009 Intel Corporation
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main/mtypes.h"
#include "main/imports.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_parameter_layout.h"
#include "program/prog_statevars.h"
#include "program/prog_instruction.h"

#include "program/symbol_table.h"
#include "program/program_parser.h"

extern void *yy_scan_string(char *);
extern void yy_delete_buffer(void *);

static struct asm_symbol *declare_variable(struct asm_parser_state *state,
    char *name, enum asm_type t, struct YYLTYPE *locp);

static int add_state_reference(struct gl_program_parameter_list *param_list,
    const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_state(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_param(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_const(struct gl_program *prog,
    struct asm_symbol *param_var, const struct asm_vector *vec,
    GLboolean allowSwizzle);

static int yyparse(struct asm_parser_state *state);

static char *make_error_string(const char *fmt, ...);

static void yyerror(struct YYLTYPE *locp, struct asm_parser_state *state,
    const char *s);

static int validate_inputs(struct YYLTYPE *locp,
    struct asm_parser_state *state);

static void init_dst_reg(struct prog_dst_register *r);

static void set_dst_reg(struct prog_dst_register *r,
                        gl_register_file file, GLint index);

static void init_src_reg(struct asm_src_register *r);

static void set_src_reg(struct asm_src_register *r,
                        gl_register_file file, GLint index);

static void set_src_reg_swz(struct asm_src_register *r,
                            gl_register_file file, GLint index, GLuint swizzle);

static void asm_instruction_set_operands(struct asm_instruction *inst,
    const struct prog_dst_register *dst, const struct asm_src_register *src0,
    const struct asm_src_register *src1, const struct asm_src_register *src2);

static struct asm_instruction *asm_instruction_ctor(gl_inst_opcode op,
    const struct prog_dst_register *dst, const struct asm_src_register *src0,
    const struct asm_src_register *src1, const struct asm_src_register *src2);

static struct asm_instruction *asm_instruction_copy_ctor(
    const struct prog_instruction *base, const struct prog_dst_register *dst,
    const struct asm_src_register *src0, const struct asm_src_register *src1,
    const struct asm_src_register *src2);

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define YYLLOC_DEFAULT(Current, Rhs, N)					\
   do {									\
      if (YYID(N)) {							\
	 (Current).first_line = YYRHSLOC(Rhs, 1).first_line;		\
	 (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
	 (Current).position = YYRHSLOC(Rhs, 1).position;		\
	 (Current).last_line = YYRHSLOC(Rhs, N).last_line;		\
	 (Current).last_column = YYRHSLOC(Rhs, N).last_column;		\
      } else {								\
	 (Current).first_line = YYRHSLOC(Rhs, 0).last_line;		\
	 (Current).last_line = (Current).first_line;			\
	 (Current).first_column = YYRHSLOC(Rhs, 0).last_column;		\
	 (Current).last_column = (Current).first_column;		\
	 (Current).position = YYRHSLOC(Rhs, 0).position			\
	    + (Current).first_column;					\
      }									\
   } while(YYID(0))

#define YYLEX_PARAM state->scanner
%}

%pure-parser
%locations
%parse-param { struct asm_parser_state *state }
%error-verbose
%lex-param { void *scanner }

%union {
   struct asm_instruction *inst;
   struct asm_symbol *sym;
   struct asm_symbol temp_sym;
   struct asm_swizzle_mask swiz_mask;
   struct asm_src_register src_reg;
   struct prog_dst_register dst_reg;
   struct prog_instruction temp_inst;
   char *string;
   unsigned result;
   unsigned attrib;
   int integer;
   float real;
   gl_state_index state[STATE_LENGTH];
   int negate;
   struct asm_vector vector;
   gl_inst_opcode opcode;

   struct {
      unsigned swz;
      unsigned rgba_valid:1;
      unsigned xyzw_valid:1;
      unsigned negate:1;
   } ext_swizzle;
}

%token ARBvp_10 ARBfp_10

/* Tokens for assembler pseudo-ops */
%token <integer> ADDRESS
%token ALIAS ATTRIB
%token OPTION OUTPUT
%token PARAM
%token <integer> TEMP
%token END

 /* Tokens for instructions */
%token <temp_inst> BIN_OP BINSC_OP SAMPLE_OP SCALAR_OP TRI_OP VECTOR_OP
%token <temp_inst> ARL KIL SWZ TXD_OP

%token <integer> INTEGER
%token <real> REAL

%token AMBIENT ATTENUATION
%token BACK
%token CLIP COLOR
%token DEPTH DIFFUSE DIRECTION
%token EMISSION ENV EYE
%token FOG FOGCOORD FRAGMENT FRONT
%token HALF
%token INVERSE INVTRANS
%token LIGHT LIGHTMODEL LIGHTPROD LOCAL
%token MATERIAL MAT_PROGRAM MATRIX MATRIXINDEX MODELVIEW MVP
%token NORMAL
%token OBJECT
%token PALETTE PARAMS PLANE POINT_TOK POINTSIZE POSITION PRIMARY PROGRAM PROJECTION
%token RANGE RESULT ROW
%token SCENECOLOR SECONDARY SHININESS SIZE_TOK SPECULAR SPOT STATE
%token TEXCOORD TEXENV TEXGEN TEXGEN_Q TEXGEN_R TEXGEN_S TEXGEN_T TEXTURE TRANSPOSE
%token TEXTURE_UNIT TEX_1D TEX_2D TEX_3D TEX_CUBE TEX_RECT
%token TEX_SHADOW1D TEX_SHADOW2D TEX_SHADOWRECT
%token TEX_ARRAY1D TEX_ARRAY2D TEX_ARRAYSHADOW1D TEX_ARRAYSHADOW2D 
%token VERTEX VTXATTRIB
%token WEIGHT

%token <string> IDENTIFIER USED_IDENTIFIER
%type <string> string
%token <swiz_mask> MASK4 MASK3 MASK2 MASK1 SWIZZLE
%token DOT_DOT
%token DOT

%type <inst> instruction ALU_instruction TexInstruction
%type <inst> ARL_instruction VECTORop_instruction
%type <inst> SCALARop_instruction BINSCop_instruction BINop_instruction
%type <inst> TRIop_instruction TXD_instruction SWZ_instruction SAMPLE_instruction
%type <inst> KIL_instruction

%type <dst_reg> dstReg maskedDstReg maskedAddrReg
%type <src_reg> srcReg scalarUse scalarSrcReg swizzleSrcReg
%type <swiz_mask> scalarSuffix swizzleSuffix extendedSwizzle
%type <ext_swizzle> extSwizComp extSwizSel
%type <swiz_mask> optionalMask

%type <sym> progParamArray
%type <integer> addrRegRelOffset addrRegPosOffset addrRegNegOffset
%type <src_reg> progParamArrayMem progParamArrayAbs progParamArrayRel
%type <sym> addrReg
%type <swiz_mask> addrComponent addrWriteMask

%type <dst_reg> ccMaskRule ccTest ccMaskRule2 ccTest2 optionalCcMask

%type <result> resultBinding resultColBinding
%type <integer> optFaceType optColorType
%type <integer> optResultFaceType optResultColorType

%type <integer> optTexImageUnitNum texImageUnitNum
%type <integer> optTexCoordUnitNum texCoordUnitNum
%type <integer> optLegacyTexUnitNum legacyTexUnitNum
%type <integer> texImageUnit texTarget
%type <integer> vtxAttribNum

%type <attrib> attribBinding vtxAttribItem fragAttribItem

%type <temp_sym> paramSingleInit paramSingleItemDecl
%type <integer> optArraySize

%type <state> stateSingleItem stateMultipleItem
%type <state> stateMaterialItem
%type <state> stateLightItem stateLightModelItem stateLightProdItem
%type <state> stateTexGenItem stateFogItem stateClipPlaneItem statePointItem
%type <state> stateMatrixItem stateMatrixRow stateMatrixRows
%type <state> stateTexEnvItem stateDepthItem

%type <state> stateLModProperty
%type <state> stateMatrixName optMatrixRows

%type <integer> stateMatProperty
%type <integer> stateLightProperty stateSpotProperty
%type <integer> stateLightNumber stateLProdProperty
%type <integer> stateTexGenType stateTexGenCoord
%type <integer> stateTexEnvProperty
%type <integer> stateFogProperty
%type <integer> stateClipPlaneNum
%type <integer> statePointProperty

%type <integer> stateOptMatModifier stateMatModifier stateMatrixRowNum
%type <integer> stateOptModMatNum stateModMatNum statePaletteMatNum 
%type <integer> stateProgramMatNum

%type <integer> ambDiffSpecProperty

%type <state> programSingleItem progEnvParam progLocalParam
%type <state> programMultipleItem progEnvParams progLocalParams

%type <temp_sym> paramMultipleInit paramMultInitList paramMultipleItem
%type <temp_sym> paramSingleItemUse

%type <integer> progEnvParamNum progLocalParamNum
%type <state> progEnvParamNums progLocalParamNums

%type <vector> paramConstDecl paramConstUse
%type <vector> paramConstScalarDecl paramConstScalarUse paramConstVector
%type <real> signedFloatConstant
%type <negate> optionalSign

%{
extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param,
    void *yyscanner);
%}

%%

program: language optionSequence statementSequence END
	;

language: ARBvp_10
	{
	   if (state->prog->Target != GL_VERTEX_PROGRAM_ARB) {
	      yyerror(& @1, state, "invalid fragment program header");

	   }
	   state->mode = ARB_vertex;
	}
	| ARBfp_10
	{
	   if (state->prog->Target != GL_FRAGMENT_PROGRAM_ARB) {
	      yyerror(& @1, state, "invalid vertex program header");
	   }
	   state->mode = ARB_fragment;

	   state->option.TexRect =
	      (state->ctx->Extensions.NV_texture_rectangle != GL_FALSE);
	}
	;

optionSequence: optionSequence option
	|
	;

option: OPTION string ';'
	{
	   int valid = 0;

	   if (state->mode == ARB_vertex) {
	      valid = _mesa_ARBvp_parse_option(state, $2);
	   } else if (state->mode == ARB_fragment) {
	      valid = _mesa_ARBfp_parse_option(state, $2);
	   }


	   free($2);

	   if (!valid) {
	      const char *const err_str = (state->mode == ARB_vertex)
		 ? "invalid ARB vertex program option"
		 : "invalid ARB fragment program option";

	      yyerror(& @2, state, err_str);
	      YYERROR;
	   }
	}
	;

statementSequence: statementSequence statement
	|
	;

statement: instruction ';'
	{
	   if ($1 != NULL) {
	      if (state->inst_tail == NULL) {
		 state->inst_head = $1;
	      } else {
		 state->inst_tail->next = $1;
	      }

	      state->inst_tail = $1;
	      $1->next = NULL;

	      state->prog->NumInstructions++;
	   }
	}
	| namingStatement ';'
	;

instruction: ALU_instruction
	{
	   $$ = $1;
	   state->prog->NumAluInstructions++;
	}
	| TexInstruction
	{
	   $$ = $1;
	   state->prog->NumTexInstructions++;
	}
	;

ALU_instruction: ARL_instruction
	| VECTORop_instruction
	| SCALARop_instruction
	| BINSCop_instruction
	| BINop_instruction
	| TRIop_instruction
	| SWZ_instruction
	;

TexInstruction: SAMPLE_instruction
	| KIL_instruction
	| TXD_instruction
	;

ARL_instruction: ARL maskedAddrReg ',' scalarSrcReg
	{
	   $$ = asm_instruction_ctor(OPCODE_ARL, & $2, & $4, NULL, NULL);
	}
	;

VECTORop_instruction: VECTOR_OP maskedDstReg ',' swizzleSrcReg
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, NULL, NULL);
	}
	;

SCALARop_instruction: SCALAR_OP maskedDstReg ',' scalarSrcReg
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, NULL, NULL);
	}
	;

BINSCop_instruction: BINSC_OP maskedDstReg ',' scalarSrcReg ',' scalarSrcReg
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, & $6, NULL);
	}
	;


BINop_instruction: BIN_OP maskedDstReg ',' swizzleSrcReg ',' swizzleSrcReg
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, & $6, NULL);
	}
	;

TRIop_instruction: TRI_OP maskedDstReg ','
                   swizzleSrcReg ',' swizzleSrcReg ',' swizzleSrcReg
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, & $6, & $8);
	}
	;

SAMPLE_instruction: SAMPLE_OP maskedDstReg ',' swizzleSrcReg ',' texImageUnit ',' texTarget
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, NULL, NULL);
	   if ($$ != NULL) {
	      const GLbitfield tex_mask = (1U << $6);
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      $$->Base.TexSrcUnit = $6;

	      if ($8 < 0) {
		 shadow_tex = tex_mask;

		 $$->Base.TexSrcTarget = -$8;
		 $$->Base.TexShadow = 1;
	      } else {
		 $$->Base.TexSrcTarget = $8;
	      }

	      target_mask = (1U << $$->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[$6] != 0)
		  && ((state->prog->TexturesUsed[$6] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& @8, state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[$6] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	}
	;

KIL_instruction: KIL swizzleSrcReg
	{
	   $$ = asm_instruction_ctor(OPCODE_KIL, NULL, & $2, NULL, NULL);
	   state->fragment.UsesKill = 1;
	}
	| KIL ccTest
	{
	   $$ = asm_instruction_ctor(OPCODE_KIL_NV, NULL, NULL, NULL, NULL);
	   $$->Base.DstReg.CondMask = $2.CondMask;
	   $$->Base.DstReg.CondSwizzle = $2.CondSwizzle;
	   $$->Base.DstReg.CondSrc = $2.CondSrc;
	   state->fragment.UsesKill = 1;
	}
	;

TXD_instruction: TXD_OP maskedDstReg ',' swizzleSrcReg ',' swizzleSrcReg ',' swizzleSrcReg ',' texImageUnit ',' texTarget
	{
	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, & $6, & $8);
	   if ($$ != NULL) {
	      const GLbitfield tex_mask = (1U << $10);
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      $$->Base.TexSrcUnit = $10;

	      if ($12 < 0) {
		 shadow_tex = tex_mask;

		 $$->Base.TexSrcTarget = -$12;
		 $$->Base.TexShadow = 1;
	      } else {
		 $$->Base.TexSrcTarget = $12;
	      }

	      target_mask = (1U << $$->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[$10] != 0)
		  && ((state->prog->TexturesUsed[$10] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& @12, state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[$10] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	}
	;

texImageUnit: TEXTURE_UNIT optTexImageUnitNum
	{
	   $$ = $2;
	}
	;

texTarget: TEX_1D  { $$ = TEXTURE_1D_INDEX; }
	| TEX_2D   { $$ = TEXTURE_2D_INDEX; }
	| TEX_3D   { $$ = TEXTURE_3D_INDEX; }
	| TEX_CUBE { $$ = TEXTURE_CUBE_INDEX; }
	| TEX_RECT { $$ = TEXTURE_RECT_INDEX; }
	| TEX_SHADOW1D   { $$ = -TEXTURE_1D_INDEX; }
	| TEX_SHADOW2D   { $$ = -TEXTURE_2D_INDEX; }
	| TEX_SHADOWRECT { $$ = -TEXTURE_RECT_INDEX; }
	| TEX_ARRAY1D         { $$ = TEXTURE_1D_ARRAY_INDEX; }
	| TEX_ARRAY2D         { $$ = TEXTURE_2D_ARRAY_INDEX; }
	| TEX_ARRAYSHADOW1D   { $$ = -TEXTURE_1D_ARRAY_INDEX; }
	| TEX_ARRAYSHADOW2D   { $$ = -TEXTURE_2D_ARRAY_INDEX; }
	;

SWZ_instruction: SWZ maskedDstReg ',' srcReg ',' extendedSwizzle
	{
	   /* FIXME: Is this correct?  Should the extenedSwizzle be applied
	    * FIXME: to the existing swizzle?
	    */
	   $4.Base.Swizzle = $6.swizzle;
	   $4.Base.Negate = $6.mask;

	   $$ = asm_instruction_copy_ctor(& $1, & $2, & $4, NULL, NULL);
	}
	;

scalarSrcReg: optionalSign scalarUse
	{
	   $$ = $2;

	   if ($1) {
	      $$.Base.Negate = ~$$.Base.Negate;
	   }
	}
	| optionalSign '|' scalarUse '|'
	{
	   $$ = $3;

	   if (!state->option.NV_fragment) {
	      yyerror(& @2, state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ($1) {
	      $$.Base.Negate = ~$$.Base.Negate;
	   }

	   $$.Base.Abs = 1;
	}
	;

scalarUse:  srcReg scalarSuffix
	{
	   $$ = $1;

	   $$.Base.Swizzle = _mesa_combine_swizzles($$.Base.Swizzle,
						    $2.swizzle);
	}
	| paramConstScalarUse
	{
	   struct asm_symbol temp_sym;

	   if (!state->option.NV_fragment) {
	      yyerror(& @1, state, "expected scalar suffix");
	      YYERROR;
	   }

	   memset(& temp_sym, 0, sizeof(temp_sym));
	   temp_sym.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & temp_sym, & $1, GL_TRUE);

	   set_src_reg_swz(& $$, PROGRAM_CONSTANT,
                           temp_sym.param_binding_begin,
                           temp_sym.param_binding_swizzle);
	}
	;

swizzleSrcReg: optionalSign srcReg swizzleSuffix
	{
	   $$ = $2;

	   if ($1) {
	      $$.Base.Negate = ~$$.Base.Negate;
	   }

	   $$.Base.Swizzle = _mesa_combine_swizzles($$.Base.Swizzle,
						    $3.swizzle);
	}
	| optionalSign '|' srcReg swizzleSuffix '|'
	{
	   $$ = $3;

	   if (!state->option.NV_fragment) {
	      yyerror(& @2, state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ($1) {
	      $$.Base.Negate = ~$$.Base.Negate;
	   }

	   $$.Base.Abs = 1;
	   $$.Base.Swizzle = _mesa_combine_swizzles($$.Base.Swizzle,
						    $4.swizzle);
	}

	;

maskedDstReg: dstReg optionalMask optionalCcMask
	{
	   $$ = $1;
	   $$.WriteMask = $2.mask;
	   $$.CondMask = $3.CondMask;
	   $$.CondSwizzle = $3.CondSwizzle;
	   $$.CondSrc = $3.CondSrc;

	   if ($$.File == PROGRAM_OUTPUT) {
	      /* Technically speaking, this should check that it is in
	       * vertex program mode.  However, PositionInvariant can never be
	       * set in fragment program mode, so it is somewhat irrelevant.
	       */
	      if (state->option.PositionInvariant
	       && ($$.Index == VERT_RESULT_HPOS)) {
		 yyerror(& @1, state, "position-invariant programs cannot "
			 "write position");
		 YYERROR;
	      }

	      state->prog->OutputsWritten |= BITFIELD64_BIT($$.Index);
	   }
	}
	;

maskedAddrReg: addrReg addrWriteMask
	{
	   set_dst_reg(& $$, PROGRAM_ADDRESS, 0);
	   $$.WriteMask = $2.mask;
	}
	;

extendedSwizzle: extSwizComp ',' extSwizComp ',' extSwizComp ',' extSwizComp
	{
	   const unsigned xyzw_valid =
	      ($1.xyzw_valid << 0)
	      | ($3.xyzw_valid << 1)
	      | ($5.xyzw_valid << 2)
	      | ($7.xyzw_valid << 3);
	   const unsigned rgba_valid =
	      ($1.rgba_valid << 0)
	      | ($3.rgba_valid << 1)
	      | ($5.rgba_valid << 2)
	      | ($7.rgba_valid << 3);

	   /* All of the swizzle components have to be valid in either RGBA
	    * or XYZW.  Note that 0 and 1 are valid in both, so both masks
	    * can have some bits set.
	    *
	    * We somewhat deviate from the spec here.  It would be really hard
	    * to figure out which component is the error, and there probably
	    * isn't a lot of benefit.
	    */
	   if ((rgba_valid != 0x0f) && (xyzw_valid != 0x0f)) {
	      yyerror(& @1, state, "cannot combine RGBA and XYZW swizzle "
		      "components");
	      YYERROR;
	   }

	   $$.swizzle = MAKE_SWIZZLE4($1.swz, $3.swz, $5.swz, $7.swz);
	   $$.mask = ($1.negate) | ($3.negate << 1) | ($5.negate << 2)
	      | ($7.negate << 3);
	}
	;

extSwizComp: optionalSign extSwizSel
	{
	   $$ = $2;
	   $$.negate = ($1) ? 1 : 0;
	}
	;

extSwizSel: INTEGER
	{
	   if (($1 != 0) && ($1 != 1)) {
	      yyerror(& @1, state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   $$.swz = ($1 == 0) ? SWIZZLE_ZERO : SWIZZLE_ONE;

	   /* 0 and 1 are valid for both RGBA swizzle names and XYZW
	    * swizzle names.
	    */
	   $$.xyzw_valid = 1;
	   $$.rgba_valid = 1;
	}
	| string
	{
	   char s;

	   if (strlen($1) > 1) {
	      yyerror(& @1, state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   s = $1[0];
	   free($1);

	   switch (s) {
	   case 'x':
	      $$.swz = SWIZZLE_X;
	      $$.xyzw_valid = 1;
	      break;
	   case 'y':
	      $$.swz = SWIZZLE_Y;
	      $$.xyzw_valid = 1;
	      break;
	   case 'z':
	      $$.swz = SWIZZLE_Z;
	      $$.xyzw_valid = 1;
	      break;
	   case 'w':
	      $$.swz = SWIZZLE_W;
	      $$.xyzw_valid = 1;
	      break;

	   case 'r':
	      $$.swz = SWIZZLE_X;
	      $$.rgba_valid = 1;
	      break;
	   case 'g':
	      $$.swz = SWIZZLE_Y;
	      $$.rgba_valid = 1;
	      break;
	   case 'b':
	      $$.swz = SWIZZLE_Z;
	      $$.rgba_valid = 1;
	      break;
	   case 'a':
	      $$.swz = SWIZZLE_W;
	      $$.rgba_valid = 1;
	      break;

	   default:
	      yyerror(& @1, state, "invalid extended swizzle selector");
	      YYERROR;
	      break;
	   }
	}
	;

srcReg: USED_IDENTIFIER /* temporaryReg | progParamSingle */
	{
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $1);

	   free($1);

	   if (s == NULL) {
	      yyerror(& @1, state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) && (s->type != at_temp)
		      && (s->type != at_attrib)) {
	      yyerror(& @1, state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type == at_param) && s->param_is_array) {
	      yyerror(& @1, state, "non-array access to array PARAM");
	      YYERROR;
	   }

	   init_src_reg(& $$);
	   switch (s->type) {
	   case at_temp:
	      set_src_reg(& $$, PROGRAM_TEMPORARY, s->temp_binding);
	      break;
	   case at_param:
              set_src_reg_swz(& $$, s->param_binding_type,
                              s->param_binding_begin,
                              s->param_binding_swizzle);
	      break;
	   case at_attrib:
	      set_src_reg(& $$, PROGRAM_INPUT, s->attrib_binding);
	      state->prog->InputsRead |= BITFIELD64_BIT($$.Base.Index);

	      if (!validate_inputs(& @1, state)) {
		 YYERROR;
	      }
	      break;

	   default:
	      YYERROR;
	      break;
	   }
	}
	| attribBinding
	{
	   set_src_reg(& $$, PROGRAM_INPUT, $1);
	   state->prog->InputsRead |= BITFIELD64_BIT($$.Base.Index);

	   if (!validate_inputs(& @1, state)) {
	      YYERROR;
	   }
	}
	| progParamArray '[' progParamArrayMem ']'
	{
	   if (! $3.Base.RelAddr
	       && ((unsigned) $3.Base.Index >= $1->param_binding_length)) {
	      yyerror(& @3, state, "out of bounds array access");
	      YYERROR;
	   }

	   init_src_reg(& $$);
	   $$.Base.File = $1->param_binding_type;

	   if ($3.Base.RelAddr) {
              state->prog->IndirectRegisterFiles |= (1 << $$.Base.File);
	      $1->param_accessed_indirectly = 1;

	      $$.Base.RelAddr = 1;
	      $$.Base.Index = $3.Base.Index;
	      $$.Symbol = $1;
	   } else {
	      $$.Base.Index = $1->param_binding_begin + $3.Base.Index;
	   }
	}
	| paramSingleItemUse
	{
           gl_register_file file = ($1.name != NULL) 
	      ? $1.param_binding_type
	      : PROGRAM_CONSTANT;
           set_src_reg_swz(& $$, file, $1.param_binding_begin,
                           $1.param_binding_swizzle);
	}
	;

dstReg: resultBinding
	{
	   set_dst_reg(& $$, PROGRAM_OUTPUT, $1);
	}
	| USED_IDENTIFIER /* temporaryReg | vertexResultReg */
	{
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $1);

	   free($1);

	   if (s == NULL) {
	      yyerror(& @1, state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_output) && (s->type != at_temp)) {
	      yyerror(& @1, state, "invalid operand variable");
	      YYERROR;
	   }

	   switch (s->type) {
	   case at_temp:
	      set_dst_reg(& $$, PROGRAM_TEMPORARY, s->temp_binding);
	      break;
	   case at_output:
	      set_dst_reg(& $$, PROGRAM_OUTPUT, s->output_binding);
	      break;
	   default:
	      set_dst_reg(& $$, s->param_binding_type, s->param_binding_begin);
	      break;
	   }
	}
	;

progParamArray: USED_IDENTIFIER
	{
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $1);

	   free($1);

	   if (s == NULL) {
	      yyerror(& @1, state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) || !s->param_is_array) {
	      yyerror(& @1, state, "array access to non-PARAM variable");
	      YYERROR;
	   } else {
	      $$ = s;
	   }
	}
	;

progParamArrayMem: progParamArrayAbs | progParamArrayRel;

progParamArrayAbs: INTEGER
	{
	   init_src_reg(& $$);
	   $$.Base.Index = $1;
	}
	;

progParamArrayRel: addrReg addrComponent addrRegRelOffset
	{
	   /* FINISHME: Add support for multiple address registers.
	    */
	   /* FINISHME: Add support for 4-component address registers.
	    */
	   init_src_reg(& $$);
	   $$.Base.RelAddr = 1;
	   $$.Base.Index = $3;
	}
	;

addrRegRelOffset:              { $$ = 0; }
	| '+' addrRegPosOffset { $$ = $2; }
	| '-' addrRegNegOffset { $$ = -$2; }
	;

addrRegPosOffset: INTEGER
	{
	   if (($1 < 0) || ($1 > (state->limits->MaxAddressOffset - 1))) {
              char s[100];
              _mesa_snprintf(s, sizeof(s),
                             "relative address offset too large (%d)", $1);
	      yyerror(& @1, state, s);
	      YYERROR;
	   } else {
	      $$ = $1;
	   }
	}
	;

addrRegNegOffset: INTEGER
	{
	   if (($1 < 0) || ($1 > state->limits->MaxAddressOffset)) {
              char s[100];
              _mesa_snprintf(s, sizeof(s),
                             "relative address offset too large (%d)", $1);
	      yyerror(& @1, state, s);
	      YYERROR;
	   } else {
	      $$ = $1;
	   }
	}
	;

addrReg: USED_IDENTIFIER
	{
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $1);

	   free($1);

	   if (s == NULL) {
	      yyerror(& @1, state, "invalid array member");
	      YYERROR;
	   } else if (s->type != at_address) {
	      yyerror(& @1, state,
		      "invalid variable for indexed array access");
	      YYERROR;
	   } else {
	      $$ = s;
	   }
	}
	;

addrComponent: MASK1
	{
	   if ($1.mask != WRITEMASK_X) {
	      yyerror(& @1, state, "invalid address component selector");
	      YYERROR;
	   } else {
	      $$ = $1;
	   }
	}
	;

addrWriteMask: MASK1
	{
	   if ($1.mask != WRITEMASK_X) {
	      yyerror(& @1, state,
		      "address register write mask must be \".x\"");
	      YYERROR;
	   } else {
	      $$ = $1;
	   }
	}
	;

scalarSuffix: MASK1;

swizzleSuffix: MASK1
	| MASK4
	| SWIZZLE
	|              { $$.swizzle = SWIZZLE_NOOP; $$.mask = WRITEMASK_XYZW; }
	;

optionalMask: MASK4 | MASK3 | MASK2 | MASK1 
	|              { $$.swizzle = SWIZZLE_NOOP; $$.mask = WRITEMASK_XYZW; }
	;

optionalCcMask: '(' ccTest ')'
	{
	   $$ = $2;
	}
	| '(' ccTest2 ')'
	{
	   $$ = $2;
	}
	|
	{
	   $$.CondMask = COND_TR;
	   $$.CondSwizzle = SWIZZLE_NOOP;
	   $$.CondSrc = 0;
	}
	;

ccTest: ccMaskRule swizzleSuffix
	{
	   $$ = $1;
	   $$.CondSwizzle = $2.swizzle;
	}
	;

ccTest2: ccMaskRule2 swizzleSuffix
	{
	   $$ = $1;
	   $$.CondSwizzle = $2.swizzle;
	}
	;

ccMaskRule: IDENTIFIER
	{
	   const int cond = _mesa_parse_cc($1);
	   if ((cond == 0) || ($1[2] != '\0')) {
	      char *const err_str =
		 make_error_string("invalid condition code \"%s\"", $1);

	      yyerror(& @1, state, (err_str != NULL)
		      ? err_str : "invalid condition code");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }

	   $$.CondMask = cond;
	   $$.CondSwizzle = SWIZZLE_NOOP;
	   $$.CondSrc = 0;
	}
	;

ccMaskRule2: USED_IDENTIFIER
	{
	   const int cond = _mesa_parse_cc($1);
	   if ((cond == 0) || ($1[2] != '\0')) {
	      char *const err_str =
		 make_error_string("invalid condition code \"%s\"", $1);

	      yyerror(& @1, state, (err_str != NULL)
		      ? err_str : "invalid condition code");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }

	   $$.CondMask = cond;
	   $$.CondSwizzle = SWIZZLE_NOOP;
	   $$.CondSrc = 0;
	}
	;

namingStatement: ATTRIB_statement
	| PARAM_statement
	| TEMP_statement
	| ADDRESS_statement
	| OUTPUT_statement
	| ALIAS_statement
	;

ATTRIB_statement: ATTRIB IDENTIFIER '=' attribBinding
	{
	   struct asm_symbol *const s =
	      declare_variable(state, $2, at_attrib, & @2);

	   if (s == NULL) {
	      free($2);
	      YYERROR;
	   } else {
	      s->attrib_binding = $4;
	      state->InputsBound |= BITFIELD64_BIT(s->attrib_binding);

	      if (!validate_inputs(& @4, state)) {
		 YYERROR;
	      }
	   }
	}
	;

attribBinding: VERTEX vtxAttribItem
	{
	   $$ = $2;
	}
	| FRAGMENT fragAttribItem
	{
	   $$ = $2;
	}
	;

vtxAttribItem: POSITION
	{
	   $$ = VERT_ATTRIB_POS;
	}
	| WEIGHT vtxOptWeightNum
	{
	   $$ = VERT_ATTRIB_WEIGHT;
	}
	| NORMAL
	{
	   $$ = VERT_ATTRIB_NORMAL;
	}
	| COLOR optColorType
	{
	   if (!state->ctx->Extensions.EXT_secondary_color) {
	      yyerror(& @2, state, "GL_EXT_secondary_color not supported");
	      YYERROR;
	   }

	   $$ = VERT_ATTRIB_COLOR0 + $2;
	}
	| FOGCOORD
	{
	   if (!state->ctx->Extensions.EXT_fog_coord) {
	      yyerror(& @1, state, "GL_EXT_fog_coord not supported");
	      YYERROR;
	   }

	   $$ = VERT_ATTRIB_FOG;
	}
	| TEXCOORD optTexCoordUnitNum
	{
	   $$ = VERT_ATTRIB_TEX0 + $2;
	}
	| MATRIXINDEX '[' vtxWeightNum ']'
	{
	   yyerror(& @1, state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	}
	| VTXATTRIB '[' vtxAttribNum ']'
	{
	   $$ = VERT_ATTRIB_GENERIC0 + $3;
	}
	;

vtxAttribNum: INTEGER
	{
	   if ((unsigned) $1 >= state->limits->MaxAttribs) {
	      yyerror(& @1, state, "invalid vertex attribute reference");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

vtxOptWeightNum:  | '[' vtxWeightNum ']';
vtxWeightNum: INTEGER;

fragAttribItem: POSITION
	{
	   $$ = FRAG_ATTRIB_WPOS;
	}
	| COLOR optColorType
	{
	   $$ = FRAG_ATTRIB_COL0 + $2;
	}
	| FOGCOORD
	{
	   $$ = FRAG_ATTRIB_FOGC;
	}
	| TEXCOORD optTexCoordUnitNum
	{
	   $$ = FRAG_ATTRIB_TEX0 + $2;
	}
	;

PARAM_statement: PARAM_singleStmt | PARAM_multipleStmt;

PARAM_singleStmt: PARAM IDENTIFIER paramSingleInit
	{
	   struct asm_symbol *const s =
	      declare_variable(state, $2, at_param, & @2);

	   if (s == NULL) {
	      free($2);
	      YYERROR;
	   } else {
	      s->param_binding_type = $3.param_binding_type;
	      s->param_binding_begin = $3.param_binding_begin;
	      s->param_binding_length = $3.param_binding_length;
              s->param_binding_swizzle = $3.param_binding_swizzle;
	      s->param_is_array = 0;
	   }
	}
	;

PARAM_multipleStmt: PARAM IDENTIFIER '[' optArraySize ']' paramMultipleInit
	{
	   if (($4 != 0) && ((unsigned) $4 != $6.param_binding_length)) {
	      free($2);
	      yyerror(& @4, state, 
		      "parameter array size and number of bindings must match");
	      YYERROR;
	   } else {
	      struct asm_symbol *const s =
		 declare_variable(state, $2, $6.type, & @2);

	      if (s == NULL) {
		 free($2);
		 YYERROR;
	      } else {
		 s->param_binding_type = $6.param_binding_type;
		 s->param_binding_begin = $6.param_binding_begin;
		 s->param_binding_length = $6.param_binding_length;
                 s->param_binding_swizzle = SWIZZLE_XYZW;
		 s->param_is_array = 1;
	      }
	   }
	}
	;

optArraySize:
	{
	   $$ = 0;
	}
	| INTEGER
        {
	   if (($1 < 1) || ((unsigned) $1 > state->limits->MaxParameters)) {
              char msg[100];
              _mesa_snprintf(msg, sizeof(msg),
                             "invalid parameter array size (size=%d max=%u)",
                             $1, state->limits->MaxParameters);
	      yyerror(& @1, state, msg);
	      YYERROR;
	   } else {
	      $$ = $1;
	   }
	}
	;

paramSingleInit: '=' paramSingleItemDecl
	{
	   $$ = $2;
	}
	;

paramMultipleInit: '=' '{' paramMultInitList '}'
	{
	   $$ = $3;
	}
	;

paramMultInitList: paramMultipleItem
	| paramMultInitList ',' paramMultipleItem
	{
	   $1.param_binding_length += $3.param_binding_length;
	   $$ = $1;
	}
	;

paramSingleItemDecl: stateSingleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & $$, $1);
	}
	| programSingleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & $$, $1);
	}
	| paramConstDecl
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & $$, & $1, GL_TRUE);
	}
	;

paramSingleItemUse: stateSingleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & $$, $1);
	}
	| programSingleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & $$, $1);
	}
	| paramConstUse
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & $$, & $1, GL_TRUE);
	}
	;

paramMultipleItem: stateMultipleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & $$, $1);
	}
	| programMultipleItem
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & $$, $1);
	}
	| paramConstDecl
	{
	   memset(& $$, 0, sizeof($$));
	   $$.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & $$, & $1, GL_FALSE);
	}
	;

stateMultipleItem: stateSingleItem        { memcpy($$, $1, sizeof($$)); }
	| STATE stateMatrixRows           { memcpy($$, $2, sizeof($$)); }
	;

stateSingleItem: STATE stateMaterialItem  { memcpy($$, $2, sizeof($$)); }
	| STATE stateLightItem            { memcpy($$, $2, sizeof($$)); }
	| STATE stateLightModelItem       { memcpy($$, $2, sizeof($$)); }
	| STATE stateLightProdItem        { memcpy($$, $2, sizeof($$)); }
	| STATE stateTexGenItem           { memcpy($$, $2, sizeof($$)); }
	| STATE stateTexEnvItem           { memcpy($$, $2, sizeof($$)); }
	| STATE stateFogItem              { memcpy($$, $2, sizeof($$)); }
	| STATE stateClipPlaneItem        { memcpy($$, $2, sizeof($$)); }
	| STATE statePointItem            { memcpy($$, $2, sizeof($$)); }
	| STATE stateMatrixRow            { memcpy($$, $2, sizeof($$)); }
	| STATE stateDepthItem            { memcpy($$, $2, sizeof($$)); }
	;

stateMaterialItem: MATERIAL optFaceType stateMatProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_MATERIAL;
	   $$[1] = $2;
	   $$[2] = $3;
	}
	;

stateMatProperty: ambDiffSpecProperty
	{
	   $$ = $1;
	}
	| EMISSION
	{
	   $$ = STATE_EMISSION;
	}
	| SHININESS
	{
	   $$ = STATE_SHININESS;
	}
	;

stateLightItem: LIGHT '[' stateLightNumber ']' stateLightProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_LIGHT;
	   $$[1] = $3;
	   $$[2] = $5;
	}
	;

stateLightProperty: ambDiffSpecProperty
	{
	   $$ = $1;
	}
	| POSITION
	{
	   $$ = STATE_POSITION;
	}
	| ATTENUATION
	{
	   if (!state->ctx->Extensions.EXT_point_parameters) {
	      yyerror(& @1, state, "GL_ARB_point_parameters not supported");
	      YYERROR;
	   }

	   $$ = STATE_ATTENUATION;
	}
	| SPOT stateSpotProperty
	{
	   $$ = $2;
	}
	| HALF
	{
	   $$ = STATE_HALF_VECTOR;
	}
	;

stateSpotProperty: DIRECTION
	{
	   $$ = STATE_SPOT_DIRECTION;
	}
	;

stateLightModelItem: LIGHTMODEL stateLModProperty
	{
	   $$[0] = $2[0];
	   $$[1] = $2[1];
	}
	;

stateLModProperty: AMBIENT
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_LIGHTMODEL_AMBIENT;
	}
	| optFaceType SCENECOLOR
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_LIGHTMODEL_SCENECOLOR;
	   $$[1] = $1;
	}
	;

stateLightProdItem: LIGHTPROD '[' stateLightNumber ']' optFaceType stateLProdProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_LIGHTPROD;
	   $$[1] = $3;
	   $$[2] = $5;
	   $$[3] = $6;
	}
	;

stateLProdProperty: ambDiffSpecProperty;

stateTexEnvItem: TEXENV optLegacyTexUnitNum stateTexEnvProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = $3;
	   $$[1] = $2;
	}
	;

stateTexEnvProperty: COLOR
	{
	   $$ = STATE_TEXENV_COLOR;
	}
	;

ambDiffSpecProperty: AMBIENT
	{
	   $$ = STATE_AMBIENT;
	}
	| DIFFUSE
	{
	   $$ = STATE_DIFFUSE;
	}
	| SPECULAR
	{
	   $$ = STATE_SPECULAR;
	}
	;

stateLightNumber: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxLights) {
	      yyerror(& @1, state, "invalid light selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

stateTexGenItem: TEXGEN optTexCoordUnitNum stateTexGenType stateTexGenCoord
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_TEXGEN;
	   $$[1] = $2;
	   $$[2] = $3 + $4;
	}
	;

stateTexGenType: EYE
	{
	   $$ = STATE_TEXGEN_EYE_S;
	}
	| OBJECT
	{
	   $$ = STATE_TEXGEN_OBJECT_S;
	}
	;
stateTexGenCoord: TEXGEN_S
	{
	   $$ = STATE_TEXGEN_EYE_S - STATE_TEXGEN_EYE_S;
	}
	| TEXGEN_T
	{
	   $$ = STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S;
	}
	| TEXGEN_R
	{
	   $$ = STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S;
	}
	| TEXGEN_Q
	{
	   $$ = STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S;
	}
	;

stateFogItem: FOG stateFogProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = $2;
	}
	;

stateFogProperty: COLOR
	{
	   $$ = STATE_FOG_COLOR;
	}
	| PARAMS
	{
	   $$ = STATE_FOG_PARAMS;
	}
	;

stateClipPlaneItem: CLIP '[' stateClipPlaneNum ']' PLANE
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_CLIPPLANE;
	   $$[1] = $3;
	}
	;

stateClipPlaneNum: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxClipPlanes) {
	      yyerror(& @1, state, "invalid clip plane selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

statePointItem: POINT_TOK statePointProperty
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = $2;
	}
	;

statePointProperty: SIZE_TOK
	{
	   $$ = STATE_POINT_SIZE;
	}
	| ATTENUATION
	{
	   $$ = STATE_POINT_ATTENUATION;
	}
	;

stateMatrixRow: stateMatrixItem ROW '[' stateMatrixRowNum ']'
	{
	   $$[0] = $1[0];
	   $$[1] = $1[1];
	   $$[2] = $4;
	   $$[3] = $4;
	   $$[4] = $1[2];
	}
	;

stateMatrixRows: stateMatrixItem optMatrixRows
	{
	   $$[0] = $1[0];
	   $$[1] = $1[1];
	   $$[2] = $2[2];
	   $$[3] = $2[3];
	   $$[4] = $1[2];
	}
	;

optMatrixRows:
	{
	   $$[2] = 0;
	   $$[3] = 3;
	}
	| ROW '[' stateMatrixRowNum DOT_DOT stateMatrixRowNum ']'
	{
	   /* It seems logical that the matrix row range specifier would have
	    * to specify a range or more than one row (i.e., $5 > $3).
	    * However, the ARB_vertex_program spec says "a program will fail
	    * to load if <a> is greater than <b>."  This means that $3 == $5
	    * is valid.
	    */
	   if ($3 > $5) {
	      yyerror(& @3, state, "invalid matrix row range");
	      YYERROR;
	   }

	   $$[2] = $3;
	   $$[3] = $5;
	}
	;

stateMatrixItem: MATRIX stateMatrixName stateOptMatModifier
	{
	   $$[0] = $2[0];
	   $$[1] = $2[1];
	   $$[2] = $3;
	}
	;

stateOptMatModifier: 
	{
	   $$ = 0;
	}
	| stateMatModifier
	{
	   $$ = $1;
	}
	;

stateMatModifier: INVERSE 
	{
	   $$ = STATE_MATRIX_INVERSE;
	}
	| TRANSPOSE 
	{
	   $$ = STATE_MATRIX_TRANSPOSE;
	}
	| INVTRANS
	{
	   $$ = STATE_MATRIX_INVTRANS;
	}
	;

stateMatrixRowNum: INTEGER
	{
	   if ($1 > 3) {
	      yyerror(& @1, state, "invalid matrix row reference");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

stateMatrixName: MODELVIEW stateOptModMatNum
	{
	   $$[0] = STATE_MODELVIEW_MATRIX;
	   $$[1] = $2;
	}
	| PROJECTION
	{
	   $$[0] = STATE_PROJECTION_MATRIX;
	   $$[1] = 0;
	}
	| MVP
	{
	   $$[0] = STATE_MVP_MATRIX;
	   $$[1] = 0;
	}
	| TEXTURE optTexCoordUnitNum
	{
	   $$[0] = STATE_TEXTURE_MATRIX;
	   $$[1] = $2;
	}
	| PALETTE '[' statePaletteMatNum ']'
	{
	   yyerror(& @1, state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	}
	| MAT_PROGRAM '[' stateProgramMatNum ']'
	{
	   $$[0] = STATE_PROGRAM_MATRIX;
	   $$[1] = $3;
	}
	;

stateOptModMatNum:
	{
	   $$ = 0;
	}
	| '[' stateModMatNum ']'
	{
	   $$ = $2;
	}
	;
stateModMatNum: INTEGER
	{
	   /* Since GL_ARB_vertex_blend isn't supported, only modelview matrix
	    * zero is valid.
	    */
	   if ($1 != 0) {
	      yyerror(& @1, state, "invalid modelview matrix index");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;
statePaletteMatNum: INTEGER
	{
	   /* Since GL_ARB_matrix_palette isn't supported, just let any value
	    * through here.  The error will be generated later.
	    */
	   $$ = $1;
	}
	;
stateProgramMatNum: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxProgramMatrices) {
	      yyerror(& @1, state, "invalid program matrix selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

stateDepthItem: DEPTH RANGE
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = STATE_DEPTH_RANGE;
	}
	;


programSingleItem: progEnvParam | progLocalParam;

programMultipleItem: progEnvParams | progLocalParams;

progEnvParams: PROGRAM ENV '[' progEnvParamNums ']'
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = state->state_param_enum;
	   $$[1] = STATE_ENV;
	   $$[2] = $4[0];
	   $$[3] = $4[1];
	}
	;

progEnvParamNums: progEnvParamNum
	{
	   $$[0] = $1;
	   $$[1] = $1;
	}
	| progEnvParamNum DOT_DOT progEnvParamNum
	{
	   $$[0] = $1;
	   $$[1] = $3;
	}
	;

progEnvParam: PROGRAM ENV '[' progEnvParamNum ']'
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = state->state_param_enum;
	   $$[1] = STATE_ENV;
	   $$[2] = $4;
	   $$[3] = $4;
	}
	;

progLocalParams: PROGRAM LOCAL '[' progLocalParamNums ']'
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = state->state_param_enum;
	   $$[1] = STATE_LOCAL;
	   $$[2] = $4[0];
	   $$[3] = $4[1];
	}

progLocalParamNums: progLocalParamNum
	{
	   $$[0] = $1;
	   $$[1] = $1;
	}
	| progLocalParamNum DOT_DOT progLocalParamNum
	{
	   $$[0] = $1;
	   $$[1] = $3;
	}
	;

progLocalParam: PROGRAM LOCAL '[' progLocalParamNum ']'
	{
	   memset($$, 0, sizeof($$));
	   $$[0] = state->state_param_enum;
	   $$[1] = STATE_LOCAL;
	   $$[2] = $4;
	   $$[3] = $4;
	}
	;

progEnvParamNum: INTEGER
	{
	   if ((unsigned) $1 >= state->limits->MaxEnvParams) {
	      yyerror(& @1, state, "invalid environment parameter reference");
	      YYERROR;
	   }
	   $$ = $1;
	}
	;

progLocalParamNum: INTEGER
	{
	   if ((unsigned) $1 >= state->limits->MaxLocalParams) {
	      yyerror(& @1, state, "invalid local parameter reference");
	      YYERROR;
	   }
	   $$ = $1;
	}
	;



paramConstDecl: paramConstScalarDecl | paramConstVector;
paramConstUse: paramConstScalarUse | paramConstVector;

paramConstScalarDecl: signedFloatConstant
	{
	   $$.count = 4;
	   $$.data[0].f = $1;
	   $$.data[1].f = $1;
	   $$.data[2].f = $1;
	   $$.data[3].f = $1;
	}
	;

paramConstScalarUse: REAL
	{
	   $$.count = 1;
	   $$.data[0].f = $1;
	   $$.data[1].f = $1;
	   $$.data[2].f = $1;
	   $$.data[3].f = $1;
	}
	| INTEGER
	{
	   $$.count = 1;
	   $$.data[0].f = (float) $1;
	   $$.data[1].f = (float) $1;
	   $$.data[2].f = (float) $1;
	   $$.data[3].f = (float) $1;
	}
	;

paramConstVector: '{' signedFloatConstant '}'
	{
	   $$.count = 4;
	   $$.data[0].f = $2;
	   $$.data[1].f = 0.0f;
	   $$.data[2].f = 0.0f;
	   $$.data[3].f = 1.0f;
	}
	| '{' signedFloatConstant ',' signedFloatConstant '}'
	{
	   $$.count = 4;
	   $$.data[0].f = $2;
	   $$.data[1].f = $4;
	   $$.data[2].f = 0.0f;
	   $$.data[3].f = 1.0f;
	}
	| '{' signedFloatConstant ',' signedFloatConstant ','
              signedFloatConstant '}'
	{
	   $$.count = 4;
	   $$.data[0].f = $2;
	   $$.data[1].f = $4;
	   $$.data[2].f = $6;
	   $$.data[3].f = 1.0f;
	}
	| '{' signedFloatConstant ',' signedFloatConstant ','
              signedFloatConstant ',' signedFloatConstant '}'
	{
	   $$.count = 4;
	   $$.data[0].f = $2;
	   $$.data[1].f = $4;
	   $$.data[2].f = $6;
	   $$.data[3].f = $8;
	}
	;

signedFloatConstant: optionalSign REAL
	{
	   $$ = ($1) ? -$2 : $2;
	}
	| optionalSign INTEGER
	{
	   $$ = (float)(($1) ? -$2 : $2);
	}
	;

optionalSign: '+'        { $$ = FALSE; }
	| '-'            { $$ = TRUE;  }
	|                { $$ = FALSE; }
	;

TEMP_statement: optVarSize TEMP { $<integer>$ = $2; } varNameList
	;

optVarSize: string
	{
	   /* NV_fragment_program_option defines the size qualifiers in a
	    * fairly broken way.  "SHORT" or "LONG" can optionally be used
	    * before TEMP or OUTPUT.  However, neither is a reserved word!
	    * This means that we have to parse it as an identifier, then check
	    * to make sure it's one of the valid values.  *sigh*
	    *
	    * In addition, the grammar in the extension spec does *not* allow
	    * the size specifier to be optional, but all known implementations
	    * do.
	    */
	   if (!state->option.NV_fragment) {
	      yyerror(& @1, state, "unexpected IDENTIFIER");
	      YYERROR;
	   }

	   if (strcmp("SHORT", $1) == 0) {
	   } else if (strcmp("LONG", $1) == 0) {
	   } else {
	      char *const err_str =
		 make_error_string("invalid storage size specifier \"%s\"",
				   $1);

	      yyerror(& @1, state, (err_str != NULL)
		      ? err_str : "invalid storage size specifier");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }
	}
	|
	{
	}
	;

ADDRESS_statement: ADDRESS { $<integer>$ = $1; } varNameList
	;

varNameList: varNameList ',' IDENTIFIER
	{
	   if (!declare_variable(state, $3, $<integer>0, & @3)) {
	      free($3);
	      YYERROR;
	   }
	}
	| IDENTIFIER
	{
	   if (!declare_variable(state, $1, $<integer>0, & @1)) {
	      free($1);
	      YYERROR;
	   }
	}
	;

OUTPUT_statement: optVarSize OUTPUT IDENTIFIER '=' resultBinding
	{
	   struct asm_symbol *const s =
	      declare_variable(state, $3, at_output, & @3);

	   if (s == NULL) {
	      free($3);
	      YYERROR;
	   } else {
	      s->output_binding = $5;
	   }
	}
	;

resultBinding: RESULT POSITION
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_HPOS;
	   } else {
	      yyerror(& @2, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| RESULT FOGCOORD
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_FOGC;
	   } else {
	      yyerror(& @2, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| RESULT resultColBinding
	{
	   $$ = $2;
	}
	| RESULT POINTSIZE
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_PSIZ;
	   } else {
	      yyerror(& @2, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| RESULT TEXCOORD optTexCoordUnitNum
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_TEX0 + $3;
	   } else {
	      yyerror(& @2, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| RESULT DEPTH
	{
	   if (state->mode == ARB_fragment) {
	      $$ = FRAG_RESULT_DEPTH;
	   } else {
	      yyerror(& @2, state, "invalid program result name");
	      YYERROR;
	   }
	}
	;

resultColBinding: COLOR optResultFaceType optResultColorType
	{
	   $$ = $2 + $3;
	}
	;

optResultFaceType:
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_COL0;
	   } else {
	      if (state->option.DrawBuffers)
		 $$ = FRAG_RESULT_DATA0;
	      else
		 $$ = FRAG_RESULT_COLOR;
	   }
	}
	| '[' INTEGER ']'
	{
	   if (state->mode == ARB_vertex) {
	      yyerror(& @1, state, "invalid program result name");
	      YYERROR;
	   } else {
	      if (!state->option.DrawBuffers) {
		 /* From the ARB_draw_buffers spec (same text exists
		  * for ATI_draw_buffers):
		  *
		  *     If this option is not specified, a fragment
		  *     program that attempts to bind
		  *     "result.color[n]" will fail to load, and only
		  *     "result.color" will be allowed.
		  */
		 yyerror(& @1, state,
			 "result.color[] used without "
			 "`OPTION ARB_draw_buffers' or "
			 "`OPTION ATI_draw_buffers'");
		 YYERROR;
	      } else if ($2 >= state->MaxDrawBuffers) {
		 yyerror(& @1, state,
			 "result.color[] exceeds MAX_DRAW_BUFFERS_ARB");
		 YYERROR;
	      }
	      $$ = FRAG_RESULT_DATA0 + $2;
	   }
	}
	| FRONT
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_COL0;
	   } else {
	      yyerror(& @1, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| BACK
	{
	   if (state->mode == ARB_vertex) {
	      $$ = VERT_RESULT_BFC0;
	   } else {
	      yyerror(& @1, state, "invalid program result name");
	      YYERROR;
	   }
	}
	;

optResultColorType:
	{
	   $$ = 0; 
	}
	| PRIMARY
	{
	   if (state->mode == ARB_vertex) {
	      $$ = 0;
	   } else {
	      yyerror(& @1, state, "invalid program result name");
	      YYERROR;
	   }
	}
	| SECONDARY
	{
	   if (state->mode == ARB_vertex) {
	      $$ = 1;
	   } else {
	      yyerror(& @1, state, "invalid program result name");
	      YYERROR;
	   }
	}
	;

optFaceType:    { $$ = 0; }
	| FRONT	{ $$ = 0; }
	| BACK  { $$ = 1; }
	;

optColorType:       { $$ = 0; }
	| PRIMARY   { $$ = 0; }
	| SECONDARY { $$ = 1; }
	;

optTexCoordUnitNum:                { $$ = 0; }
	| '[' texCoordUnitNum ']'  { $$ = $2; }
	;

optTexImageUnitNum:                { $$ = 0; }
	| '[' texImageUnitNum ']'  { $$ = $2; }
	;

optLegacyTexUnitNum:               { $$ = 0; }
	| '[' legacyTexUnitNum ']' { $$ = $2; }
	;

texCoordUnitNum: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxTextureCoordUnits) {
	      yyerror(& @1, state, "invalid texture coordinate unit selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

texImageUnitNum: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxTextureImageUnits) {
	      yyerror(& @1, state, "invalid texture image unit selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

legacyTexUnitNum: INTEGER
	{
	   if ((unsigned) $1 >= state->MaxTextureUnits) {
	      yyerror(& @1, state, "invalid texture unit selector");
	      YYERROR;
	   }

	   $$ = $1;
	}
	;

ALIAS_statement: ALIAS IDENTIFIER '=' USED_IDENTIFIER
	{
	   struct asm_symbol *exist = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $2);
	   struct asm_symbol *target = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, $4);

	   free($4);

	   if (exist != NULL) {
	      char m[1000];
	      _mesa_snprintf(m, sizeof(m), "redeclared identifier: %s", $2);
	      free($2);
	      yyerror(& @2, state, m);
	      YYERROR;
	   } else if (target == NULL) {
	      free($2);
	      yyerror(& @4, state,
		      "undefined variable binding in ALIAS statement");
	      YYERROR;
	   } else {
	      _mesa_symbol_table_add_symbol(state->st, 0, $2, target);
	   }
	}
	;

string: IDENTIFIER
	| USED_IDENTIFIER
	;

%%

void
asm_instruction_set_operands(struct asm_instruction *inst,
			     const struct prog_dst_register *dst,
			     const struct asm_src_register *src0,
			     const struct asm_src_register *src1,
			     const struct asm_src_register *src2)
{
   /* In the core ARB extensions only the KIL instruction doesn't have a
    * destination register.
    */
   if (dst == NULL) {
      init_dst_reg(& inst->Base.DstReg);
   } else {
      inst->Base.DstReg = *dst;
   }

   /* The only instruction that doesn't have any source registers is the
    * condition-code based KIL instruction added by NV_fragment_program_option.
    */
   if (src0 != NULL) {
      inst->Base.SrcReg[0] = src0->Base;
      inst->SrcReg[0] = *src0;
   } else {
      init_src_reg(& inst->SrcReg[0]);
   }

   if (src1 != NULL) {
      inst->Base.SrcReg[1] = src1->Base;
      inst->SrcReg[1] = *src1;
   } else {
      init_src_reg(& inst->SrcReg[1]);
   }

   if (src2 != NULL) {
      inst->Base.SrcReg[2] = src2->Base;
      inst->SrcReg[2] = *src2;
   } else {
      init_src_reg(& inst->SrcReg[2]);
   }
}


struct asm_instruction *
asm_instruction_ctor(gl_inst_opcode op,
		     const struct prog_dst_register *dst,
		     const struct asm_src_register *src0,
		     const struct asm_src_register *src1,
		     const struct asm_src_register *src2)
{
   struct asm_instruction *inst = CALLOC_STRUCT(asm_instruction);

   if (inst) {
      _mesa_init_instructions(& inst->Base, 1);
      inst->Base.Opcode = op;

      asm_instruction_set_operands(inst, dst, src0, src1, src2);
   }

   return inst;
}


struct asm_instruction *
asm_instruction_copy_ctor(const struct prog_instruction *base,
			  const struct prog_dst_register *dst,
			  const struct asm_src_register *src0,
			  const struct asm_src_register *src1,
			  const struct asm_src_register *src2)
{
   struct asm_instruction *inst = CALLOC_STRUCT(asm_instruction);

   if (inst) {
      _mesa_init_instructions(& inst->Base, 1);
      inst->Base.Opcode = base->Opcode;
      inst->Base.CondUpdate = base->CondUpdate;
      inst->Base.CondDst = base->CondDst;
      inst->Base.SaturateMode = base->SaturateMode;
      inst->Base.Precision = base->Precision;

      asm_instruction_set_operands(inst, dst, src0, src1, src2);
   }

   return inst;
}


void
init_dst_reg(struct prog_dst_register *r)
{
   memset(r, 0, sizeof(*r));
   r->File = PROGRAM_UNDEFINED;
   r->WriteMask = WRITEMASK_XYZW;
   r->CondMask = COND_TR;
   r->CondSwizzle = SWIZZLE_NOOP;
}


/** Like init_dst_reg() but set the File and Index fields. */
void
set_dst_reg(struct prog_dst_register *r, gl_register_file file, GLint index)
{
   const GLint maxIndex = 1 << INST_INDEX_BITS;
   const GLint minIndex = 0;
   ASSERT(index >= minIndex);
   (void) minIndex;
   ASSERT(index <= maxIndex);
   (void) maxIndex;
   ASSERT(file == PROGRAM_TEMPORARY ||
	  file == PROGRAM_ADDRESS ||
	  file == PROGRAM_OUTPUT);
   memset(r, 0, sizeof(*r));
   r->File = file;
   r->Index = index;
   r->WriteMask = WRITEMASK_XYZW;
   r->CondMask = COND_TR;
   r->CondSwizzle = SWIZZLE_NOOP;
}


void
init_src_reg(struct asm_src_register *r)
{
   memset(r, 0, sizeof(*r));
   r->Base.File = PROGRAM_UNDEFINED;
   r->Base.Swizzle = SWIZZLE_NOOP;
   r->Symbol = NULL;
}


/** Like init_src_reg() but set the File and Index fields.
 * \return GL_TRUE if a valid src register, GL_FALSE otherwise
 */
void
set_src_reg(struct asm_src_register *r, gl_register_file file, GLint index)
{
   set_src_reg_swz(r, file, index, SWIZZLE_XYZW);
}


void
set_src_reg_swz(struct asm_src_register *r, gl_register_file file, GLint index,
                GLuint swizzle)
{
   const GLint maxIndex = (1 << INST_INDEX_BITS) - 1;
   const GLint minIndex = -(1 << INST_INDEX_BITS);
   ASSERT(file < PROGRAM_FILE_MAX);
   ASSERT(index >= minIndex);
   (void) minIndex;
   ASSERT(index <= maxIndex);
   (void) maxIndex;
   memset(r, 0, sizeof(*r));
   r->Base.File = file;
   r->Base.Index = index;
   r->Base.Swizzle = swizzle;
   r->Symbol = NULL;
}


/**
 * Validate the set of inputs used by a program
 *
 * Validates that legal sets of inputs are used by the program.  In this case
 * "used" included both reading the input or binding the input to a name using
 * the \c ATTRIB command.
 *
 * \return
 * \c TRUE if the combination of inputs used is valid, \c FALSE otherwise.
 */
int
validate_inputs(struct YYLTYPE *locp, struct asm_parser_state *state)
{
   const GLbitfield64 inputs = state->prog->InputsRead | state->InputsBound;

   if (((inputs & VERT_BIT_FF_ALL) & (inputs >> VERT_ATTRIB_GENERIC0)) != 0) {
      yyerror(locp, state, "illegal use of generic attribute and name attribute");
      return 0;
   }

   return 1;
}


struct asm_symbol *
declare_variable(struct asm_parser_state *state, char *name, enum asm_type t,
		 struct YYLTYPE *locp)
{
   struct asm_symbol *s = NULL;
   struct asm_symbol *exist = (struct asm_symbol *)
      _mesa_symbol_table_find_symbol(state->st, 0, name);


   if (exist != NULL) {
      yyerror(locp, state, "redeclared identifier");
   } else {
      s = calloc(1, sizeof(struct asm_symbol));
      s->name = name;
      s->type = t;

      switch (t) {
      case at_temp:
	 if (state->prog->NumTemporaries >= state->limits->MaxTemps) {
	    yyerror(locp, state, "too many temporaries declared");
	    free(s);
	    return NULL;
	 }

	 s->temp_binding = state->prog->NumTemporaries;
	 state->prog->NumTemporaries++;
	 break;

      case at_address:
	 if (state->prog->NumAddressRegs >= state->limits->MaxAddressRegs) {
	    yyerror(locp, state, "too many address registers declared");
	    free(s);
	    return NULL;
	 }

	 /* FINISHME: Add support for multiple address registers.
	  */
	 state->prog->NumAddressRegs++;
	 break;

      default:
	 break;
      }

      _mesa_symbol_table_add_symbol(state->st, 0, s->name, s);
      s->next = state->sym;
      state->sym = s;
   }

   return s;
}


int add_state_reference(struct gl_program_parameter_list *param_list,
			const gl_state_index tokens[STATE_LENGTH])
{
   const GLuint size = 4; /* XXX fix */
   char *name;
   GLint index;

   name = _mesa_program_state_string(tokens);
   index = _mesa_add_parameter(param_list, PROGRAM_STATE_VAR, name,
                               size, GL_NONE, NULL, tokens, 0x0);
   param_list->StateFlags |= _mesa_program_state_flags(tokens);

   /* free name string here since we duplicated it in add_parameter() */
   free(name);

   return index;
}


int
initialize_symbol_from_state(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* If we are adding a STATE_MATRIX that has multiple rows, we need to
    * unroll it and call add_state_reference() for each row
    */
   if ((state_tokens[0] == STATE_MODELVIEW_MATRIX ||
	state_tokens[0] == STATE_PROJECTION_MATRIX ||
	state_tokens[0] == STATE_MVP_MATRIX ||
	state_tokens[0] == STATE_TEXTURE_MATRIX ||
	state_tokens[0] == STATE_PROGRAM_MATRIX)
       && (state_tokens[2] != state_tokens[3])) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U) {
	    param_var->param_binding_begin = idx;
            param_var->param_binding_swizzle = SWIZZLE_XYZW;
         }

	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U) {
	 param_var->param_binding_begin = idx;
         param_var->param_binding_swizzle = SWIZZLE_XYZW;
      }
      param_var->param_binding_length++;
   }

   return idx;
}


int
initialize_symbol_from_param(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   assert((state_tokens[0] == STATE_VERTEX_PROGRAM)
	  || (state_tokens[0] == STATE_FRAGMENT_PROGRAM));
   assert((state_tokens[1] == STATE_ENV)
	  || (state_tokens[1] == STATE_LOCAL));

   /*
    * The param type is STATE_VAR.  The program parameter entry will
    * effectively be a pointer into the LOCAL or ENV parameter array.
    */
   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* If we are adding a STATE_ENV or STATE_LOCAL that has multiple elements,
    * we need to unroll it and call add_state_reference() for each row
    */
   if (state_tokens[2] != state_tokens[3]) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U) {
	    param_var->param_binding_begin = idx;
            param_var->param_binding_swizzle = SWIZZLE_XYZW;
         }
	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U) {
	 param_var->param_binding_begin = idx;
         param_var->param_binding_swizzle = SWIZZLE_XYZW;
      }
      param_var->param_binding_length++;
   }

   return idx;
}


/**
 * Put a float/vector constant/literal into the parameter list.
 * \param param_var  returns info about the parameter/constant's location,
 *                   binding, type, etc.
 * \param vec  the vector/constant to add
 * \param allowSwizzle  if true, try to consolidate constants which only differ
 *                      by a swizzle.  We don't want to do this when building
 *                      arrays of constants that may be indexed indirectly.
 * \return index of the constant in the parameter list.
 */
int
initialize_symbol_from_const(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const struct asm_vector *vec,
                             GLboolean allowSwizzle)
{
   unsigned swizzle;
   const int idx = _mesa_add_unnamed_constant(prog->Parameters,
                                              vec->data, vec->count,
                                              allowSwizzle ? &swizzle : NULL);

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_CONSTANT;

   if (param_var->param_binding_begin == ~0U) {
      param_var->param_binding_begin = idx;
      param_var->param_binding_swizzle = allowSwizzle ? swizzle : SWIZZLE_XYZW;
   }
   param_var->param_binding_length++;

   return idx;
}


char *
make_error_string(const char *fmt, ...)
{
   int length;
   char *str;
   va_list args;


   /* Call vsnprintf once to determine how large the final string is.  Call it
    * again to do the actual formatting.  from the vsnprintf manual page:
    *
    *    Upon successful return, these functions return the number of
    *    characters printed  (not including the trailing '\0' used to end
    *    output to strings).
    */
   va_start(args, fmt);
   length = 1 + vsnprintf(NULL, 0, fmt, args);
   va_end(args);

   str = malloc(length);
   if (str) {
      va_start(args, fmt);
      vsnprintf(str, length, fmt, args);
      va_end(args);
   }

   return str;
}


void
yyerror(YYLTYPE *locp, struct asm_parser_state *state, const char *s)
{
   char *err_str;


   err_str = make_error_string("glProgramStringARB(%s)\n", s);
   if (err_str) {
      _mesa_error(state->ctx, GL_INVALID_OPERATION, "%s", err_str);
      free(err_str);
   }

   err_str = make_error_string("line %u, char %u: error: %s\n",
			       locp->first_line, locp->first_column, s);
   _mesa_set_program_error(state->ctx, locp->position, err_str);

   if (err_str) {
      free(err_str);
   }
}


GLboolean
_mesa_parse_arb_program(struct gl_context *ctx, GLenum target, const GLubyte *str,
			GLsizei len, struct asm_parser_state *state)
{
   struct asm_instruction *inst;
   unsigned i;
   GLubyte *strz;
   GLboolean result = GL_FALSE;
   void *temp;
   struct asm_symbol *sym;

   state->ctx = ctx;
   state->prog->Target = target;
   state->prog->Parameters = _mesa_new_parameter_list();

   /* Make a copy of the program string and force it to be NUL-terminated.
    */
   strz = (GLubyte *) malloc(len + 1);
   if (strz == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return GL_FALSE;
   }
   memcpy (strz, str, len);
   strz[len] = '\0';

   state->prog->String = strz;

   state->st = _mesa_symbol_table_ctor();

   state->limits = (target == GL_VERTEX_PROGRAM_ARB)
      ? & ctx->Const.VertexProgram
      : & ctx->Const.FragmentProgram;

   state->MaxTextureImageUnits = ctx->Const.MaxTextureImageUnits;
   state->MaxTextureCoordUnits = ctx->Const.MaxTextureCoordUnits;
   state->MaxTextureUnits = ctx->Const.MaxTextureUnits;
   state->MaxClipPlanes = ctx->Const.MaxClipPlanes;
   state->MaxLights = ctx->Const.MaxLights;
   state->MaxProgramMatrices = ctx->Const.MaxProgramMatrices;
   state->MaxDrawBuffers = ctx->Const.MaxDrawBuffers;

   state->state_param_enum = (target == GL_VERTEX_PROGRAM_ARB)
      ? STATE_VERTEX_PROGRAM : STATE_FRAGMENT_PROGRAM;

   _mesa_set_program_error(ctx, -1, NULL);

   _mesa_program_lexer_ctor(& state->scanner, state, (const char *) str, len);
   yyparse(state);
   _mesa_program_lexer_dtor(state->scanner);


   if (ctx->Program.ErrorPos != -1) {
      goto error;
   }

   if (! _mesa_layout_parameters(state)) {
      struct YYLTYPE loc;

      loc.first_line = 0;
      loc.first_column = 0;
      loc.position = len;

      yyerror(& loc, state, "invalid PARAM usage");
      goto error;
   }


   
   /* Add one instruction to store the "END" instruction.
    */
   state->prog->Instructions =
      _mesa_alloc_instructions(state->prog->NumInstructions + 1);
   inst = state->inst_head;
   for (i = 0; i < state->prog->NumInstructions; i++) {
      struct asm_instruction *const temp = inst->next;

      state->prog->Instructions[i] = inst->Base;
      inst = temp;
   }

   /* Finally, tag on an OPCODE_END instruction */
   {
      const GLuint numInst = state->prog->NumInstructions;
      _mesa_init_instructions(state->prog->Instructions + numInst, 1);
      state->prog->Instructions[numInst].Opcode = OPCODE_END;
   }
   state->prog->NumInstructions++;

   state->prog->NumParameters = state->prog->Parameters->NumParameters;
   state->prog->NumAttributes = _mesa_bitcount_64(state->prog->InputsRead);

   /*
    * Initialize native counts to logical counts.  The device driver may
    * change them if program is translated into a hardware program.
    */
   state->prog->NumNativeInstructions = state->prog->NumInstructions;
   state->prog->NumNativeTemporaries = state->prog->NumTemporaries;
   state->prog->NumNativeParameters = state->prog->NumParameters;
   state->prog->NumNativeAttributes = state->prog->NumAttributes;
   state->prog->NumNativeAddressRegs = state->prog->NumAddressRegs;

   result = GL_TRUE;

error:
   for (inst = state->inst_head; inst != NULL; inst = temp) {
      temp = inst->next;
      free(inst);
   }

   state->inst_head = NULL;
   state->inst_tail = NULL;

   for (sym = state->sym; sym != NULL; sym = temp) {
      temp = sym->next;

      free((void *) sym->name);
      free(sym);
   }
   state->sym = NULL;

   _mesa_symbol_table_dtor(state->st);
   state->st = NULL;

   return result;
}
