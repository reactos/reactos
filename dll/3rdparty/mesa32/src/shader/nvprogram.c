/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file nvprogram.c
 * NVIDIA vertex/fragment program state management functions.
 * \author Brian Paul
 */

/*
 * Regarding GL_NV_fragment/vertex_program, GL_NV_vertex_program1_1, etc:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */

#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "prog_parameter.h"
#include "prog_instruction.h"
#include "nvfragparse.h"
#include "nvvertparse.h"
#include "nvprogram.h"
#include "program.h"



/**
 * Execute a vertex state program.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_ExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
   struct gl_vertex_program *vprog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_VERTEX_STATE_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glExecuteProgramNV");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   vprog = (struct gl_vertex_program *) _mesa_lookup_program(ctx, id);

   if (!vprog || vprog->Base.Target != GL_VERTEX_STATE_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glExecuteProgramNV");
      return;
   }
   
   _mesa_problem(ctx, "glExecuteProgramNV() not supported");
}


/**
 * Determine if a set of programs is resident in hardware.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
GLboolean GLAPIENTRY
_mesa_AreProgramsResidentNV(GLsizei n, const GLuint *ids,
                            GLboolean *residences)
{
   GLint i, j;
   GLboolean allResident = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV(n)");
      return GL_FALSE;
   }

   for (i = 0; i < n; i++) {
      const struct gl_program *prog;
      if (ids[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV");
         return GL_FALSE;
      }
      prog = _mesa_lookup_program(ctx, ids[i]);
      if (!prog) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV");
         return GL_FALSE;
      }
      if (prog->Resident) {
	 if (!allResident)
	    residences[i] = GL_TRUE;
      }
      else {
         if (allResident) {
	    allResident = GL_FALSE;
	    for (j = 0; j < i; j++)
	       residences[j] = GL_TRUE;
	 }
	 residences[i] = GL_FALSE;
      }
   }

   return allResident;
}


/**
 * Request that a set of programs be resident in hardware.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_RequestResidentProgramsNV(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(n)");
      return;
   }

   /* just error checking for now */
   for (i = 0; i < n; i++) {
      struct gl_program *prog;

      if (ids[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(id)");
         return;
      }

      prog = _mesa_lookup_program(ctx, ids[i]);
      if (!prog) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(id)");
         return;
      }

      /* XXX this is really a hardware thing we should hook out */
      prog->Resident = GL_TRUE;
   }
}


