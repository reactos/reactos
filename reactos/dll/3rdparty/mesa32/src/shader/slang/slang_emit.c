/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 2005-2008  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_emit.c
 * Emit program instructions (PI code) from IR trees.
 * \author Brian Paul
 */

/***
 *** NOTES
 ***
 *** To emit GPU instructions, we basically just do an in-order traversal
 *** of the IR tree.
 ***/


#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "slang_builtin.h"
#include "slang_emit.h"
#include "slang_mem.h"


#define PEEPHOLE_OPTIMIZATIONS 1
#define ANNOTATE 0


typedef struct
{
   slang_info_log *log;
   slang_var_table *vt;
   struct gl_program *prog;
   struct gl_program **Subroutines;
   GLuint NumSubroutines;

   /* code-gen options */
   GLboolean EmitHighLevelInstructions;
   GLboolean EmitCondCodes;
   GLboolean EmitComments;
   GLboolean EmitBeginEndSub; /* XXX TEMPORARY */
} slang_emit_info;



static struct gl_program *
new_subroutine(slang_emit_info *emitInfo, GLuint *id)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint n = emitInfo->NumSubroutines;

   emitInfo->Subroutines = (struct gl_program **)
      _mesa_realloc(emitInfo->Subroutines,
                    n * sizeof(struct gl_program),
                    (n + 1) * sizeof(struct gl_program));
   emitInfo->Subroutines[n] = ctx->Driver.NewProgram(ctx, emitInfo->prog->Target, 0);
   emitInfo->Subroutines[n]->Parameters = emitInfo->prog->Parameters;
   emitInfo->NumSubroutines++;
   *id = n;
   return emitInfo->Subroutines[n];
}


/**
 * Convert a writemask to a swizzle.  Used for testing cond codes because
 * we only want to test the cond code component(s) that was set by the
 * previous instruction.
 */
static GLuint
writemask_to_swizzle(GLuint writemask)
{
   if (writemask == WRITEMASK_X)
      return SWIZZLE_XXXX;
   if (writemask == WRITEMASK_Y)
      return SWIZZLE_YYYY;
   if (writemask == WRITEMASK_Z)
      return SWIZZLE_ZZZZ;
   if (writemask == WRITEMASK_W)
      return SWIZZLE_WWWW;
   return SWIZZLE_XYZW;  /* shouldn't be hit */
}


/**
 * Swizzle a swizzle (function composition).
 * That is, return swz2(swz1), or said another way: swz1.szw2
 * Example: swizzle_swizzle(".zwxx", ".xxyw") yields ".zzwx"
 */
GLuint
_slang_swizzle_swizzle(GLuint swz1, GLuint swz2)
{
   GLuint i, swz, s[4];
   for (i = 0; i < 4; i++) {
      GLuint c = GET_SWZ(swz2, i);
      if (c <= SWIZZLE_W)
         s[i] = GET_SWZ(swz1, c);
      else
         s[i] = c;
   }
   swz = MAKE_SWIZZLE4(s[0], s[1], s[2], s[3]);
   return swz;
}


/**
 * Allocate storage for the given node (if it hasn't already been allocated).
 *
 * Typically this is temporary storage for an intermediate result (such as
 * for a multiply or add, etc).
 *
 * If n->Store does not exist it will be created and will be of the size
 * specified by defaultSize.
 */
static GLboolean
alloc_node_storage(slang_emit_info *emitInfo, slang_ir_node *n,
                   GLint defaultSize)
{
   assert(!n->Var);
   if (!n->Store) {
      assert(defaultSize > 0);
      n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, defaultSize);
   }

   /* now allocate actual register(s).  I.e. set n->Store->Index >= 0 */
   if (n->Store->Index < 0) {
      if (!_slang_alloc_temp(emitInfo->vt, n->Store)) {
         slang_info_log_error(emitInfo->log,
                              "Ran out of registers, too many temporaries");
         _slang_free(n->Store);
         n->Store = NULL;
         return GL_FALSE;
      }
   }
   return GL_TRUE;
}


/**
 * Free temporary storage, if n->Store is, in fact, temp storage.
 * Otherwise, no-op.
 */
static void
free_node_storage(slang_var_table *vt, slang_ir_node *n)
{
   if (n->Store->File == PROGRAM_TEMPORARY &&
       n->Store->Index >= 0 &&
       n->Opcode != IR_SWIZZLE) {
      if (_slang_is_temp(vt, n->Store)) {
         _slang_free_temp(vt, n->Store);
         n->Store->Index = -1;
         n->Store = NULL; /* XXX this may not be needed */
      }
   }
}


/**
 * Helper function to allocate a short-term temporary.
 * Free it with _slang_free_temp().
 */
static GLboolean
alloc_local_temp(slang_emit_info *emitInfo, slang_ir_storage *temp, GLint size)
{
   assert(size >= 1);
   assert(size <= 4);
   _mesa_bzero(temp, sizeof(*temp));
   temp->Size = size;
   temp->File = PROGRAM_TEMPORARY;
   temp->Index = -1;
   return _slang_alloc_temp(emitInfo->vt, temp);
}


/**
 * Remove any SWIZZLE_NIL terms from given swizzle mask.
 * For a swizzle like .z??? generate .zzzz (replicate single component).
 * Else, for .wx?? generate .wxzw (insert default component for the position).
 */
static GLuint
fix_swizzle(GLuint swizzle)
{
   GLuint c0 = GET_SWZ(swizzle, 0),
      c1 = GET_SWZ(swizzle, 1),
      c2 = GET_SWZ(swizzle, 2),
      c3 = GET_SWZ(swizzle, 3);
   if (c1 == SWIZZLE_NIL && c2 == SWIZZLE_NIL && c3 == SWIZZLE_NIL) {
      /* smear first component across all positions */
      c1 = c2 = c3 = c0;
   }
   else {
      /* insert default swizzle components */
      if (c0 == SWIZZLE_NIL)
         c0 = SWIZZLE_X;
      if (c1 == SWIZZLE_NIL)
         c1 = SWIZZLE_Y;
      if (c2 == SWIZZLE_NIL)
         c2 = SWIZZLE_Z;
      if (c3 == SWIZZLE_NIL)
         c3 = SWIZZLE_W;
   }
   return MAKE_SWIZZLE4(c0, c1, c2, c3);
}



/**
 * Convert IR storage to an instruction dst register.
 */
static void
storage_to_dst_reg(struct prog_dst_register *dst, const slang_ir_storage *st,
                   GLuint writemask)
{
   const GLint size = st->Size;
   GLint index = st->Index;
   GLuint swizzle = st->Swizzle;

   /* if this is storage relative to some parent storage, walk up the tree */
   while (st->Parent) {
      st = st->Parent;
      index += st->Index;
      swizzle = _slang_swizzle_swizzle(st->Swizzle, swizzle);
   }

   assert(st->File != PROGRAM_UNDEFINED);
   dst->File = st->File;

   assert(index >= 0);
   dst->Index = index;

   assert(size >= 1);
   assert(size <= 4);

   if (size == 1) {
      GLuint comp = GET_SWZ(swizzle, 0);
      assert(comp < 4);
      dst->WriteMask = WRITEMASK_X << comp;
   }
   else {
      dst->WriteMask = writemask;
   }
}


/**
 * Convert IR storage to an instruction src register.
 */
