/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file shader_query.cpp
 * C-to-C++ bridge functions to query GLSL shader data
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "main/core.h"
#include "glsl_symbol_table.h"
#include "ir.h"
#include "shaderobj.h"
#include "program/hash_table.h"
#include "../glsl/program.h"

extern "C" {
#include "shaderapi.h"
}

void GLAPIENTRY
_mesa_BindAttribLocationARB(GLhandleARB program, GLuint index,
                            const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glBindAttribLocation");
   if (!shProg)
      return;

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindAttribLocation(illegal name)");
      return;
   }

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(index)");
      return;
   }

   /* Replace the current value if it's already in the list.  Add
    * VERT_ATTRIB_GENERIC0 because that's how the linker differentiates
    * between built-in attributes and user-defined attributes.
    */
   shProg->AttributeBindings->put(index + VERT_ATTRIB_GENERIC0, name);

   /*
    * Note that this attribute binding won't go into effect until
    * glLinkProgram is called again.
    */
}

void GLAPIENTRY
_mesa_GetActiveAttribARB(GLhandleARB program, GLuint desired_index,
                         GLsizei maxLength, GLsizei * length, GLint * size,
                         GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
      return;

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetActiveAttrib(program not linked)");
      return;
   }

   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(no vertex shader)");
      return;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   unsigned current_index = 0;

   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if (var == NULL
	  || var->mode != ir_var_in
	  || var->location == -1)
	 continue;

      if (current_index == desired_index) {
	 _mesa_copy_string(name, maxLength, length, var->name);

	 if (size)
	    *size = (var->type->is_array()) ? var->type->length : 1;

	 if (type)
	    *type = var->type->gl_type;

	 return;
      }

      current_index++;
   }

   /* If the loop did not return early, the caller must have asked for
    * an index that did not exit.  Set an error.
    */
   _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB(GLhandleARB program, const GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetAttribLocation");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetAttribLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   /* Not having a vertex shader is not an error.
    */
   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL)
      return -1;

   exec_list *ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      /* The extra check against VERT_ATTRIB_GENERIC0 is because
       * glGetAttribLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
	  || var->mode != ir_var_in
	  || var->location == -1
	  || var->location < VERT_ATTRIB_GENERIC0)
	 continue;

      if (strcmp(var->name, name) == 0)
	 return var->location - VERT_ATTRIB_GENERIC0;
   }

   return -1;
}


unsigned
_mesa_count_active_attribs(struct gl_shader_program *shProg)
{
   if (!shProg->LinkStatus
       || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      return 0;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   unsigned i = 0;

   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if (var == NULL
	  || var->mode != ir_var_in
	  || var->location == -1)
	 continue;

      i++;
   }

   return i;
}


size_t
_mesa_longest_attribute_name_length(struct gl_shader_program *shProg)
{
   if (!shProg->LinkStatus
       || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      return 0;
   }

   exec_list *const ir = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->ir;
   size_t longest = 0;

   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if (var == NULL
	  || var->mode != ir_var_in
	  || var->location == -1)
	 continue;

      const size_t len = strlen(var->name);
      if (len >= longest)
	 longest = len + 1;
   }

   return longest;
}

void GLAPIENTRY
_mesa_BindFragDataLocation(GLuint program, GLuint colorNumber,
			   const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glBindFragDataLocation");
   if (!shProg)
      return;

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindFragDataLocation(illegal name)");
      return;
   }

   if (colorNumber >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocation(index)");
      return;
   }

   /* Replace the current value if it's already in the list.  Add
    * FRAG_RESULT_DATA0 because that's how the linker differentiates
    * between built-in attributes and user-defined attributes.
    */
   shProg->FragDataBindings->put(colorNumber + FRAG_RESULT_DATA0, name);

   /*
    * Note that this binding won't go into effect until
    * glLinkProgram is called again.
    */
}

GLint GLAPIENTRY
_mesa_GetFragDataLocation(GLuint program, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetFragDataLocation");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFragDataLocation(illegal name)");
      return -1;
   }

   /* Not having a fragment shader is not an error.
    */
   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL)
      return -1;

   exec_list *ir = shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->ir;
   foreach_list(node, ir) {
      const ir_variable *const var = ((ir_instruction *) node)->as_variable();

      /* The extra check against FRAG_RESULT_DATA0 is because
       * glGetFragDataLocation cannot be used on "conventional" attributes.
       *
       * From page 95 of the OpenGL 3.0 spec:
       *
       *     "If name is not an active attribute, if name is a conventional
       *     attribute, or if an error occurs, -1 will be returned."
       */
      if (var == NULL
	  || var->mode != ir_var_out
	  || var->location == -1
	  || var->location < FRAG_RESULT_DATA0)
	 continue;

      if (strcmp(var->name, name) == 0)
	 return var->location - FRAG_RESULT_DATA0;
   }

   return -1;
}
