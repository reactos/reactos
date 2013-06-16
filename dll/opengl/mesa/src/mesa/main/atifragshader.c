/**
 * \file atifragshader.c
 * \author David Airlie
 * Copyright (C) 2004  David Airlie   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID AIRLIE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "main/glheader.h"
#include "main/context.h"
#include "main/hash.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/enums.h"
#include "main/mtypes.h"
#include "main/dispatch.h"
#include "main/atifragshader.h"

#if FEATURE_ATI_fragment_shader

#define MESA_DEBUG_ATI_FS 0

static struct ati_fragment_shader DummyShader;


void
_mesa_init_ati_fragment_shader_dispatch(struct _glapi_table *disp)
{
   SET_GenFragmentShadersATI(disp, _mesa_GenFragmentShadersATI);
   SET_BindFragmentShaderATI(disp, _mesa_BindFragmentShaderATI);
   SET_DeleteFragmentShaderATI(disp, _mesa_DeleteFragmentShaderATI);
   SET_BeginFragmentShaderATI(disp, _mesa_BeginFragmentShaderATI);
   SET_EndFragmentShaderATI(disp, _mesa_EndFragmentShaderATI);
   SET_PassTexCoordATI(disp, _mesa_PassTexCoordATI);
   SET_SampleMapATI(disp, _mesa_SampleMapATI);
   SET_ColorFragmentOp1ATI(disp, _mesa_ColorFragmentOp1ATI);
   SET_ColorFragmentOp2ATI(disp, _mesa_ColorFragmentOp2ATI);
   SET_ColorFragmentOp3ATI(disp, _mesa_ColorFragmentOp3ATI);
   SET_AlphaFragmentOp1ATI(disp, _mesa_AlphaFragmentOp1ATI);
   SET_AlphaFragmentOp2ATI(disp, _mesa_AlphaFragmentOp2ATI);
   SET_AlphaFragmentOp3ATI(disp, _mesa_AlphaFragmentOp3ATI);
   SET_SetFragmentShaderConstantATI(disp, _mesa_SetFragmentShaderConstantATI);
}


/**
 * Allocate and initialize a new ATI fragment shader object.
 */
struct ati_fragment_shader *
_mesa_new_ati_fragment_shader(struct gl_context *ctx, GLuint id)
{
   struct ati_fragment_shader *s = CALLOC_STRUCT(ati_fragment_shader);
   (void) ctx;
   if (s) {
      s->Id = id;
      s->RefCount = 1;
   }
   return s;
}


/**
 * Delete the given ati fragment shader
 */
void
_mesa_delete_ati_fragment_shader(struct gl_context *ctx, struct ati_fragment_shader *s)
{
   GLuint i;
   for (i = 0; i < MAX_NUM_PASSES_ATI; i++) {
      if (s->Instructions[i])
         free(s->Instructions[i]);
      if (s->SetupInst[i])
         free(s->SetupInst[i]);
   }
   free(s);
}



static void
new_arith_inst(struct ati_fragment_shader *prog)
{
/* set "default" instruction as not all may get defined.
   there is no specified way to express a nop with ati fragment shaders we use
   GL_NONE as the op enum and just set some params to 0 - so nothing to do here */
   prog->numArithInstr[prog->cur_pass >> 1]++;
}

static void
new_tex_inst(struct ati_fragment_shader *prog)
{
}

static void match_pair_inst(struct ati_fragment_shader *curProg, GLuint optype)
{
   if (optype == curProg->last_optype) {
      curProg->last_optype = 1;
   }
}

#if MESA_DEBUG_ATI_FS
static char *
create_dst_mod_str(GLuint mod)
{
   static char ret_str[1024];

   memset(ret_str, 0, 1024);
   if (mod & GL_2X_BIT_ATI)
      strncat(ret_str, "|2X", 1024);

   if (mod & GL_4X_BIT_ATI)
      strncat(ret_str, "|4X", 1024);

   if (mod & GL_8X_BIT_ATI)
      strncat(ret_str, "|8X", 1024);
   if (mod & GL_HALF_BIT_ATI)
      strncat(ret_str, "|HA", 1024);
   if (mod & GL_QUARTER_BIT_ATI)
      strncat(ret_str, "|QU", 1024);
   if (mod & GL_EIGHTH_BIT_ATI)
      strncat(ret_str, "|EI", 1024);

   if (mod & GL_SATURATE_BIT_ATI)
      strncat(ret_str, "|SAT", 1024);

   if (strlen(ret_str) == 0)
      strncat(ret_str, "NONE", 1024);
   return ret_str;
}