static void
storage_to_src_reg(struct prog_src_register *src, const slang_ir_storage *st)
{
   const GLboolean relAddr = st->RelAddr;
   GLint index = st->Index;
   GLuint swizzle = st->Swizzle;

   /* if this is storage relative to some parent storage, walk up the tree */
   while (st->Parent) {
      st = st->Parent;
      index += st->Index;
      swizzle = _slang_swizzle_swizzle(fix_swizzle(st->Swizzle), swizzle);
   }

   assert(st->File >= 0);
#if 1 /* XXX temporary */
   if (st->File == PROGRAM_UNDEFINED) {
      slang_ir_storage *st0 = (slang_ir_storage *) st;
      st0->File = PROGRAM_TEMPORARY;
   }
#endif
   assert(st->File < PROGRAM_UNDEFINED);
   src->File = st->File;

   assert(index >= 0);
   src->Index = index;

   swizzle = fix_swizzle(swizzle);
   assert(GET_SWZ(swizzle, 0) <= SWIZZLE_W);
   assert(GET_SWZ(swizzle, 1) <= SWIZZLE_W);
   assert(GET_SWZ(swizzle, 2) <= SWIZZLE_W);
   assert(GET_SWZ(swizzle, 3) <= SWIZZLE_W);
   src->Swizzle = swizzle;

   src->RelAddr = relAddr;
}


/*
 * Setup an instrucion src register to point to a scalar constant.
 */
static void
constant_to_src_reg(struct prog_src_register *src, GLfloat val,
                    slang_emit_info *emitInfo)
{
   GLuint zeroSwizzle;
   GLint zeroReg;
   GLfloat value[4];

   value[0] = val;
   zeroReg = _mesa_add_unnamed_constant(emitInfo->prog->Parameters,
                                        value, 1, &zeroSwizzle);
   assert(zeroReg >= 0);

   src->File = PROGRAM_CONSTANT;
   src->Index = zeroReg;
   src->Swizzle = zeroSwizzle;
}


/**
 * Add new instruction at end of given program.
 * \param prog  the program to append instruction onto
 * \param opcode  opcode for the new instruction
 * \return pointer to the new instruction
 */
static struct prog_instruction *
new_instruction(slang_emit_info *emitInfo, gl_inst_opcode opcode)
{
   struct gl_program *prog = emitInfo->prog;
   struct prog_instruction *inst;

#if 0
   /* print prev inst */
   if (prog->NumInstructions > 0) {
      _mesa_print_instruction(prog->Instructions + prog->NumInstructions - 1);
   }
#endif
   prog->Instructions = _mesa_realloc_instructions(prog->Instructions,
                                                   prog->NumInstructions,
                                                   prog->NumInstructions + 1);
   inst = prog->Instructions + prog->NumInstructions;
   prog->NumInstructions++;
   _mesa_init_instructions(inst, 1);
   inst->Opcode = opcode;
   inst->BranchTarget = -1; /* invalid */
   /*
   printf("New inst %d: %p %s\n", prog->NumInstructions-1,(void*)inst,
          _mesa_opcode_string(inst->Opcode));
   */
   return inst;
}


/**
 * Return pointer to last instruction in program.
 */
static struct prog_instruction *
prev_instruction(slang_emit_info *emitInfo)
{
   struct gl_program *prog = emitInfo->prog;
   if (prog->NumInstructions == 0)
      return NULL;
   else
      return prog->Instructions + prog->NumInstructions - 1;
}


static struct prog_instruction *
emit(slang_emit_info *emitInfo, slang_ir_node *n);


/**
 * Return an annotation string for given node's storage.
 */
static char *
storage_annotation(const slang_ir_node *n, const struct gl_program *prog)
{
#if ANNOTATE
   const slang_ir_storage *st = n->Store;
   static char s[100] = "";

   if (!st)
      return _mesa_strdup("");

   switch (st->File) {
   case PROGRAM_CONSTANT:
      if (st->Index >= 0) {
         const GLfloat *val = prog->Parameters->ParameterValues[st->Index];
         if (st->Swizzle == SWIZZLE_NOOP)
            sprintf(s, "{%g, %g, %g, %g}", val[0], val[1], val[2], val[3]);
         else {
            sprintf(s, "%g", val[GET_SWZ(st->Swizzle, 0)]);
         }
      }
      break;
   case PROGRAM_TEMPORARY:
      if (n->Var)
         sprintf(s, "%s", (char *) n->Var->a_name);
      else
         sprintf(s, "t[%d]", st->Index);
      break;
   case PROGRAM_STATE_VAR:
   case PROGRAM_UNIFORM:
      sprintf(s, "%s", prog->Parameters->Parameters[st->Index].Name);
      break;
   case PROGRAM_VARYING:
      sprintf(s, "%s", prog->Varying->Parameters[st->Index].Name);
      break;
   case PROGRAM_INPUT:
      sprintf(s, "input[%d]", st->Index);
      break;
   case PROGRAM_OUTPUT:
      sprintf(s, "output[%d]", st->Index);
      break;
   default:
      s[0] = 0;
   }
   return _mesa_strdup(s);
#else
   return NULL;
#endif
}


/**
 * Return an annotation string for an instruction.
 */
static char *
instruction_annotation(gl_inst_opcode opcode, char *dstAnnot,
                       char *srcAnnot0, char *srcAnnot1, char *srcAnnot2)
{
#if ANNOTATE
   const char *operator;
   char *s;
   int len = 50;

   if (dstAnnot)
      len += strlen(dstAnnot);
   else
      dstAnnot = _mesa_strdup("");

   if (srcAnnot0)
      len += strlen(srcAnnot0);
   else
      srcAnnot0 = _mesa_strdup("");

   if (srcAnnot1)
      len += strlen(srcAnnot1);
   else
      srcAnnot1 = _mesa_strdup("");

   if (srcAnnot2)
      len += strlen(srcAnnot2);
   else
      srcAnnot2 = _mesa_strdup("");

   switch (opcode) {
   case OPCODE_ADD:
      operator = "+";
      break;
   case OPCODE_SUB:
      operator = "-";
      break;
   case OPCODE_MUL:
      operator = "*";
      break;
   case OPCODE_DP3:
      operator = "DP3";
      break;
   case OPCODE_DP4:
      operator = "DP4";
      break;
   case OPCODE_XPD:
      operator = "XPD";
      break;
   case OPCODE_RSQ:
      operator = "RSQ";
      break;
   case OPCODE_SGT:
      operator = ">";
      break;
   default:
      operator = ",";
   }

   s = (char *) malloc(len);
   sprintf(s, "%s = %s %s %s %s", dstAnnot,
           srcAnnot0, operator, srcAnnot1, srcAnnot2);
   assert(_mesa_strlen(s) < len);

   free(dstAnnot);
   free(srcAnnot0);
   free(srcAnnot1);
   free(srcAnnot2);

   return s;
#else
   return NULL;
#endif
}


/**
 * Emit an instruction that's just a comment.
 */
static struct prog_instruction *
emit_comment(slang_emit_info *emitInfo, const char *s)
{
   struct prog_instruction *inst = new_instruction(emitInfo, OPCODE_NOP);
   if (inst) {
      inst->Comment = _mesa_strdup(s);
   }
   return inst;
}


/**
 * Generate code for a simple arithmetic instruction.
 * Either 1, 2 or 3 operands.
 */
static struct prog_instruction *
emit_arith(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;
   const slang_ir_info *info = _slang_ir_info(n->Opcode);
   char *srcAnnot[3], *dstAnnot;
   GLuint i;
   slang_ir_node *temps[3];

   /* we'll save pointers to nodes/storage to free in temps[] until
    * the very end.
    */
   temps[0] = temps[1] = temps[2] = NULL;

   assert(info);
   assert(info->InstOpcode != OPCODE_NOP);

   srcAnnot[0] = srcAnnot[1] = srcAnnot[2] = dstAnnot = NULL;

#if PEEPHOLE_OPTIMIZATIONS
   /* Look for MAD opportunity */
   if (info->NumParams == 2 &&
       n->Opcode == IR_ADD && n->Children[0]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(IR_MUL(A, B), C) */
      emit(emitInfo, n->Children[0]->Children[0]);  /* A */
      emit(emitInfo, n->Children[0]->Children[1]);  /* B */
      emit(emitInfo, n->Children[1]);  /* C */
      /* generate MAD instruction */
      inst = new_instruction(emitInfo, OPCODE_MAD);
      /* operands: A, B, C: */
      storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[0]->Children[1]->Store);
      storage_to_src_reg(&inst->SrcReg[2], n->Children[1]->Store);
      temps[0] = n->Children[0]->Children[0];
      temps[1] = n->Children[0]->Children[1];
      temps[2] = n->Children[1];
   }
   else if (info->NumParams == 2 &&
            n->Opcode == IR_ADD && n->Children[1]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(A, IR_MUL(B, C)) */
      emit(emitInfo, n->Children[0]);  /* A */
      emit(emitInfo, n->Children[1]->Children[0]);  /* B */
      emit(emitInfo, n->Children[1]->Children[1]);  /* C */
      /* generate MAD instruction */
      inst = new_instruction(emitInfo, OPCODE_MAD);
      /* operands: B, C, A */
      storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Children[1]->Store);
      storage_to_src_reg(&inst->SrcReg[2], n->Children[0]->Store);
      temps[0] = n->Children[1]->Children[0];
      temps[1] = n->Children[1]->Children[1];
      temps[2] = n->Children[0];
   }
   else
