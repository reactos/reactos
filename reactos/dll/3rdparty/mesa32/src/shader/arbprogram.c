/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "arbprogram.h"
#include "arbfragparse.h"
#include "arbvertparse.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvprogram.h"
#include "nvfragparse.h"
#include "nvfragprog.h"
#include "nvvertparse.h"
#include "nvvertprog.h"


void GLAPIENTRY
_mesa_EnableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.VertexAttrib[index].Enabled = GL_TRUE;
   ctx->Array._Enabled |= _NEW_ARRAY_ATTRIB(index);
   ctx->Array.NewState |= _NEW_ARRAY_ATTRIB(index);
}


void GLAPIENTRY
_mesa_DisableVertexAttribArrayARB(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.VertexAttrib[index].Enabled = GL_FALSE;
   ctx->Array._Enabled &= ~_NEW_ARRAY_ATTRIB(index);
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

   if (index == 0 || index >= MAX_VERTEX_PROGRAM_ATTRIBS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribfvARB(index)");
      return;
   }

   switch (pname) {
      case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Enabled;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Size;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Stride;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Type;
         break;
      case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
         params[0] = ctx->Array.VertexAttrib[index].Normalized;
         break;
      case GL_CURRENT_VERTEX_ATTRIB_ARB:
	 FLUSH_CURRENT(ctx, 0);
         /* XXX should read:
            COPY_4V(params, ctx->Current.Attrib[VERT_ATTRIB_GENERIC0 + index]);
          */
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
         if (!ctx->Extensions.ARB_vertex_buffer_object) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribfvARB(pname)");
            return;
         }
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].BufferObj->Name;
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

   if (index >= ctx->Const.MaxVertexProgramAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerARB(index)");
      return;
   }

   if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerARB(pname)");
      return;
   }

   *pointer = (GLvoid *) ctx->Array.VertexAttrib[index].Ptr;;
}


void GLAPIENTRY
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      struct vertex_program *prog = ctx->VertexProgram.Current;
      if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
         return;
      }
      _mesa_parse_arb_vertex_program(ctx, target, (const GLubyte *) string,
                                     len, prog);
      
      if (ctx->Driver.ProgramStringNotify)
	 ctx->Driver.ProgramStringNotify( ctx, target, &prog->Base );
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      struct fragment_program *prog = ctx->FragmentProgram.Current;
      if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
         return;
      }
      _mesa_parse_arb_fragment_program(ctx, target, (const GLubyte *) string,
                                       len, prog);

      if (ctx->Driver.ProgramStringNotify)
	 ctx->Driver.ProgramStringNotify( ctx, target, &prog->Base );
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(target)");
      return;
   }
}


void GLAPIENTRY
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, 
		                  (GLfloat) z, (GLfloat) w);
}


void GLAPIENTRY
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                const GLdouble *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) params[0], 
	                          (GLfloat) params[1], (GLfloat) params[2], 
				  (GLfloat) params[3]);
}