static char *atifs_ops[] = {"ColorFragmentOp1ATI", "ColorFragmentOp2ATI", "ColorFragmentOp3ATI", 
			    "AlphaFragmentOp1ATI", "AlphaFragmentOp2ATI", "AlphaFragmentOp3ATI" };

static void debug_op(GLint optype, GLuint arg_count, GLenum op, GLuint dst,
		     GLuint dstMask, GLuint dstMod, GLuint arg1,
		     GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
		     GLuint arg2Rep, GLuint arg2Mod, GLuint arg3,
		     GLuint arg3Rep, GLuint arg3Mod)
{
  char *op_name;

  op_name = atifs_ops[(arg_count-1)+(optype?3:0)];
  
  fprintf(stderr, "%s(%s, %s", op_name, _mesa_lookup_enum_by_nr(op),
	      _mesa_lookup_enum_by_nr(dst));
  if (!optype)
    fprintf(stderr, ", %d", dstMask);
  
  fprintf(stderr, ", %s", create_dst_mod_str(dstMod));
  
  fprintf(stderr, ", %s, %s, %d", _mesa_lookup_enum_by_nr(arg1),
	      _mesa_lookup_enum_by_nr(arg1Rep), arg1Mod);
  if (arg_count>1)
    fprintf(stderr, ", %s, %s, %d", _mesa_lookup_enum_by_nr(arg2),
	      _mesa_lookup_enum_by_nr(arg2Rep), arg2Mod);
  if (arg_count>2)
    fprintf(stderr, ", %s, %s, %d", _mesa_lookup_enum_by_nr(arg3),
	      _mesa_lookup_enum_by_nr(arg3Rep), arg3Mod);

  fprintf(stderr,")\n");

}
#endif

static int check_arith_arg(struct ati_fragment_shader *curProg,
			GLuint optype, GLuint arg, GLuint argRep)
{
   GET_CURRENT_CONTEXT(ctx);

   if (((arg < GL_CON_0_ATI) || (arg > GL_CON_7_ATI)) &&
      ((arg < GL_REG_0_ATI) || (arg > GL_REG_5_ATI)) &&
      (arg != GL_ZERO) && (arg != GL_ONE) &&
      (arg != GL_PRIMARY_COLOR_ARB) && (arg != GL_SECONDARY_INTERPOLATOR_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "C/AFragmentOpATI(arg)");
      return 0;
   }
   if ((arg == GL_SECONDARY_INTERPOLATOR_ATI) && (((optype == 0) && (argRep == GL_ALPHA)) ||
      ((optype == 1) && ((arg == GL_ALPHA) || (argRep == GL_NONE))))) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(sec_interp)");
      return 0;
   }
   if ((arg == GL_SECONDARY_INTERPOLATOR_ATI) && (((optype == 0) && (argRep == GL_ALPHA)) ||
      ((optype == 1) && ((arg == GL_ALPHA) || (argRep == GL_NONE))))) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(sec_interp)");
      return 0;
   }
   if ((curProg->cur_pass == 1) &&
      ((arg == GL_PRIMARY_COLOR_ARB) || (arg == GL_SECONDARY_INTERPOLATOR_ATI))) {
      curProg->interpinp1 = GL_TRUE;
   }
   return 1;
}

GLuint GLAPIENTRY
_mesa_GenFragmentShadersATI(GLuint range)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);

   if (range == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenFragmentShadersATI(range)");
      return 0;
   }

   if (ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGenFragmentShadersATI(insideShader)");
      return 0;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->ATIShaders, range);
   for (i = 0; i < range; i++) {
      _mesa_HashInsert(ctx->Shared->ATIShaders, first + i, &DummyShader);
   }

   return first;
}