#endif
   {
      /* normal case */

      /* gen code for children */
      for (i = 0; i < info->NumParams; i++) {
         emit(emitInfo, n->Children[i]);
         if (!n->Children[i] || !n->Children[i]->Store) {
            /* error recovery */
            return NULL;
         }
      }

      /* gen this instruction and src registers */
      inst = new_instruction(emitInfo, info->InstOpcode);
      for (i = 0; i < info->NumParams; i++)
         storage_to_src_reg(&inst->SrcReg[i], n->Children[i]->Store);

      /* annotation */
      for (i = 0; i < info->NumParams; i++)
         srcAnnot[i] = storage_annotation(n->Children[i], emitInfo->prog);

      /* record (potential) temps to free */
      for (i = 0; i < info->NumParams; i++)
         temps[i] = n->Children[i];
   }

   /* result storage */
   alloc_node_storage(emitInfo, n, -1);
   assert(n->Store->Index >= 0);
   if (n->Store->Size == 2)
      n->Writemask = WRITEMASK_XY;
   else if (n->Store->Size == 3)
      n->Writemask = WRITEMASK_XYZ;
   else if (n->Store->Size == 1)
      n->Writemask = WRITEMASK_X << GET_SWZ(n->Store->Swizzle, 0);


   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   dstAnnot = storage_annotation(n, emitInfo->prog);

   inst->Comment = instruction_annotation(inst->Opcode, dstAnnot, srcAnnot[0],
                                          srcAnnot[1], srcAnnot[2]);

   /* really free temps now */
   for (i = 0; i < 3; i++)
      if (temps[i])
         free_node_storage(emitInfo->vt, temps[i]);

   /*_mesa_print_instruction(inst);*/
   return inst;
}


/**
 * Emit code for == and != operators.  These could normally be handled
 * by emit_arith() except we need to be able to handle structure comparisons.
 */
static struct prog_instruction *
emit_compare(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;
   GLint size;

   assert(n->Opcode == IR_EQUAL || n->Opcode == IR_NOTEQUAL);

   /* gen code for children */
   emit(emitInfo, n->Children[0]);
   emit(emitInfo, n->Children[1]);

   if (n->Children[0]->Store->Size != n->Children[1]->Store->Size) {
      slang_info_log_error(emitInfo->log, "invalid operands to == or !=");
      return NULL;
   }

   /* final result is 1 bool */
   if (!alloc_node_storage(emitInfo, n, 1))
      return NULL;

   size = n->Children[0]->Store->Size;

   if (size == 1) {
      gl_inst_opcode opcode;

      opcode = n->Opcode == IR_EQUAL ? OPCODE_SEQ : OPCODE_SNE;
      inst = new_instruction(emitInfo, opcode);
      storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store);
      storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   }
   else if (size <= 4) {
      GLuint swizzle;
      gl_inst_opcode dotOp;
      slang_ir_storage tempStore;

      if (!alloc_local_temp(emitInfo, &tempStore, 4)) {
         return NULL;
         /* out of temps */
      }

      if (size == 4) {
         dotOp = OPCODE_DP4;
         swizzle = SWIZZLE_XYZW;
      }
      else if (size == 3) {
         dotOp = OPCODE_DP3;
         swizzle = SWIZZLE_XYZW;
      }
      else {
         assert(size == 2);
         dotOp = OPCODE_DP3;
         swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y);
      }

      /* Compute inequality (temp = (A != B)) */
      inst = new_instruction(emitInfo, OPCODE_SNE);
      storage_to_dst_reg(&inst->DstReg, &tempStore, n->Writemask);
      storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store);
      inst->Comment = _mesa_strdup("Compare values");

      /* Compute val = DOT(temp, temp)  (reduction) */
      inst = new_instruction(emitInfo, dotOp);
      storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
      storage_to_src_reg(&inst->SrcReg[0], &tempStore);
      storage_to_src_reg(&inst->SrcReg[1], &tempStore);
      inst->SrcReg[0].Swizzle = inst->SrcReg[1].Swizzle = swizzle; /*override*/
      inst->Comment = _mesa_strdup("Reduce vec to bool");

      _slang_free_temp(emitInfo->vt, &tempStore); /* free temp */

      if (n->Opcode == IR_EQUAL) {
         /* compute val = !val.x  with SEQ val, val, 0; */
         inst = new_instruction(emitInfo, OPCODE_SEQ);
         storage_to_src_reg(&inst->SrcReg[0], n->Store);
         constant_to_src_reg(&inst->SrcReg[1], 0.0, emitInfo);
         storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
         inst->Comment = _mesa_strdup("Invert true/false");
      }
   }
   else {
      /* size > 4, struct or array compare.
       * XXX this won't work reliably for structs with padding!!
       */
      GLint i, num = (n->Children[0]->Store->Size + 3) / 4;
      slang_ir_storage accTemp, sneTemp;

      if (!alloc_local_temp(emitInfo, &accTemp, 4))
         return NULL;

      if (!alloc_local_temp(emitInfo, &sneTemp, 4))
         return NULL;

      for (i = 0; i < num; i++) {
         /* SNE sneTemp, left[i], right[i] */
         inst = new_instruction(emitInfo, OPCODE_SNE);
         storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
         storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store);
         inst->SrcReg[0].Index += i;
         inst->SrcReg[1].Index += i;
         if (i == 0) {
            storage_to_dst_reg(&inst->DstReg, &accTemp, WRITEMASK_XYZW);
            inst->Comment = _mesa_strdup("Begin struct/array comparison");
         }
         else {
            storage_to_dst_reg(&inst->DstReg, &sneTemp, WRITEMASK_XYZW);

            /* ADD accTemp, accTemp, sneTemp; # like logical-OR */
            inst = new_instruction(emitInfo, OPCODE_ADD);
            storage_to_dst_reg(&inst->DstReg, &accTemp, WRITEMASK_XYZW);
            storage_to_src_reg(&inst->SrcReg[0], &accTemp);
            storage_to_src_reg(&inst->SrcReg[1], &sneTemp);
         }
      }

      /* compute accTemp.x || accTemp.y || accTemp.z || accTemp.w with DOT4 */
      inst = new_instruction(emitInfo, OPCODE_DP4);
      storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
      storage_to_src_reg(&inst->SrcReg[0], &accTemp);
      storage_to_src_reg(&inst->SrcReg[1], &accTemp);
      inst->Comment = _mesa_strdup("End struct/array comparison");

      if (n->Opcode == IR_EQUAL) {
         /* compute tmp.x = !tmp.x  via tmp.x = (tmp.x == 0) */
         inst = new_instruction(emitInfo, OPCODE_SEQ);
         storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
         storage_to_src_reg(&inst->SrcReg[0], n->Store);
         constant_to_src_reg(&inst->SrcReg[1], 0.0, emitInfo);
         inst->Comment = _mesa_strdup("Invert true/false");
      }

      _slang_free_temp(emitInfo->vt, &accTemp);
      _slang_free_temp(emitInfo->vt, &sneTemp);
   }

   /* free temps */
   free_node_storage(emitInfo->vt, n->Children[0]);
   free_node_storage(emitInfo->vt, n->Children[1]);

   return inst;
}



