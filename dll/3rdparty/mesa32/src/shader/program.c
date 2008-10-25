/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * \file program.c
 * Vertex and fragment program support functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "program.h"
#include "prog_parameter.h"
#include "prog_instruction.h"


/**
 * A pointer to this dummy program is put into the hash table when
 * glGenPrograms is called.
 */
struct gl_program _mesa_DummyProgram;


/**
 * Init context's vertex/fragment program state
 */
void
_mesa_init_program(GLcontext *ctx)
{
   GLuint i;

   ctx->Program.ErrorPos = -1;
   ctx->Program.ErrorString = _mesa_strdup("");

#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   ctx->VertexProgram.Enabled = GL_FALSE;
   ctx->VertexProgram.PointSizeEnabled = GL_FALSE;
   ctx->VertexProgram.TwoSideEnabled = GL_FALSE;
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
                            ctx->Shared->DefaultVertexProgram);
   assert(ctx->VertexProgram.Current);
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      ctx->VertexProgram.TrackMatrix[i] = GL_NONE;
      ctx->VertexProgram.TrackMatrixTransform[i] = GL_IDENTITY_NV;
   }
#endif

#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   ctx->FragmentProgram.Enabled = GL_FALSE;
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                            ctx->Shared->DefaultFragmentProgram);
   assert(ctx->FragmentProgram.Current);
#endif

   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   ctx->ATIFragmentShader.Enabled = GL_FALSE;
   ctx->ATIFragmentShader.Current = ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
   ctx->ATIFragmentShader.Current->RefCount++;
#endif
}


/**
 * Free a context's vertex/fragment program state
 */
void
_mesa_free_program_data(GLcontext *ctx)
{
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current, NULL);
#endif
#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current, NULL);
#endif
   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->RefCount--;
      if (ctx->ATIFragmentShader.Current->RefCount <= 0) {
         _mesa_free(ctx->ATIFragmentShader.Current);
      }
   }
#endif
   _mesa_free((void *) ctx->Program.ErrorString);
}


/**
 * Update the default program objects in the given context to reference those
 * specified in the shared state and release those referencing the old
 * shared state.
 */
void
_mesa_update_default_objects_program(GLcontext *ctx)
{
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
                            (struct gl_vertex_program *)
                            ctx->Shared->DefaultVertexProgram);
   assert(ctx->VertexProgram.Current);
#endif

#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                            (struct gl_fragment_program *)
                            ctx->Shared->DefaultFragmentProgram);
   assert(ctx->FragmentProgram.Current);
#endif

   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->RefCount--;
      if (ctx->ATIFragmentShader.Current->RefCount <= 0) {
         _mesa_free(ctx->ATIFragmentShader.Current);
      }
   }
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
   ctx->ATIFragmentShader.Current->RefCount++;
#endif
}


/**
 * Set the vertex/fragment program error state (position and error string).
 * This is generally called from within the parsers.
 */
void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string)
{
   ctx->Program.ErrorPos = pos;
   _mesa_free((void *) ctx->Program.ErrorString);
   if (!string)
      string = "";
   ctx->Program.ErrorString = _mesa_strdup(string);
}


/**
 * Find the line number and column for 'pos' within 'string'.
 * Return a copy of the line which contains 'pos'.  Free the line with
 * _mesa_free().
 * \param string  the program string
 * \param pos     the position within the string
 * \param line    returns the line number corresponding to 'pos'.
 * \param col     returns the column number corresponding to 'pos'.
 * \return copy of the line containing 'pos'.
 */
const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col)
{
   const GLubyte *lineStart = string;
   const GLubyte *p = string;
   GLubyte *s;
   int len;

   *line = 1;

   while (p != pos) {
      if (*p == (GLubyte) '\n') {
         (*line)++;
         lineStart = p + 1;
      }
      p++;
   }

   *col = (pos - lineStart) + 1;

   /* return copy of this line */
   while (*p != 0 && *p != '\n')
      p++;
   len = p - lineStart;
   s = (GLubyte *) _mesa_malloc(len + 1);
   _mesa_memcpy(s, lineStart, len);
   s[len] = 0;

   return s;
}


/**
 * Initialize a new vertex/fragment program object.
 */