void GLAPIENTRY
_mesa_BindFragmentShaderATI(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   struct ati_fragment_shader *newProg;

   if (ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBindFragmentShaderATI(insideShader)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (curProg->Id == id) {
      return;
   }

   /* unbind current */
   if (curProg->Id != 0) {
      curProg->RefCount--;
      if (curProg->RefCount <= 0) {
	 _mesa_HashRemove(ctx->Shared->ATIShaders, id);
      }
   }

   /* find new shader */
   if (id == 0) {
      newProg = ctx->Shared->DefaultFragmentShader;
   }
   else {
      newProg = (struct ati_fragment_shader *)
         _mesa_HashLookup(ctx->Shared->ATIShaders, id);
      if (!newProg || newProg == &DummyShader) {
	 /* allocate a new program now */
	 newProg = _mesa_new_ati_fragment_shader(ctx, id);
	 if (!newProg) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindFragmentShaderATI");
	    return;
	 }
	 _mesa_HashInsert(ctx->Shared->ATIShaders, id, newProg);
      }

   }

   /* do actual bind */
   ctx->ATIFragmentShader.Current = newProg;

   ASSERT(ctx->ATIFragmentShader.Current);
   if (newProg)
      newProg->RefCount++;

   /*if (ctx->Driver.BindProgram)
      ctx->Driver.BindProgram(ctx, target, prog); */
}

void GLAPIENTRY
_mesa_DeleteFragmentShaderATI(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteFragmentShaderATI(insideShader)");
      return;
   }

   if (id != 0) {
      struct ati_fragment_shader *prog = (struct ati_fragment_shader *)
	 _mesa_HashLookup(ctx->Shared->ATIShaders, id);
      if (prog == &DummyShader) {
	 _mesa_HashRemove(ctx->Shared->ATIShaders, id);
      }
      else if (prog) {
	 if (ctx->ATIFragmentShader.Current &&
	     ctx->ATIFragmentShader.Current->Id == id) {
	     FLUSH_VERTICES(ctx, _NEW_PROGRAM);
	    _mesa_BindFragmentShaderATI(0);
	 }
      }

      /* The ID is immediately available for re-use now */
      _mesa_HashRemove(ctx->Shared->ATIShaders, id);
      if (prog) {
	 prog->RefCount--;
	 if (prog->RefCount <= 0) {
	    assert(prog != &DummyShader);
	    free(prog);
	 }
      }
   }
}


void GLAPIENTRY
_mesa_BeginFragmentShaderATI(void)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginFragmentShaderATI(insideShader)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   /* if the shader was already defined free instructions and get new ones
      (or, could use the same mem but would need to reinitialize) */
   /* no idea if it's allowed to redefine a shader */
   for (i = 0; i < MAX_NUM_PASSES_ATI; i++) {
         if (ctx->ATIFragmentShader.Current->Instructions[i])
            free(ctx->ATIFragmentShader.Current->Instructions[i]);
         if (ctx->ATIFragmentShader.Current->SetupInst[i])
            free(ctx->ATIFragmentShader.Current->SetupInst[i]);
   }

   /* malloc the instructions here - not sure if the best place but its
      a start */
   for (i = 0; i < MAX_NUM_PASSES_ATI; i++) {
      ctx->ATIFragmentShader.Current->Instructions[i] =
	 (struct atifs_instruction *)
	 calloc(1, sizeof(struct atifs_instruction) *
		   (MAX_NUM_INSTRUCTIONS_PER_PASS_ATI));
      ctx->ATIFragmentShader.Current->SetupInst[i] =
	 (struct atifs_setupinst *)
	 calloc(1, sizeof(struct atifs_setupinst) *
		   (MAX_NUM_FRAGMENT_REGISTERS_ATI));
   }

/* can't rely on calloc for initialization as it's possible to redefine a shader (?) */
   ctx->ATIFragmentShader.Current->LocalConstDef = 0;
   ctx->ATIFragmentShader.Current->numArithInstr[0] = 0;
   ctx->ATIFragmentShader.Current->numArithInstr[1] = 0;
   ctx->ATIFragmentShader.Current->regsAssigned[0] = 0;
   ctx->ATIFragmentShader.Current->regsAssigned[1] = 0;
   ctx->ATIFragmentShader.Current->NumPasses = 0;
   ctx->ATIFragmentShader.Current->cur_pass = 0;
   ctx->ATIFragmentShader.Current->last_optype = 0;
   ctx->ATIFragmentShader.Current->interpinp1 = GL_FALSE;
   ctx->ATIFragmentShader.Current->isValid = GL_FALSE;
   ctx->ATIFragmentShader.Current->swizzlerq = 0;
   ctx->ATIFragmentShader.Compiling = 1;
}