/**
 * Generate code for an IR_CLAMP instruction.
 */
static struct prog_instruction *
emit_clamp(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;
   slang_ir_node tmpNode;

   assert(n->Opcode == IR_CLAMP);
   /* ch[0] = value
    * ch[1] = min limit
    * ch[2] = max limit
    */

   inst = emit(emitInfo, n->Children[0]);

   /* If lower limit == 0.0 and upper limit == 1.0,
    *    set prev instruction's SaturateMode field to SATURATE_ZERO_ONE.
    * Else,
    *    emit OPCODE_MIN, OPCODE_MAX sequence.
    */
#if 0
   /* XXX this isn't quite finished yet */
   if (n->Children[1]->Opcode == IR_FLOAT &&
       n->Children[1]->Value[0] == 0.0 &&
       n->Children[1]->Value[1] == 0.0 &&
       n->Children[1]->Value[2] == 0.0 &&
       n->Children[1]->Value[3] == 0.0 &&
       n->Children[2]->Opcode == IR_FLOAT &&
       n->Children[2]->Value[0] == 1.0 &&
       n->Children[2]->Value[1] == 1.0 &&
       n->Children[2]->Value[2] == 1.0 &&
       n->Children[2]->Value[3] == 1.0) {
      if (!inst) {
         inst = prev_instruction(prog);
      }
      if (inst && inst->Opcode != OPCODE_NOP) {
         /* and prev instruction's DstReg matches n->Children[0]->Store */
         inst->SaturateMode = SATURATE_ZERO_ONE;
         n->Store = n->Children[0]->Store;
         return inst;
      }
   }
#endif

   if (!alloc_node_storage(emitInfo, n, n->Children[0]->Store->Size))
      return NULL;

   emit(emitInfo, n->Children[1]);
   emit(emitInfo, n->Children[2]);

   /* Some GPUs don't allow reading from output registers.  So if the
    * dest for this clamp() is an output reg, we can't use that reg for
    * the intermediate result.  Use a temp register instead.
    */
   _mesa_bzero(&tmpNode, sizeof(tmpNode));
   alloc_node_storage(emitInfo, &tmpNode, n->Store->Size);

   /* tmp = max(ch[0], ch[1]) */
   inst = new_instruction(emitInfo, OPCODE_MAX);
   storage_to_dst_reg(&inst->DstReg, tmpNode.Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store);

   /* n->dest = min(tmp, ch[2]) */
   inst = new_instruction(emitInfo, OPCODE_MIN);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], tmpNode.Store);
   storage_to_src_reg(&inst->SrcReg[1], n->Children[2]->Store);

   free_node_storage(emitInfo->vt, &tmpNode);

   return inst;
}


static struct prog_instruction *
emit_negation(slang_emit_info *emitInfo, slang_ir_node *n)
{
   /* Implement as MOV dst, -src; */
   /* XXX we could look at the previous instruction and in some circumstances
    * modify it to accomplish the negation.
    */
   struct prog_instruction *inst;

   emit(emitInfo, n->Children[0]);

   if (!alloc_node_storage(emitInfo, n, n->Children[0]->Store->Size))
      return NULL;

   inst = new_instruction(emitInfo, OPCODE_MOV);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   inst->SrcReg[0].NegateBase = NEGATE_XYZW;
   return inst;
}


static struct prog_instruction *
emit_label(slang_emit_info *emitInfo, const slang_ir_node *n)
{
   assert(n->Label);
#if 0
   /* XXX this fails in loop tail code - investigate someday */
   assert(_slang_label_get_location(n->Label) < 0);
   _slang_label_set_location(n->Label, emitInfo->prog->NumInstructions,
                             emitInfo->prog);
#else
   if (_slang_label_get_location(n->Label) < 0)
      _slang_label_set_location(n->Label, emitInfo->prog->NumInstructions,
                                emitInfo->prog);
#endif
   return NULL;
}


/**
 * Emit code for a function call.
 * Note that for each time a function is called, we emit the function's
 * body code again because the set of available registers may be different.
 */
static struct prog_instruction *
emit_fcall(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct gl_program *progSave;
   struct prog_instruction *inst;
   GLuint subroutineId;

   assert(n->Opcode == IR_CALL);
   assert(n->Label);

   /* save/push cur program */
   progSave = emitInfo->prog;
   emitInfo->prog = new_subroutine(emitInfo, &subroutineId);

   _slang_label_set_location(n->Label, emitInfo->prog->NumInstructions,
                             emitInfo->prog);

   if (emitInfo->EmitBeginEndSub) {
      /* BGNSUB isn't a real instruction.
       * We require a label (i.e. "foobar:") though, if we're going to
       * print the program in the NV format.  The BNGSUB instruction is
       * really just a NOP to attach the label to.
       */
      inst = new_instruction(emitInfo, OPCODE_BGNSUB);
      inst->Comment = _mesa_strdup(n->Label->Name);
   }

   /* body of function: */
   emit(emitInfo, n->Children[0]);
   n->Store = n->Children[0]->Store;

   /* add RET instruction now, if needed */
   inst = prev_instruction(emitInfo);
   if (inst && inst->Opcode != OPCODE_RET) {
      inst = new_instruction(emitInfo, OPCODE_RET);
   }

   if (emitInfo->EmitBeginEndSub) {
      inst = new_instruction(emitInfo, OPCODE_ENDSUB);
      inst->Comment = _mesa_strdup(n->Label->Name);
   }

   /* pop/restore cur program */
   emitInfo->prog = progSave;

   /* emit the function call */
   inst = new_instruction(emitInfo, OPCODE_CAL);
   /* The branch target is just the subroutine number (changed later) */
   inst->BranchTarget = subroutineId;
   inst->Comment = _mesa_strdup(n->Label->Name);
   assert(inst->BranchTarget >= 0);

   return inst;
}


/**
 * Emit code for a 'return' statement.
 */
static struct prog_instruction *
emit_return(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;
   assert(n);
   assert(n->Opcode == IR_RETURN);
   assert(n->Label);
   inst = new_instruction(emitInfo, OPCODE_RET);
   inst->DstReg.CondMask = COND_TR;  /* always return */
   return inst;
}


static struct prog_instruction *
emit_kill(slang_emit_info *emitInfo)
{
   struct gl_fragment_program *fp;
   struct prog_instruction *inst;
   /* NV-KILL - discard fragment depending on condition code.
    * Note that ARB-KILL depends on sign of vector operand.
    */
   inst = new_instruction(emitInfo, OPCODE_KIL_NV);
   inst->DstReg.CondMask = COND_TR;  /* always kill */

   assert(emitInfo->prog->Target == GL_FRAGMENT_PROGRAM_ARB);
   fp = (struct gl_fragment_program *) emitInfo->prog;
   fp->UsesKill = GL_TRUE;

   return inst;
}


static struct prog_instruction *
emit_tex(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   (void) emit(emitInfo, n->Children[1]);

   if (n->Opcode == IR_TEX) {
      inst = new_instruction(emitInfo, OPCODE_TEX);
   }
   else if (n->Opcode == IR_TEXB) {
      inst = new_instruction(emitInfo, OPCODE_TXB);
   }
   else {
      assert(n->Opcode == IR_TEXP);
      inst = new_instruction(emitInfo, OPCODE_TXP);
   }

   if (!alloc_node_storage(emitInfo, n, 4))
      return NULL;

   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   /* Child[1] is the coord */
   assert(n->Children[1]->Store->Index >= 0);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store);

   /* Child[0] is the sampler (a uniform which'll indicate the texture unit) */
   assert(n->Children[0]->Store);
   /* Store->Index is the sampler index */
   assert(n->Children[0]->Store->Index >= 0);
   /* Store->Size is the texture target */
   assert(n->Children[0]->Store->Size >= TEXTURE_1D_INDEX);
   assert(n->Children[0]->Store->Size <= TEXTURE_RECT_INDEX);

   inst->TexSrcTarget = n->Children[0]->Store->Size;