/**
 * Get a program parameter register.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetProgramParameterfvNV(GLenum target, GLuint index,
                              GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV) {
      if (pname == GL_PROGRAM_PARAMETER_NV) {
         if (index < MAX_NV_VERTEX_PROGRAM_PARAMS) {
            COPY_4V(params, ctx->VertexProgram.Parameters[index]);
         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramParameterfvNV(index)");
            return;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterfvNV(pname)");
         return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterfvNV(target)");
      return;
   }
}


/**
 * Get a program parameter register.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetProgramParameterdvNV(GLenum target, GLuint index,
                              GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV) {
      if (pname == GL_PROGRAM_PARAMETER_NV) {
         if (index < MAX_NV_VERTEX_PROGRAM_PARAMS) {
            COPY_4V(params, ctx->VertexProgram.Parameters[index]);
         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramParameterdvNV(index)");
            return;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterdvNV(pname)");
         return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterdvNV(target)");
      return;
   }
}


/**
 * Get a program attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetProgramivNV(GLuint id, GLenum pname, GLint *params)
{
   struct gl_program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   prog = _mesa_lookup_program(ctx, id);
   if (!prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramivNV");
      return;
   }

   switch (pname) {
      case GL_PROGRAM_TARGET_NV:
         *params = prog->Target;
         return;
      case GL_PROGRAM_LENGTH_NV:
         *params = prog->String ?(GLint)_mesa_strlen((char *) prog->String) : 0;
         return;
      case GL_PROGRAM_RESIDENT_NV:
         *params = prog->Resident;
         return;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivNV(pname)");
         return;
   }
}


/**
 * Get the program source code.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetProgramStringNV(GLuint id, GLenum pname, GLubyte *program)
{
   struct gl_program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname != GL_PROGRAM_STRING_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringNV(pname)");
      return;
   }

   prog = _mesa_lookup_program(ctx, id);
   if (!prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramStringNV");
      return;
   }

   if (prog->String) {
      MEMCPY(program, prog->String, _mesa_strlen((char *) prog->String));
   }
   else {
      program[0] = 0;
   }
}


/**
 * Get matrix tracking information.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetTrackMatrixivNV(GLenum target, GLuint address,
                         GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV
       && ctx->Extensions.NV_vertex_program) {
      GLuint i;

      if ((address & 0x3) || address >= MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetTrackMatrixivNV(address)");
         return;
      }

      i = address / 4;

      switch (pname) {
         case GL_TRACK_MATRIX_NV:
            params[0] = (GLint) ctx->VertexProgram.TrackMatrix[i];
            return;
         case GL_TRACK_MATRIX_TRANSFORM_NV:
            params[0] = (GLint) ctx->VertexProgram.TrackMatrixTransform[i];
            return;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTrackMatrixivNV");
            return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTrackMatrixivNV");
      return;
   }
}


/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
         if (index == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetVertexAttribdvNV(index == 0)");
            return;
         }
	 FLUSH_CURRENT(ctx, 0);
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}

/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = (GLfloat) ctx->Array.ArrayObj->VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
         if (index == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetVertexAttribfvNV(index == 0)");
            return;
         }
	 FLUSH_CURRENT(ctx, 0);
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}

/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetVertexAttribivNV(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
         if (index == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetVertexAttribivNV(index == 0)");
            return;
         }
	 FLUSH_CURRENT(ctx, 0);
         params[0] = (GLint) ctx->Current.Attrib[index][0];
         params[1] = (GLint) ctx->Current.Attrib[index][1];
         params[2] = (GLint) ctx->Current.Attrib[index][2];
         params[3] = (GLint) ctx->Current.Attrib[index][3];
         break;
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
         if (!ctx->Extensions.ARB_vertex_buffer_object) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
            return;
         }
         params[0] = ctx->Array.ArrayObj->VertexAttrib[index].BufferObj->Name;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}


/**
 * Get a vertex array attribute pointer.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_GetVertexAttribPointervNV(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerNV(index)");
      return;
   }

   if (pname != GL_ATTRIB_ARRAY_POINTER_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerNV(pname)");
      return;
   }

   *pointer = (GLvoid *) ctx->Array.ArrayObj->VertexAttrib[index].Ptr;
}



/**
 * Load/parse/compile a program.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_LoadProgramNV(GLenum target, GLuint id, GLsizei len,
                    const GLubyte *program)
{
   struct gl_program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glLoadProgramNV(id)");
      return;
   }

   if (len < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glLoadProgramNV(len)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   prog = _mesa_lookup_program(ctx, id);

   if (prog && prog->Target != 0 && prog->Target != target) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(target)");
      return;
   }

   if ((target == GL_VERTEX_PROGRAM_NV ||
        target == GL_VERTEX_STATE_PROGRAM_NV)
       && ctx->Extensions.NV_vertex_program) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      if (!vprog || prog == &_mesa_DummyProgram) {
         vprog = (struct gl_vertex_program *)
            ctx->Driver.NewProgram(ctx, target, id);
         if (!vprog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
            return;
         }
         _mesa_HashInsert(ctx->Shared->Programs, id, vprog);
      }
      _mesa_parse_nv_vertex_program(ctx, target, program, len, vprog);
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV
            && ctx->Extensions.NV_fragment_program) {
      struct gl_fragment_program *fprog = (struct gl_fragment_program *) prog;
      if (!fprog || prog == &_mesa_DummyProgram) {
         fprog = (struct gl_fragment_program *)
            ctx->Driver.NewProgram(ctx, target, id);
         if (!fprog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
            return;
         }
         _mesa_HashInsert(ctx->Shared->Programs, id, fprog);
      }
      _mesa_parse_nv_fragment_program(ctx, target, program, len, fprog);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glLoadProgramNV(target)");
   }
}



/**
 * Set a sequence of program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_ProgramParameters4dvNV(GLenum target, GLuint index,
                             GLuint num, const GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      GLuint i;
      if (index + num > MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramParameters4dvNV");
         return;
      }
      for (i = 0; i < num; i++) {
         ctx->VertexProgram.Parameters[index + i][0] = (GLfloat) params[0];
         ctx->VertexProgram.Parameters[index + i][1] = (GLfloat) params[1];
         ctx->VertexProgram.Parameters[index + i][2] = (GLfloat) params[2];
         ctx->VertexProgram.Parameters[index + i][3] = (GLfloat) params[3];
         params += 4;
      };
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameters4dvNV");
      return;
   }
}


/**
 * Set a sequence of program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_ProgramParameters4fvNV(GLenum target, GLuint index,
                             GLuint num, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      GLuint i;
      if (index + num > MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramParameters4fvNV");
         return;
      }
      for (i = 0; i < num; i++) {
         COPY_4V(ctx->VertexProgram.Parameters[index + i], params);
         params += 4;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameters4fvNV");
      return;
   }
}



/**
 * Setup tracking of matrices into program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void GLAPIENTRY
_mesa_TrackMatrixNV(GLenum target, GLuint address,
                    GLenum matrix, GLenum transform)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      if (address & 0x3) {
         /* addr must be multiple of four */
         _mesa_error(ctx, GL_INVALID_VALUE, "glTrackMatrixNV(address)");
         return;
      }

      switch (matrix) {
         case GL_NONE:
         case GL_MODELVIEW:
         case GL_PROJECTION:
         case GL_TEXTURE:
         case GL_COLOR:
         case GL_MODELVIEW_PROJECTION_NV:
         case GL_MATRIX0_NV:
         case GL_MATRIX1_NV:
         case GL_MATRIX2_NV:
         case GL_MATRIX3_NV:
         case GL_MATRIX4_NV:
         case GL_MATRIX5_NV:
         case GL_MATRIX6_NV:
         case GL_MATRIX7_NV:
            /* OK, fallthrough */
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(matrix)");
            return;
      }

      switch (transform) {
         case GL_IDENTITY_NV:
         case GL_INVERSE_NV:
         case GL_TRANSPOSE_NV:
         case GL_INVERSE_TRANSPOSE_NV:
            /* OK, fallthrough */
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(transform)");
            return;
      }

      ctx->VertexProgram.TrackMatrix[address / 4] = matrix;
      ctx->VertexProgram.TrackMatrixTransform[address / 4] = transform;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(target)");
      return;
   }
}


