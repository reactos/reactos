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
   ctx->VertexProgram.Current = (struct gl_vertex_program *) ctx->Shared->DefaultVertexProgram;
   assert(ctx->VertexProgram.Current);
   ctx->VertexProgram.Current->Base.RefCount++;
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      ctx->VertexProgram.TrackMatrix[i] = GL_NONE;
      ctx->VertexProgram.TrackMatrixTransform[i] = GL_IDENTITY_NV;
   }
#endif

#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   ctx->FragmentProgram.Enabled = GL_FALSE;
   ctx->FragmentProgram.Current = (struct gl_fragment_program *) ctx->Shared->DefaultFragmentProgram;
   assert(ctx->FragmentProgram.Current);
   ctx->FragmentProgram.Current->Base.RefCount++;
#endif

   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   ctx->ATIFragmentShader.Enabled = GL_FALSE;
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) ctx->Shared->DefaultFragmentShader;
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
   if (ctx->VertexProgram.Current) {
      ctx->VertexProgram.Current->Base.RefCount--;
      if (ctx->VertexProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->VertexProgram.Current->Base));
   }
#endif
#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   if (ctx->FragmentProgram.Current) {
      ctx->FragmentProgram.Current->Base.RefCount--;
      if (ctx->FragmentProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->FragmentProgram.Current->Base));
   }
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
      _mesa_bzero(prog, sizeof(*prog));
      prog->Id = id;
      prog->Target = target;
      prog->Resident = GL_TRUE;
      prog->RefCount = 1;
      prog->Format = GL_PROGRAM_FORMAT_ASCII_ARB;
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
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
      return _mesa_init_vertex_program(ctx, CALLOC_STRUCT(gl_vertex_program),
                                       target, id );
   case GL_FRAGMENT_PROGRAM_NV:
   case GL_FRAGMENT_PROGRAM_ARB:
      return _mesa_init_fragment_program(ctx,
                                         CALLOC_STRUCT(gl_fragment_program),
                                         target, id );
   default:
      _mesa_problem(ctx, "bad target in _mesa_new_program");
      return NULL;
   }
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

   if (prog == &_mesa_DummyProgram)
      return;
                 
   if (prog->String)
      _mesa_free(prog->String);

   if (prog->Instructions) {
      GLuint i;
      for (i = 0; i < prog->NumInstructions; i++) {
         if (prog->Instructions[i].Data)
            _mesa_free(prog->Instructions[i].Data);
         if (prog->Instructions[i].Comment)
            _mesa_free((char *) prog->Instructions[i].Comment);
      }
      _mesa_free(prog->Instructions);
   }

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
   clone->String = (GLubyte *) _mesa_strdup((char *) prog->String);
   clone->RefCount = 1;
   clone->Format = prog->Format;
   clone->Instructions = _mesa_alloc_instructions(prog->NumInstructions);
   if (!clone->Instructions) {
      _mesa_delete_program(ctx, clone);
      return NULL;
   }
   _mesa_copy_instructions(clone->Instructions, prog->Instructions,
                           prog->NumInstructions);
   clone->InputsRead = prog->InputsRead;
   clone->OutputsWritten = prog->OutputsWritten;
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
 * Mixing ARB and NV vertex/fragment programs can be tricky.
 * Note: GL_VERTEX_PROGRAM_ARB == GL_VERTEX_PROGRAM_NV
 *  but, GL_FRAGMENT_PROGRAM_ARB != GL_FRAGMENT_PROGRAM_NV
 * The two different fragment program targets are supposed to be compatible
 * to some extent (see GL_ARB_fragment_program spec).
 * This function does the compatibility check.
 */
static GLboolean
compatible_program_targets(GLenum t1, GLenum t2)
{
   if (t1 == t2)
      return GL_TRUE;
   if (t1 == GL_FRAGMENT_PROGRAM_ARB && t2 == GL_FRAGMENT_PROGRAM_NV)
      return GL_TRUE;
   if (t1 == GL_FRAGMENT_PROGRAM_NV && t2 == GL_FRAGMENT_PROGRAM_ARB)
      return GL_TRUE;
   return GL_FALSE;
}



/**********************************************************************/
/* API functions                                                      */
/**********************************************************************/


/**
 * Bind a program (make it current)
 * \note Called from the GL API dispatcher by both glBindProgramNV
 * and glBindProgramARB.
 */