#if 0
   inst->TexSrcUnit = 27; /* Dummy value; the TexSrcUnit will be computed at
                           * link time, using the sampler uniform's value.
                           */
   inst->Sampler = n->Children[0]->Store->Index; /* i.e. uniform's index */
#else
   inst->TexSrcUnit = n->Children[0]->Store->Index; /* i.e. uniform's index */
#endif
   return inst;
}


/**
 * Assignment/copy
 */
static struct prog_instruction *
emit_copy(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   assert(n->Opcode == IR_COPY);

   /* lhs */
   emit(emitInfo, n->Children[0]);
   if (!n->Children[0]->Store || n->Children[0]->Store->Index < 0) {
      /* an error should have been already recorded */
      return NULL;
   }

   /* rhs */
   assert(n->Children[1]);
   inst = emit(emitInfo, n->Children[1]);

   if (!n->Children[1]->Store || n->Children[1]->Store->Index < 0) {
      if (!emitInfo->log->text) {
         slang_info_log_error(emitInfo->log, "invalid assignment");
      }
      return NULL;
   }

   assert(n->Children[1]->Store->Index >= 0);

   /*assert(n->Children[0]->Store->Size == n->Children[1]->Store->Size);*/

   n->Store = n->Children[0]->Store;

#if PEEPHOLE_OPTIMIZATIONS
   if (inst &&
       _slang_is_temp(emitInfo->vt, n->Children[1]->Store) &&
       (inst->DstReg.File == n->Children[1]->Store->File) &&
       (inst->DstReg.Index == n->Children[1]->Store->Index)) {
      /* Peephole optimization:
       * The Right-Hand-Side has its results in a temporary place.
       * Modify the RHS (and the prev instruction) to store its results
       * in the destination specified by n->Children[0].
       * Then, this MOVE is a no-op.
       */
      if (n->Children[1]->Opcode != IR_SWIZZLE)
         _slang_free_temp(emitInfo->vt, n->Children[1]->Store);
      *n->Children[1]->Store = *n->Children[0]->Store;

      /* fixup the previous instruction (which stored the RHS result) */
      assert(n->Children[0]->Store->Index >= 0);

      /* use tighter writemask when possible */
      if (n->Writemask == WRITEMASK_XYZW)
         n->Writemask = inst->DstReg.WriteMask;

      storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
      return inst;
   }
   else
#endif
   {
      if (n->Children[0]->Store->Size > 4) {
         /* move matrix/struct etc (block of registers) */
         slang_ir_storage dstStore = *n->Children[0]->Store;
         slang_ir_storage srcStore = *n->Children[1]->Store;
         GLint size = srcStore.Size;
         ASSERT(n->Children[0]->Writemask == WRITEMASK_XYZW);
         ASSERT(n->Children[1]->Store->Swizzle == SWIZZLE_NOOP);
         dstStore.Size = 4;
         srcStore.Size = 4;
         while (size >= 4) {
            inst = new_instruction(emitInfo, OPCODE_MOV);
            inst->Comment = _mesa_strdup("IR_COPY block");
            storage_to_dst_reg(&inst->DstReg, &dstStore, n->Writemask);
            storage_to_src_reg(&inst->SrcReg[0], &srcStore);
            srcStore.Index++;
            dstStore.Index++;
            size -= 4;
         }
      }
      else {
         /* single register move */
         char *srcAnnot, *dstAnnot;
         inst = new_instruction(emitInfo, OPCODE_MOV);
         assert(n->Children[0]->Store->Index >= 0);
         storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
         storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store);
         dstAnnot = storage_annotation(n->Children[0], emitInfo->prog);
         srcAnnot = storage_annotation(n->Children[1], emitInfo->prog);
         inst->Comment = instruction_annotation(inst->Opcode, dstAnnot,
                                                srcAnnot, NULL, NULL);
      }
      free_node_storage(emitInfo->vt, n->Children[1]);
      return inst;
   }
}


/**
 * An IR_COND node wraps a boolean expression which is used by an
 * IF or WHILE test.  This is where we'll set condition codes, if needed.
 */
static struct prog_instruction *
emit_cond(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   assert(n->Opcode == IR_COND);

   if (!n->Children[0])
      return NULL;

   /* emit code for the expression */
   inst = emit(emitInfo, n->Children[0]);

   if (!n->Children[0]->Store) {
      /* error recovery */
      return NULL;
   }

   assert(n->Children[0]->Store);
   /*assert(n->Children[0]->Store->Size == 1);*/

   if (emitInfo->EmitCondCodes) {
      if (inst &&
          n->Children[0]->Store &&
          inst->DstReg.File == n->Children[0]->Store->File &&
          inst->DstReg.Index == n->Children[0]->Store->Index) {
         /* The previous instruction wrote to the register who's value
          * we're testing.  Just fix that instruction so that the
          * condition codes are computed.
          */
         inst->CondUpdate = GL_TRUE;
         n->Store = n->Children[0]->Store;
         return inst;
      }
      else {
         /* This'll happen for things like "if (i) ..." where no code
          * is normally generated for the expression "i".
          * Generate a move instruction just to set condition codes.
          */
         if (!alloc_node_storage(emitInfo, n, 1))
            return NULL;
         inst = new_instruction(emitInfo, OPCODE_MOV);
         inst->CondUpdate = GL_TRUE;
         storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
         storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
         _slang_free_temp(emitInfo->vt, n->Store);
         inst->Comment = _mesa_strdup("COND expr");
         return inst;
      }
   }
   else {
      /* No-op: the boolean result of the expression is in a regular reg */
      n->Store = n->Children[0]->Store;
      return inst;
   }
}


/**
 * Logical-NOT
 */
static struct prog_instruction *
emit_not(slang_emit_info *emitInfo, slang_ir_node *n)
{
   static const struct {
      gl_inst_opcode op, opNot;
   } operators[] = {
      { OPCODE_SLT, OPCODE_SGE },
      { OPCODE_SLE, OPCODE_SGT },
      { OPCODE_SGT, OPCODE_SLE },
      { OPCODE_SGE, OPCODE_SLT },
      { OPCODE_SEQ, OPCODE_SNE },
      { OPCODE_SNE, OPCODE_SEQ },
      { 0, 0 }
   };
   struct prog_instruction *inst;
   GLuint i;

   /* child expr */
   inst = emit(emitInfo, n->Children[0]);

#if PEEPHOLE_OPTIMIZATIONS
   if (inst) {
      /* if the prev instruction was a comparison instruction, invert it */
      for (i = 0; operators[i].op; i++) {
         if (inst->Opcode == operators[i].op) {
            inst->Opcode = operators[i].opNot;
            n->Store = n->Children[0]->Store;
            return inst;
         }
      }
   }
#endif

   /* else, invert using SEQ (v = v == 0) */
   if (!alloc_node_storage(emitInfo, n, n->Children[0]->Store->Size))
      return NULL;

   inst = new_instruction(emitInfo, OPCODE_SEQ);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   constant_to_src_reg(&inst->SrcReg[1], 0.0, emitInfo);
   free_node_storage(emitInfo->vt, n->Children[0]);

   inst->Comment = _mesa_strdup("NOT");
   return inst;
}


