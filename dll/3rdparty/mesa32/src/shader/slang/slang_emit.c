/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008 VMware, Inc.   All Rights Reserved.
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

   GLuint MaxInstructions;  /**< size of prog->Instructions[] buffer */

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
 * Convert a swizzle mask to a writemask.
 * Note that the slang_ir_storage->Swizzle field can represent either a
 * swizzle mask or a writemask, depending on how it's used.  For example,
 * when we parse "direction.yz" alone, we don't know whether .yz is a
 * writemask or a swizzle.  In this case, we encode ".yz" in store->Swizzle
 * as a swizzle mask (.yz?? actually).  Later, if direction.yz is used as
 * an R-value, we use store->Swizzle as-is.  Otherwise, if direction.yz is
 * used as an L-value, we convert it to a writemask.
 */
static GLuint
swizzle_to_writemask(GLuint swizzle)
{
   GLuint i, writemask = 0x0;
   for (i = 0; i < 4; i++) {
      GLuint swz = GET_SWZ(swizzle, i);
      if (swz <= SWIZZLE_W) {
         writemask |= (1 << swz);
      }
   }
   return writemask;
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
 * Return the default swizzle mask for accessing a variable of the
 * given size (in floats).  If size = 1, comp is used to identify
 * which component [0..3] of the register holds the variable.
 */
GLuint
_slang_var_swizzle(GLint size, GLint comp)
{
   switch (size) {
   case 1:
      return MAKE_SWIZZLE4(comp, SWIZZLE_NIL, SWIZZLE_NIL, SWIZZLE_NIL);
   case 2:
      return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_NIL, SWIZZLE_NIL);
   case 3:
      return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_NIL);
   default:
      return SWIZZLE_XYZW;
   }
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
storage_to_dst_reg(struct prog_dst_register *dst, const slang_ir_storage *st)
{
   const GLboolean relAddr = st->RelAddr;
   const GLint size = st->Size;
   GLint index = st->Index;
   GLuint swizzle = st->Swizzle;

   assert(index >= 0);
   /* if this is storage relative to some parent storage, walk up the tree */
   while (st->Parent) {
      st = st->Parent;
      assert(st->Index >= 0);
      index += st->Index;
      swizzle = _slang_swizzle_swizzle(st->Swizzle, swizzle);
   }

   assert(st->File != PROGRAM_UNDEFINED);
   dst->File = st->File;

   assert(index >= 0);
   dst->Index = index;

   assert(size >= 1);
   assert(size <= 4);

   if (swizzle != SWIZZLE_XYZW) {
      dst->WriteMask = swizzle_to_writemask(swizzle);
   }
   else {
      switch (size) {
      case 1:
         dst->WriteMask = WRITEMASK_X << GET_SWZ(st->Swizzle, 0);
         break;
      case 2:
         dst->WriteMask = WRITEMASK_XY;
         break;
      case 3:
         dst->WriteMask = WRITEMASK_XYZ;
         break;
      case 4:
         dst->WriteMask = WRITEMASK_XYZW;
         break;
      default:
         ; /* error would have been caught above */
      }
   }

   dst->RelAddr = relAddr;
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
   assert(index >= 0);
   while (st->Parent) {
      st = st->Parent;
      if (st->Index < 0) {
         /* an error should have been reported already */
         return;
      }
      assert(st->Index >= 0);
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
 * Setup storage pointing to a scalar constant/literal.
 */
static void
constant_to_storage(slang_emit_info *emitInfo,
                    GLfloat val,
                    slang_ir_storage *store)
{
   GLuint swizzle;
   GLint reg;
   GLfloat value[4];

   value[0] = val;
   reg = _mesa_add_unnamed_constant(emitInfo->prog->Parameters,
                                        value, 1, &swizzle);

   memset(store, 0, sizeof(*store));
   store->File = PROGRAM_CONSTANT;
   store->Index = reg;
   store->Swizzle = swizzle;
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
   assert(prog->NumInstructions <= emitInfo->MaxInstructions);

   if (prog->NumInstructions == emitInfo->MaxInstructions) {
      /* grow the instruction buffer */
      emitInfo->MaxInstructions += 20;
      prog->Instructions =
         _mesa_realloc_instructions(prog->Instructions,
                                    prog->NumInstructions,
                                    emitInfo->MaxInstructions);
   }

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


static struct prog_instruction *
emit_arl_load(slang_emit_info *emitInfo,
              enum register_file file, GLint index, GLuint swizzle)
{
   struct prog_instruction *inst = new_instruction(emitInfo, OPCODE_ARL);
   inst->SrcReg[0].File = file;
   inst->SrcReg[0].Index = index;
   inst->SrcReg[0].Swizzle = fix_swizzle(swizzle);
   inst->DstReg.File = PROGRAM_ADDRESS;
   inst->DstReg.Index = 0;
   inst->DstReg.WriteMask = WRITEMASK_X;
   return inst;
}


/**
 * Emit a new instruction with given opcode, operands.
 * At this point the instruction may have multiple indirect register
 * loads/stores.  We convert those into ARL loads and address-relative
 * operands.  See comments inside.
 * At some point in the future we could directly emit indirectly addressed
 * registers in Mesa GPU instructions.
 */
static struct prog_instruction *
emit_instruction(slang_emit_info *emitInfo,
                 gl_inst_opcode opcode,
                 const slang_ir_storage *dst,
                 const slang_ir_storage *src0,
                 const slang_ir_storage *src1,
                 const slang_ir_storage *src2)
{
   struct prog_instruction *inst;
   GLuint numIndirect = 0;
   const slang_ir_storage *src[3];
   slang_ir_storage newSrc[3], newDst;
   GLuint i;
   GLboolean isTemp[3];

   isTemp[0] = isTemp[1] = isTemp[2] = GL_FALSE;

   src[0] = src0;
   src[1] = src1;
   src[2] = src2;

   /* count up how many operands are indirect loads */
   for (i = 0; i < 3; i++) {
      if (src[i] && src[i]->IsIndirect)
         numIndirect++;
   }
   if (dst && dst->IsIndirect)
      numIndirect++;

   /* Take special steps for indirect register loads.
    * If we had multiple address registers this would be simpler.
    * For example, this GLSL code:
    *    x[i] = y[j] + z[k];
    * would translate into something like:
    *    ARL ADDR.x, i;
    *    ARL ADDR.y, j;
    *    ARL ADDR.z, k;
    *    ADD TEMP[ADDR.x+5], TEMP[ADDR.y+9], TEMP[ADDR.z+4];
    * But since we currently only have one address register we have to do this:
    *    ARL ADDR.x, i;
    *    MOV t1, TEMP[ADDR.x+9];
    *    ARL ADDR.x, j;
    *    MOV t2, TEMP[ADDR.x+4];
    *    ARL ADDR.x, k;
    *    ADD TEMP[ADDR.x+5], t1, t2;
    * The code here figures this out...
    */
   if (numIndirect > 0) {
      for (i = 0; i < 3; i++) {
         if (src[i] && src[i]->IsIndirect) {
            /* load the ARL register with the indirect register */
            emit_arl_load(emitInfo,
                          src[i]->IndirectFile,
                          src[i]->IndirectIndex,
                          src[i]->IndirectSwizzle);

            if (numIndirect > 1) {
               /* Need to load src[i] into a temporary register */
               slang_ir_storage srcRelAddr;
               alloc_local_temp(emitInfo, &newSrc[i], src[i]->Size);
               isTemp[i] = GL_TRUE;

               /* set RelAddr flag on src register */
               srcRelAddr = *src[i];
               srcRelAddr.RelAddr = GL_TRUE;
               srcRelAddr.IsIndirect = GL_FALSE; /* not really needed */

               /* MOV newSrc, srcRelAddr; */
               inst = emit_instruction(emitInfo,
                                       OPCODE_MOV,
                                       &newSrc[i],
                                       &srcRelAddr,
                                       NULL,
                                       NULL);

               src[i] = &newSrc[i];
            }
            else {
               /* just rewrite the src[i] storage to be ARL-relative */
               newSrc[i] = *src[i];
               newSrc[i].RelAddr = GL_TRUE;
               newSrc[i].IsIndirect = GL_FALSE; /* not really needed */
               src[i] = &newSrc[i];
            }
         }
      }
   }

   /* Take special steps for indirect dest register write */
   if (dst && dst->IsIndirect) {
      /* load the ARL register with the indirect register */
      emit_arl_load(emitInfo,
                    dst->IndirectFile,
                    dst->IndirectIndex,
                    dst->IndirectSwizzle);
      newDst = *dst;
      newDst.RelAddr = GL_TRUE;
      newDst.IsIndirect = GL_FALSE;
      dst = &newDst;
   }

   /* OK, emit the instruction and its dst, src regs */
   inst = new_instruction(emitInfo, opcode);
   if (!inst)
      return NULL;

   if (dst)
      storage_to_dst_reg(&inst->DstReg, dst);

   for (i = 0; i < 3; i++) {
      if (src[i])
         storage_to_src_reg(&inst->SrcReg[i], src[i]);
   }

   /* Free any temp registers that we allocated above */
   for (i = 0; i < 3; i++) {
      if (isTemp[i])
         _slang_free_temp(emitInfo->vt, &newSrc[i]);
   }

   return inst;
}



/**
 * Put a comment on the given instruction.
 */
static void
inst_comment(struct prog_instruction *inst, const char *comment)
{
   if (inst)
      inst->Comment = _mesa_strdup(comment);
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
   case OPCODE_DP2:
      operator = "DP2";
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
emit_comment(slang_emit_info *emitInfo, const char *comment)
{
   struct prog_instruction *inst = new_instruction(emitInfo, OPCODE_NOP);
   inst_comment(inst, comment);
   return inst;
}


/**
 * Generate code for a simple arithmetic instruction.
 * Either 1, 2 or 3 operands.
 */
static struct prog_instruction *
emit_arith(slang_emit_info *emitInfo, slang_ir_node *n)
{
   const slang_ir_info *info = _slang_ir_info(n->Opcode);
   struct prog_instruction *inst;
   GLuint i;

   assert(info);
   assert(info->InstOpcode != OPCODE_NOP);

#if PEEPHOLE_OPTIMIZATIONS
   /* Look for MAD opportunity */
   if (info->NumParams == 2 &&
       n->Opcode == IR_ADD && n->Children[0]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(IR_MUL(A, B), C) */
      emit(emitInfo, n->Children[0]->Children[0]);  /* A */
      emit(emitInfo, n->Children[0]->Children[1]);  /* B */
      emit(emitInfo, n->Children[1]);  /* C */
      alloc_node_storage(emitInfo, n, -1);  /* dest */

      inst = emit_instruction(emitInfo,
                              OPCODE_MAD,
                              n->Store,
                              n->Children[0]->Children[0]->Store,
                              n->Children[0]->Children[1]->Store,
                              n->Children[1]->Store);

      free_node_storage(emitInfo->vt, n->Children[0]->Children[0]);
      free_node_storage(emitInfo->vt, n->Children[0]->Children[1]);
      free_node_storage(emitInfo->vt, n->Children[1]);
      return inst;
   }

   if (info->NumParams == 2 &&
       n->Opcode == IR_ADD && n->Children[1]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(A, IR_MUL(B, C)) */
      emit(emitInfo, n->Children[0]);  /* A */
      emit(emitInfo, n->Children[1]->Children[0]);  /* B */
      emit(emitInfo, n->Children[1]->Children[1]);  /* C */
      alloc_node_storage(emitInfo, n, -1);  /* dest */

      inst = emit_instruction(emitInfo,
                              OPCODE_MAD,
                              n->Store,
                              n->Children[1]->Children[0]->Store,
                              n->Children[1]->Children[1]->Store,
                              n->Children[0]->Store);

      free_node_storage(emitInfo->vt, n->Children[1]->Children[0]);
      free_node_storage(emitInfo->vt, n->Children[1]->Children[1]);
      free_node_storage(emitInfo->vt, n->Children[0]);
      return inst;
   }
#endif

   /* gen code for children, may involve temp allocation */
   for (i = 0; i < info->NumParams; i++) {
      emit(emitInfo, n->Children[i]);
      if (!n->Children[i] || !n->Children[i]->Store) {
         /* error recovery */
         return NULL;
      }
   }

   /* result storage */
   alloc_node_storage(emitInfo, n, -1);

   inst = emit_instruction(emitInfo,
                           info->InstOpcode,
                           n->Store,  /* dest */
                           (info->NumParams > 0 ? n->Children[0]->Store : NULL),
                           (info->NumParams > 1 ? n->Children[1]->Store : NULL),
                           (info->NumParams > 2 ? n->Children[2]->Store : NULL)
                           );

   /* free temps */
   for (i = 0; i < info->NumParams; i++)
      free_node_storage(emitInfo->vt, n->Children[i]);

   return inst;
}


/**
 * Emit code for == and != operators.  These could normally be handled
 * by emit_arith() except we need to be able to handle structure comparisons.
 */
static struct prog_instruction *
emit_compare(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst = NULL;
   GLint size;

   assert(n->Opcode == IR_EQUAL || n->Opcode == IR_NOTEQUAL);

   /* gen code for children */
   emit(emitInfo, n->Children[0]);
   emit(emitInfo, n->Children[1]);

   if (n->Children[0]->Store->Size != n->Children[1]->Store->Size) {
      slang_info_log_error(emitInfo->log, "invalid operands to == or !=");
      n->Store = NULL;
      return NULL;
   }

   /* final result is 1 bool */
   if (!alloc_node_storage(emitInfo, n, 1))
      return NULL;

   size = n->Children[0]->Store->Size;

   if (size == 1) {
      gl_inst_opcode opcode = n->Opcode == IR_EQUAL ? OPCODE_SEQ : OPCODE_SNE;
      inst =  emit_instruction(emitInfo,
                               opcode,
                               n->Store, /* dest */
                               n->Children[0]->Store,
                               n->Children[1]->Store,
                               NULL);
   }
   else if (size <= 4) {
      /* compare two vectors.
       * Unfortunately, there's no instruction to compare vectors and
       * return a scalar result.  Do it with some compare and dot product
       * instructions...
       */
      GLuint swizzle;
      gl_inst_opcode dotOp;
      slang_ir_storage tempStore;

      if (!alloc_local_temp(emitInfo, &tempStore, 4)) {
         n->Store = NULL;
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
         dotOp = OPCODE_DP3; /* XXX use OPCODE_DP2 eventually */
         swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y);
      }

      /* Compute inequality (temp = (A != B)) */
      inst = emit_instruction(emitInfo,
                              OPCODE_SNE,
                              &tempStore,
                              n->Children[0]->Store,
                              n->Children[1]->Store,
                              NULL);
      inst_comment(inst, "Compare values");

      /* Compute val = DOT(temp, temp)  (reduction) */
      inst = emit_instruction(emitInfo,
                              dotOp,
                              n->Store,
                              &tempStore,
                              &tempStore,
                              NULL);
      inst->SrcReg[0].Swizzle = inst->SrcReg[1].Swizzle = swizzle; /*override*/
      inst_comment(inst, "Reduce vec to bool");

      _slang_free_temp(emitInfo->vt, &tempStore); /* free temp */

      if (n->Opcode == IR_EQUAL) {
         /* compute val = !val.x  with SEQ val, val, 0; */
         slang_ir_storage zero;
         constant_to_storage(emitInfo, 0.0, &zero);
         inst = emit_instruction(emitInfo,
                                 OPCODE_SEQ,
                                 n->Store, /* dest */
                                 n->Store,
                                 &zero,
                                 NULL);
         inst_comment(inst, "Invert true/false");
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
         slang_ir_storage srcStore0 = *n->Children[0]->Store;
         slang_ir_storage srcStore1 = *n->Children[1]->Store;
         srcStore0.Index += i;
         srcStore1.Index += i;

         if (i == 0) {
            /* SNE accTemp, left[i], right[i] */
            inst = emit_instruction(emitInfo, OPCODE_SNE,
                                    &accTemp, /* dest */
                                    &srcStore0,
                                    &srcStore1,
                                    NULL);
            inst_comment(inst, "Begin struct/array comparison");
         }
         else {
            /* SNE sneTemp, left[i], right[i] */
            inst = emit_instruction(emitInfo, OPCODE_SNE,
                                    &sneTemp, /* dest */
                                    &srcStore0,
                                    &srcStore1,
                                    NULL);
            /* ADD accTemp, accTemp, sneTemp; # like logical-OR */
            inst = emit_instruction(emitInfo, OPCODE_ADD,
                                    &accTemp, /* dest */
                                    &accTemp,
                                    &sneTemp,
                                    NULL);
         }
      }

      /* compute accTemp.x || accTemp.y || accTemp.z || accTemp.w with DOT4 */
      inst = emit_instruction(emitInfo, OPCODE_DP4,
                              n->Store,
                              &accTemp,
                              &accTemp,
                              NULL);
      inst_comment(inst, "End struct/array comparison");

      if (n->Opcode == IR_EQUAL) {
         /* compute tmp.x = !tmp.x  via tmp.x = (tmp.x == 0) */
         slang_ir_storage zero;
         constant_to_storage(emitInfo, 0.0, &zero);
         inst = emit_instruction(emitInfo, OPCODE_SEQ,
                                 n->Store, /* dest */
                                 n->Store,
                                 &zero,
                                 NULL);
         inst_comment(inst, "Invert true/false");
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
   inst = emit_instruction(emitInfo, OPCODE_MAX,
                           tmpNode.Store, /* dest */
                           n->Children[0]->Store,
                           n->Children[1]->Store,
                           NULL);

   /* n->dest = min(tmp, ch[2]) */
   inst = emit_instruction(emitInfo, OPCODE_MIN,
                           n->Store, /* dest */
                           tmpNode.Store,
                           n->Children[2]->Store,
                           NULL);

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

   inst = emit_instruction(emitInfo,
                           OPCODE_MOV,
                           n->Store, /* dest */
                           n->Children[0]->Store,
                           NULL,
                           NULL);
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
   GLuint maxInstSave;

   assert(n->Opcode == IR_CALL);
   assert(n->Label);

   /* save/push cur program */
   maxInstSave = emitInfo->MaxInstructions;
   progSave = emitInfo->prog;

   emitInfo->prog = new_subroutine(emitInfo, &subroutineId);
   emitInfo->MaxInstructions = emitInfo->prog->NumInstructions;

   _slang_label_set_location(n->Label, emitInfo->prog->NumInstructions,
                             emitInfo->prog);

   if (emitInfo->EmitBeginEndSub) {
      /* BGNSUB isn't a real instruction.
       * We require a label (i.e. "foobar:") though, if we're going to
       * print the program in the NV format.  The BNGSUB instruction is
       * really just a NOP to attach the label to.
       */
      inst = new_instruction(emitInfo, OPCODE_BGNSUB);
      inst_comment(inst, n->Label->Name);
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
      inst_comment(inst, n->Label->Name);
   }

   /* pop/restore cur program */
   emitInfo->prog = progSave;
   emitInfo->MaxInstructions = maxInstSave;

   /* emit the function call */
   inst = new_instruction(emitInfo, OPCODE_CAL);
   /* The branch target is just the subroutine number (changed later) */
   inst->BranchTarget = subroutineId;
   inst_comment(inst, n->Label->Name);
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
   gl_inst_opcode opcode;

   if (n->Opcode == IR_TEX) {
      opcode = OPCODE_TEX;
   }
   else if (n->Opcode == IR_TEXB) {
      opcode = OPCODE_TXB;
   }
   else {
      assert(n->Opcode == IR_TEXP);
      opcode = OPCODE_TXP;
   }

   if (n->Children[0]->Opcode == IR_ELEMENT) {
      /* array is the sampler (a uniform which'll indicate the texture unit) */
      assert(n->Children[0]->Children[0]->Store);
      assert(n->Children[0]->Children[0]->Store->File == PROGRAM_SAMPLER);

      emit(emitInfo, n->Children[0]);

      n->Children[0]->Var = n->Children[0]->Children[0]->Var;
   } else {
      /* this is the sampler (a uniform which'll indicate the texture unit) */
      assert(n->Children[0]->Store);
      assert(n->Children[0]->Store->File == PROGRAM_SAMPLER);
   }

   /* emit code for the texcoord operand */
   (void) emit(emitInfo, n->Children[1]);

   /* alloc storage for result of texture fetch */
   if (!alloc_node_storage(emitInfo, n, 4))
      return NULL;

   /* emit TEX instruction;  Child[1] is the texcoord */
   inst = emit_instruction(emitInfo,
                           opcode,
                           n->Store,
                           n->Children[1]->Store,
                           NULL,
                           NULL);

   /* Store->Index is the uniform/sampler index */
   assert(n->Children[0]->Store->Index >= 0);
   inst->TexSrcUnit = n->Children[0]->Store->Index;
   inst->TexSrcTarget = n->Children[0]->Store->TexTarget;

   /* mark the sampler as being used */
   _mesa_use_uniform(emitInfo->prog->Parameters,
                     (char *) n->Children[0]->Var->a_name);

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

   if (n->Store->File == PROGRAM_SAMPLER) {
      /* no code generated for sampler assignments,
       * just copy the sampler index/target at compile time.
       */
      n->Store->Index = n->Children[1]->Store->Index;
      n->Store->TexTarget = n->Children[1]->Store->TexTarget;
      return NULL;
   }

#if PEEPHOLE_OPTIMIZATIONS
   if (inst &&
       (n->Children[1]->Opcode != IR_SWIZZLE) &&
       _slang_is_temp(emitInfo->vt, n->Children[1]->Store) &&
       (inst->DstReg.File == n->Children[1]->Store->File) &&
       (inst->DstReg.Index == n->Children[1]->Store->Index) &&
       !n->Children[0]->Store->IsIndirect &&
       n->Children[0]->Store->Size <= 4) {
      /* Peephole optimization:
       * The Right-Hand-Side has its results in a temporary place.
       * Modify the RHS (and the prev instruction) to store its results
       * in the destination specified by n->Children[0].
       * Then, this MOVE is a no-op.
       * Ex:
       *   MUL tmp, x, y;
       *   MOV a, tmp;
       * becomes:
       *   MUL a, x, y;
       */

      /* fixup the previous instruction (which stored the RHS result) */
      assert(n->Children[0]->Store->Index >= 0);
      storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store);
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
         ASSERT(n->Children[1]->Store->Swizzle == SWIZZLE_NOOP);
         dstStore.Size = 4;
         srcStore.Size = 4;
         while (size >= 4) {
            inst = emit_instruction(emitInfo, OPCODE_MOV,
                                    &dstStore,
                                    &srcStore,
                                    NULL,
                                    NULL);
            inst_comment(inst, "IR_COPY block");
            srcStore.Index++;
            dstStore.Index++;
            size -= 4;
         }
      }
      else {
         /* single register move */
         char *srcAnnot, *dstAnnot;
         assert(n->Children[0]->Store->Index >= 0);
         inst = emit_instruction(emitInfo, OPCODE_MOV,
                                 n->Children[0]->Store, /* dest */
                                 n->Children[1]->Store,
                                 NULL,
                                 NULL);
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
         inst = emit_instruction(emitInfo, OPCODE_MOV,
                                 n->Store, /* dest */
                                 n->Children[0]->Store,
                                 NULL,
                                 NULL);
         inst->CondUpdate = GL_TRUE;
         inst_comment(inst, "COND expr");
         _slang_free_temp(emitInfo->vt, n->Store);
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
   slang_ir_storage zero;
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

   constant_to_storage(emitInfo, 0.0, &zero);
   inst = emit_instruction(emitInfo,
                           OPCODE_SEQ,
                           n->Store,
                           n->Children[0]->Store,
                           &zero,
                           NULL);
   inst_comment(inst, "NOT");

   free_node_storage(emitInfo->vt, n->Children[0]);

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
      if (emitInfo->EmitCondCodes) {
         /* IF condcode THEN ... */
         struct prog_instruction *ifInst;
         ifInst = new_instruction(emitInfo, OPCODE_IF);
         ifInst->DstReg.CondMask = COND_NE;  /* if cond is non-zero */
         /* only test the cond code (1 of 4) that was updated by the
          * previous instruction.
          */
         ifInst->DstReg.CondSwizzle = writemask_to_swizzle(condWritemask);
      }
      else {
         /* IF src[0] THEN ... */
         emit_instruction(emitInfo, OPCODE_IF,
                          NULL, /* dst */
                          n->Children[0]->Store, /* op0 */
                          NULL,
                          NULL);
      }
   }
   else {
      /* conditional jump to else, or endif */
      struct prog_instruction *ifInst = new_instruction(emitInfo, OPCODE_BRA);
      ifInst->DstReg.CondMask = COND_EQ;  /* BRA if cond is zero */
      inst_comment(ifInst, "if zero");
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
         inst_comment(inst, "else");
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
         inst = emit_instruction(emitInfo, OPCODE_IF,
                                 NULL, /* dest */
                                 n->Children[0]->Store,
                                 NULL,
                                 NULL);
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


/**
 * Return the size of a swizzle mask given that some swizzle components
 * may be NIL/undefined.  For example:
 *  swizzle_size(".zzxx") = 4
 *  swizzle_size(".xy??") = 2
 *  swizzle_size(".w???") = 1
 */
static GLuint
swizzle_size(GLuint swizzle)
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      if (GET_SWZ(swizzle, i) == SWIZZLE_NIL)
         return i;
   }
   return 4;
}


static struct prog_instruction *
emit_swizzle(slang_emit_info *emitInfo, slang_ir_node *n)
{
   struct prog_instruction *inst;

   inst = emit(emitInfo, n->Children[0]);

   if (!n->Store->Parent) {
      /* this covers a case such as "(b ? p : q).x" */
      n->Store->Parent = n->Children[0]->Store;
      assert(n->Store->Parent);
   }

   {
      const GLuint swizzle = n->Store->Swizzle;
      /* new storage is parent storage with updated Swizzle + Size fields */
      _slang_copy_ir_storage(n->Store, n->Store->Parent);
      /* Apply this node's swizzle to parent's storage */
      n->Store->Swizzle = _slang_swizzle_swizzle(n->Store->Swizzle, swizzle);
      /* Update size */
      n->Store->Size = swizzle_size(n->Store->Swizzle);
   }

   assert(!n->Store->Parent);
   assert(n->Store->Index >= 0);

   return inst;
}


/**
 * Dereference array element:  element == array[index]
 * This basically involves emitting code for computing the array index
 * and updating the node/element's storage info.
 */
static struct prog_instruction *
emit_array_element(slang_emit_info *emitInfo, slang_ir_node *n)
{
   slang_ir_storage *arrayStore, *indexStore;
   const int elemSize = n->Store->Size;           /* number of floats */
   const GLint elemSizeVec = (elemSize + 3) / 4;  /* number of vec4 */
   struct prog_instruction *inst;

   assert(n->Opcode == IR_ELEMENT);
   assert(elemSize > 0);

   /* special case for built-in state variables, like light state */
   {
      slang_ir_storage *root = n->Store;
      assert(!root->Parent);
      while (root->Parent)
         root = root->Parent;

      if (root->File == PROGRAM_STATE_VAR) {
         GLboolean direct;
         GLint index =
            _slang_alloc_statevar(n, emitInfo->prog->Parameters, &direct);
         if (index < 0) {
            /* error */
            return NULL;
         }
         if (direct) {
            n->Store->Index = index;
            return NULL; /* all done */
         }
      }
   }

   /* do codegen for array itself */
   emit(emitInfo, n->Children[0]);
   arrayStore = n->Children[0]->Store;

   /* The initial array element storage is the array's storage,
    * then modified below.
    */
   _slang_copy_ir_storage(n->Store, arrayStore);


   if (n->Children[1]->Opcode == IR_FLOAT) {
      /* Constant array index */
      const GLint element = (GLint) n->Children[1]->Value[0];

      /* this element's storage is the array's storage, plus constant offset */
      n->Store->Index += elemSizeVec * element;
   }
   else {
      /* Variable array index */

      /* do codegen for array index expression */
      emit(emitInfo, n->Children[1]);
      indexStore = n->Children[1]->Store;

      if (indexStore->IsIndirect) {
         /* need to put the array index into a temporary since we can't
          * directly support a[b[i]] constructs.
          */


         /*indexStore = tempstore();*/
      }


      if (elemSize > 4) {
         /* need to multiply array index by array element size */
         struct prog_instruction *inst;
         slang_ir_storage *indexTemp;
         slang_ir_storage elemSizeStore;

         /* allocate 1 float indexTemp */
         indexTemp = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, 1);
         _slang_alloc_temp(emitInfo->vt, indexTemp);

         /* allocate a constant containing the element size */
         constant_to_storage(emitInfo, (float) elemSizeVec, &elemSizeStore);

         /* multiply array index by element size */
         inst = emit_instruction(emitInfo,
                                 OPCODE_MUL,
                                 indexTemp, /* dest */
                                 indexStore, /* the index */
                                 &elemSizeStore,
                                 NULL);

         indexStore = indexTemp;
      }

      if (arrayStore->IsIndirect) {
         /* ex: in a[i][j], a[i] (the arrayStore) is indirect */
         /* Need to add indexStore to arrayStore->Indirect store */
         slang_ir_storage indirectArray;
         slang_ir_storage *indexTemp;

         _slang_init_ir_storage(&indirectArray,
                                arrayStore->IndirectFile,
                                arrayStore->IndirectIndex,
                                1,
                                arrayStore->IndirectSwizzle);

         /* allocate 1 float indexTemp */
         indexTemp = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, 1);
         _slang_alloc_temp(emitInfo->vt, indexTemp);

         inst = emit_instruction(emitInfo,
                                 OPCODE_ADD,
                                 indexTemp,      /* dest */
                                 indexStore,     /* the index */
                                 &indirectArray, /* indirect array base */
                                 NULL);

         indexStore = indexTemp;
      }

      /* update the array element storage info */
      n->Store->IsIndirect = GL_TRUE;
      n->Store->IndirectFile = indexStore->File;
      n->Store->IndirectIndex = indexStore->Index;
      n->Store->IndirectSwizzle = indexStore->Swizzle;
   }

   n->Store->Size = elemSize;
   n->Store->Swizzle = _slang_var_swizzle(elemSize, 0);

   return NULL; /* no instruction */
}


/**
 * Resolve storage for accessing a structure field.
 */
static struct prog_instruction *
emit_struct_field(slang_emit_info *emitInfo, slang_ir_node *n)
{
   slang_ir_storage *root = n->Store;
   GLint fieldOffset, fieldSize;

   assert(n->Opcode == IR_FIELD);

   assert(!root->Parent);
   while (root->Parent)
      root = root->Parent;

   /* If this is the field of a state var, allocate constant/uniform
    * storage for it now if we haven't already.
    * Note that we allocate storage (uniform/constant slots) for state
    * variables here rather than at declaration time so we only allocate
    * space for the ones that we actually use!
    */
   if (root->File == PROGRAM_STATE_VAR) {
      GLboolean direct;
      GLint index = _slang_alloc_statevar(n, emitInfo->prog->Parameters, &direct);
      if (index < 0) {
         slang_info_log_error(emitInfo->log, "Error parsing state variable");
         return NULL;
      }
      if (direct) {
         root->Index = index;
         return NULL; /* all done */
      }
   }

   /* do codegen for struct */
   emit(emitInfo, n->Children[0]);
   assert(n->Children[0]->Store->Index >= 0);


   fieldOffset = n->Store->Index;
   fieldSize = n->Store->Size;

   _slang_copy_ir_storage(n->Store, n->Children[0]->Store);

   n->Store->Index = n->Children[0]->Store->Index + fieldOffset / 4;
   n->Store->Size = fieldSize;

   switch (fieldSize) {
   case 1:
      {
         GLint swz = fieldOffset % 4;
         n->Store->Swizzle = MAKE_SWIZZLE4(swz, swz, swz, swz);
      }
      break;
   case 2:
      n->Store->Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y,
                                        SWIZZLE_NIL, SWIZZLE_NIL);
      break;
   case 3:
      n->Store->Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y,
                                        SWIZZLE_Z, SWIZZLE_NIL);
      break;
   default:
      n->Store->Swizzle = SWIZZLE_XYZW;
   }

   assert(n->Store->Index >= 0);

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
      assert(n->Var->store == n->Store);
   }
   if (emitInfo->EmitComments) {
      /* emit NOP with comment describing the variable's storage location */
      char s[1000];
      sprintf(s, "TEMP[%d]%s = variable %s (size %d)",
              n->Store->Index,
              _mesa_swizzle_string(n->Store->Swizzle, 0, GL_FALSE), 
              (n->Var ? (char *) n->Var->a_name : "anonymous"),
              n->Store->Size);
      emit_comment(emitInfo, s);
   }
   return NULL;
}


