/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "extensions.h"

#include "shader/program.h"
#include "shader/prog_instruction.h"
/*#include "shader/arbprogparse.h"*/
#include "tnl/tnl.h"

#include "nouveau_context.h"
#include "nouveau_shader.h"

/*****************************************************************************
 * Mesa entry points
 */
static void
nouveauBindProgram(GLcontext *ctx, GLenum target, struct gl_program *prog)
{
   NVSDBG("target=%s, prog=%p\n", _mesa_lookup_enum_by_nr(target), prog);
}

static struct gl_program *
nouveauNewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
   nouveauShader *nvs;

   NVSDBG("target=%s, id=%d\n", _mesa_lookup_enum_by_nr(target), id);

   nvs = CALLOC_STRUCT(_nouveauShader);
   NVSDBG("prog=%p\n", nvs);
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB:
      return _mesa_init_vertex_program(ctx, &nvs->mesa.vp, target, id);
   case GL_FRAGMENT_PROGRAM_ARB:
      return _mesa_init_fragment_program(ctx, &nvs->mesa.fp, target, id);
   default:
      _mesa_problem(ctx, "Unsupported shader target");
      break;
   }

   FREE(nvs);
   return NULL;
}

static void
nouveauDeleteProgram(GLcontext *ctx, struct gl_program *prog)
{
   nouveauShader *nvs = (nouveauShader *)prog;

   NVSDBG("prog=%p\n", prog);

   if (nvs->translated)
      FREE(nvs->program);
   _mesa_delete_program(ctx, prog);
}

static void
nouveauProgramStringNotify(GLcontext *ctx, GLenum target,
      			   struct gl_program *prog)
{
   nouveauShader *nvs = (nouveauShader *)prog;

   NVSDBG("target=%s, prog=%p\n", _mesa_lookup_enum_by_nr(target), prog);

   if (nvs->translated)
      FREE(nvs->program);

   nvs->error      = GL_FALSE;
   nvs->translated = GL_FALSE;

   _tnl_program_string(ctx, target, prog);
}

static GLboolean
nouveauIsProgramNative(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
   nouveauShader *nvs = (nouveauShader *)prog;

   NVSDBG("target=%s, prog=%p\n", _mesa_lookup_enum_by_nr(target), prog);

   return nvs->translated;
}

GLboolean
nvsUpdateShader(GLcontext *ctx, nouveauShader *nvs)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   struct gl_program_parameter_list *plist;
   int i;

   NVSDBG("prog=%p\n", nvs);

   /* Translate to HW format now if necessary */
   if (!nvs->translated) {
      /* Mesa ASM shader -> nouveauShader */
      if (!nouveau_shader_pass0(ctx, nvs))
	 return GL_FALSE;
      /* Basic dead code elimination + register usage info */
      if (!nouveau_shader_pass1(nvs))
	 return GL_FALSE;
      /* nouveauShader -> HW bytecode, HW register alloc */
      if (!nouveau_shader_pass2(nvs))
	 return GL_FALSE;
      assert(nvs->translated);
      assert(nvs->program);
   }
   
   /* Update state parameters */
   plist = nvs->mesa.vp.Base.Parameters;
   _mesa_load_state_parameters(ctx, plist);
   for (i=0; i<nvs->param_high; i++) {
      if (!nvs->params[i].in_use)
	 continue;

      if (!nvs->on_hardware) {
	 /* if we've been kicked off the hardware there's no guarantee our
	  * consts are still there.. reupload them all
	  */
	 nvs->func->UpdateConst(ctx, nvs, i);
      } else if (nvs->params[i].source_val) {
	 /* update any changed state parameters */
	 if (!TEST_EQ_4V(nvs->params[i].val, nvs->params[i].source_val))
	    nvs->func->UpdateConst(ctx, nvs, i);
      }
   }

   /* Upload program to hardware, this must come after state param update
    * as >=NV30 fragprogs inline consts into the bytecode.
    */
   if (!nvs->on_hardware) {
      nouveauShader **current;

      if (nvs->mesa.vp.Base.Target == GL_VERTEX_PROGRAM_ARB)
	 current = &nmesa->current_vertprog;
      else
	 current = &nmesa->current_fragprog;
      if (*current) (*current)->on_hardware = 0;

      nvs->func->UploadToHW(ctx, nvs);
      nvs->on_hardware = 1;

      *current = nvs;
   }

   return GL_TRUE;
}