static struct prog_instruction *
emit_if(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct gl_program *prog = emitInfo->prog;
   GLuint ifInstLoc, elseInstLoc = 0;
   GLuint condWritemask = 0;

   /* emit condition expression code */
   {
      struct prog_instruction *inst;
      inst = emit(emitInfo, n->Children[0]);
      if (emitInfo->EmitCondCodes) {
         if (!inst) {
            /* error recovery */
            return NULL;
         }
         condWritemask = inst->DstReg.WriteMask;
      }
   }

   if (!n->Children[0]->Store)
      return NULL;

#if 0
   assert(n->Children[0]->Store->Size == 1); /* a bool! */
#endif

   ifInstLoc = prog->NumInstructions;
   if (emitInfo->EmitHighLevelInstructions) {
      struct prog_instruction *ifInst = new_instruction(emitInfo, OPCODE_IF);
      if (emitInfo->EmitCondCodes) {
         ifInst->DstReg.CondMask = COND_NE;  /* if cond is non-zero */
         /* only test the cond code (1 of 4) that was updated by the
          * previous instruction.
          */
         ifInst->DstReg.CondSwizzle = writemask_to_swizzle(condWritemask);
      }
      else {
         /* test reg.x */
         storage_to_src_reg(&ifInst->SrcReg[0], n->Children[0]->Store);
      }
   }
   else {
      /* conditional jump to else, or endif */
      struct prog_instruction *ifInst = new_instruction(emitInfo, OPCODE_BRA);
      ifInst->DstReg.CondMask = COND_EQ;  /* BRA if cond is zero */
      ifInst->Comment = _mesa_strdup("if zero");
      ifInst->DstReg.CondSwizzle = writemask_to_swizzle(condWritemask);
   }

   /* if body */
   emit(emitInfo, n->Children[1]);

   if (n->Children[2]) {
      /* have else body */
      elseInstLoc = prog->NumInstructions;
      if (emitInfo->EmitHighLevelInstructions) {
         (void) new_instruction(emitInfo, OPCODE_ELSE);
      }
      else {
         /* jump to endif instruction */
         struct prog_instruction *inst;
         inst = new_instruction(emitInfo, OPCODE_BRA);
         inst->Comment = _mesa_strdup("else");
         inst->DstReg.CondMask = COND_TR;  /* always branch */
      }
      prog->Instructions[ifInstLoc].BranchTarget = prog->NumInstructions;
      emit(emitInfo, n->Children[2]);
   }
   else {
      /* no else body */
      prog->Instructions[ifInstLoc].BranchTarget = prog->NumInstructions;
   }

   if (emitInfo->EmitHighLevelInstructions) {
      (void) new_instruction(emitInfo, OPCODE_ENDIF);
   }

   if (n->Children[2]) {
      prog->Instructions[elseInstLoc].BranchTarget = prog->NumInstructions;
   }
   return NULL;
}


static struct prog_instruction *
emit_loop(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct gl_program *prog = emitInfo->prog;
   struct prog_instruction *endInst;
   GLuint beginInstLoc, tailInstLoc, endInstLoc;
   slang_ir_node *ir;

   /* emit OPCODE_BGNLOOP */
   beginInstLoc = prog->NumInstructions;
   if (emitInfo->EmitHighLevelInstructions) {
      (void) new_instruction(emitInfo, OPCODE_BGNLOOP);
   }

   /* body */
   emit(emitInfo, n->Children[0]);

   /* tail */
   tailInstLoc = prog->NumInstructions;
   if (n->Children[1]) {
      if (emitInfo->EmitComments)
         emit_comment(emitInfo, "Loop tail code:");
      emit(emitInfo, n->Children[1]);
   }

   endInstLoc = prog->NumInstructions;
   if (emitInfo->EmitHighLevelInstructions) {
      /* emit OPCODE_ENDLOOP */
      endInst = new_instruction(emitInfo, OPCODE_ENDLOOP);
   }
   else {
      /* emit unconditional BRA-nch */
      endInst = new_instruction(emitInfo, OPCODE_BRA);
      endInst->DstReg.CondMask = COND_TR;  /* always true */
   }
   /* ENDLOOP's BranchTarget points to the BGNLOOP inst */
   endInst->BranchTarget = beginInstLoc;

   if (emitInfo->EmitHighLevelInstructions) {
      /* BGNLOOP's BranchTarget points to the ENDLOOP inst */
      prog->Instructions[beginInstLoc].BranchTarget = prog->NumInstructions -1;
   }

   /* Done emitting loop code.  Now walk over the loop's linked list of
    * BREAK and CONT nodes, filling in their BranchTarget fields (which
    * will point to the ENDLOOP+1 or BGNLOOP instructions, respectively).
    */
   for (ir = n->List; ir; ir = ir->List) {
      struct prog_instruction *inst = prog->Instructions + ir->InstLocation;
      assert(inst->BranchTarget < 0);
      if (ir->Opcode == IR_BREAK ||
          ir->Opcode == IR_BREAK_IF_TRUE) {
         assert(inst->Opcode == OPCODE_BRK ||
                inst->Opcode == OPCODE_BRA);
         /* go to instruction after end of loop */
         inst->BranchTarget = endInstLoc + 1;
      }
      else {
         assert(ir->Opcode == IR_CONT ||
                ir->Opcode == IR_CONT_IF_TRUE);
         assert(inst->Opcode == OPCODE_CONT ||
                inst->Opcode == OPCODE_BRA);
         /* go to instruction at tail of loop */
         inst->BranchTarget = endInstLoc;
      }
   }
   return NULL;
}


/**
 * Unconditional "continue" or "break" statement.
 * Either OPCODE_CONT, OPCODE_BRK or OPCODE_BRA will be emitted.
 */
static struct prog_instruction *
emit_cont_break(slang_emit_info *emitInfo, slang_ir_node *n)
{
   gl_inst_opcode opcode;
   struct prog_instruction *inst;

   if (n->Opcode == IR_CONT) {
      /* we need to execute the loop's tail code before doing CONT */
      assert(n->Parent);
      assert(n->Parent->Opcode == IR_LOOP);
      if (n->Parent->Children[1]) {
         /* emit tail code */
         if (emitInfo->EmitComments) {
            emit_comment(emitInfo, "continue - tail code:");
         }
         emit(emitInfo, n->Parent->Children[1]);
      }
   }

   /* opcode selection */
   if (emitInfo->EmitHighLevelInstructions) {
      opcode = (n->Opcode == IR_CONT) ? OPCODE_CONT : OPCODE_BRK;
   }
   else {
      opcode = OPCODE_BRA;
   }
   n->InstLocation = emitInfo->prog->NumInstructions;
   inst = new_instruction(emitInfo, opcode);
   inst->DstReg.CondMask = COND_TR;  /* always true */
   return inst;
}


/**
 * Conditional "continue" or "break" statement.
 * Either OPCODE_CONT, OPCODE_BRK or OPCODE_BRA will be emitted.
 */
static struct prog_instruction *
emit_cont_break_if_true(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   assert(n->Opcode == IR_CONT_IF_TRUE ||
          n->Opcode == IR_BREAK_IF_TRUE);

   /* evaluate condition expr, setting cond codes */
   inst = emit(emitInfo, n->Children[0]);
   if (emitInfo->EmitCondCodes) {
      assert(inst);
      inst->CondUpdate = GL_TRUE;
   }

   n->InstLocation = emitInfo->prog->NumInstructions;

   /* opcode selection */
   if (emitInfo->EmitHighLevelInstructions) {
      const gl_inst_opcode opcode
         = (n->Opcode == IR_CONT_IF_TRUE) ? OPCODE_CONT : OPCODE_BRK;
      if (emitInfo->EmitCondCodes) {
         /* Get the writemask from the previous instruction which set
          * the condcodes.  Use that writemask as the CondSwizzle.
          */
         const GLuint condWritemask = inst->DstReg.WriteMask;
         inst = new_instruction(emitInfo, opcode);
         inst->DstReg.CondMask = COND_NE;
         inst->DstReg.CondSwizzle = writemask_to_swizzle(condWritemask);
         return inst;
      }
      else {
         /* IF reg
          *    BRK/CONT;
          * ENDIF
          */
         GLint ifInstLoc;
         ifInstLoc = emitInfo->prog->NumInstructions;
         inst = new_instruction(emitInfo, OPCODE_IF);
         storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
         n->InstLocation = emitInfo->prog->NumInstructions;

         inst = new_instruction(emitInfo, opcode);
         inst = new_instruction(emitInfo, OPCODE_ENDIF);

         emitInfo->prog->Instructions[ifInstLoc].BranchTarget
            = emitInfo->prog->NumInstructions;
         return inst;
      }
   }
   else {
      const GLuint condWritemask = inst->DstReg.WriteMask;
      assert(emitInfo->EmitCondCodes);
      inst = new_instruction(emitInfo, OPCODE_BRA);
      inst->DstReg.CondMask = COND_NE;
      inst->DstReg.CondSwizzle = writemask_to_swizzle(condWritemask);
      return inst;
   }
}


