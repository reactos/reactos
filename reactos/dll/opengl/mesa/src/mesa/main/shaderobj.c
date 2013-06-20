/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
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
 * \file shaderobj.c
 * \author Brian Paul
 *
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/hash.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/hash_table.h"
#include "ralloc.h"

/**********************************************************************/
/*** Shader object functions                                        ***/
/**********************************************************************/


/**
 * Set ptr to point to sh.
 * If ptr is pointing to another shader, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to sh, incrementing its refcount.
 */
void
_mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh)
{
   assert(ptr);
   if (*ptr == sh) {
      /* no-op */
      return;
   }
   if (*ptr) {
      /* Unreference the old shader */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_shader *old = *ptr;

      ASSERT(old->RefCount > 0);
      old->RefCount--;
      /*printf("SHADER DECR %p (%d) to %d\n",
        (void*) old, old->Name, old->RefCount);*/
      deleteFlag = (old->RefCount == 0);

      if (deleteFlag) {
	 if (old->Name != 0)
	    _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         ctx->Driver.DeleteShader(ctx, old);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (sh) {
      /* reference new */
      sh->RefCount++;
      /*printf("SHADER INCR %p (%d) to %d\n",
        (void*) sh, sh->Name, sh->RefCount);*/
      *ptr = sh;
   }
}

void
_mesa_init_shader(struct gl_context *ctx, struct gl_shader *shader)
{
   shader->RefCount = 1;
}

/**
 * Allocate a new gl_shader object, initialize it.
 * Called via ctx->Driver.NewShader()
 */
struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER ||
          type == GL_GEOMETRY_SHADER_ARB);
   shader = rzalloc(NULL, struct gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      _mesa_init_shader(ctx, shader);
   }
   return shader;
}


/**
 * Delete a shader object.
 * Called via ctx->Driver.DeleteShader().
 */
static void
_mesa_delete_shader(struct gl_context *ctx, struct gl_shader *sh)
{
   if (sh->Source)
      free((void *) sh->Source);
   _mesa_reference_program(ctx, &sh->Program, NULL);
   ralloc_free(sh);
}


/**
 * Lookup a GLSL shader object.
 */
struct gl_shader *
_mesa_lookup_shader(struct gl_context *ctx, GLuint name)
{
   if (name) {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (sh && sh->Type == GL_SHADER_PROGRAM_MESA) {
         return NULL;
      }
      return sh;
   }
   return NULL;
}


/**
 * As above, but record an error if shader is not found.
 */
struct gl_shader *
_mesa_lookup_shader_err(struct gl_context *ctx, GLuint name, const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
      return NULL;
   }
   else {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!sh) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
         return NULL;
      }
      if (sh->Type == GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
         return NULL;
      }
      return sh;
   }
}



/**********************************************************************/
/*** Shader Program object functions                                ***/
/**********************************************************************/


/**
 * Set ptr to point to shProg.
 * If ptr is pointing to another object, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to shProg, incrementing its refcount.
 */
void
_mesa_reference_shader_program(struct gl_context *ctx,
                               struct gl_shader_program **ptr,
                               struct gl_shader_program *shProg)
{
   assert(ptr);
   if (*ptr == shProg) {
      /* no-op */
      return;
   }
   if (*ptr) {
      /* Unreference the old shader program */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_shader_program *old = *ptr;

      ASSERT(old->RefCount > 0);
      old->RefCount--;
#if 0
      printf("ShaderProgram %p ID=%u  RefCount-- to %d\n",
             (void *) old, old->Name, old->RefCount);
#endif
      deleteFlag = (old->RefCount == 0);

      if (deleteFlag) {
	 if (old->Name != 0)
	    _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         ctx->Driver.DeleteShaderProgram(ctx, old);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (shProg) {
      shProg->RefCount++;
#if 0
      printf("ShaderProgram %p ID=%u  RefCount++ to %d\n",
             (void *) shProg, shProg->Name, shProg->RefCount);
#endif
      *ptr = shProg;
   }
}

void
_mesa_init_shader_program(struct gl_context *ctx, struct gl_shader_program *prog)
{
   prog->Type = GL_SHADER_PROGRAM_MESA;
   prog->RefCount = 1;

   prog->AttributeBindings = string_to_uint_map_ctor();
   prog->FragDataBindings = string_to_uint_map_ctor();

#if FEATURE_ARB_geometry_shader4
   prog->Geom.VerticesOut = 0;
   prog->Geom.InputType = GL_TRIANGLES;
   prog->Geom.OutputType = GL_TRIANGLE_STRIP;
#endif

   prog->InfoLog = ralloc_strdup(prog, "");
}

/**
 * Allocate a new gl_shader_program object, initialize it.
 * Called via ctx->Driver.NewShaderProgram()
 */
static struct gl_shader_program *
_mesa_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   shProg = rzalloc(NULL, struct gl_shader_program);
   if (shProg) {
      shProg->Name = name;
      _mesa_init_shader_program(ctx, shProg);
   }
   return shProg;
}


/**
 * Clear (free) the shader program state that gets produced by linking.
 */
void
_mesa_clear_shader_program_data(struct gl_context *ctx,
                                struct gl_shader_program *shProg)
{
   if (shProg->UniformStorage) {
      unsigned i;
      for (i = 0; i < shProg->NumUserUniformStorage; ++i)
         _mesa_uniform_detach_all_driver_storage(&shProg->UniformStorage[i]);
      ralloc_free(shProg->UniformStorage);
      shProg->NumUserUniformStorage = 0;
      shProg->UniformStorage = NULL;
   }

