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

#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "enums.h"
#include "mtypes.h"
#include "atifragshader.h"

#define MESA_DEBUG_ATI_FS 0

extern struct program _mesa_DummyProgram;

static void
new_inst(struct ati_fragment_shader *prog)
{
   prog->Base.NumInstructions++;
}

#if MESA_DEBUG_ATI_FS
static char *
create_dst_mod_str(GLuint mod)
{
   static char ret_str[1024];

   _mesa_memset(ret_str, 0, 1024);
   if (mod & GL_2X_BIT_ATI)
      _mesa_strncat(ret_str, "|2X", 1024);

   if (mod & GL_4X_BIT_ATI)
      _mesa_strncat(ret_str, "|4X", 1024);

   if (mod & GL_8X_BIT_ATI)
      _mesa_strncat(ret_str, "|8X", 1024);
   if (mod & GL_HALF_BIT_ATI)
      _mesa_strncat(ret_str, "|HA", 1024);
   if (mod & GL_QUARTER_BIT_ATI)
      _mesa_strncat(ret_str, "|QU", 1024);
   if (mod & GL_EIGHTH_BIT_ATI)
      _mesa_strncat(ret_str, "|EI", 1024);

   if (mod & GL_SATURATE_BIT_ATI)
      _mesa_strncat(ret_str, "|SAT", 1024);

   if (_mesa_strlen(ret_str) == 0)
      _mesa_strncat(ret_str, "NONE", 1024);
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

GLuint GLAPIENTRY
_mesa_GenFragmentShadersATI(GLuint range)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->Programs, range);
   for (i = 0; i < range; i++) {
      _mesa_HashInsert(ctx->Shared->Programs, first + i, &_mesa_DummyProgram);
   }

   return first;
}

void GLAPIENTRY
_mesa_BindFragmentShaderATI(GLuint id)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (curProg->Base.Id == id) {
      return;
   }

   if (curProg->Base.Id != 0) {
      curProg->Base.RefCount--;
      if (curProg->Base.RefCount <= 0) {
	 _mesa_HashRemove(ctx->Shared->Programs, id);
      }
   }

   /* Go bind */
   if (id == 0) {
      prog = ctx->Shared->DefaultFragmentShader;
   }
   else {
      prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
      if (!prog || prog == &_mesa_DummyProgram) {
	 /* allocate a new program now */
	 prog = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_SHADER_ATI, id);
	 if (!prog) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindFragmentShaderATI");
	    return;
	 }
	 _mesa_HashInsert(ctx->Shared->Programs, id, prog);
      }

   }

   /* do actual bind */
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) prog;

   ASSERT(ctx->ATIFragmentShader.Current);
   if (prog)
      prog->RefCount++;

   /*if (ctx->Driver.BindProgram)
      ctx->Driver.BindProgram(ctx, target, prog); */
}

void GLAPIENTRY
_mesa_DeleteFragmentShaderATI(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);

   if (id != 0) {
      struct program *prog = (struct program *)
	 _mesa_HashLookup(ctx->Shared->Programs, id);
      if (prog == &_mesa_DummyProgram) {
	 _mesa_HashRemove(ctx->Shared->Programs, id);
      }
      else if (prog) {
	 if (ctx->ATIFragmentShader.Current &&
	     ctx->ATIFragmentShader.Current->Base.Id == id) {
	    _mesa_BindFragmentShaderATI(0);
	 }
      }
#if 0
      if (!prog->DeletePending) {
	 prog->DeletePending = GL_TRUE;
	 prog->RefCount--;
      }
      if (prog->RefCount <= 0) {
	 _mesa_HashRemove(ctx->Shared->Programs, id);
	 ctx->Driver.DeleteProgram(ctx, prog);
      }
#else
      /* The ID is immediately available for re-use now */
      _mesa_HashRemove(ctx->Shared->Programs, id);
      prog->RefCount--;
      if (prog->RefCount <= 0) {
         ctx->Driver.DeleteProgram(ctx, prog);
      }
#endif
   }
}

