/*
 * Mesa 3-D graphics library
 * Version:  7.0
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
 * \file arbprogram.c
 * ARB_vertex/fragment_program state management functions.
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/hash.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "arbprogram.h"
#include "arbprogparse.h"
#include "program.h"



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
         newProg = &ctx->Shared->DefaultVertexProgram->Base;
      else
         newProg = &ctx->Shared->DefaultFragmentProgram->Base;
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

   /* bind newProg */
   if (target == GL_VERTEX_PROGRAM_ARB) { /* == GL_VERTEX_PROGRAM_NV */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
                               (struct gl_vertex_program *) newProg);
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV ||
            target == GL_FRAGMENT_PROGRAM_ARB) {
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                               (struct gl_fragment_program *) newProg);
   }

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
            _mesa_reference_program(ctx, &prog, NULL);
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


void GLAPIENTRY
_mesa_EnableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ArrayObj->VertexAttrib[index].Enabled = GL_TRUE;
   ctx->Array.ArrayObj->_Enabled |= _NEW_ARRAY_ATTRIB(index);
   ctx->Array.NewState |= _NEW_ARRAY_ATTRIB(index);
}


void GLAPIENTRY
_mesa_DisableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ArrayObj->VertexAttrib[index].Enabled = GL_FALSE;
   ctx->Array.ArrayObj->_Enabled &= ~_NEW_ARRAY_ATTRIB(index);
   ctx->Array.NewState |= _NEW_ARRAY_ATTRIB(index);
}


void GLAPIENTRY
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params)
{
   GLfloat fparams[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_GetVertexAttribfvARB(index, pname, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
         COPY_4V(params, fparams);
      }
      else {
         params[0] = fparams[0];
      }
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_VERTEX_PROGRAM_ATTRIBS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribfvARB(index)");
      return;
   }

   switch (pname) {
      case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Enabled;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Size;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Stride;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Type;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Normalized;
         break;
      case GL_CURRENT_VERTEX_ATTRIB_ARB:
         if (index == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetVertexAttribfvARB(index==0)");
            return;
         }
         FLUSH_CURRENT(ctx, 0);
         COPY_4V(params, ctx->Current.Attrib[VERT_ATTRIB_GENERIC0 + index]);
         break;
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
         if (!ctx->Extensions.ARB_vertex_buffer_object) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribfvARB(pname)");
            return;
         }
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].BufferObj->Name;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribfvARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params)
{
   GLfloat fparams[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_GetVertexAttribfvARB(index, pname, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
         COPY_4V_CAST(params, fparams, GLint);  /* float to int */
      }
      else {
         params[0] = (GLint) fparams[0];
      }
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerARB(index)");
      return;
   }

   if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerARB(pname)");
      return;
   }

   *pointer = (GLvoid *) ctx->Array.ArrayObj->VertexAttrib[index].Ptr;
}


/**
 * Determine if id names a vertex or fragment program.
 * \note Not compiled into display lists.
 * \note Called from both glIsProgramNV and glIsProgramARB.
 * \param id is the program identifier
 * \return GL_TRUE if id is a program, else GL_FALSE.
 */
GLboolean GLAPIENTRY
_mesa_IsProgramARB(GLuint id)
{
   struct gl_program *prog = NULL; 
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   prog = _mesa_lookup_program(ctx, id);
   if (prog && (prog != &_mesa_DummyProgram))
      return GL_TRUE;
   else
      return GL_FALSE;
}


void GLAPIENTRY
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
      return;
   }

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      struct gl_vertex_program *prog = ctx->VertexProgram.Current;
      _mesa_parse_arb_vertex_program(ctx, target, string, len, prog);
      
      if (ctx->Program.ErrorPos == -1 && ctx->Driver.ProgramStringNotify)
	 ctx->Driver.ProgramStringNotify( ctx, target, &prog->Base );
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      struct gl_fragment_program *prog = ctx->FragmentProgram.Current;
      _mesa_parse_arb_fragment_program(ctx, target, string, len, prog);

      if (ctx->Program.ErrorPos == -1 && ctx->Driver.ProgramStringNotify)
	 ctx->Driver.ProgramStringNotify( ctx, target, &prog->Base );
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(target)");
      return;
   }
}


/**
 * Set a program env parameter register.
 * \note Called from the GL API dispatcher.
 * Note, this function is also used by the GL_NV_vertex_program extension
 * (alias to ProgramParameterdNV)
 */
void GLAPIENTRY
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, 
		                  (GLfloat) z, (GLfloat) w);
}


/**
 * Set a program env parameter register.
 * \note Called from the GL API dispatcher.
 * Note, this function is also used by the GL_NV_vertex_program extension
 * (alias to ProgramParameterdvNV)
 */
void GLAPIENTRY
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                const GLdouble *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) params[0], 
	                          (GLfloat) params[1], (GLfloat) params[2], 
				  (GLfloat) params[3]);
}