nouveauShader *
nvsBuildTextShader(GLcontext *ctx, GLenum target, const char *text)
{
   nouveauShader *nvs;

   nvs = CALLOC_STRUCT(_nouveauShader);
   if (!nvs)
      return NULL;

   if (target == GL_VERTEX_PROGRAM_ARB) {
      _mesa_init_vertex_program(ctx, &nvs->mesa.vp, GL_VERTEX_PROGRAM_ARB, 0);
      _mesa_parse_arb_vertex_program(ctx,
	    			     GL_VERTEX_PROGRAM_ARB,
				     text,
				     strlen(text),
				     &nvs->mesa.vp);
   } else if (target == GL_FRAGMENT_PROGRAM_ARB) {
      _mesa_init_fragment_program(ctx, &nvs->mesa.fp, GL_FRAGMENT_PROGRAM_ARB, 0);
      _mesa_parse_arb_fragment_program(ctx,
	    			       GL_FRAGMENT_PROGRAM_ARB,
				       text,
				       strlen(text),
				       &nvs->mesa.fp);
   }

   nouveau_shader_pass0(ctx, nvs);
   nouveau_shader_pass1(nvs);
   nouveau_shader_pass2(nvs);

   return nvs;
}

static void
nvsBuildPassthroughVP(GLcontext *ctx)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

   const char *vp_text =
      "!!ARBvp1.0\n"
      "OPTION ARB_position_invariant;"
      ""
      "MOV result.color, vertex.color;\n"
      "MOV result.texcoord[0], vertex.texcoord[0];\n"
      "MOV result.texcoord[1], vertex.texcoord[1];\n"
      "MOV result.texcoord[2], vertex.texcoord[2];\n"
      "MOV result.texcoord[3], vertex.texcoord[3];\n"
      "MOV result.texcoord[4], vertex.texcoord[4];\n"
      "MOV result.texcoord[5], vertex.texcoord[5];\n"
      "MOV result.texcoord[6], vertex.texcoord[6];\n"
      "MOV result.texcoord[7], vertex.texcoord[7];\n"
      "END";

   nmesa->passthrough_vp = nvsBuildTextShader(ctx,
	 				      GL_VERTEX_PROGRAM_ARB,
					      vp_text);
}

static void
nvsBuildPassthroughFP(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	const char *fp_text =
		"!!ARBfp1.0\n"
		"MOV result.color, fragment.color;\n"
		"END";

	nmesa->passthrough_fp = nvsBuildTextShader(ctx,
						   GL_FRAGMENT_PROGRAM_ARB,
						   fp_text);
}