static struct gl_program *
_mesa_init_program_struct( GLcontext *ctx, struct gl_program *prog,
                           GLenum target, GLuint id)
{
   (void) ctx;
   if (prog) {
      GLuint i;
      _mesa_bzero(prog, sizeof(*prog));
      prog->Id = id;
      prog->Target = target;
      prog->Resident = GL_TRUE;
      prog->RefCount = 1;
      prog->Format = GL_PROGRAM_FORMAT_ASCII_ARB;

      /* default mapping from samplers to texture units */
      for (i = 0; i < MAX_SAMPLERS; i++)
         prog->SamplerUnits[i] = i;
   }

   return prog;
}


/**
 * Initialize a new fragment program object.
 */
struct gl_program *
_mesa_init_fragment_program( GLcontext *ctx, struct gl_fragment_program *prog,
                             GLenum target, GLuint id)
{
   if (prog)
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}


/**
 * Initialize a new vertex program object.
 */
struct gl_program *
_mesa_init_vertex_program( GLcontext *ctx, struct gl_vertex_program *prog,
                           GLenum target, GLuint id)
{
   if (prog)
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}


/**
 * Allocate and initialize a new fragment/vertex program object but
 * don't put it into the program hash table.  Called via
 * ctx->Driver.NewProgram.  May be overridden (ie. replaced) by a
 * device driver function to implement OO deriviation with additional
 * types not understood by this function.
 *
 * \param ctx  context
 * \param id   program id/number
 * \param target  program target/type
 * \return  pointer to new program object
 */
struct gl_program *
_mesa_new_program(GLcontext *ctx, GLenum target, GLuint id)
{
   struct gl_program *prog;
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
      prog = _mesa_init_vertex_program(ctx, CALLOC_STRUCT(gl_vertex_program),
                                       target, id );
      break;
   case GL_FRAGMENT_PROGRAM_NV:
   case GL_FRAGMENT_PROGRAM_ARB:
      prog =_mesa_init_fragment_program(ctx,
                                         CALLOC_STRUCT(gl_fragment_program),
                                         target, id );
      break;
   default:
      _mesa_problem(ctx, "bad target in _mesa_new_program");
      prog = NULL;
   }
   return prog;
}


/**
 * Delete a program and remove it from the hash table, ignoring the
 * reference count.
 * Called via ctx->Driver.DeleteProgram.  May be wrapped (OO deriviation)
 * by a device driver function.
 */
void
_mesa_delete_program(GLcontext *ctx, struct gl_program *prog)
{
   (void) ctx;
   ASSERT(prog);
   ASSERT(prog->RefCount==0);

   if (prog == &_mesa_DummyProgram)
      return;

   if (prog->String)
      _mesa_free(prog->String);

   _mesa_free_instructions(prog->Instructions, prog->NumInstructions);

   if (prog->Parameters) {
      _mesa_free_parameter_list(prog->Parameters);
   }
   if (prog->Varying) {
      _mesa_free_parameter_list(prog->Varying);
   }
   if (prog->Attributes) {
      _mesa_free_parameter_list(prog->Attributes);
   }

   /* XXX this is a little ugly */
   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      if (vprog->TnlData)
         _mesa_free(vprog->TnlData);
   }

   _mesa_free(prog);
}


/**
 * Return the gl_program object for a given ID.
 * Basically just a wrapper for _mesa_HashLookup() to avoid a lot of
 * casts elsewhere.
 */