/**
 * Set a program env parameter register.
 * \note Called from the GL API dispatcher.
 * Note, this function is also used by the GL_NV_vertex_program extension
 * (alias to ProgramParameterfNV)
 */
void GLAPIENTRY
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if (index >= ctx->Const.FragmentProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameter(index)");
         return;
      }
      ASSIGN_4V(ctx->FragmentProgram.Parameters[index], x, y, z, w);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB /* == GL_VERTEX_PROGRAM_NV */
       && (ctx->Extensions.ARB_vertex_program || ctx->Extensions.NV_vertex_program)) {
      if (index >= ctx->Const.VertexProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameter(index)");
         return;
      }
      ASSIGN_4V(ctx->VertexProgram.Parameters[index], x, y, z, w);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramEnvParameter(target)");
      return;
   }
}

/**
 * Set a program env parameter register.
 * \note Called from the GL API dispatcher.
 * Note, this function is also used by the GL_NV_vertex_program extension
 * (alias to ProgramParameterfvNV)
 */
void GLAPIENTRY
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                   const GLfloat *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, params[0], params[1],
                                  params[2], params[3]);
}


void GLAPIENTRY
_mesa_ProgramEnvParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				 const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLfloat * dest;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (count <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameters4fv(count)");
   }

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if ((index + count) > ctx->Const.FragmentProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameters4fv(index + count)");
         return;
      }
      dest = ctx->FragmentProgram.Parameters[index];
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if ((index + count) > ctx->Const.VertexProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameters4fv(index + count)");
         return;
      }
      dest = ctx->VertexProgram.Parameters[index];
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramEnvParameters4fv(target)");
      return;
   }

   for ( i = 0 ; i < count ; i++ ) {
      COPY_4V(dest, params);
      params += 4;
      dest += 4;
   }
}


void GLAPIENTRY
_mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                  GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat fparams[4];

   _mesa_GetProgramEnvParameterfvARB(target, index, fparams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      params[0] = fparams[0];
      params[1] = fparams[1];
      params[2] = fparams[2];
      params[3] = fparams[3];
   }
}


void GLAPIENTRY
_mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index, 
                                  GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if (index >= ctx->Const.FragmentProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramEnvParameter(index)");
         return;
      }
      COPY_4V(params, ctx->FragmentProgram.Parameters[index]);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.VertexProgram.MaxEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramEnvParameter(index)");
         return;
      }
      COPY_4V(params, ctx->VertexProgram.Parameters[index]);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramEnvParameter(target)");
      return;
   }
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *prog;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if ((target == GL_FRAGMENT_PROGRAM_NV
        && ctx->Extensions.NV_fragment_program) ||
       (target == GL_FRAGMENT_PROGRAM_ARB
        && ctx->Extensions.ARB_fragment_program)) {
      if (index >= ctx->Const.FragmentProgram.MaxLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
            && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.VertexProgram.MaxLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameterARB");
      return;
   }

   ASSERT(index < MAX_PROGRAM_LOCAL_PARAMS);
   prog->LocalParams[index][0] = x;
   prog->LocalParams[index][1] = y;
   prog->LocalParams[index][2] = z;
   prog->LocalParams[index][3] = w;
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params)
{
   _mesa_ProgramLocalParameter4fARB(target, index, params[0], params[1],
                                    params[2], params[3]);
}


void GLAPIENTRY
_mesa_ProgramLocalParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				   const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *prog;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (count <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameters4fv(count)");
   }

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if ((index + count) > ctx->Const.FragmentProgram.MaxLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameters4fvEXT(index + count)");
         return;
      }
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
            && ctx->Extensions.ARB_vertex_program) {
      if ((index + count) > ctx->Const.VertexProgram.MaxLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameters4fvEXT(index + count)");
         return;
      }
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramLocalParameters4fvEXT(target)");
      return;
   }

   for (i = 0; i < count; i++) {
      ASSERT((index + i) < MAX_PROGRAM_LOCAL_PARAMS);
      COPY_4V(prog->LocalParams[index + i], params);
      params += 4;
   }
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w)
{
   _mesa_ProgramLocalParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, 
                                    (GLfloat) z, (GLfloat) w);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params)
{
   _mesa_ProgramLocalParameter4fARB(target, index,
                                    (GLfloat) params[0], (GLfloat) params[1],
                                    (GLfloat) params[2], (GLfloat) params[3]);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index,
                                    GLfloat *params)
{
   const struct gl_program *prog;
   GLuint maxParams;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
      maxParams = ctx->Const.VertexProgram.MaxLocalParams;
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = ctx->Const.FragmentProgram.MaxLocalParams;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV
            && ctx->Extensions.NV_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetProgramLocalParameterARB(target)");
      return;
   }

   if (index >= maxParams) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetProgramLocalParameterARB(index)");
      return;
   }

   ASSERT(prog);
   ASSERT(index < MAX_PROGRAM_LOCAL_PARAMS);
   COPY_4V(params, prog->LocalParams[index]);
}


/**
 * Note, this function is also used by the GL_NV_fragment_program extension.
 */