void GLAPIENTRY
_mesa_BeginFragmentShaderATI(void)
{
   GET_CURRENT_CONTEXT(ctx);

   /* malloc the instructions here - not sure if the best place but its
      a start */
   ctx->ATIFragmentShader.Current->Instructions =
      (struct atifs_instruction *)
      _mesa_calloc(sizeof(struct atifs_instruction) * MAX_NUM_PASSES_ATI *
		   MAX_NUM_INSTRUCTIONS_PER_PASS_ATI * 2);

   ctx->ATIFragmentShader.Current->cur_pass = 0;
   ctx->ATIFragmentShader.Compiling = 1;
}

void GLAPIENTRY
_mesa_EndFragmentShaderATI(void)
{
   GET_CURRENT_CONTEXT(ctx);
#if MESA_DEBUG_ATI_FS
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   GLint i;
#endif

   ctx->ATIFragmentShader.Compiling = 0;
   ctx->ATIFragmentShader.Current->NumPasses = ctx->ATIFragmentShader.Current->cur_pass;
   ctx->ATIFragmentShader.Current->cur_pass=0;
#if MESA_DEBUG_ATI_FS
   for (i = 0; i < curProg->Base.NumInstructions; i++) {
      GLuint op0 = curProg->Instructions[i].Opcode[0];
      GLuint op1 = curProg->Instructions[i].Opcode[1];
      const char *op0_enum = op0 > 5 ? _mesa_lookup_enum_by_nr(op0) : "0";
      const char *op1_enum = op1 > 5 ? _mesa_lookup_enum_by_nr(op1) : "0";
      GLuint count0 = curProg->Instructions[i].ArgCount[0];
      GLuint count1 = curProg->Instructions[i].ArgCount[1];

      fprintf(stderr, "%2d %04X %s %d %04X %s %d\n", i, op0, op0_enum, count0,
	      op1, op1_enum, count1);
   }
#endif
}

void GLAPIENTRY
_mesa_PassTexCoordATI(GLuint dst, GLuint coord, GLenum swizzle)
{
   GET_CURRENT_CONTEXT(ctx);
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;
   GLint ci;
   struct atifs_instruction *curI;

   if (ctx->ATIFragmentShader.Current->cur_pass==1)
     ctx->ATIFragmentShader.Current->cur_pass=2;

   new_inst(curProg);
   ci = curProg->Base.NumInstructions - 1;
   /* some validation 
      if ((swizzle != GL_SWIZZLE_STR_ATI) ||
      (swizzle != GL_SWIZZLE_STQ_ATI) ||
      (swizzle != GL_SWIZZLE_STR_DR_ATI) ||
      (swizzle != GL_SWIZZLE_STQ_DQ_ATI))
    */

   /* add the instructions */
   curI = &curProg->Instructions[ci];

   curI->Opcode[0] = ATI_FRAGMENT_SHADER_PASS_OP;
   curI->DstReg[0].Index = dst;
   curI->SrcReg[0][0].Index = coord;
   curI->DstReg[0].Swizzle = swizzle;

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
   GLint ci;
   struct atifs_instruction *curI;

   if (ctx->ATIFragmentShader.Current->cur_pass==1)
     ctx->ATIFragmentShader.Current->cur_pass=2;


   new_inst(curProg);

   ci = curProg->Base.NumInstructions - 1;
   /* add the instructions */
   curI = &curProg->Instructions[ci];

   curI->Opcode[0] = ATI_FRAGMENT_SHADER_SAMPLE_OP;
   curI->DstReg[0].Index = dst;
   curI->DstReg[0].Swizzle = swizzle;

   curI->SrcReg[0][0].Index = interp;

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

   if (ctx->ATIFragmentShader.Current->cur_pass==0)
     ctx->ATIFragmentShader.Current->cur_pass=1;

   /* decide whether this is a new instruction or not ... all color instructions are new */
   if (optype == 0)
      new_inst(curProg);

   ci = curProg->Base.NumInstructions - 1;

   /* add the instructions */
   curI = &curProg->Instructions[ci];

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
   GET_CURRENT_CONTEXT(ctx);
   GLuint dstindex = dst - GL_CON_0_ATI;
   struct ati_fragment_shader *curProg = ctx->ATIFragmentShader.Current;

   COPY_4V(curProg->Constants[dstindex], value);
}