void
nouveauShaderInitFuncs(GLcontext * ctx)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

   switch (nmesa->screen->card->type) {
   case NV_20:
      NV20VPInitShaderFuncs(&nmesa->VPfunc);
      break;
   case NV_30:
      NV30VPInitShaderFuncs(&nmesa->VPfunc);
      NV30FPInitShaderFuncs(&nmesa->FPfunc);
      break;
   case NV_40:
   case NV_44:
      NV40VPInitShaderFuncs(&nmesa->VPfunc);
      NV40FPInitShaderFuncs(&nmesa->FPfunc);
      break;
   case NV_50:
   default:
      return;
   }

   /* Build a vertex program that simply passes through all attribs.
    * Needed to do swtcl on nv40
    */
   if (nmesa->screen->card->type >= NV_40)
      nvsBuildPassthroughVP(ctx);

   /* Needed on NV30, even when using swtcl, if you want to get colours */
   if (nmesa->screen->card->type >= NV_30)
      nvsBuildPassthroughFP(ctx);

   ctx->Const.VertexProgram.MaxNativeInstructions    = nmesa->VPfunc.MaxInst;
   ctx->Const.VertexProgram.MaxNativeAluInstructions = nmesa->VPfunc.MaxInst;
   ctx->Const.VertexProgram.MaxNativeTexInstructions = nmesa->VPfunc.MaxInst;
   ctx->Const.VertexProgram.MaxNativeTexIndirections =
      ctx->Const.VertexProgram.MaxNativeTexInstructions;
   ctx->Const.VertexProgram.MaxNativeAttribs         = nmesa->VPfunc.MaxAttrib;
   ctx->Const.VertexProgram.MaxNativeTemps           = nmesa->VPfunc.MaxTemp;
   ctx->Const.VertexProgram.MaxNativeAddressRegs     = nmesa->VPfunc.MaxAddress;
   ctx->Const.VertexProgram.MaxNativeParameters      = nmesa->VPfunc.MaxConst;

   if (nmesa->screen->card->type >= NV_30) {
      ctx->Const.FragmentProgram.MaxNativeInstructions    = nmesa->FPfunc.MaxInst;
      ctx->Const.FragmentProgram.MaxNativeAluInstructions = nmesa->FPfunc.MaxInst;
      ctx->Const.FragmentProgram.MaxNativeTexInstructions = nmesa->FPfunc.MaxInst;
      ctx->Const.FragmentProgram.MaxNativeTexIndirections =
	 ctx->Const.FragmentProgram.MaxNativeTexInstructions;
      ctx->Const.FragmentProgram.MaxNativeAttribs         = nmesa->FPfunc.MaxAttrib;
      ctx->Const.FragmentProgram.MaxNativeTemps           = nmesa->FPfunc.MaxTemp;
      ctx->Const.FragmentProgram.MaxNativeAddressRegs     = nmesa->FPfunc.MaxAddress;
      ctx->Const.FragmentProgram.MaxNativeParameters      = nmesa->FPfunc.MaxConst;
   }

   ctx->Driver.NewProgram		= nouveauNewProgram;
   ctx->Driver.BindProgram		= nouveauBindProgram;
   ctx->Driver.DeleteProgram		= nouveauDeleteProgram;
   ctx->Driver.ProgramStringNotify	= nouveauProgramStringNotify;
   ctx->Driver.IsProgramNative		= nouveauIsProgramNative;
}


/*****************************************************************************
 * Disassembly support structs
 */
#define CHECK_RANGE(idx, arr) ((idx)<sizeof(_##arr)/sizeof(const char *)) \
	? _##arr[(idx)] : #arr"_OOB"

#define NODS      (1<<0)
#define BRANCH_TR (1<<1)
#define BRANCH_EL (1<<2)
#define BRANCH_EN (1<<3)
#define BRANCH_RE (1<<4)
#define BRANCH_ALL (BRANCH_TR|BRANCH_EL|BRANCH_EN)
#define COUNT_INC (1<<4)
#define COUNT_IND (1<<5)
#define COUNT_NUM (1<<6)
#define COUNT_ALL (COUNT_INC|COUNT_IND|COUNT_NUM)
#define TI_UNIT   (1<<7)
struct _opcode_info
{
   const char *name;
   int numsrc;
   int flags;
};