void GLAPIENTRY
_mesa_EndFragmentShaderATI(void)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
#if MESA_DEBUG_ATI_FS
   GLint i, j;
#endif

   if (!ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndFragmentShaderATI(outsideShader)");
      return;
   }
   if (curProg->interpinp1 && (ctx->ATIFragmentShader.Current->cur_pass > 1)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndFragmentShaderATI(interpinfirstpass)");
   /* according to spec, DON'T return here */
   }

   match_pair_inst(curProg, 0);
   ctx->ATIFragmentShader.Compiling = 0;
   ctx->ATIFragmentShader.Current->isValid = GL_TRUE;
   if ((ctx->ATIFragmentShader.Current->cur_pass == 0) ||
      (ctx->ATIFragmentShader.Current->cur_pass == 2)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndFragmentShaderATI(noarithinst)");
   }
   if (ctx->ATIFragmentShader.Current->cur_pass > 1)
      ctx->ATIFragmentShader.Current->NumPasses = 2;
   else
      ctx->ATIFragmentShader.Current->NumPasses = 1;

   ctx->ATIFragmentShader.Current->cur_pass = 0;

#if MESA_DEBUG_ATI_FS
   for (j = 0; j < MAX_NUM_PASSES_ATI; j++) {
      for (i = 0; i < MAX_NUM_FRAGMENT_REGISTERS_ATI; i++) {
	 GLuint op = curProg->SetupInst[j][i].Opcode;
	 const char *op_enum = op > 5 ? _mesa_lookup_enum_by_nr(op) : "0";
	 GLuint src = curProg->SetupInst[j][i].src;
	 GLuint swizzle = curProg->SetupInst[j][i].swizzle;
	 fprintf(stderr, "%2d %04X %s %d %04X\n", i, op, op_enum, src,
	      swizzle);
      }
      for (i = 0; i < curProg->numArithInstr[j]; i++) {
	 GLuint op0 = curProg->Instructions[j][i].Opcode[0];
	 GLuint op1 = curProg->Instructions[j][i].Opcode[1];
	 const char *op0_enum = op0 > 5 ? _mesa_lookup_enum_by_nr(op0) : "0";
	 const char *op1_enum = op1 > 5 ? _mesa_lookup_enum_by_nr(op1) : "0";
	 GLuint count0 = curProg->Instructions[j][i].ArgCount[0];
	 GLuint count1 = curProg->Instructions[j][i].ArgCount[1];
	 fprintf(stderr, "%2d %04X %s %d %04X %s %d\n", i, op0, op0_enum, count0,
	      op1, op1_enum, count1);
      }
   }
#endif

   if (!ctx->Driver.ProgramStringNotify(ctx, GL_FRAGMENT_SHADER_ATI, NULL)) {
      ctx->ATIFragmentShader.Current->isValid = GL_FALSE;
      /* XXX is this the right error? */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndFragmentShaderATI(driver rejected shader)");
   }
}