struct gl_program *
_mesa_lookup_program(GLcontext *ctx, GLuint id)
{
   if (id)
      return (struct gl_program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   else
      return NULL;
}


/**
 * Reference counting for vertex/fragment programs
 */
void
_mesa_reference_program(GLcontext *ctx,
                        struct gl_program **ptr,
                        struct gl_program *prog)
{
   assert(ptr);
   if (*ptr && prog) {
      /* sanity check */
      ASSERT((*ptr)->Target == prog->Target);
   }
   if (*ptr == prog) {
      return;  /* no change */
   }
   if (*ptr) {
      GLboolean deleteFlag;

      /*_glthread_LOCK_MUTEX((*ptr)->Mutex);*/
#if 0
      printf("Program %p ID=%u Target=%s  Refcount-- to %d\n",
             *ptr, (*ptr)->Id,
             ((*ptr)->Target == GL_VERTEX_PROGRAM_ARB ? "VP" : "FP"),
             (*ptr)->RefCount - 1);
#endif
      ASSERT((*ptr)->RefCount > 0);
      (*ptr)->RefCount--;

      deleteFlag = ((*ptr)->RefCount == 0);
      /*_glthread_UNLOCK_MUTEX((*ptr)->Mutex);*/

      if (deleteFlag) {
         ASSERT(ctx);
         ctx->Driver.DeleteProgram(ctx, *ptr);
      }

      *ptr = NULL;
   }

   assert(!*ptr);
   if (prog) {
      /*_glthread_LOCK_MUTEX(prog->Mutex);*/
      prog->RefCount++;
#if 0
      printf("Program %p ID=%u Target=%s  Refcount++ to %d\n",
             prog, prog->Id,
             (prog->Target == GL_VERTEX_PROGRAM_ARB ? "VP" : "FP"),
             prog->RefCount);
#endif
      /*_glthread_UNLOCK_MUTEX(prog->Mutex);*/
   }

   *ptr = prog;
}


/**
 * Return a copy of a program.
 * XXX Problem here if the program object is actually OO-derivation
 * made by a device driver.
 */
struct gl_program *
_mesa_clone_program(GLcontext *ctx, const struct gl_program *prog)
{
   struct gl_program *clone;

   clone = ctx->Driver.NewProgram(ctx, prog->Target, prog->Id);
   if (!clone)
      return NULL;

   assert(clone->Target == prog->Target);
   assert(clone->RefCount == 1);

   clone->String = (GLubyte *) _mesa_strdup((char *) prog->String);
   clone->Format = prog->Format;
   clone->Instructions = _mesa_alloc_instructions(prog->NumInstructions);
   if (!clone->Instructions) {
      _mesa_reference_program(ctx, &clone, NULL);
      return NULL;
   }
   _mesa_copy_instructions(clone->Instructions, prog->Instructions,
                           prog->NumInstructions);
   clone->InputsRead = prog->InputsRead;
   clone->OutputsWritten = prog->OutputsWritten;
   clone->SamplersUsed = prog->SamplersUsed;
   clone->ShadowSamplers = prog->ShadowSamplers;
   memcpy(clone->TexturesUsed, prog->TexturesUsed, sizeof(prog->TexturesUsed));

   if (prog->Parameters)
      clone->Parameters = _mesa_clone_parameter_list(prog->Parameters);
   memcpy(clone->LocalParams, prog->LocalParams, sizeof(clone->LocalParams));
   if (prog->Varying)
      clone->Varying = _mesa_clone_parameter_list(prog->Varying);
   if (prog->Attributes)
      clone->Attributes = _mesa_clone_parameter_list(prog->Attributes);
   memcpy(clone->LocalParams, prog->LocalParams, sizeof(clone->LocalParams));
   clone->NumInstructions = prog->NumInstructions;
   clone->NumTemporaries = prog->NumTemporaries;
   clone->NumParameters = prog->NumParameters;
   clone->NumAttributes = prog->NumAttributes;
   clone->NumAddressRegs = prog->NumAddressRegs;
   clone->NumNativeInstructions = prog->NumNativeInstructions;
   clone->NumNativeTemporaries = prog->NumNativeTemporaries;
   clone->NumNativeParameters = prog->NumNativeParameters;
   clone->NumNativeAttributes = prog->NumNativeAttributes;
   clone->NumNativeAddressRegs = prog->NumNativeAddressRegs;
   clone->NumAluInstructions = prog->NumAluInstructions;
   clone->NumTexInstructions = prog->NumTexInstructions;
   clone->NumTexIndirections = prog->NumTexIndirections;
   clone->NumNativeAluInstructions = prog->NumNativeAluInstructions;
   clone->NumNativeTexInstructions = prog->NumNativeTexInstructions;
   clone->NumNativeTexIndirections = prog->NumNativeTexIndirections;

   switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         const struct gl_vertex_program *vp
            = (const struct gl_vertex_program *) prog;
         struct gl_vertex_program *vpc = (struct gl_vertex_program *) clone;
         vpc->IsPositionInvariant = vp->IsPositionInvariant;
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         const struct gl_fragment_program *fp
            = (const struct gl_fragment_program *) prog;
         struct gl_fragment_program *fpc = (struct gl_fragment_program *) clone;
         fpc->FogOption = fp->FogOption;
         fpc->UsesKill = fp->UsesKill;
      }
      break;
   default:
      _mesa_problem(NULL, "Unexpected target in _mesa_clone_program");
   }

   return clone;
}