static struct _opcode_info ops[] = {
   [NVS_OP_ABS] = {"ABS", 1, 0},
   [NVS_OP_ADD] = {"ADD", 2, 0},
   [NVS_OP_ARA] = {"ARA", 1, 0},
   [NVS_OP_ARL] = {"ARL", 1, 0},
   [NVS_OP_ARR] = {"ARR", 1, 0},
   [NVS_OP_BRA] = {"BRA", 0, NODS | BRANCH_TR},
   [NVS_OP_BRK] = {"BRK", 0, NODS},
   [NVS_OP_CAL] = {"CAL", 0, NODS | BRANCH_TR},
   [NVS_OP_CMP] = {"CMP", 2, 0},
   [NVS_OP_COS] = {"COS", 1, 0},
   [NVS_OP_DIV] = {"DIV", 2, 0},
   [NVS_OP_DDX] = {"DDX", 1, 0},
   [NVS_OP_DDY] = {"DDY", 1, 0},
   [NVS_OP_DP2] = {"DP2", 2, 0},
   [NVS_OP_DP2A] = {"DP2A", 3, 0},
   [NVS_OP_DP3] = {"DP3", 2, 0},
   [NVS_OP_DP4] = {"DP4", 2, 0},
   [NVS_OP_DPH] = {"DPH", 2, 0},
   [NVS_OP_DST] = {"DST", 2, 0},
   [NVS_OP_EX2] = {"EX2", 1, 0},
   [NVS_OP_EXP] = {"EXP", 1, 0},
   [NVS_OP_FLR] = {"FLR", 1, 0},
   [NVS_OP_FRC] = {"FRC", 1, 0},
   [NVS_OP_IF] = {"IF", 0, NODS | BRANCH_EL | BRANCH_EN},
   [NVS_OP_KIL] = {"KIL", 1, 0},
   [NVS_OP_LG2] = {"LG2", 1, 0},
   [NVS_OP_LIT] = {"LIT", 1, 0},
   [NVS_OP_LOG] = {"LOG", 1, 0},
   [NVS_OP_LOOP] = {"LOOP", 0, NODS | COUNT_ALL | BRANCH_EN},
   [NVS_OP_LRP] = {"LRP", 3, 0},
   [NVS_OP_MAD] = {"MAD", 3, 0},
   [NVS_OP_MAX] = {"MAX", 2, 0},
   [NVS_OP_MIN] = {"MIN", 2, 0},
   [NVS_OP_MOV] = {"MOV", 1, 0},
   [NVS_OP_MUL] = {"MUL", 2, 0},
   [NVS_OP_NRM] = {"NRM", 1, 0},
   [NVS_OP_PK2H] = {"PK2H", 1, 0},
   [NVS_OP_PK2US] = {"PK2US", 1, 0},
   [NVS_OP_PK4B] = {"PK4B", 1, 0},
   [NVS_OP_PK4UB] = {"PK4UB", 1, 0},
   [NVS_OP_POW] = {"POW", 2, 0},
   [NVS_OP_POPA] = {"POPA", 0, 0},
   [NVS_OP_PUSHA] = {"PUSHA", 1, NODS},
   [NVS_OP_RCC] = {"RCC", 1, 0},
   [NVS_OP_RCP] = {"RCP", 1, 0},
   [NVS_OP_REP] = {"REP", 0, NODS | BRANCH_EN | COUNT_NUM},
   [NVS_OP_RET] = {"RET", 0, NODS},
   [NVS_OP_RFL] = {"RFL", 1, 0},
   [NVS_OP_RSQ] = {"RSQ", 1, 0},
   [NVS_OP_SCS] = {"SCS", 1, 0},
   [NVS_OP_SEQ] = {"SEQ", 2, 0},
   [NVS_OP_SFL] = {"SFL", 2, 0},
   [NVS_OP_SGE] = {"SGE", 2, 0},
   [NVS_OP_SGT] = {"SGT", 2, 0},
   [NVS_OP_SIN] = {"SIN", 1, 0},
   [NVS_OP_SLE] = {"SLE", 2, 0},
   [NVS_OP_SLT] = {"SLT", 2, 0},
   [NVS_OP_SNE] = {"SNE", 2, 0},
   [NVS_OP_SSG] = {"SSG", 1, 0},
   [NVS_OP_STR] = {"STR", 2, 0},
   [NVS_OP_SUB] = {"SUB", 2, 0},
   [NVS_OP_TEX] = {"TEX", 1, TI_UNIT},
   [NVS_OP_TXB] = {"TXB", 1, TI_UNIT},
   [NVS_OP_TXD] = {"TXD", 3, TI_UNIT},
   [NVS_OP_TXL] = {"TXL", 1, TI_UNIT},
   [NVS_OP_TXP] = {"TXP", 1, TI_UNIT},
   [NVS_OP_UP2H] = {"UP2H", 1, 0},
   [NVS_OP_UP2US] = {"UP2US", 1, 0},
   [NVS_OP_UP4B] = {"UP4B", 1, 0},
   [NVS_OP_UP4UB] = {"UP4UB", 1, 0},
   [NVS_OP_X2D] = {"X2D", 3, 0},
   [NVS_OP_XPD] = {"XPD", 2, 0},
   [NVS_OP_NOP] = {"NOP", 0, NODS},
};