void GLAPIENTRY
_mesa_PassTexCoordATI(GLuint dst, GLuint coord, GLenum swizzle)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   struct atifs_setupinst *curI;

   if (!ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPassTexCoordATI(outsideShader)");
      return;
   }

   if (curProg->cur_pass == 1) {
      match_pair_inst(curProg, 0);
      curProg->cur_pass = 2;
   }
   if ((curProg->cur_pass > 2) ||
      ((1 << (dst - GL_REG_0_ATI)) & curProg->regsAssigned[curProg->cur_pass >> 1])) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPassTexCoord(pass)");
      return;
   }
   if ((dst < GL_REG_0_ATI) || (dst > GL_REG_5_ATI) ||
      ((dst - GL_REG_0_ATI) >= ctx->Const.MaxTextureUnits)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPassTexCoordATI(dst)");
      return;
   }
   if (((coord < GL_REG_0_ATI) || (coord > GL_REG_5_ATI)) &&
       ((coord < GL_TEXTURE0_ARB) || (coord > GL_TEXTURE7_ARB) ||
       ((coord - GL_TEXTURE0_ARB) >= ctx->Const.MaxTextureUnits))) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPassTexCoordATI(coord)");
      return;
   }
   if ((curProg->cur_pass == 0) && (coord >= GL_REG_0_ATI)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPassTexCoordATI(coord)");
      return;
   }
   if (!(swizzle >= GL_SWIZZLE_STR_ATI) && (swizzle <= GL_SWIZZLE_STQ_DQ_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPassTexCoordATI(swizzle)");
      return;
   }
   if ((swizzle & 1) && (coord >= GL_REG_0_ATI)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPassTexCoordATI(swizzle)");
      return;
   }
   if (coord <= GL_TEXTURE7_ARB) {
      GLuint tmp = coord - GL_TEXTURE0_ARB;
      if ((((curProg->swizzlerq >> (tmp * 2)) & 3) != 0) &&
	   (((swizzle & 1) + 1) != ((curProg->swizzlerq >> (tmp * 2)) & 3))) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "glPassTexCoordATI(swizzle)");
	 return;
      } else {
	 curProg->swizzlerq |= (((swizzle & 1) + 1) << (tmp * 2));
      }
   }

   curProg->regsAssigned[curProg->cur_pass >> 1] |= 1 << (dst - GL_REG_0_ATI);
   new_tex_inst(curProg);

   /* add the instructions */
   curI = &curProg->SetupInst[curProg->cur_pass >> 1][dst - GL_REG_0_ATI];

   curI->Opcode = ATI_FRAGMENT_SHADER_PASS_OP;
   curI->src = coord;
   curI->swizzle = swizzle;

#if MESA_DEBUG_ATI_FS
   _mesa_debug(ctx, "%s(%s, %s, %s)\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr(dst), _mesa_lookup_enum_by_nr(coord),
	       _mesa_lookup_enum_by_nr(swizzle));
#endif
}

void GLAPIENTRY
_mesa_SampleMapATI(GLuint dst, GLuint interp, GLenum swizzle)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   struct atifs_setupinst *curI;

   if (!ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMapATI(outsideShader)");
      return;
   }

   if (curProg->cur_pass == 1) {
      match_pair_inst(curProg, 0);
      curProg->cur_pass = 2;
   }
   if ((curProg->cur_pass > 2) ||
      ((1 << (dst - GL_REG_0_ATI)) & curProg->regsAssigned[curProg->cur_pass >> 1])) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMapATI(pass)");
      return;
   }
   if ((dst < GL_REG_0_ATI) || (dst > GL_REG_5_ATI) ||
      ((dst - GL_REG_0_ATI) >= ctx->Const.MaxTextureUnits)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glSampleMapATI(dst)");
      return;
   }
   if (((interp < GL_REG_0_ATI) || (interp > GL_REG_5_ATI)) &&
       ((interp < GL_TEXTURE0_ARB) || (interp > GL_TEXTURE7_ARB) ||
       ((interp - GL_TEXTURE0_ARB) >= ctx->Const.MaxTextureUnits))) {
   /* is this texture5 or texture7? spec is a bit unclear there */
      _mesa_error(ctx, GL_INVALID_ENUM, "glSampleMapATI(interp)");
      return;
   }
   if ((curProg->cur_pass == 0) && (interp >= GL_REG_0_ATI)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMapATI(interp)");
      return;
   }
   if (!(swizzle >= GL_SWIZZLE_STR_ATI) && (swizzle <= GL_SWIZZLE_STQ_DQ_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glSampleMapATI(swizzle)");
      return;
   }
   if ((swizzle & 1) && (interp >= GL_REG_0_ATI)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMapATI(swizzle)");
      return;
   }
   if (interp <= GL_TEXTURE7_ARB) {
      GLuint tmp = interp - GL_TEXTURE0_ARB;
      if ((((curProg->swizzlerq >> (tmp * 2)) & 3) != 0) &&
	   (((swizzle & 1) + 1) != ((curProg->swizzlerq >> (tmp * 2)) & 3))) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMapATI(swizzle)");
	 return;
      } else {
	 curProg->swizzlerq |= (((swizzle & 1) + 1) << (tmp * 2));
      }
   }

   curProg->regsAssigned[curProg->cur_pass >> 1] |= 1 << (dst - GL_REG_0_ATI);
   new_tex_inst(curProg);

   /* add the instructions */
   curI = &curProg->SetupInst[curProg->cur_pass >> 1][dst - GL_REG_0_ATI];

   curI->Opcode = ATI_FRAGMENT_SHADER_SAMPLE_OP;
   curI->src = interp;
   curI->swizzle = swizzle;