void GLAPIENTRY
_mesa_ProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   struct gl_program *prog;
   struct gl_fragment_program *fragProg;
   GLfloat *v;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   prog = _mesa_lookup_program(ctx, id);
   if (!prog || prog->Target != GL_FRAGMENT_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glProgramNamedParameterNV");
      return;
   }

   if (len <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramNamedParameterNV(len)");
      return;
   }

   fragProg = (struct gl_fragment_program *) prog;
   v = _mesa_lookup_parameter_value(fragProg->Base.Parameters, len,
                                    (char *) name);
   if (v) {
      v[0] = x;
      v[1] = y;
      v[2] = z;
      v[3] = w;
      return;
   }

   _mesa_error(ctx, GL_INVALID_VALUE, "glProgramNamedParameterNV(name)");
}


void GLAPIENTRY
_mesa_ProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const float v[])
{
   _mesa_ProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}


void GLAPIENTRY
_mesa_ProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramNamedParameter4fNV(id, len, name, (GLfloat)x, (GLfloat)y, 
                                   (GLfloat)z, (GLfloat)w);
}


void GLAPIENTRY
_mesa_ProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const double v[])
{
   _mesa_ProgramNamedParameter4fNV(id, len, name,
                                   (GLfloat)v[0], (GLfloat)v[1],
                                   (GLfloat)v[2], (GLfloat)v[3]);
}


void GLAPIENTRY
_mesa_GetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLfloat *params)
{
   struct gl_program *prog;
   struct gl_fragment_program *fragProg;
   const GLfloat *v;

   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   prog = _mesa_lookup_program(ctx, id);
   if (!prog || prog->Target != GL_FRAGMENT_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramNamedParameterNV");
      return;
   }

   if (len <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramNamedParameterNV");
      return;
   }

   fragProg = (struct gl_fragment_program *) prog;
   v = _mesa_lookup_parameter_value(fragProg->Base.Parameters,
                                    len, (char *) name);
   if (v) {
      params[0] = v[0];
      params[1] = v[1];
      params[2] = v[2];
      params[3] = v[3];
      return;
   }

   _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramNamedParameterNV");
}


void GLAPIENTRY
_mesa_GetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLdouble *params)
{
   GLfloat floatParams[4];
   _mesa_GetProgramNamedParameterfvNV(id, len, name, floatParams);
   COPY_4V(params, floatParams);
}