static struct _opcode_info *
_get_op_info(int op)
{
   if (op >= (sizeof(ops) / sizeof(struct _opcode_info)))
      return NULL;
   if (ops[op].name == NULL)
      return NULL;
   return &ops[op];
}

static const char *_SFR_STRING[] = {
   [NVS_FR_POSITION] = "position",
   [NVS_FR_WEIGHT] = "weight",
   [NVS_FR_NORMAL] = "normal",
   [NVS_FR_COL0] = "color",
   [NVS_FR_COL1] = "color.secondary",
   [NVS_FR_BFC0] = "bfc",
   [NVS_FR_BFC1] = "bfc.secondary",
   [NVS_FR_FOGCOORD] = "fogcoord",
   [NVS_FR_POINTSZ] = "pointsize",
   [NVS_FR_TEXCOORD0] = "texcoord[0]",
   [NVS_FR_TEXCOORD1] = "texcoord[1]",
   [NVS_FR_TEXCOORD2] = "texcoord[2]",
   [NVS_FR_TEXCOORD3] = "texcoord[3]",
   [NVS_FR_TEXCOORD4] = "texcoord[4]",
   [NVS_FR_TEXCOORD5] = "texcoord[5]",
   [NVS_FR_TEXCOORD6] = "texcoord[6]",
   [NVS_FR_TEXCOORD7] = "texcoord[7]",
   [NVS_FR_FRAGDATA0] = "data[0]",
   [NVS_FR_FRAGDATA1] = "data[1]",
   [NVS_FR_FRAGDATA2] = "data[2]",
   [NVS_FR_FRAGDATA3] = "data[3]",
   [NVS_FR_CLIP0] = "clip_plane[0]",
   [NVS_FR_CLIP1] = "clip_plane[1]",
   [NVS_FR_CLIP2] = "clip_plane[2]",
   [NVS_FR_CLIP3] = "clip_plane[3]",
   [NVS_FR_CLIP4] = "clip_plane[4]",
   [NVS_FR_CLIP5] = "clip_plane[5]",
   [NVS_FR_CLIP6] = "clip_plane[6]",
   [NVS_FR_FACING] = "facing",
};

#define SFR_STRING(idx) CHECK_RANGE((idx), SFR_STRING)

static const char *_SWZ_STRING[] = {
   [NVS_SWZ_X] = "x",
   [NVS_SWZ_Y] = "y",
   [NVS_SWZ_Z] = "z",
   [NVS_SWZ_W] = "w"
};

#define SWZ_STRING(idx) CHECK_RANGE((idx), SWZ_STRING)

static const char *_NVS_PREC_STRING[] = {
   [NVS_PREC_FLOAT32] = "R",
   [NVS_PREC_FLOAT16] = "H",
   [NVS_PREC_FIXED12] = "X",
   [NVS_PREC_UNKNOWN] = "?"
};

#define NVS_PREC_STRING(idx) CHECK_RANGE((idx), NVS_PREC_STRING)