#if MESA_DEBUG_ATI_FS
   _mesa_debug(ctx, "%s(%s, %s, %s)\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr(dst), _mesa_lookup_enum_by_nr(interp),
	       _mesa_lookup_enum_by_nr(swizzle));
#endif
}

static void
_mesa_FragmentOpXATI(GLint optype, GLuint arg_count, GLenum op, GLuint dst,
		     GLuint dstMask, GLuint dstMod, GLuint arg1,
		     GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
		     GLuint arg2Rep, GLuint arg2Mod, GLuint arg3,
		     GLuint arg3Rep, GLuint arg3Mod)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   GLint ci;
   struct atifs_instruction *curI;
   GLuint modtemp = dstMod & ~GL_SATURATE_BIT_ATI;

   if (!ctx->ATIFragmentShader.Compiling) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(outsideShader)");
      return;
   }

   if (curProg->cur_pass==0)
      curProg->cur_pass=1;

   else if (curProg->cur_pass==2)
      curProg->cur_pass=3;

   /* decide whether this is a new instruction or not ... all color instructions are new,
      and alpha instructions might also be new if there was no preceding color inst */
   if ((optype == 0) || (curProg->last_optype == optype)) {
      if (curProg->numArithInstr[curProg->cur_pass >> 1] > 7) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(instrCount)");
	 return;
      }
      /* easier to do that here slight side effect invalid instr will still be inserted as nops */
      match_pair_inst(curProg, optype);
      new_arith_inst(curProg);
   }
   curProg->last_optype = optype;
   ci = curProg->numArithInstr[curProg->cur_pass >> 1] - 1;

   /* add the instructions */
   curI = &curProg->Instructions[curProg->cur_pass >> 1][ci];

   /* error checking */
   if ((dst < GL_REG_0_ATI) || (dst > GL_REG_5_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "C/AFragmentOpATI(dst)");
      return;
   }
   if ((modtemp != GL_NONE) && (modtemp != GL_2X_BIT_ATI) &&
      (modtemp != GL_4X_BIT_ATI) && (modtemp != GL_8X_BIT_ATI) &&
      (modtemp != GL_HALF_BIT_ATI) && !(modtemp != GL_QUARTER_BIT_ATI) &&
      (modtemp != GL_EIGHTH_BIT_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "C/AFragmentOpATI(dstMod)%x", modtemp);
      return;
   }
   /* op checking? Actually looks like that's missing in the spec but we'll do it anyway */
   if (((op < GL_ADD_ATI) || (op > GL_DOT2_ADD_ATI)) && !(op == GL_MOV_ATI)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "C/AFragmentOpATI(op)");
      return;
   }
   if (optype == 1) {
      if (((op == GL_DOT2_ADD_ATI) && (curI->Opcode[0] != GL_DOT2_ADD_ATI)) ||
	 ((op == GL_DOT3_ATI) && (curI->Opcode[0] != GL_DOT3_ATI)) ||
	 ((op == GL_DOT4_ATI) && (curI->Opcode[0] != GL_DOT4_ATI)) ||
	 ((op != GL_DOT4_ATI) && (curI->Opcode[0] == GL_DOT4_ATI))) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "AFragmentOpATI(op)");
	 return;
      }
   }
   if ((op == GL_DOT4_ATI) &&
      (((arg1 == GL_SECONDARY_INTERPOLATOR_ATI) && ((arg1Rep == GL_ALPHA) || (arg1Rep == GL_NONE))) ||
      (((arg2 == GL_SECONDARY_INTERPOLATOR_ATI) && ((arg2Rep == GL_ALPHA) || (arg2Rep == GL_NONE)))))) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(sec_interp)");
   }

   if (!check_arith_arg(curProg, optype, arg1, arg1Rep)) {
      return;
   }
   if (arg2) {
      if (!check_arith_arg(curProg, optype, arg2, arg2Rep)) {
	 return;
      }
   }
   if (arg3) {
      if (!check_arith_arg(curProg, optype, arg3, arg3Rep)) {
	 return;
      }
      if ((arg1 >= GL_CON_0_ATI) && (arg1 <= GL_CON_7_ATI) &&
	  (arg2 >= GL_CON_0_ATI) && (arg2 <= GL_CON_7_ATI) &&
	  (arg3 >= GL_CON_0_ATI) && (arg3 <= GL_CON_7_ATI) &&
	  (arg1 != arg2) && (arg1 != arg3) && (arg2 != arg3)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "C/AFragmentOpATI(3Consts)");
	 return;
      }
   }

   /* all ok - not all fully validated though (e.g. argNMod - spec doesn't say anything) */

   curI->Opcode[optype] = op;
   curI->SrcReg[optype][0].Index = arg1;
   curI->SrcReg[optype][0].argRep = arg1Rep;
   curI->SrcReg[optype][0].argMod = arg1Mod;
   curI->ArgCount[optype] = arg_count;

   if (arg2) {
      curI->SrcReg[optype][1].Index = arg2;
      curI->SrcReg[optype][1].argRep = arg2Rep;
      curI->SrcReg[optype][1].argMod = arg2Mod;
   }

   if (arg3) {
      curI->SrcReg[optype][2].Index = arg3;
      curI->SrcReg[optype][2].argRep = arg3Rep;
      curI->SrcReg[optype][2].argMod = arg3Mod;
   }

   curI->DstReg[optype].Index = dst;
   curI->DstReg[optype].dstMod = dstMod;
   curI->DstReg[optype].dstMask = dstMask;