   if (shProg->UniformHash) {
      string_to_uint_map_dtor(shProg->UniformHash);
      shProg->UniformHash = NULL;
   }

   assert(shProg->InfoLog != NULL);
   ralloc_free(shProg->InfoLog);
   shProg->InfoLog = ralloc_strdup(shProg, "");
}


/**
 * Free all the data that hangs off a shader program object, but not the
 * object itself.
 */
void
_mesa_free_shader_program_data(struct gl_context *ctx,
                               struct gl_shader_program *shProg)
{
   GLuint i;
   gl_shader_type sh;

   assert(shProg->Type == GL_SHADER_PROGRAM_MESA);

   _mesa_clear_shader_program_data(ctx, shProg);

   if (shProg->AttributeBindings) {
      string_to_uint_map_dtor(shProg->AttributeBindings);
      shProg->AttributeBindings = NULL;
   }

   if (shProg->FragDataBindings) {
      string_to_uint_map_dtor(shProg->FragDataBindings);
      shProg->FragDataBindings = NULL;
   }

   /* detach shaders */
   for (i = 0; i < shProg->NumShaders; i++) {
      _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);
   }
   shProg->NumShaders = 0;

   if (shProg->Shaders) {
      free(shProg->Shaders);
      shProg->Shaders = NULL;
   }

   /* Transform feedback varying vars */
   for (i = 0; i < shProg->TransformFeedback.NumVarying; i++) {
      free(shProg->TransformFeedback.VaryingNames[i]);
   }
   free(shProg->TransformFeedback.VaryingNames);
   shProg->TransformFeedback.VaryingNames = NULL;
   shProg->TransformFeedback.NumVarying = 0;


   for (sh = 0; sh < MESA_SHADER_TYPES; sh++) {
      if (shProg->_LinkedShaders[sh] != NULL) {
	 ctx->Driver.DeleteShader(ctx, shProg->_LinkedShaders[sh]);
	 shProg->_LinkedShaders[sh] = NULL;
      }
   }
}


/**
 * Free/delete a shader program object.
 * Called via ctx->Driver.DeleteShaderProgram().
 */
static void
_mesa_delete_shader_program(struct gl_context *ctx, struct gl_shader_program *shProg)
{
   _mesa_free_shader_program_data(ctx, shProg);

   ralloc_free(shProg);
}


/**
 * Lookup a GLSL program object.
 */
struct gl_shader_program *
_mesa_lookup_shader_program(struct gl_context *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   if (name) {
      shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (shProg && shProg->Type != GL_SHADER_PROGRAM_MESA) {
         return NULL;
      }
      return shProg;
   }
   return NULL;
}


/**
 * As above, but record an error if program is not found.
 */
struct gl_shader_program *
_mesa_lookup_shader_program_err(struct gl_context *ctx, GLuint name,
                                const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
      return NULL;
   }
   else {
      struct gl_shader_program *shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!shProg) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
         return NULL;
      }
      if (shProg->Type != GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
         return NULL;
      }
      return shProg;
   }
}


void
_mesa_init_shader_object_functions(struct dd_function_table *driver)
{
   driver->NewShader = _mesa_new_shader;
   driver->DeleteShader = _mesa_delete_shader;
   driver->NewShaderProgram = _mesa_new_shader_program;
   driver->DeleteShaderProgram = _mesa_delete_shader_program;
   driver->LinkShader = _mesa_ir_link_shader;
}