static const char *_NVS_COND_STRING[] = {
   [NVS_COND_FL] = "FL",
   [NVS_COND_LT] = "LT",
   [NVS_COND_EQ] = "EQ",
   [NVS_COND_LE] = "LE",
   [NVS_COND_GT] = "GT",
   [NVS_COND_NE] = "NE",
   [NVS_COND_GE] = "GE",
   [NVS_COND_TR] = "TR",
   [NVS_COND_UNKNOWN] = "??"
};

#define NVS_COND_STRING(idx) CHECK_RANGE((idx), NVS_COND_STRING)

/*****************************************************************************
 * ShaderFragment dumping
 */
static void
nvsDumpIndent(int lvl)
{
   while (lvl--)
      printf("  ");
}

static void
nvsDumpSwizzle(nvsSwzComp *swz)
{
   printf(".%s%s%s%s",
	  SWZ_STRING(swz[0]),
	  SWZ_STRING(swz[1]), SWZ_STRING(swz[2]), SWZ_STRING(swz[3])
      );
}

static void
nvsDumpReg(nvsInstruction * inst, nvsRegister * reg)
{
   if (reg->negate)
      printf("-");
   if (reg->abs)
      printf("abs(");

   switch (reg->file) {
   case NVS_FILE_TEMP:
      printf("R%d", reg->index);
      nvsDumpSwizzle(reg->swizzle);
      break;
   case NVS_FILE_ATTRIB:
      printf("attrib.%s", SFR_STRING(reg->index));
      nvsDumpSwizzle(reg->swizzle);
      break;
   case NVS_FILE_ADDRESS:
      printf("A%d", reg->index);
      break;
   case NVS_FILE_CONST:
      if (reg->indexed)
	 printf("const[A%d.%s + %d]",
		reg->addr_reg, SWZ_STRING(reg->addr_comp), reg->index);
      else
	 printf("const[%d]", reg->index);
      nvsDumpSwizzle(reg->swizzle);
      break;
   default:
      printf("UNKNOWN_FILE");
      break;
   }

   if (reg->abs)
      printf(")");
}

static void
nvsDumpInstruction(nvsInstruction * inst, int slot, int lvl)
{
   struct _opcode_info *opr = &ops[inst->op];
   int i;

   nvsDumpIndent(lvl);
   printf("%s ", opr->name);

   if (!opr->flags & NODS) {
      switch (inst->dest.file) {
      case NVS_FILE_RESULT:
	 printf("result.%s", SFR_STRING(inst->dest.index));
	 break;
      case NVS_FILE_TEMP:
	 printf("R%d", inst->dest.index);
	 break;
      case NVS_FILE_ADDRESS:
	 printf("A%d", inst->dest.index);
	 break;
      default:
	 printf("UNKNOWN_DST_FILE");
	 break;
      }

      if (inst->mask != SMASK_ALL) {
	 printf(".");
	 if (inst->mask & SMASK_X)
	    printf("x");
	 if (inst->mask & SMASK_Y)
	    printf("y");
	 if (inst->mask & SMASK_Z)
	    printf("z");
	 if (inst->mask & SMASK_W)
	    printf("w");
      }

      if (opr->numsrc)
	 printf(", ");
   }

   for (i = 0; i < opr->numsrc; i++) {
      nvsDumpReg(inst, &inst->src[i]);
      if (i != opr->numsrc - 1)
	 printf(", ");
   }
   if (opr->flags & TI_UNIT)
      printf(", texture[%d]", inst->tex_unit);

   printf("\n");
}

void
nvsDumpFragmentList(nvsFragmentHeader *f, int lvl)
{
   while (f) {
      switch (f->type) {
      case NVS_INSTRUCTION:
	 nvsDumpInstruction((nvsInstruction*)f, 0, lvl);
	 break;
      default:
	 fprintf(stderr, "%s: Only NVS_INSTRUCTION fragments can be in"
	                 "nvsFragmentList!\n", __func__);
	 return;
      }
      f = f->next;
   }
}