/**
 * Insert 'count' NOP instructions at 'start' in the given program.
 * Adjust branch targets accordingly.
 */
GLboolean
_mesa_insert_instructions(struct gl_program *prog, GLuint start, GLuint count)
{
   const GLuint origLen = prog->NumInstructions;
   const GLuint newLen = origLen + count;
   struct prog_instruction *newInst;
   GLuint i;

   /* adjust branches */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      if (inst->BranchTarget > 0) {
         if (inst->BranchTarget >= start) {
            inst->BranchTarget += count;
         }
      }
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      return GL_FALSE;
   }

   /* Copy 'start' instructions into new instruction buffer */
   _mesa_copy_instructions(newInst, prog->Instructions, start);

   /* init the new instructions */
   _mesa_init_instructions(newInst + start, count);

   /* Copy the remaining/tail instructions to new inst buffer */
   _mesa_copy_instructions(newInst + start + count,
                           prog->Instructions + start,
                           origLen - start);

   /* free old instructions */
   _mesa_free_instructions(prog->Instructions, origLen);

   /* install new instructions */
   prog->Instructions = newInst;
   prog->NumInstructions = newLen;

   return GL_TRUE;
}


/**
 * Delete 'count' instructions at 'start' in the given program.
 * Adjust branch targets accordingly.
 */
GLboolean
_mesa_delete_instructions(struct gl_program *prog, GLuint start, GLuint count)
{
   const GLuint origLen = prog->NumInstructions;
   const GLuint newLen = origLen - count;
   struct prog_instruction *newInst;
   GLuint i;

   /* adjust branches */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      if (inst->BranchTarget > 0) {
         if (inst->BranchTarget >= start) {
            inst->BranchTarget -= count;
         }
      }
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      return GL_FALSE;
   }

   /* Copy 'start' instructions into new instruction buffer */
   _mesa_copy_instructions(newInst, prog->Instructions, start);

   /* Copy the remaining/tail instructions to new inst buffer */
   _mesa_copy_instructions(newInst + start,
                           prog->Instructions + start + count,
                           newLen - start);

   /* free old instructions */
   _mesa_free_instructions(prog->Instructions, origLen);

   /* install new instructions */
   prog->Instructions = newInst;
   prog->NumInstructions = newLen;

   return GL_TRUE;
}


/**
 * Search instructions for registers that match (oldFile, oldIndex),
 * replacing them with (newFile, newIndex).
 */
static void
replace_registers(struct prog_instruction *inst, GLuint numInst,
                  GLuint oldFile, GLuint oldIndex,
                  GLuint newFile, GLuint newIndex)
{
   GLuint i, j;
   for (i = 0; i < numInst; i++) {
      /* src regs */
      for (j = 0; j < _mesa_num_inst_src_regs(inst->Opcode); j++) {
         if (inst[i].SrcReg[j].File == oldFile &&
             inst[i].SrcReg[j].Index == oldIndex) {
            inst[i].SrcReg[j].File = newFile;
            inst[i].SrcReg[j].Index = newIndex;
         }
      }
      /* dst reg */
      if (inst[i].DstReg.File == oldFile && inst[i].DstReg.Index == oldIndex) {
         inst[i].DstReg.File = newFile;
         inst[i].DstReg.Index = newIndex;
      }
   }
}


/**
 * Search instructions for references to program parameters.  When found,
 * increment the parameter index by 'offset'.
 * Used when combining programs.
 */
static void
adjust_param_indexes(struct prog_instruction *inst, GLuint numInst,
                     GLuint offset)
{
   GLuint i, j;
   for (i = 0; i < numInst; i++) {
      for (j = 0; j < _mesa_num_inst_src_regs(inst->Opcode); j++) {
         GLuint f = inst[i].SrcReg[j].File;
         if (f == PROGRAM_CONSTANT ||
             f == PROGRAM_UNIFORM ||
             f == PROGRAM_STATE_VAR) {
            inst[i].SrcReg[j].Index += offset;
         }
      }
   }
}


/**
 * Combine two programs into one.  Fix instructions so the outputs of
 * the first program go to the inputs of the second program.
 */