static struct prog_instruction *
emit_swizzle(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   inst = emit(emitInfo, n->Children[0]);

   /* setup storage info, if needed */
   if (!n->Store->Parent)
      n->Store->Parent = n->Children[0]->Store;

   assert(n->Store->Parent);

   return inst;
}


/**
 * Dereference array element.  Just resolve storage for the array
 * element represented by this node.
 */
static struct prog_instruction *
emit_array_element(slang_emit_info *emitInfo, slang_ir_node *n)
{
   slang_ir_storage *root;

   assert(n->Opcode == IR_ELEMENT);
   assert(n->Store);
   assert(n->Store->File == PROGRAM_UNDEFINED);
   assert(n->Store->Parent);
   assert(n->Store->Size > 0);

   root = n->Store;
   while (root->Parent)
      root = root->Parent;

   if (root->File == PROGRAM_STATE_VAR) {
      GLint index = _slang_alloc_statevar(n, emitInfo->prog->Parameters);
      assert(n->Store->Index == index);
      return NULL;
   }

   /* do codegen for array */
   emit(emitInfo, n->Children[0]);

   if (n->Children[1]->Opcode == IR_FLOAT) {
      /* Constant array index.
       * Set Store's index to be the offset of the array element in
       * the register file.
       */
      const GLint element = (GLint) n->Children[1]->Value[0];
      const GLint sz = (n->Store->Size + 3) / 4; /* size in slots/registers */

      n->Store->Index = sz * element;
      assert(n->Store->Parent);
   }
   else {
      /* Variable array index */
      struct prog_instruction *inst;

      /* do codegen for array index expression */
      emit(emitInfo, n->Children[1]);

      inst = new_instruction(emitInfo, OPCODE_ARL);

      storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
      storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store);

      inst->DstReg.File = PROGRAM_ADDRESS;
      inst->DstReg.Index = 0; /* always address register [0] */
      inst->Comment = _mesa_strdup("ARL ADDR");

      n->Store->RelAddr = GL_TRUE;
   }

   /* if array element size is one, make sure we only access X */
   if (n->Store->Size == 1)
      n->Store->Swizzle = SWIZZLE_XXXX;

   return NULL; /* no instruction */
}


/**
 * Resolve storage for accessing a structure field.
 */
static struct prog_instruction *
emit_struct_field(slang_emit_info *emitInfo, slang_ir_node *n)
{
   slang_ir_storage *root = n->Store;

   assert(n->Opcode == IR_FIELD);

   while (root->Parent)
      root = root->Parent;

   /* If this is the field of a state var, allocate constant/uniform
    * storage for it now if we haven't already.
    * Note that we allocate storage (uniform/constant slots) for state
    * variables here rather than at declaration time so we only allocate
    * space for the ones that we actually use!
    */
   if (root->File == PROGRAM_STATE_VAR) {
      root->Index = _slang_alloc_statevar(n, emitInfo->prog->Parameters);
      if (root->Index < 0) {
         slang_info_log_error(emitInfo->log, "Error parsing state variable");
         return NULL;
      }
   }
   else {
      /* do codegen for struct */
      emit(emitInfo, n->Children[0]);
   }

   return NULL; /* no instruction */
}


/**
 * Emit code for a variable declaration.
 * This usually doesn't result in any code generation, but just
 * memory allocation.
 */
static struct prog_instruction *
emit_var_decl(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   assert(n->Store);
   assert(n->Store->File != PROGRAM_UNDEFINED);
   assert(n->Store->Size > 0);
   /*assert(n->Store->Index < 0);*/

   if (!n->Var || n->Var->isTemp) {
      /* a nameless/temporary variable, will be freed after first use */
      /*NEW*/
      if (n->Store->Index < 0 && !_slang_alloc_temp(emitInfo->vt, n->Store)) {
         slang_info_log_error(emitInfo->log,
                              "Ran out of registers, too many temporaries");
         return NULL;
      }
   }
   else {
      /* a regular variable */
      _slang_add_variable(emitInfo->vt, n->Var);
      if (!_slang_alloc_var(emitInfo->vt, n->Store)) {
         slang_info_log_error(emitInfo->log,
                              "Ran out of registers, too many variables");
         return NULL;
      }
      /*
        printf("IR_VAR_DECL %s %d store %p\n",
        (char*) n->Var->a_name, n->Store->Index, (void*) n->Store);
      */
      assert(n->Var->aux == n->Store);
   }
   if (emitInfo->EmitComments) {
      /* emit NOP with comment describing the variable's storage location */
      char s[1000];
      sprintf(s, "TEMP[%d]%s = variable %s (size %d)",
              n->Store->Index,
              _mesa_swizzle_string(n->Store->Swizzle, 0, GL_FALSE), 
              (n->Var ? (char *) n->Var->a_name : "anonymous"),
              n->Store->Size);
      inst = emit_comment(emitInfo, s);
      return inst;
   }
   return NULL;
}


/**
 * Emit code for a reference to a variable.
 * Actually, no code is generated but we may do some memory alloation.
 * In particular, state vars (uniforms) are allocated on an as-needed basis.
 */
static struct prog_instruction *
emit_var_ref(slang_emit_info *emitInfo, slang_ir_node *n)
{
   assert(n->Store);
   assert(n->Store->File != PROGRAM_UNDEFINED);

   if (n->Store->File == PROGRAM_STATE_VAR && n->Store->Index < 0) {
      n->Store->Index = _slang_alloc_statevar(n, emitInfo->prog->Parameters);
   }
   else if (n->Store->File == PROGRAM_UNIFORM) {
      /* mark var as used */
      _mesa_use_uniform(emitInfo->prog->Parameters, (char *) n->Var->a_name);
   }

   if (n->Store->Index < 0) {
      /* probably ran out of registers */
      return NULL;
   }
   assert(n->Store->Size > 0);

   return NULL;
}