/*****************************************************************************
 * HW shader disassembly
 */
static void
nvsDisasmHWShaderOp(nvsFunc * shader, int merged)
{
   struct _opcode_info *opi;
   nvsOpcode op;
   nvsRegFile file;
   nvsSwzComp swz[4];
   int i;

   op = shader->GetOpcode(shader, merged);
   opi = _get_op_info(op);
   if (!opi) {
      printf("NO OPINFO!");
      return;
   }

   printf("%s", opi->name);
   if (shader->GetPrecision &&
       (!(opi->flags & BRANCH_ALL)) && (!(opi->flags * NODS)) &&
       (op != NVS_OP_NOP))
      printf("%s", NVS_PREC_STRING(shader->GetPrecision(shader)));
   if (shader->SupportsConditional && shader->SupportsConditional(shader)) {
      if (shader->GetConditionUpdate(shader)) {
	 printf("C%d", shader->GetCondRegID(shader));
      }
   }
   if (shader->GetSaturate && shader->GetSaturate(shader))
      printf("_SAT");

   if (!(opi->flags & NODS)) {
      int mask = shader->GetDestMask(shader, merged);

      switch (shader->GetDestFile(shader, merged)) {
      case NVS_FILE_ADDRESS:
	 printf(" A%d", shader->GetDestID(shader, merged));
	 break;
      case NVS_FILE_TEMP:
	 printf(" R%d", shader->GetDestID(shader, merged));
	 break;
      case NVS_FILE_RESULT:
	 printf(" result.%s", (SFR_STRING(shader->GetDestID(shader, merged))));
	 break;
      default:
	 printf(" BAD_RESULT_FILE");
	 break;
      }

      if (mask != SMASK_ALL) {
	 printf(".");
	 if (mask & SMASK_X) printf("x");
	 if (mask & SMASK_Y) printf("y");
	 if (mask & SMASK_Z) printf("z");
	 if (mask & SMASK_W) printf("w");
      }
   }

   if (shader->SupportsConditional && shader->SupportsConditional(shader) &&
       shader->GetConditionTest(shader)) {
      shader->GetCondRegSwizzle(shader, swz);

      printf(" (%s%d.%s%s%s%s)",
	    NVS_COND_STRING(shader->GetCondition(shader)),
	    shader->GetCondRegID(shader),
	    SWZ_STRING(swz[NVS_SWZ_X]),
	    SWZ_STRING(swz[NVS_SWZ_Y]),
	    SWZ_STRING(swz[NVS_SWZ_Z]),
	    SWZ_STRING(swz[NVS_SWZ_W])
	    );
   }

   /* looping */
   if (opi->flags & COUNT_ALL) {
      printf(" { ");
      if (opi->flags & COUNT_NUM) {
	 printf("%d", shader->GetLoopCount(shader));
      }
      if (opi->flags & COUNT_IND) {
	 printf(", %d", shader->GetLoopInitial(shader));
      }
      if (opi->flags & COUNT_INC) {
	 printf(", %d", shader->GetLoopIncrement(shader));
      }
      printf(" }");
   }

   /* branching */
   if (opi->flags & BRANCH_TR)
      printf(" %d", shader->GetBranch(shader));
   if (opi->flags & BRANCH_EL)
      printf(" ELSE %d", shader->GetBranchElse(shader));
   if (opi->flags & BRANCH_EN)
      printf(" END %d", shader->GetBranchEnd(shader));

   if (!(opi->flags & NODS) && opi->numsrc)
      printf(",");
   printf(" ");

   for (i = 0; i < opi->numsrc; i++) {
      if (shader->GetSourceAbs(shader, merged, i))
	 printf("abs(");
      if (shader->GetSourceNegate(shader, merged, i))
	 printf("-");

      file = shader->GetSourceFile(shader, merged, i);
      switch (file) {
      case NVS_FILE_TEMP:
	 printf("R%d", shader->GetSourceID(shader, merged, i));
	 break;
      case NVS_FILE_CONST:
	 if (shader->GetSourceIndexed(shader, merged, i)) {
	    printf("c[A%d.%s + 0x%x]",
		     shader->GetRelAddressRegID(shader),
		     SWZ_STRING(shader->GetRelAddressSwizzle(shader)),
		     shader->GetSourceID(shader, merged, i)
		  );
	 } else {
	    float val[4];

	    if (shader->GetSourceConstVal) {
	       shader->GetSourceConstVal(shader, merged, i, val);
	       printf("{ %.02f, %.02f, %.02f, %.02f }",
		     val[0], val[1], val[2], val[3]);
	    } else {
	       printf("c[0x%x]", shader->GetSourceID(shader, merged, i));
	    }
	 }
	 break;
      case NVS_FILE_ATTRIB:
	 if (shader->GetSourceIndexed(shader, merged, i)) {
	    printf("attrib[A%d.%s + %d]",
		     shader->GetRelAddressRegID(shader),
		     SWZ_STRING(shader->GetRelAddressSwizzle(shader)),
		     shader->GetSourceID(shader, merged, i)
		  );
	 }
	 else {
	    printf("attrib.%s",
		     SFR_STRING(shader->GetSourceID(shader, merged, i))
		  );
	 }
	 break;
      case NVS_FILE_ADDRESS:
	 printf("A%d", shader->GetRelAddressRegID(shader));
	 break;
      default:
	 printf("UNKNOWN_SRC_FILE");
	 break;
      }

      shader->GetSourceSwizzle(shader, merged, i, swz);
      if (file != NVS_FILE_ADDRESS &&
	  (swz[NVS_SWZ_X] != NVS_SWZ_X || swz[NVS_SWZ_Y] != NVS_SWZ_Y ||
	   swz[NVS_SWZ_Z] != NVS_SWZ_Z || swz[NVS_SWZ_W] != NVS_SWZ_W)) {
	 printf(".%s%s%s%s", SWZ_STRING(swz[NVS_SWZ_X]),
	       		     SWZ_STRING(swz[NVS_SWZ_Y]),
			     SWZ_STRING(swz[NVS_SWZ_Z]),
			     SWZ_STRING(swz[NVS_SWZ_W]));
      }

      if (shader->GetSourceAbs(shader, merged, i))
	 printf(")");
      if (shader->GetSourceScale) {
	 int scale = shader->GetSourceScale(shader, merged, i);
	 if (scale > 1)
	    printf("{scaled %dx}", scale);
      }
      if (i < (opi->numsrc - 1))
	 printf(", ");
   }

   if (shader->IsLastInst(shader))
      printf(" + END");
}

void
nvsDisasmHWShader(nvsPtr nvs)
{
   nvsFunc *shader = nvs->func;
   unsigned int iaddr = 0;

   if (!nvs->program) {
      fprintf(stderr, "No HW program present");
      return;
   }

   shader->inst = nvs->program;
   while (1) {
      if (shader->inst >= (nvs->program + nvs->program_size)) {
	 fprintf(stderr, "Reached end of program, but HW inst has no END");
	 break;
      }

      printf("\t0x%08x:\n", shader->inst[0]);
      printf("\t0x%08x:\n", shader->inst[1]);
      printf("\t0x%08x:\n", shader->inst[2]);
      printf("\t0x%08x:", shader->inst[3]);

      printf("\n\t\tINST %d.0: ", iaddr);
      nvsDisasmHWShaderOp(shader, 0);
      if (shader->HasMergedInst(shader)) {
	 printf("\n\t\tINST %d.1: ", iaddr);
	 nvsDisasmHWShaderOp(shader, 1);
      }
      printf("\n");

      if (shader->IsLastInst(shader))
	 break;

      shader->inst += shader->GetOffsetNext(shader);
      iaddr++;
   }

   printf("\n");
}