struct gl_program *
_mesa_combine_programs(GLcontext *ctx,
                       const struct gl_program *progA,
                       const struct gl_program *progB)
{
   struct prog_instruction *newInst;
   struct gl_program *newProg;
   const GLuint lenA = progA->NumInstructions - 1; /* omit END instr */
   const GLuint lenB = progB->NumInstructions;
   const GLuint numParamsA = _mesa_num_parameters(progA->Parameters);
   const GLuint newLength = lenA + lenB;
   GLbitfield inputsB;
   GLuint i;

   ASSERT(progA->Target == progB->Target);

   newInst = _mesa_alloc_instructions(newLength);
   if (!newInst)
      return GL_FALSE;

   _mesa_copy_instructions(newInst, progA->Instructions, lenA);
   _mesa_copy_instructions(newInst + lenA, progB->Instructions, lenB);

   /* adjust branch / instruction addresses for B's instructions */
   for (i = 0; i < lenB; i++) {
      newInst[lenA + i].BranchTarget += lenA;
   }

   newProg = ctx->Driver.NewProgram(ctx, progA->Target, 0);
   newProg->Instructions = newInst;
   newProg->NumInstructions = newLength;

   if (newProg->Target == GL_FRAGMENT_PROGRAM_ARB) {
      struct gl_fragment_program *fprogA, *fprogB, *newFprog;
      fprogA = (struct gl_fragment_program *) progA;
      fprogB = (struct gl_fragment_program *) progB;
      newFprog = (struct gl_fragment_program *) newProg;

      newFprog->UsesKill = fprogA->UsesKill || fprogB->UsesKill;

      /* Connect color outputs of fprogA to color inputs of fprogB, via a
       * new temporary register.
       */
      if ((progA->OutputsWritten & (1 << FRAG_RESULT_COLR)) &&
          (progB->InputsRead & (1 << FRAG_ATTRIB_COL0))) {
         GLint tempReg = _mesa_find_free_register(newProg, PROGRAM_TEMPORARY);
         if (tempReg < 0) {
            _mesa_problem(ctx, "No free temp regs found in "
                          "_mesa_combine_programs(), using 31");
            tempReg = 31;
         }
         /* replace writes to result.color[0] with tempReg */
         replace_registers(newInst, lenA,
                           PROGRAM_OUTPUT, FRAG_RESULT_COLR,
                           PROGRAM_TEMPORARY, tempReg);
         /* replace reads from input.color[0] with tempReg */
         replace_registers(newInst + lenA, lenB,
                           PROGRAM_INPUT, FRAG_ATTRIB_COL0,
                           PROGRAM_TEMPORARY, tempReg);
      }

      inputsB = progB->InputsRead;
      if (progA->OutputsWritten & (1 << FRAG_RESULT_COLR)) {
         inputsB &= ~(1 << FRAG_ATTRIB_COL0);
      }
      newProg->InputsRead = progA->InputsRead | inputsB;
      newProg->OutputsWritten = progB->OutputsWritten;
      newProg->SamplersUsed = progA->SamplersUsed | progB->SamplersUsed;
   }
   else {
      /* vertex program */
      assert(0);      /* XXX todo */
   }

   /*
    * Merge parameters (uniforms, constants, etc)
    */
   newProg->Parameters = _mesa_combine_parameter_lists(progA->Parameters,
                                                       progB->Parameters);

   adjust_param_indexes(newInst + lenA, lenB, numParamsA);


   return newProg;
}




/**
 * Scan the given program to find a free register of the given type.
 * \param regFile - PROGRAM_INPUT, PROGRAM_OUTPUT or PROGRAM_TEMPORARY
 */
GLint
_mesa_find_free_register(const struct gl_program *prog, GLuint regFile)
{
   GLboolean used[MAX_PROGRAM_TEMPS];
   GLuint i, k;

   assert(regFile == PROGRAM_INPUT ||
          regFile == PROGRAM_OUTPUT ||
          regFile == PROGRAM_TEMPORARY);

   _mesa_memset(used, 0, sizeof(used));

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint n = _mesa_num_inst_src_regs(inst->Opcode);

      for (k = 0; k < n; k++) {
         if (inst->SrcReg[k].File == regFile) {
            used[inst->SrcReg[k].Index] = GL_TRUE;
         }
      }
   }

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      if (!used[i])
         return i;
   }

   return -1;
}