static struct prog_instruction *
emit(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;
   if (!n)
      return NULL;

   if (emitInfo->log->error_flag) {
      return NULL;
   }

   switch (n->Opcode) {
   case IR_SEQ:
      /* sequence of two sub-trees */
      assert(n->Children[0]);
      assert(n->Children[1]);
      emit(emitInfo, n->Children[0]);
      if (emitInfo->log->error_flag)
         return NULL;
      inst = emit(emitInfo, n->Children[1]);
#if 0
      assert(!n->Store);
#endif
      n->Store = n->Children[1]->Store;
      return inst;

   case IR_SCOPE:
      /* new variable scope */
      _slang_push_var_table(emitInfo->vt);
      inst = emit(emitInfo, n->Children[0]);
      _slang_pop_var_table(emitInfo->vt);
      return inst;

   case IR_VAR_DECL:
      /* Variable declaration - allocate a register for it */
      inst = emit_var_decl(emitInfo, n);
      return inst;

   case IR_VAR:
      /* Reference to a variable
       * Storage should have already been resolved/allocated.
       */
      return emit_var_ref(emitInfo, n);

   case IR_ELEMENT:
      return emit_array_element(emitInfo, n);
   case IR_FIELD:
      return emit_struct_field(emitInfo, n);
   case IR_SWIZZLE:
      return emit_swizzle(emitInfo, n);

   /* Simple arithmetic */
   /* unary */
   case IR_MOVE:
   case IR_RSQ:
   case IR_RCP:
   case IR_FLOOR:
   case IR_FRAC:
   case IR_F_TO_I:
   case IR_I_TO_F:
   case IR_ABS:
   case IR_SIN:
   case IR_COS:
   case IR_DDX:
   case IR_DDY:
   case IR_EXP:
   case IR_EXP2:
   case IR_LOG2:
   case IR_NOISE1:
   case IR_NOISE2:
   case IR_NOISE3:
   case IR_NOISE4:
   /* binary */
   case IR_ADD:
   case IR_SUB:
   case IR_MUL:
   case IR_DOT4:
   case IR_DOT3:
   case IR_CROSS:
   case IR_MIN:
   case IR_MAX:
   case IR_SEQUAL:
   case IR_SNEQUAL:
   case IR_SGE:
   case IR_SGT:
   case IR_SLE:
   case IR_SLT:
   case IR_POW:
   /* trinary operators */
   case IR_LRP:
      return emit_arith(emitInfo, n);

   case IR_EQUAL:
   case IR_NOTEQUAL:
      return emit_compare(emitInfo, n);

   case IR_CLAMP:
      return emit_clamp(emitInfo, n);
   case IR_TEX:
   case IR_TEXB:
   case IR_TEXP:
      return emit_tex(emitInfo, n);
   case IR_NEG:
      return emit_negation(emitInfo, n);
   case IR_FLOAT:
      /* find storage location for this float constant */
      n->Store->Index = _mesa_add_unnamed_constant(emitInfo->prog->Parameters,
                                                   n->Value,
                                                   n->Store->Size,
                                                   &n->Store->Swizzle);
      if (n->Store->Index < 0) {
         slang_info_log_error(emitInfo->log, "Ran out of space for constants");
         return NULL;
      }
      return NULL;

   case IR_COPY:
      return emit_copy(emitInfo, n);

   case IR_COND:
      return emit_cond(emitInfo, n);

   case IR_NOT:
      return emit_not(emitInfo, n);

   case IR_LABEL:
      return emit_label(emitInfo, n);

   case IR_KILL:
      return emit_kill(emitInfo);

   case IR_CALL:
      /* new variable scope for subroutines/function calls */
      _slang_push_var_table(emitInfo->vt);
      inst = emit_fcall(emitInfo, n);
      _slang_pop_var_table(emitInfo->vt);
      return inst;

   case IR_IF:
      return emit_if(emitInfo, n);

   case IR_LOOP:
      return emit_loop(emitInfo, n);
   case IR_BREAK_IF_TRUE:
   case IR_CONT_IF_TRUE:
      return emit_cont_break_if_true(emitInfo, n);
   case IR_BREAK:
      /* fall-through */
   case IR_CONT:
      return emit_cont_break(emitInfo, n);

   case IR_BEGIN_SUB:
      return new_instruction(emitInfo, OPCODE_BGNSUB);
   case IR_END_SUB:
      return new_instruction(emitInfo, OPCODE_ENDSUB);
   case IR_RETURN:
      return emit_return(emitInfo, n);

   case IR_NOP:
      return NULL;

   default:
      _mesa_problem(NULL, "Unexpected IR opcode in emit()\n");
   }
   return NULL;
}


/**
 * After code generation, any subroutines will be in separate program
 * objects.  This function appends all the subroutines onto the main
 * program and resolves the linking of all the branch/call instructions.
 * XXX this logic should really be part of the linking process...
 */
static void
_slang_resolve_subroutines(slang_emit_info *emitInfo)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *mainP = emitInfo->prog;
   GLuint *subroutineLoc, i, total;

   subroutineLoc
      = (GLuint *) _mesa_malloc(emitInfo->NumSubroutines * sizeof(GLuint));

   /* total number of instructions */
   total = mainP->NumInstructions;
   for (i = 0; i < emitInfo->NumSubroutines; i++) {
      subroutineLoc[i] = total;
      total += emitInfo->Subroutines[i]->NumInstructions;
   }

   /* adjust BrancTargets within the functions */
   for (i = 0; i < emitInfo->NumSubroutines; i++) {
      struct gl_program *sub = emitInfo->Subroutines[i];
      GLuint j;
      for (j = 0; j < sub->NumInstructions; j++) {
         struct prog_instruction *inst = sub->Instructions + j;
         if (inst->Opcode != OPCODE_CAL && inst->BranchTarget >= 0) {
            inst->BranchTarget += subroutineLoc[i];
         }
      }
   }

   /* append subroutines' instructions after main's instructions */
   mainP->Instructions = _mesa_realloc_instructions(mainP->Instructions,
                                                    mainP->NumInstructions,
                                                    total);
   mainP->NumInstructions = total;
   for (i = 0; i < emitInfo->NumSubroutines; i++) {
      struct gl_program *sub = emitInfo->Subroutines[i];
      _mesa_copy_instructions(mainP->Instructions + subroutineLoc[i],
                              sub->Instructions,
                              sub->NumInstructions);
      /* delete subroutine code */
      sub->Parameters = NULL; /* prevent double-free */
      _mesa_reference_program(ctx, &emitInfo->Subroutines[i], NULL);
   }

   /* free subroutine list */
   if (emitInfo->Subroutines) {
      _mesa_free(emitInfo->Subroutines);
      emitInfo->Subroutines = NULL;
   }
   emitInfo->NumSubroutines = 0;

   /* Examine CAL instructions.
    * At this point, the BranchTarget field of the CAL instruction is
    * the number/id of the subroutine to call (an index into the
    * emitInfo->Subroutines list).
    * Translate that into an actual instruction location now.
    */
   for (i = 0; i < mainP->NumInstructions; i++) {
      struct prog_instruction *inst = mainP->Instructions + i;
      if (inst->Opcode == OPCODE_CAL) {
         const GLuint f = inst->BranchTarget;
         inst->BranchTarget = subroutineLoc[f];
      }
   }

   _mesa_free(subroutineLoc);
}




GLboolean
_slang_emit_code(slang_ir_node *n, slang_var_table *vt,
                 struct gl_program *prog, GLboolean withEnd,
                 slang_info_log *log)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean success;
   slang_emit_info emitInfo;
   GLuint maxUniforms;

   emitInfo.log = log;
   emitInfo.vt = vt;
   emitInfo.prog = prog;
   emitInfo.Subroutines = NULL;
   emitInfo.NumSubroutines = 0;

   emitInfo.EmitHighLevelInstructions = ctx->Shader.EmitHighLevelInstructions;
   emitInfo.EmitCondCodes = ctx->Shader.EmitCondCodes;
   emitInfo.EmitComments = ctx->Shader.EmitComments;
   emitInfo.EmitBeginEndSub = GL_TRUE;

   if (!emitInfo.EmitCondCodes) {
      emitInfo.EmitHighLevelInstructions = GL_TRUE;
   }      

   /* Check uniform/constant limits */
   if (prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
      maxUniforms = ctx->Const.FragmentProgram.MaxUniformComponents / 4;
   }
   else {
      assert(prog->Target == GL_VERTEX_PROGRAM_ARB);
      maxUniforms = ctx->Const.VertexProgram.MaxUniformComponents / 4;
   }
   if (prog->Parameters->NumParameters > maxUniforms) {
      slang_info_log_error(log, "Constant/uniform register limit exceeded");
      return GL_FALSE;
   }

   (void) emit(&emitInfo, n);

   /* finish up by adding the END opcode to program */
   if (withEnd) {
      struct prog_instruction *inst;
      inst = new_instruction(&emitInfo, OPCODE_END);
   }

   _slang_resolve_subroutines(&emitInfo);

   success = GL_TRUE;

#if 0
   printf("*********** End emit code (%u inst):\n", prog->NumInstructions);
   _mesa_print_program(prog);
   _mesa_print_program_parameters(ctx,prog);
#endif

   return success;
}