#if MESA_DEBUG_ATI_FS
   debug_op(optype, arg_count, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
#endif

}

void GLAPIENTRY
_mesa_ColorFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_COLOR_OP, 1, op, dst, dstMask,
			dstMod, arg1, arg1Rep, arg1Mod, 0, 0, 0, 0, 0, 0);
}

void GLAPIENTRY
_mesa_ColorFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_COLOR_OP, 2, op, dst, dstMask,
			dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep,
			arg2Mod, 0, 0, 0);
}

void GLAPIENTRY
_mesa_ColorFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod, GLuint arg3, GLuint arg3Rep,
			  GLuint arg3Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_COLOR_OP, 3, op, dst, dstMask,
			dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep,
			arg2Mod, arg3, arg3Rep, arg3Mod);
}

void GLAPIENTRY
_mesa_AlphaFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_ALPHA_OP, 1, op, dst, 0, dstMod,
			arg1, arg1Rep, arg1Mod, 0, 0, 0, 0, 0, 0);
}

void GLAPIENTRY
_mesa_AlphaFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_ALPHA_OP, 2, op, dst, 0, dstMod,
			arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, 0, 0,
			0);
}

void GLAPIENTRY
_mesa_AlphaFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod, GLuint arg3,
			  GLuint arg3Rep, GLuint arg3Mod)
{
   _mesa_FragmentOpXATI(ATI_FRAGMENT_SHADER_ALPHA_OP, 3, op, dst, 0, dstMod,
			arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3,
			arg3Rep, arg3Mod);
}

void GLAPIENTRY
_mesa_SetFragmentShaderConstantATI(GLuint dst, const GLfloat * value)
{
   GLuint dstindex;
   GET_CURRENT_CONTEXT(ctx);

   if ((dst < GL_CON_0_ATI) || (dst > GL_CON_7_ATI)) {
      /* spec says nothing about what should happen here but we can't just segfault...*/
      _mesa_error(ctx, GL_INVALID_ENUM, "glSetFragmentShaderConstantATI(dst)");
      return;
   }

   dstindex = dst - GL_CON_0_ATI;
   if (ctx->ATIFragmentShader.Compiling) {
      struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
      COPY_4V(curProg->Constants[dstindex], value);
      curProg->LocalConstDef |= 1 << dstindex;
   }
   else {
      FLUSH_VERTICES(ctx, _NEW_PROGRAM);
      COPY_4V(ctx->ATIFragmentShader.GlobalConstants[dstindex], value);
   }
}

#endif /* FEATURE_ATI_fragment_shader */