void GLAPIENTRY
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (target == GL_FRAGMENT_PROGRAM_ARB
       && ctx->Extensions.ARB_fragment_program) {
      if (index >= ctx->Const.MaxFragmentProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameter(index)");
         return;
      }
      ASSIGN_4V(ctx->FragmentProgram.Parameters[index], x, y, z, w);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.MaxVertexProgramEnvParams) {
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


void GLAPIENTRY
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                   const GLfloat *params)
{
   _mesa_ProgramEnvParameter4fARB(target, index, params[0], params[1],
                                  params[2], params[3]);
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
      if (index >= ctx->Const.MaxFragmentProgramEnvParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramEnvParameter(index)");
         return;
      }
      COPY_4V(params, ctx->FragmentProgram.Parameters[index]);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.MaxVertexProgramEnvParams) {
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
   struct program *prog;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if ((target == GL_FRAGMENT_PROGRAM_NV
        && ctx->Extensions.NV_fragment_program) ||
       (target == GL_FRAGMENT_PROGRAM_ARB
        && ctx->Extensions.ARB_fragment_program)) {
      if (index >= ctx->Const.MaxFragmentProgramLocalParams) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramLocalParameterARB");
         return;
      }
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB
            && ctx->Extensions.ARB_vertex_program) {
      if (index >= ctx->Const.MaxVertexProgramLocalParams) {
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
   const struct program *prog;
   GLuint maxParams;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
      maxParams = ctx->Const.MaxVertexProgramLocalParams;
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
      maxParams = ctx->Const.MaxFragmentProgramLocalParams;
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
   _mesa_GetProgramLocalParameterfvARB(target, index, floatParams);
   if (ctx->ErrorValue == GL_NO_ERROR) {
      COPY_4V(params, floatParams);
   }
}


void GLAPIENTRY
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_ARB
       && ctx->Extensions.ARB_vertex_program) {
      prog = &(ctx->VertexProgram.Current->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
            && ctx->Extensions.ARB_fragment_program) {
      prog = &(ctx->FragmentProgram.Current->Base);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
      return;
   }

   ASSERT(prog);

   switch (pname) {
      case GL_PROGRAM_LENGTH_ARB:
         *params = prog->String ? (GLint)_mesa_strlen((char *) prog->String) : 0;
         break;
      case GL_PROGRAM_FORMAT_ARB:
         *params = prog->Format;
         break;
      case GL_PROGRAM_BINDING_ARB:
         *params = prog->Id;
         break;
      case GL_PROGRAM_INSTRUCTIONS_ARB:
         *params = prog->NumInstructions;
         break;
      case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramInstructions;
         else
            *params = ctx->Const.MaxFragmentProgramInstructions;
         break;
      case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         *params = prog->NumInstructions;
         break;
      case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramInstructions;
         else
            *params = ctx->Const.MaxFragmentProgramInstructions;
         break;
      case GL_PROGRAM_TEMPORARIES_ARB:
         *params = prog->NumTemporaries;
         break;
      case GL_MAX_PROGRAM_TEMPORARIES_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramTemps;
         else
            *params = ctx->Const.MaxFragmentProgramTemps;
         break;
      case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
         /* XXX same as GL_PROGRAM_TEMPORARIES_ARB? */
         *params = prog->NumTemporaries;
         break;
      case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
         /* XXX same as GL_MAX_PROGRAM_TEMPORARIES_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramTemps;
         else
            *params = ctx->Const.MaxFragmentProgramTemps;
         break;
      case GL_PROGRAM_PARAMETERS_ARB:
         *params = prog->NumParameters;
         break;
      case GL_MAX_PROGRAM_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
         *params = prog->NumParameters;
         break;
      case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_PROGRAM_ATTRIBS_ARB:
         *params = prog->NumAttributes;
         break;
      case GL_MAX_PROGRAM_ATTRIBS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAttribs;
         else
            *params = ctx->Const.MaxFragmentProgramAttribs;
         break;
      case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
         /* XXX same as GL_PROGRAM_ATTRIBS_ARB? */
         *params = prog->NumAttributes;
         break;
      case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
         /* XXX same as GL_MAX_PROGRAM_ATTRIBS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAttribs;
         else
            *params = ctx->Const.MaxFragmentProgramAttribs;
         break;
      case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
         *params = prog->NumAddressRegs;
         break;
      case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAddressRegs;
         else
            *params = ctx->Const.MaxFragmentProgramAddressRegs;
         break;
      case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         /* XXX same as GL_PROGRAM_ADDRESS_REGISTERS_ARB? */
         *params = prog->NumAddressRegs;
         break;
      case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
         /* XXX same as GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB? */
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramAddressRegs;
         else
            *params = ctx->Const.MaxFragmentProgramAddressRegs;
         break;
      case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramLocalParams;
         else
            *params = ctx->Const.MaxFragmentProgramLocalParams;
         break;
      case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
         if (target == GL_VERTEX_PROGRAM_ARB)
            *params = ctx->Const.MaxVertexProgramEnvParams;
         else
            *params = ctx->Const.MaxFragmentProgramEnvParams;
         break;
      case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
	 if (ctx->Driver.IsProgramNative) 
	    *params = ctx->Driver.IsProgramNative( ctx, target, prog );
	 else
	    *params = GL_TRUE;
         break;

      /*
       * The following apply to fragment programs only.
       */
      case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
      case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumAluInstructions;
         break;
      case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
      case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumTexInstructions;
         break;
      case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
      case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->FragmentProgram.Current->NumTexIndirections;
         break;
      case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramAluInstructions;
         break;
      case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramTexInstructions;
         break;
      case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
      case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
         if (target != GL_FRAGMENT_PROGRAM_ARB) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(target)");
            return;
         }
         *params = ctx->Const.MaxFragmentProgramTexIndirections;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string)
{
   struct program *prog;
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

   MEMCPY(string, prog->String, _mesa_strlen((char *) prog->String));
}