/**
 * Emit code for a reference to a variable.
 * Actually, no code is generated but we may do some memory allocation.
 * In particular, state vars (uniforms) are allocated on an as-needed basis.
 */
static struct prog_instruction *
emit_var_ref(slang_emit_info *emitInfo, slang_ir_node *n)
{
   assert(n->Store);
   assert(n->Store->File != PROGRAM_UNDEFINED);

   if (n->Store->File == PROGRAM_STATE_VAR && n->Store->Index < 0) {
      GLboolean direct;
      GLint index = _slang_alloc_statevar(n, emitInfo->prog->Parameters, &direct);
      if (index < 0) {
         /* error */
         char s[100];
         _mesa_snprintf(s, sizeof(s), "Undefined variable '%s'",
                        (char *) n->Var->a_name);
         slang_info_log_error(emitInfo->log, s);
         return NULL;
      }

      n->Store->Index = index;
   }
   else if (n->Store->File == PROGRAM_UNIFORM ||
            n->Store->File == PROGRAM_SAMPLER) {
      /* mark var as used */
      _mesa_use_uniform(emitInfo->prog->Parameters, (char *) n->Var->a_name);
   }
   else if (n->Store->File == PROGRAM_INPUT) {
      assert(n->Store->Index >= 0);
      emitInfo->prog->InputsRead |= (1 << n->Store->Index);
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
   case IR_NRM4:
   case IR_NRM3:
   /* binary */
   case IR_ADD:
   case IR_SUB:
   case IR_MUL:
   case IR_DOT4:
   case IR_DOT3:
   case IR_DOT2:
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

   /* adjust BranchTargets within the functions */
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



/**
 * Convert the IR tree into GPU instructions.
 * \param n  root of IR tree
 * \param vt  variable table
 * \param prog  program to put GPU instructions into
 * \param pragmas  controls codegen options
 * \param withEnd  if true, emit END opcode at end
 * \param log  log for emitting errors/warnings/info
 */
GLboolean
_slang_emit_code(slang_ir_node *n, slang_var_table *vt,
                 struct gl_program *prog,
                 const struct gl_sl_pragmas *pragmas,
                 GLboolean withEnd,
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
   emitInfo.MaxInstructions = prog->NumInstructions;

   emitInfo.EmitHighLevelInstructions = ctx->Shader.EmitHighLevelInstructions;
   emitInfo.EmitCondCodes = ctx->Shader.EmitCondCodes;
   emitInfo.EmitComments = ctx->Shader.EmitComments || pragmas->Debug;
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
      slang_info_log_error(log, "Constant/uniform register limit exceeded "
                           "(max=%u vec4)", maxUniforms);

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
