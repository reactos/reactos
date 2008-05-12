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


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "nvfragparse.h"
#include "nvvertparse.h"
#include "program.h"
#include "prog_debug.h"
#include "prog_parameter.h"
#include "prog_instruction.h"



/**
 * Functions for the experimental GL_MESA_program_debug extension.
 */


/* XXX temporary */
GLAPI void GLAPIENTRY
glProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                      GLvoid *data)
{
   _mesa_ProgramCallbackMESA(target, callback, data);
}


void
_mesa_ProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                          GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (target) {
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->VertexProgram.Callback = callback;
         ctx->VertexProgram.CallbackData = data;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
         return;
   }
}


/* XXX temporary */
GLAPI void GLAPIENTRY
glGetProgramRegisterfvMESA(GLenum target,
                           GLsizei len, const GLubyte *registerName,
                           GLfloat *v)
{
   _mesa_GetProgramRegisterfvMESA(target, len, registerName, v);
}


void
_mesa_GetProgramRegisterfvMESA(GLenum target,
                               GLsizei len, const GLubyte *registerName,
                               GLfloat *v)
{
   char reg[1000];
   GET_CURRENT_CONTEXT(ctx);

   /* We _should_ be inside glBegin/glEnd */
#if 0
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramRegisterfvMESA");
      return;
   }
#endif

   /* make null-terminated copy of registerName */
   len = MIN2((unsigned int) len, sizeof(reg) - 1);
   _mesa_memcpy(reg, registerName, len);
   reg[len] = 0;

   switch (target) {
      case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->VertexProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* GL_NV_vertex_program */
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.VertexProgram.MaxTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
            ctx->Driver.GetProgramRegister(ctx, PROGRAM_TEMPORARY, i, v);
         }
         else if (reg[0] == 'v' && reg[1] == '[') {
            /* Vertex Input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.VertexProgram.MaxAttribs; i++) {
               const char *name = _mesa_nv_vertex_input_register_name(i);
               char number[10];
               _mesa_sprintf(number, "%d", i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0 ||
                   _mesa_strncmp(reg + 2, number, _mesa_strlen(number)) == 0) {
                  ctx->Driver.GetProgramRegister(ctx, PROGRAM_INPUT, i, v);
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (reg[0] == 'o' && reg[1] == '[') {
            /* Vertex output attribute */
         }
         /* GL_ARB_vertex_program */
         else if (_mesa_strncmp(reg, "vertex.", 7) == 0) {

         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         break;
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* XXX to do */
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.FragmentProgram.MaxTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
            ctx->Driver.GetProgramRegister(ctx, PROGRAM_TEMPORARY,
                                                   i, v);
         }
         else if (reg[0] == 'f' && reg[1] == '[') {
            /* Fragment input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.FragmentProgram.MaxAttribs; i++) {
               const char *name = _mesa_nv_fragment_input_register_name(i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0) {
                  ctx->Driver.GetProgramRegister(ctx, PROGRAM_INPUT, i, v);
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (_mesa_strcmp(reg, "o[COLR]") == 0) {
            /* Fragment output color */
            ctx->Driver.GetProgramRegister(ctx, PROGRAM_OUTPUT,
                                           FRAG_RESULT_COLR, v);
         }
         else if (_mesa_strcmp(reg, "o[COLH]") == 0) {
            /* Fragment output color */
            ctx->Driver.GetProgramRegister(ctx, PROGRAM_OUTPUT,
                                           FRAG_RESULT_COLH, v);
         }
         else if (_mesa_strcmp(reg, "o[DEPR]") == 0) {
            /* Fragment output depth */
            ctx->Driver.GetProgramRegister(ctx, PROGRAM_OUTPUT,
                                           FRAG_RESULT_DEPR, v);
         }
         else {
            /* try user-defined identifiers */
            const GLfloat *value = _mesa_lookup_parameter_value(
                       ctx->FragmentProgram.Current->Base.Parameters, -1, reg);
            if (value) {
               COPY_4V(v, value);
            }
            else {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetProgramRegisterfvMESA(target)");
         return;
   }
}