void GLAPIENTRY
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat floatParams[4];
   ASSIGN_4V(floatParams, 0.0F, 0.0F, 0.0F, 0.0F);
   _mesa_GetProgramLocalParameterfvARB(target, index, floatParams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      COPY_4V(params, floatParams);
   }
}


void GLAPIENTRY
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params)
{
   const struct gl_program_constants *limits;
   struct gl_program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
      limits = &ctx->Const.VertexProgram;
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      limits = &ctx->Const.FragmentProgram;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
      return;
   }

   ASSERT(prog);
   ASSERT(limits);

   /* Queries supported for both vertex and fragment programs */
   switch (pname) {
      case GL_PROGRAM_LENGTH_ARB:
         *params
            = prog->String ? (GLint) _mesa_strlen((char *) prog->String) : 0;
         return;
      case GL_PROGRAM_FORMAT_ARB:
         *params = prog->Format;
         return;
      case GL_PROGRAM_BINDING_ARB:
         *params = prog->Id;
         return;
      case GL_PROGRAM_INSTRUCTIONS_ARB:
         *params = prog->NumInstructions;
         return;
      case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
         *params = limits->MaxInstructions;
         return;
      case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         *params = prog->NumNativeInstructions;
         return;
      case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         *params = limits->MaxNativeInstructions;
         return;
      case GL_PROGRAM_TEMPORARIES_ARB:
         *params = prog->NumTemporaries;
         return;
      case GL_MAX_PROGRAM_TEMPORARIES_ARB:
         *params = limits->MaxTemps;
         return;
      case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
         *params = prog->NumNativeTemporaries;
         return;
      case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
         *params = limits->MaxNativeTemps;
         return;
      case GL_PROGRAM_PARAMETERS_ARB:
         *params = prog->NumParameters;
         return;
      case GL_MAX_PROGRAM_PARAMETERS_ARB:
         *params = limits->MaxParameters;
         return;
      case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
         *params = prog->NumNativeParameters;
         return;
      case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
         *params = limits->MaxNativeParameters;
         return;
      case GL_PROGRAM_ATTRIBS_ARB:
         *params = prog->NumAttributes;
         return;
      case GL_MAX_PROGRAM_ATTRIBS_ARB:
         *params = limits->MaxAttribs;
         return;
      case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
         *params = prog->NumNativeAttributes;
         return;
      case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
         *params = limits->MaxNativeAttribs;
         return;
      case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
         *params = prog->NumAddressRegs;
         return;
      case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
         *params = limits->MaxAddressRegs;
         return;
      case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         *params = prog->NumNativeAddressRegs;
         return;
      case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         *params = limits->MaxNativeAddressRegs;
         return;
      case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
         *params = limits->MaxLocalParams;
         return;
      case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
         *params = limits->MaxEnvParams;
         return;
      case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
         /*
          * XXX we may not really need a driver callback here.
          * If the number of native instructions, registers, etc. used
          * are all below the maximums, we could return true.
          * The spec says that even if this query returns true, there's
          * no guarantee that the program will run in hardware.
          */
         if (prog->Id == 0) {
            /* default/null program */
            *params = GL_FALSE;
         }
	 else if (ctx->Driver.IsProgramNative) {
            /* ask the driver */
	    *params = ctx->Driver.IsProgramNative( ctx, target, prog );
         }
	 else {
            /* probably running in software */
	    *params = GL_TRUE;
         }
         return;
      default:
         /* continue with fragment-program only queries below */
         break;
   }

   /*
    * The following apply to fragment programs only (at this time)
    */
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      const struct gl_fragment_program *fp = ctx->FragmentProgram.Current;
      switch (pname) {
         case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
            *params = fp->Base.NumNativeAluInstructions;
            return;
         case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
            *params = fp->Base.NumAluInstructions;
            return;
         case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
            *params = fp->Base.NumTexInstructions;
            return;
         case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
            *params = fp->Base.NumNativeTexInstructions;
            return;
         case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
            *params = fp->Base.NumTexIndirections;
            return;
         case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
            *params = fp->Base.NumNativeTexIndirections;
            return;
         case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
            *params = limits->MaxAluInstructions;
            return;
         case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
            *params = limits->MaxNativeAluInstructions;
            return;
         case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
            *params = limits->MaxTexInstructions;
            return;
         case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
            *params = limits->MaxNativeTexInstructions;
            return;
         case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
            *params = limits->MaxTexIndirections;
            return;
         case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
            *params = limits->MaxNativeTexIndirections;
            return;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(pname)");
            return;
      }
   }
}


void GLAPIENTRY
_mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string)
{
   const struct gl_program *prog;
   char *dst = (char *) string;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB) {
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(target)");
      return;
   }

   ASSERT(prog);

   if (pname != GL_PROGRAM_STRING_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(pname)");
      return;
   }

   if (prog->String)
      _mesa_memcpy(dst, prog->String, _mesa_strlen((char *) prog->String));
   else
      *dst = '\0';
}
