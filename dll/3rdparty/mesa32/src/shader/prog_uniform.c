/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file prog_uniform.c
 * Shader uniform functions.
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "prog_uniform.h"


struct gl_uniform_list *
_mesa_new_uniform_list(void)
{
   return CALLOC_STRUCT(gl_uniform_list);
}


void
_mesa_free_uniform_list(struct gl_uniform_list *list)
{
   GLuint i;
   for (i = 0; i < list->NumUniforms; i++) {
      _mesa_free((void *) list->Uniforms[i].Name);
   }
   _mesa_free(list->Uniforms);
   _mesa_free(list);
}


struct gl_uniform *
_mesa_append_uniform(struct gl_uniform_list *list,
                     const char *name, GLenum target, GLuint progPos)
{
   const GLuint oldNum = list->NumUniforms;
   struct gl_uniform *uniform;
   GLint index;

   assert(target == GL_VERTEX_PROGRAM_ARB ||
          target == GL_FRAGMENT_PROGRAM_ARB);

   index = _mesa_lookup_uniform(list, name);
   if (index < 0) {
      /* not found - append to list */

      if (oldNum + 1 > list->Size) {
         /* Need to grow the list array (alloc some extra) */
         list->Size += 4;

         /* realloc arrays */
         list->Uniforms = (struct gl_uniform *)
            _mesa_realloc(list->Uniforms,
                          oldNum * sizeof(struct gl_uniform),
                          list->Size * sizeof(struct gl_uniform));
      }

      if (!list->Uniforms) {
         /* out of memory */
         list->NumUniforms = 0;
         list->Size = 0;
         return GL_FALSE;
      }

      uniform = list->Uniforms + oldNum;

      uniform->Name = _mesa_strdup(name);
      uniform->VertPos = -1;
      uniform->FragPos = -1;
      uniform->Initialized = GL_FALSE;

      list->NumUniforms++;
   }
   else {
      /* found */
      uniform = list->Uniforms + index;
   }

   /* update position for the vertex or fragment program */
   if (target == GL_VERTEX_PROGRAM_ARB) {
      if (uniform->VertPos != -1) {
         /* this uniform is already in the list - that shouldn't happen */
         return GL_FALSE;
      }
      uniform->VertPos = progPos;
   }
   else {
      if (uniform->FragPos != -1) {
         /* this uniform is already in the list - that shouldn't happen */
         return GL_FALSE;
      }
      uniform->FragPos = progPos;
   }

   return uniform;
}


/**
 * Return the location/index of the named uniform in the uniform list,
 * or -1 if not found.
 */
GLint
_mesa_lookup_uniform(const struct gl_uniform_list *list, const char *name)
{
   GLuint i;
   for (i = 0; list && i < list->NumUniforms; i++) {
      if (!_mesa_strcmp(list->Uniforms[i].Name, name)) {
         return i;
      }
   }
   return -1;
}


GLint
_mesa_longest_uniform_name(const struct gl_uniform_list *list)
{
   GLint max = 0;
   GLuint i;
   for (i = 0; list && i < list->NumUniforms; i++) {
      GLuint len = _mesa_strlen(list->Uniforms[i].Name);
      if (len > (GLuint)max)
         max = len;
   }
   return max;
}


void
_mesa_print_uniforms(const struct gl_uniform_list *list)
{
   GLuint i;
   printf("Uniform list %p:\n", (void *) list);
   for (i = 0; i < list->NumUniforms; i++) {
      printf("%d: %s %d %d\n",
             i,
             list->Uniforms[i].Name,
             list->Uniforms[i].VertPos,
             list->Uniforms[i].FragPos);
   }
}