void GLAPIENTRY
_mesa_BindProgram(GLenum target, GLuint id)
{
   struct gl_program *curProg, *newProg;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   /* Error-check target and get curProg */
   if ((target == GL_VERTEX_PROGRAM_ARB) && /* == GL_VERTEX_PROGRAM_NV */
        (ctx->Extensions.NV_vertex_program ||
         ctx->Extensions.ARB_vertex_program)) {
      curProg = &ctx->VertexProgram.Current->Base;
   }
   else if ((target == GL_FRAGMENT_PROGRAM_NV
             && ctx->Extensions.NV_fragment_program) ||
            (target == GL_FRAGMENT_PROGRAM_ARB
             && ctx->Extensions.ARB_fragment_program)) {
      curProg = &ctx->FragmentProgram.Current->Base;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramNV/ARB(target)");
      return;
   }

   /*
    * Get pointer to new program to bind.
    * NOTE: binding to a non-existant program is not an error.
    * That's supposed to be caught in glBegin.
    */
   if (id == 0) {
      /* Bind a default program */
      newProg = NULL;
      if (target == GL_VERTEX_PROGRAM_ARB) /* == GL_VERTEX_PROGRAM_NV */
         newProg = ctx->Shared->DefaultVertexProgram;
      else
         newProg = ctx->Shared->DefaultFragmentProgram;
   }
   else {
      /* Bind a user program */
      newProg = _mesa_lookup_program(ctx, id);
      if (!newProg || newProg == &_mesa_DummyProgram) {
         /* allocate a new program now */
         newProg = ctx->Driver.NewProgram(ctx, target, id);
         if (!newProg) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindProgramNV/ARB");
            return;
         }
         _mesa_HashInsert(ctx->Shared->Programs, id, newProg);
      }
      else if (!compatible_program_targets(newProg->Target, target)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBindProgramNV/ARB(target mismatch)");
         return;
      }
   }

   /** All error checking is complete now **/

   if (curProg->Id == id) {
      /* binding same program - no change */
      return;
   }

   /* unbind/delete oldProg */
   if (curProg->Id != 0) {
      /* decrement refcount on previously bound fragment program */
      curProg->RefCount--;
      /* and delete if refcount goes below one */
      if (curProg->RefCount <= 0) {
         /* the program ID was already removed from the hash table */
         ctx->Driver.DeleteProgram(ctx, curProg);
      }
   }

   /* bind newProg */
   if (target == GL_VERTEX_PROGRAM_ARB) { /* == GL_VERTEX_PROGRAM_NV */
      ctx->VertexProgram.Current = (struct gl_vertex_program *) newProg;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV ||
            target == GL_FRAGMENT_PROGRAM_ARB) {
      ctx->FragmentProgram.Current = (struct gl_fragment_program *) newProg;
   }
   newProg->RefCount++;

   /* Never null pointers */
   ASSERT(ctx->VertexProgram.Current);
   ASSERT(ctx->FragmentProgram.Current);

   if (ctx->Driver.BindProgram)
      ctx->Driver.BindProgram(ctx, target, newProg);
}


/**
 * Delete a list of programs.
 * \note Not compiled into display lists.
 * \note Called by both glDeleteProgramsNV and glDeleteProgramsARB.
 */
void GLAPIENTRY 
_mesa_DeletePrograms(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteProgramsNV" );
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
         struct gl_program *prog = _mesa_lookup_program(ctx, ids[i]);
         if (prog == &_mesa_DummyProgram) {
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
         }
         else if (prog) {
            /* Unbind program if necessary */
            if (prog->Target == GL_VERTEX_PROGRAM_ARB || /* == GL_VERTEX_PROGRAM_NV */
                prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
               if (ctx->VertexProgram.Current &&
                   ctx->VertexProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else if (prog->Target == GL_FRAGMENT_PROGRAM_NV ||
                     prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
               if (ctx->FragmentProgram.Current &&
                   ctx->FragmentProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else {
               _mesa_problem(ctx, "bad target in glDeleteProgramsNV");
               return;
            }
            /* The ID is immediately available for re-use now */
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
            prog->RefCount--;
            if (prog->RefCount <= 0) {
               ctx->Driver.DeleteProgram(ctx, prog);
            }
         }
      }
   }
}


/**
 * Generate a list of new program identifiers.
 * \note Not compiled into display lists.
 * \note Called by both glGenProgramsNV and glGenProgramsARB.
 */
void GLAPIENTRY
_mesa_GenPrograms(GLsizei n, GLuint *ids)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPrograms");
      return;
   }

   if (!ids)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->Programs, n);

   /* Insert pointer to dummy program as placeholder */
   for (i = 0; i < (GLuint) n; i++) {
      _mesa_HashInsert(ctx->Shared->Programs, first + i, &_mesa_DummyProgram);
   }

   /* Return the program names */
   for (i = 0; i < (GLuint) n; i++) {
      ids[i] = first + i;
   }
}
