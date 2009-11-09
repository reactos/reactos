/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file shader_api.c
 * Implementation of GLSL-related API functions
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 * 2. Insert FLUSH_VERTICES calls in various places
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/hash.h"
#include "main/macros.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "shader/prog_uniform.h"
#include "shader/shader_api.h"
#include "shader/slang/slang_compile.h"
#include "shader/slang/slang_link.h"
#include "glapi/dispatch.h"


#ifndef GL_PROGRAM_BINARY_LENGTH_OES
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#endif


/**
 * Allocate a new gl_shader_program object, initialize it.
 */
static struct gl_shader_program *
_mesa_new_shader_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   shProg = CALLOC_STRUCT(gl_shader_program);
   if (shProg) {
      shProg->Type = GL_SHADER_PROGRAM_MESA;
      shProg->Name = name;
      shProg->RefCount = 1;
      shProg->Attributes = _mesa_new_parameter_list();
   }
   return shProg;
}


/**
 * Clear (free) the shader program state that gets produced by linking.
 */
void
_mesa_clear_shader_program_data(GLcontext *ctx,
                                struct gl_shader_program *shProg)
{
   _mesa_reference_vertprog(ctx, &shProg->VertexProgram, NULL);
   _mesa_reference_fragprog(ctx, &shProg->FragmentProgram, NULL);

   if (shProg->Uniforms) {
      _mesa_free_uniform_list(shProg->Uniforms);
      shProg->Uniforms = NULL;
   }

   if (shProg->Varying) {
      _mesa_free_parameter_list(shProg->Varying);
      shProg->Varying = NULL;
   }
}


/**
 * Free all the data that hangs off a shader program object, but not the
 * object itself.
 */
void
_mesa_free_shader_program_data(GLcontext *ctx,
                               struct gl_shader_program *shProg)
{
   GLuint i;

   assert(shProg->Type == GL_SHADER_PROGRAM_MESA);

   _mesa_clear_shader_program_data(ctx, shProg);

   if (shProg->Attributes) {
      _mesa_free_parameter_list(shProg->Attributes);
      shProg->Attributes = NULL;
   }

   /* detach shaders */
   for (i = 0; i < shProg->NumShaders; i++) {
      _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);
   }
   shProg->NumShaders = 0;

   if (shProg->Shaders) {
      _mesa_free(shProg->Shaders);
      shProg->Shaders = NULL;
   }

   if (shProg->InfoLog) {
      _mesa_free(shProg->InfoLog);
      shProg->InfoLog = NULL;
   }
}


/**
 * Free/delete a shader program object.
 */
void
_mesa_free_shader_program(GLcontext *ctx, struct gl_shader_program *shProg)
{
   _mesa_free_shader_program_data(ctx, shProg);

   _mesa_free(shProg);
}


/**
 * Set ptr to point to shProg.
 * If ptr is pointing to another object, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to shProg, incrementing its refcount.
 */
/* XXX this could be static */
void
_mesa_reference_shader_program(GLcontext *ctx,
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
         _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         _mesa_free_shader_program(ctx, old);
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


/**
 * Lookup a GLSL program object.
 */
struct gl_shader_program *
_mesa_lookup_shader_program(GLcontext *ctx, GLuint name)
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
static struct gl_shader_program *
_mesa_lookup_shader_program_err(GLcontext *ctx, GLuint name,
                                const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, caller);
      return NULL;
   }
   else {
      struct gl_shader_program *shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!shProg) {
         _mesa_error(ctx, GL_INVALID_VALUE, caller);
         return NULL;
      }
      if (shProg->Type != GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
         return NULL;
      }
      return shProg;
   }
}




/**
 * Allocate a new gl_shader object, initialize it.
 */
struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = CALLOC_STRUCT(gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}


void
_mesa_free_shader(GLcontext *ctx, struct gl_shader *sh)
{
   if (sh->Source)
      _mesa_free((void *) sh->Source);
   if (sh->InfoLog)
      _mesa_free(sh->InfoLog);
   _mesa_reference_program(ctx, &sh->Program, NULL);
   _mesa_free(sh);
}


/**
 * Set ptr to point to sh.
 * If ptr is pointing to another shader, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to sh, incrementing its refcount.
 */
/* XXX this could be static */
void
_mesa_reference_shader(GLcontext *ctx, struct gl_shader **ptr,
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
         _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         _mesa_free_shader(ctx, old);
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


/**
 * Lookup a GLSL shader object.
 */
struct gl_shader *
_mesa_lookup_shader(GLcontext *ctx, GLuint name)
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
static struct gl_shader *
_mesa_lookup_shader_err(GLcontext *ctx, GLuint name, const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, caller);
      return NULL;
   }
   else {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!sh) {
         _mesa_error(ctx, GL_INVALID_VALUE, caller);
         return NULL;
      }
      if (sh->Type == GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
         return NULL;
      }
      return sh;
   }
}



/**
 * Initialize context's shader state.
 */
void
_mesa_init_shader_state(GLcontext * ctx)
{
   /* Device drivers may override these to control what kind of instructions
    * are generated by the GLSL compiler.
    */
   ctx->Shader.EmitHighLevelInstructions = GL_TRUE;
   ctx->Shader.EmitCondCodes = GL_TRUE; /* XXX probably want GL_FALSE... */
   ctx->Shader.EmitComments = GL_FALSE;
}


/**
 * Free the per-context shader-related state.
 */
void
_mesa_free_shader_state(GLcontext *ctx)
{
   _mesa_reference_shader_program(ctx, &ctx->Shader.CurrentProgram, NULL);
}


/**
 * Copy string from <src> to <dst>, up to maxLength characters, returning
 * length of <dst> in <length>.
 * \param src  the strings source
 * \param maxLength  max chars to copy
 * \param length  returns number of chars copied
 * \param dst  the string destination
 */
static void
copy_string(GLchar *dst, GLsizei maxLength, GLsizei *length, const GLchar *src)
{
   GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
      dst[len] = src[len];
   if (maxLength > 0)
      dst[len] = 0;
   if (length)
      *length = len;
}


static GLboolean
_mesa_is_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, name);
   return shProg ? GL_TRUE : GL_FALSE;
}


static GLboolean
_mesa_is_shader(GLcontext *ctx, GLuint name)
{
   struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
   return shader ? GL_TRUE : GL_FALSE;
}


/**
 * Called via ctx->Driver.AttachShader()
 */
static void
_mesa_attach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg;
   struct gl_shader *sh;
   GLuint i, n;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glAttachShader");
   if (!shProg)
      return;

   sh = _mesa_lookup_shader_err(ctx, shader, "glAttachShader");
   if (!sh) {
      return;
   }

   n = shProg->NumShaders;
   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i] == sh) {
         /* The shader is already attched to this program.  The
          * GL_ARB_shader_objects spec says:
          *
          *     "The error INVALID_OPERATION is generated by AttachObjectARB
          *     if <obj> is already attached to <containerObj>."
          */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glAttachShader");
         return;
      }
   }

   /* grow list */
   shProg->Shaders = (struct gl_shader **)
      _mesa_realloc(shProg->Shaders,
                    n * sizeof(struct gl_shader *),
                    (n + 1) * sizeof(struct gl_shader *));
   if (!shProg->Shaders) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAttachShader");
      return;
   }

   /* append */
   shProg->Shaders[n] = NULL; /* since realloc() didn't zero the new space */
   _mesa_reference_shader(ctx, &shProg->Shaders[n], sh);
   shProg->NumShaders++;
}


static GLint
_mesa_get_attrib_location(GLcontext *ctx, GLuint program,
                          const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program_err(ctx, program, "glGetAttribLocation");

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

   if (shProg->VertexProgram) {
      const struct gl_program_parameter_list *attribs =
         shProg->VertexProgram->Base.Attributes;
      if (attribs) {
         GLint i = _mesa_lookup_parameter_index(attribs, -1, name);
         if (i >= 0) {
            return attribs->Parameters[i].StateIndexes[0];
         }
      }
   }
   return -1;
}


static void
_mesa_bind_attrib_location(GLcontext *ctx, GLuint program, GLuint index,
                           const GLchar *name)
{
   struct gl_shader_program *shProg;
   const GLint size = -1; /* unknown size */
   GLint i, oldIndex;
   GLenum datatype = GL_FLOAT_VEC4;

   shProg = _mesa_lookup_shader_program_err(ctx, program,
                                            "glBindAttribLocation");
   if (!shProg) {
      return;
   }

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

   if (shProg->LinkStatus) {
      /* get current index/location for the attribute */
      oldIndex = _mesa_get_attrib_location(ctx, program, name);
   }
   else {
      oldIndex = -1;
   }

   /* this will replace the current value if it's already in the list */
   i = _mesa_add_attribute(shProg->Attributes, name, size, datatype, index);
   if (i < 0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindAttribLocation");
      return;
   }

   /*
    * Note that this attribute binding won't go into effect until
    * glLinkProgram is called again.
    */
}


static GLuint
_mesa_create_shader(GLcontext *ctx, GLenum type)
{
   struct gl_shader *sh;
   GLuint name;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);

   switch (type) {
   case GL_FRAGMENT_SHADER:
   case GL_VERTEX_SHADER:
      sh = _mesa_new_shader(ctx, name, type);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "CreateShader(type)");
      return 0;
   }

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, sh);

   return name;
}


static GLuint 
_mesa_create_program(GLcontext *ctx)
{
   GLuint name;
   struct gl_shader_program *shProg;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
   shProg = _mesa_new_shader_program(ctx, name);

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, shProg);

   assert(shProg->RefCount == 1);

   return name;
}


/**
 * Named w/ "2" to indicate OpenGL 2.x vs GL_ARB_fragment_programs's
 * DeleteProgramARB.
 */
static void
_mesa_delete_program2(GLcontext *ctx, GLuint name)
{
   /*
    * NOTE: deleting shaders/programs works a bit differently than
    * texture objects (and buffer objects, etc).  Shader/program
    * handles/IDs exist in the hash table until the object is really
    * deleted (refcount==0).  With texture objects, the handle/ID is
    * removed from the hash table in glDeleteTextures() while the tex
    * object itself might linger until its refcount goes to zero.
    */
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, name, "glDeleteProgram");
   if (!shProg)
      return;

   shProg->DeletePending = GL_TRUE;

   /* effectively, decr shProg's refcount */
   _mesa_reference_shader_program(ctx, &shProg, NULL);
}


static void
_mesa_delete_shader(GLcontext *ctx, GLuint shader)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shader, "glDeleteShader");
   if (!sh)
      return;

   sh->DeletePending = GL_TRUE;

   /* effectively, decr sh's refcount */
   _mesa_reference_shader(ctx, &sh, NULL);
}


static void
_mesa_detach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg;
   GLuint n;
   GLuint i, j;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glDetachShader");
   if (!shProg)
      return;

   n = shProg->NumShaders;

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i]->Name == shader) {
         /* found it */
         struct gl_shader **newList;

         /* release */
         _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);

         /* alloc new, smaller array */
         newList = (struct gl_shader **)
            _mesa_malloc((n - 1) * sizeof(struct gl_shader *));
         if (!newList) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachShader");
            return;
         }
         for (j = 0; j < i; j++) {
            newList[j] = shProg->Shaders[j];
         }
         while (++i < n)
            newList[j++] = shProg->Shaders[i];
         _mesa_free(shProg->Shaders);

         shProg->Shaders = newList;
         shProg->NumShaders = n - 1;

#ifdef DEBUG
         /* sanity check */
         {
            for (j = 0; j < shProg->NumShaders; j++) {
               assert(shProg->Shaders[j]->Type == GL_VERTEX_SHADER ||
                      shProg->Shaders[j]->Type == GL_FRAGMENT_SHADER);
               assert(shProg->Shaders[j]->RefCount > 0);
            }
         }
#endif

         return;
      }
   }

   /* not found */
   {
      GLenum err;
      if (_mesa_is_shader(ctx, shader))
         err = GL_INVALID_OPERATION;
      else if (_mesa_is_program(ctx, shader))
         err = GL_INVALID_OPERATION;
      else
         err = GL_INVALID_VALUE;
      _mesa_error(ctx, err, "glDetachProgram(shader)");
      return;
   }
}


static GLint
sizeof_glsl_type(GLenum type)
{
   switch (type) {
   case GL_FLOAT:
   case GL_INT:
   case GL_BOOL:
   case GL_SAMPLER_1D:
   case GL_SAMPLER_2D:
   case GL_SAMPLER_3D:
   case GL_SAMPLER_CUBE:
   case GL_SAMPLER_1D_SHADOW:
   case GL_SAMPLER_2D_SHADOW:
   case GL_SAMPLER_2D_RECT_ARB:
   case GL_SAMPLER_2D_RECT_SHADOW_ARB:
   case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_CUBE_SHADOW_EXT:
      return 1;
   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
   case GL_BOOL_VEC2:
      return 2;
   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
   case GL_BOOL_VEC3:
      return 3;
   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
   case GL_BOOL_VEC4:
      return 4;
   case GL_FLOAT_MAT2:
   case GL_FLOAT_MAT2x3:
   case GL_FLOAT_MAT2x4:
      return 8; /* two float[4] vectors */
   case GL_FLOAT_MAT3:
   case GL_FLOAT_MAT3x2:
   case GL_FLOAT_MAT3x4:
      return 12; /* three float[4] vectors */
   case GL_FLOAT_MAT4:
   case GL_FLOAT_MAT4x2:
   case GL_FLOAT_MAT4x3:
      return 16;  /* four float[4] vectors */
   default:
      _mesa_problem(NULL, "Invalid type in sizeof_glsl_type()");
      return 1;
   }
}


static GLboolean
is_boolean_type(GLenum type)
{
   switch (type) {
   case GL_BOOL:
   case GL_BOOL_VEC2:
   case GL_BOOL_VEC3:
   case GL_BOOL_VEC4:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static GLboolean
is_integer_type(GLenum type)
{
   switch (type) {
   case GL_INT:
   case GL_INT_VEC2:
   case GL_INT_VEC3:
   case GL_INT_VEC4:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static GLboolean
is_sampler_type(GLenum type)
{
   switch (type) {
   case GL_SAMPLER_1D:
   case GL_SAMPLER_2D:
   case GL_SAMPLER_3D:
   case GL_SAMPLER_CUBE:
   case GL_SAMPLER_1D_SHADOW:
   case GL_SAMPLER_2D_SHADOW:
   case GL_SAMPLER_2D_RECT_ARB:
   case GL_SAMPLER_2D_RECT_SHADOW_ARB:
   case GL_SAMPLER_1D_ARRAY_EXT:
   case GL_SAMPLER_2D_ARRAY_EXT:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static void
_mesa_get_active_attrib(GLcontext *ctx, GLuint program, GLuint index,
                        GLsizei maxLength, GLsizei *length, GLint *size,
                        GLenum *type, GLchar *nameOut)
{
   const struct gl_program_parameter_list *attribs = NULL;
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
      return;

   if (shProg->VertexProgram)
      attribs = shProg->VertexProgram->Base.Attributes;

   if (!attribs || index >= attribs->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
      return;
   }

   copy_string(nameOut, maxLength, length, attribs->Parameters[index].Name);

   if (size)
      *size = attribs->Parameters[index].Size
         / sizeof_glsl_type(attribs->Parameters[index].DataType);

   if (type)
      *type = attribs->Parameters[index].DataType;
}


static struct gl_program_parameter *
get_uniform_parameter(const struct gl_shader_program *shProg, GLuint index)
{
   const struct gl_program *prog;
   GLint progPos;

   progPos = shProg->Uniforms->Uniforms[index].VertPos;
   if (progPos >= 0) {
      prog = &shProg->VertexProgram->Base;
   }
   else {
      progPos = shProg->Uniforms->Uniforms[index].FragPos;
      if (progPos >= 0) {
         prog = &shProg->FragmentProgram->Base;
      }
   }

   if (!prog || progPos < 0)
      return NULL; /* should never happen */

   return &prog->Parameters->Parameters[progPos];
}


/**
 * Called via ctx->Driver.GetActiveUniform().
 */
static void
_mesa_get_active_uniform(GLcontext *ctx, GLuint program, GLuint index,
                         GLsizei maxLength, GLsizei *length, GLint *size,
                         GLenum *type, GLchar *nameOut)
{
   const struct gl_shader_program *shProg;
   const struct gl_program *prog;
   const struct gl_program_parameter *param;
   GLint progPos;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveUniform");
   if (!shProg)
      return;

   if (!shProg->Uniforms || index >= shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   progPos = shProg->Uniforms->Uniforms[index].VertPos;
   if (progPos >= 0) {
      prog = &shProg->VertexProgram->Base;
   }
   else {
      progPos = shProg->Uniforms->Uniforms[index].FragPos;
      if (progPos >= 0) {
         prog = &shProg->FragmentProgram->Base;
      }
   }

   if (!prog || progPos < 0)
      return; /* should never happen */

   ASSERT(progPos < prog->Parameters->NumParameters);
   param = &prog->Parameters->Parameters[progPos];

   if (nameOut) {
      copy_string(nameOut, maxLength, length, param->Name);
   }

   if (size) {
      GLint typeSize = sizeof_glsl_type(param->DataType);
      if (param->Size > typeSize) {
         /* This is an array.
          * Array elements are placed on vector[4] boundaries so they're
          * a multiple of four floats.  We round typeSize up to next multiple
          * of four to get the right size below.
          */
         typeSize = (typeSize + 3) & ~3;
      }
      /* Note that the returned size is in units of the <type>, not bytes */
      *size = param->Size / typeSize;
   }

   if (type) {
      *type = param->DataType;
   }
}


/**
 * Called via ctx->Driver.GetAttachedShaders().
 */
static void
_mesa_get_attached_shaders(GLcontext *ctx, GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *obj)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetAttachedShaders");
   if (shProg) {
      GLuint i;
      for (i = 0; i < (GLuint) maxCount && i < shProg->NumShaders; i++) {
         obj[i] = shProg->Shaders[i]->Name;
      }
      if (count)
         *count = i;
   }
}


static GLuint
_mesa_get_handle(GLcontext *ctx, GLenum pname)
{
   GLint handle = 0;
   
   if (pname == GL_PROGRAM_OBJECT_ARB) {
      CALL_GetIntegerv(ctx->Exec, (GL_CURRENT_PROGRAM, &handle));
   } else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHandleARB");
   }

   return handle;
}


static void
_mesa_get_programiv(GLcontext *ctx, GLuint program,
                    GLenum pname, GLint *params)
{
   const struct gl_program_parameter_list *attribs;
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramiv(program)");
      return;
   }

   if (shProg->VertexProgram)
      attribs = shProg->VertexProgram->Base.Attributes;
   else
      attribs = NULL;

   switch (pname) {
   case GL_DELETE_STATUS:
      *params = shProg->DeletePending;
      break; 
   case GL_LINK_STATUS:
      *params = shProg->LinkStatus;
      break;
   case GL_VALIDATE_STATUS:
      *params = shProg->Validated;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shProg->InfoLog ? strlen(shProg->InfoLog) + 1 : 0;
      break;
   case GL_ATTACHED_SHADERS:
      *params = shProg->NumShaders;
      break;
   case GL_ACTIVE_ATTRIBUTES:
      *params = attribs ? attribs->NumParameters : 0;
      break;
   case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = _mesa_longest_parameter_name(attribs, PROGRAM_INPUT) + 1;
      break;
   case GL_ACTIVE_UNIFORMS:
      *params = shProg->Uniforms ? shProg->Uniforms->NumUniforms : 0;
      break;
   case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      *params = _mesa_longest_uniform_name(shProg->Uniforms);
      if (*params > 0)
         (*params)++;  /* add one for terminating zero */
      break;
   case GL_PROGRAM_BINARY_LENGTH_OES:
      *params = 0;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname)");
      return;
   }
}


static void
_mesa_get_shaderiv(GLcontext *ctx, GLuint name, GLenum pname, GLint *params)
{
   struct gl_shader *shader = _mesa_lookup_shader_err(ctx, name, "glGetShaderiv");

   if (!shader) {
      return;
   }

   switch (pname) {
   case GL_SHADER_TYPE:
      *params = shader->Type;
      break;
   case GL_DELETE_STATUS:
      *params = shader->DeletePending;
      break;
   case GL_COMPILE_STATUS:
      *params = shader->CompileStatus;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shader->InfoLog ? strlen(shader->InfoLog) + 1 : 0;
      break;
   case GL_SHADER_SOURCE_LENGTH:
      *params = shader->Source ? strlen((char *) shader->Source) + 1 : 0;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetShaderiv(pname)");
      return;
   }
}


static void
_mesa_get_program_info_log(GLcontext *ctx, GLuint program, GLsizei bufSize,
                           GLsizei *length, GLchar *infoLog)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramInfoLog(program)");
      return;
   }
   copy_string(infoLog, bufSize, length, shProg->InfoLog);
}


static void
_mesa_get_shader_info_log(GLcontext *ctx, GLuint shader, GLsizei bufSize,
                          GLsizei *length, GLchar *infoLog)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderInfoLog(shader)");
      return;
   }
   copy_string(infoLog, bufSize, length, sh->InfoLog);
}


/**
 * Called via ctx->Driver.GetShaderSource().
 */
static void
_mesa_get_shader_source(GLcontext *ctx, GLuint shader, GLsizei maxLength,
                        GLsizei *length, GLchar *sourceOut)
{
   struct gl_shader *sh;
   sh = _mesa_lookup_shader_err(ctx, shader, "glGetShaderSource");
   if (!sh) {
      return;
   }
   copy_string(sourceOut, maxLength, length, sh->Source);
}


static void
get_matrix_dims(GLenum type, GLint *rows, GLint *cols)
{
   switch (type) {
   case GL_FLOAT_MAT2:
      *rows = *cols = 2;
      break;
   case GL_FLOAT_MAT2x3:
      *rows = 3;
      *cols = 2;
      break;
   case GL_FLOAT_MAT2x4:
      *rows = 4;
      *cols = 2;
      break;
   case GL_FLOAT_MAT3:
      *rows = 3;
      *cols = 3;
      break;
   case GL_FLOAT_MAT3x2:
      *rows = 2;
      *cols = 3;
      break;
   case GL_FLOAT_MAT3x4:
      *rows = 4;
      *cols = 3;
      break;
   case GL_FLOAT_MAT4:
      *rows = 4;
      *cols = 4;
      break;
   case GL_FLOAT_MAT4x2:
      *rows = 2;
      *cols = 4;
      break;
   case GL_FLOAT_MAT4x3:
      *rows = 3;
      *cols = 4;
      break;
   default:
      *rows = *cols = 0;
   }
}


/**
 * Determine the number of rows and columns occupied by a uniform
 * according to its datatype.
 */
static void
get_uniform_rows_cols(const struct gl_program_parameter *p,
                      GLint *rows, GLint *cols)
{
   get_matrix_dims(p->DataType, rows, cols);
   if (*rows == 0 && *cols == 0) {
      /* not a matrix type, probably a float or vector */
      *rows = p->Size / 4 + 1;
      if (p->Size % 4 == 0)
         *cols = 4;
      else
         *cols = p->Size % 4;
   }
}


/**
 * Helper for get_uniform[fi]v() functions.
 * Given a shader program name and uniform location, return a pointer
 * to the shader program and return the program parameter position.
 */
static void
lookup_uniform_parameter(GLcontext *ctx, GLuint program, GLint location,
                         struct gl_program **progOut, GLint *paramPosOut)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program_err(ctx, program, "glGetUniform[if]v");
   struct gl_program *prog = NULL;
   GLint progPos = -1;

   /* if shProg is NULL, we'll have already recorded an error */

   if (shProg) {
      if (!shProg->Uniforms ||
          location < 0 ||
          location >= (GLint) shProg->Uniforms->NumUniforms) {
         _mesa_error(ctx, GL_INVALID_OPERATION,  "glGetUniformfv(location)");
      }
      else {
         /* OK, find the gl_program and program parameter location */
         progPos = shProg->Uniforms->Uniforms[location].VertPos;
         if (progPos >= 0) {
            prog = &shProg->VertexProgram->Base;
         }
         else {
            progPos = shProg->Uniforms->Uniforms[location].FragPos;
            if (progPos >= 0) {
               prog = &shProg->FragmentProgram->Base;
            }
         }
      }
   }

   *progOut = prog;
   *paramPosOut = progPos;
}


/**
 * Called via ctx->Driver.GetUniformfv().
 */
static void
_mesa_get_uniformfv(GLcontext *ctx, GLuint program, GLint location,
                    GLfloat *params)
{
   struct gl_program *prog;
   GLint paramPos;

   lookup_uniform_parameter(ctx, program, location, &prog, &paramPos);

   if (prog) {
      const struct gl_program_parameter *p =
         &prog->Parameters->Parameters[paramPos];
      GLint rows, cols, i, j, k;

      get_uniform_rows_cols(p, &rows, &cols);

      k = 0;
      for (i = 0; i < rows; i++) {
         for (j = 0; j < cols; j++ ) {
            params[k++] = prog->Parameters->ParameterValues[paramPos+i][j];
         }
      }
   }
}


/**
 * Called via ctx->Driver.GetUniformiv().
 * \sa _mesa_get_uniformfv, only difference is a cast.
 */
static void
_mesa_get_uniformiv(GLcontext *ctx, GLuint program, GLint location,
                    GLint *params)
{
   struct gl_program *prog;
   GLint paramPos;

   lookup_uniform_parameter(ctx, program, location, &prog, &paramPos);

   if (prog) {
      const struct gl_program_parameter *p =
         &prog->Parameters->Parameters[paramPos];
      GLint rows, cols, i, j, k;

      get_uniform_rows_cols(p, &rows, &cols);

      k = 0;
      for (i = 0; i < rows; i++) {
         for (j = 0; j < cols; j++ ) {
            params[k++] = (GLint) prog->Parameters->ParameterValues[paramPos+i][j];
         }
      }
   }
}


/**
 * The value returned by GetUniformLocation actually encodes two things:
 * 1. the index into the prog->Uniforms[] array for the uniform
 * 2. an offset in the prog->ParameterValues[] array for specifying array
 *    elements or structure fields.
 * This function merges those two values.
 */
static void
merge_location_offset(GLint *location, GLint offset)
{
   *location = *location | (offset << 16);
}


/**
 * Seperate the uniform location and parameter offset.  See above.
 */
static void
split_location_offset(GLint *location, GLint *offset)
{
   *offset = (*location >> 16);
   *location = *location & 0xffff;
}


/**
 * Called via ctx->Driver.GetUniformLocation().
 *
 * The return value will encode two values, the uniform location and an
 * offset (used for arrays, structs).
 */
static GLint
_mesa_get_uniform_location(GLcontext *ctx, GLuint program, const GLchar *name)
{
   GLint offset = 0, location = -1;

   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetUniformLocation");

   if (!shProg)
      return -1;

   if (shProg->LinkStatus == GL_FALSE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformfv(program)");
      return -1;
   }

   /* XXX we should return -1 if the uniform was declared, but not
    * actually used.
    */

   /* XXX we need to be able to parse uniform names for structs and arrays
    * such as:
    *   mymatrix[1]
    *   mystruct.field1
    */

   {
      /* handle 1-dimension arrays here... */
      char *c = strchr(name, '[');
      if (c) {
         /* truncate name at [ */
         const GLint len = c - name;
         GLchar *newName = _mesa_malloc(len + 1);
         if (!newName)
            return -1; /* out of mem */
         _mesa_memcpy(newName, name, len);
         newName[len] = 0;

         location = _mesa_lookup_uniform(shProg->Uniforms, newName);
         if (location >= 0) {
            const GLint element = _mesa_atoi(c + 1);
            if (element > 0) {
               /* get type of the uniform array element */
               struct gl_program_parameter *p;
               p = get_uniform_parameter(shProg, location);
               if (p) {
                  GLint rows, cols;
                  get_matrix_dims(p->DataType, &rows, &cols);
                  if (rows < 1)
                     rows = 1;
                  offset = element * rows;
               }
            }
         }

         _mesa_free(newName);
      }
   }

   if (location < 0) {
      location = _mesa_lookup_uniform(shProg->Uniforms, name);
   }

   if (location >= 0) {
      merge_location_offset(&location, offset);
   }

   return location;
}



/**
 * Called via ctx->Driver.ShaderSource()
 */
static void
_mesa_shader_source(GLcontext *ctx, GLuint shader, const GLchar *source)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shader, "glShaderSource");
   if (!sh)
      return;

   /* free old shader source string and install new one */
   if (sh->Source) {
      _mesa_free((void *) sh->Source);
   }
   sh->Source = source;
   sh->CompileStatus = GL_FALSE;
}


/**
 * Called via ctx->Driver.CompileShader()
 */
static void
_mesa_compile_shader(GLcontext *ctx, GLuint shaderObj)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shaderObj, "glCompileShader");
   if (!sh)
      return;

   sh->CompileStatus = _slang_compile(ctx, sh);
}


/**
 * Called via ctx->Driver.LinkProgram()
 */
static void
_mesa_link_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glLinkProgram");
   if (!shProg)
      return;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   _slang_link(ctx, program, shProg);
}


/**
 * Called via ctx->Driver.UseProgram()
 */
void
_mesa_use_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   if (ctx->Shader.CurrentProgram &&
       ctx->Shader.CurrentProgram->Name == program) {
      /* no-op */
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (program) {
      shProg = _mesa_lookup_shader_program_err(ctx, program, "glUseProgram");
      if (!shProg) {
         return;
      }
      if (!shProg->LinkStatus) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUseProgram(program %u not linked)", program);
         return;
      }
   }
   else {
      shProg = NULL;
   }

   _mesa_reference_shader_program(ctx, &ctx->Shader.CurrentProgram, shProg);
}



/**
 * Update the vertex/fragment program's TexturesUsed array.
 *
 * This needs to be called after glUniform(set sampler var) is called.
 * A call to glUniform(samplerVar, value) causes a sampler to point to a
 * particular texture unit.  We know the sampler's texture target
 * (1D/2D/3D/etc) from compile time but the sampler's texture unit is
 * set by glUniform() calls.
 *
 * So, scan the program->SamplerUnits[] and program->SamplerTargets[]
 * information to update the prog->TexturesUsed[] values.
 * Each value of TexturesUsed[unit] is one of zero, TEXTURE_1D_INDEX,
 * TEXTURE_2D_INDEX, TEXTURE_3D_INDEX, etc.
 * We'll use that info for state validation before rendering.
 */
void
_mesa_update_shader_textures_used(struct gl_program *prog)
{
   GLuint s;

   memset(prog->TexturesUsed, 0, sizeof(prog->TexturesUsed));

   for (s = 0; s < MAX_SAMPLERS; s++) {
      if (prog->SamplersUsed & (1 << s)) {
         GLuint u = prog->SamplerUnits[s];
         GLuint t = prog->SamplerTargets[s];
         assert(u < MAX_TEXTURE_IMAGE_UNITS);
         prog->TexturesUsed[u] |= (1 << t);
      }
   }
}


/**
 * Check if the type given by userType is allowed to set a uniform of the
 * target type.  Generally, equivalence is required, but setting Boolean
 * uniforms can be done with glUniformiv or glUniformfv.
 */
static GLboolean
compatible_types(GLenum userType, GLenum targetType)
{
   if (userType == targetType)
      return GL_TRUE;

   if (targetType == GL_BOOL && (userType == GL_FLOAT || userType == GL_INT))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC2 && (userType == GL_FLOAT_VEC2 ||
                                      userType == GL_INT_VEC2))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC3 && (userType == GL_FLOAT_VEC3 ||
                                      userType == GL_INT_VEC3))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC4 && (userType == GL_FLOAT_VEC4 ||
                                      userType == GL_INT_VEC4))
      return GL_TRUE;

   if (is_sampler_type(targetType) && userType == GL_INT)
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Set the value of a program's uniform variable.
 * \param program  the program whose uniform to update
 * \param index  the index of the program parameter for the uniform
 * \param offset  additional parameter slot offset (for arrays)
 * \param type  the incoming datatype of 'values'
 * \param count  the number of uniforms to set
 * \param elems  number of elements per uniform (1, 2, 3 or 4)
 * \param values  the new values, of datatype 'type'
 */
static void
set_program_uniform(GLcontext *ctx, struct gl_program *program,
                    GLint index, GLint offset,
                    GLenum type, GLsizei count, GLint elems,
                    const void *values)
{
   struct gl_program_parameter *param =
      &program->Parameters->Parameters[index];
   const GLboolean isUniformBool = is_boolean_type(param->DataType);
   const GLboolean areIntValues = is_integer_type(type);

   assert(offset >= 0);
   assert(elems >= 1);
   assert(elems <= 4);

   if (!compatible_types(type, param->DataType)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(type mismatch)");
      return;
   }

   if (index + offset > program->Parameters->Size) {
      /* out of bounds! */
      return;
   }

   if (param->Type == PROGRAM_SAMPLER) {
      /* This controls which texture unit which is used by a sampler */
      GLuint texUnit, sampler;
      GLint i;

      /* data type for setting samplers must be int */
      if (type != GL_INT) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUniform(only glUniform1i can be used "
                     "to set sampler uniforms)");
         return;
      }

      /* XXX arrays of samplers haven't been tested much, but it's not a
       * common thing...
       */
      for (i = 0; i < count; i++) {
         sampler = (GLuint) program->Parameters->ParameterValues[index + i][0];
         texUnit = ((GLuint *) values)[i];

         /* check that the sampler (tex unit index) is legal */
         if (texUnit >= ctx->Const.MaxTextureImageUnits) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glUniform1(invalid sampler/tex unit index)");
            return;
         }

         /* This maps a sampler to a texture unit: */
         if (sampler < MAX_SAMPLERS) {
            program->SamplerUnits[sampler] = texUnit;
         }
      }

      _mesa_update_shader_textures_used(program);

      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   }
   else {
      /* ordinary uniform variable */
      GLsizei k, i;
      const GLint slots = (param->Size + 3) / 4;
      const GLint typeSize = sizeof_glsl_type(param->DataType);

      if (param->Size > typeSize) {
         /* an array */
         /* we'll ignore extra data below */
      }
      else {
         /* non-array: count must be one */
         if (count != 1) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glUniform(uniform is not an array)");
            return;
         }
      }

      /* loop over number of array elements */
      for (k = 0; k < count; k++) {
         GLfloat *uniformVal;

         if (offset + k >= slots) {
            /* Extra array data is ignored */
            break;
         }

         /* uniformVal (the destination) is always float[4] */
         uniformVal = program->Parameters->ParameterValues[index + offset + k];

         if (areIntValues) {
            /* convert user's ints to floats */
            const GLint *iValues = ((const GLint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = (GLfloat) iValues[i];
            }
         }
         else {
            const GLfloat *fValues = ((const GLfloat *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = fValues[i];
            }
         }

         /* if the uniform is bool-valued, convert to 1.0 or 0.0 */
         if (isUniformBool) {
            for (i = 0; i < elems; i++) {
               uniformVal[i] = uniformVal[i] ? 1.0 : 0.0;
            }
         }
      }
   }
}


/**
 * Called via ctx->Driver.Uniform().
 */
static void
_mesa_uniform(GLcontext *ctx, GLint location, GLsizei count,
              const GLvoid *values, GLenum type)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   struct gl_uniform *uniform;
   GLint elems, offset;

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(program not linked)");
      return;
   }

   if (location == -1)
      return;   /* The standard specifies this as a no-op */

   if (location < -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(location)");
      return;
   }

   split_location_offset(&location, &offset);

   if (location < 0 || location >= (GLint) shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(location)");
      return;
   }

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(count < 0)");
      return;
   }

   switch (type) {
   case GL_FLOAT:
   case GL_INT:
      elems = 1;
      break;
   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
      elems = 2;
      break;
   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
      elems = 3;
      break;
   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
      elems = 4;
      break;
   default:
      _mesa_problem(ctx, "Invalid type in _mesa_uniform");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   uniform = &shProg->Uniforms->Uniforms[location];

   /* A uniform var may be used by both a vertex shader and a fragment
    * shader.  We may need to update one or both shader's uniform here:
    */
   if (shProg->VertexProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform(ctx, &shProg->VertexProgram->Base,
                             index, offset, type, count, elems, values);
      }
   }

   if (shProg->FragmentProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform(ctx, &shProg->FragmentProgram->Base,
                             index, offset, type, count, elems, values);
      }
   }

   uniform->Initialized = GL_TRUE;
}


/**
 * Set a matrix-valued program parameter.
 */
static void
set_program_uniform_matrix(GLcontext *ctx, struct gl_program *program,
                           GLuint index, GLuint offset,
                           GLuint count, GLuint rows, GLuint cols,
                           GLboolean transpose, const GLfloat *values)
{
   GLuint mat, row, col;
   GLuint dst = index + offset, src = 0;
   GLint nr, nc;

   /* check that the number of rows, columns is correct */
   get_matrix_dims(program->Parameters->Parameters[index].DataType, &nr, &nc);
   if (rows != nr || cols != nc) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glUniformMatrix(matrix size mismatch)");
      return;
   }

   if (index + offset > program->Parameters->Size) {
      /* out of bounds! */
      return;
   }

   /*
    * Note: the _columns_ of a matrix are stored in program registers, not
    * the rows.  So, the loops below look a little funny.
    * XXX could optimize this a bit...
    */

   /* loop over matrices */
   for (mat = 0; mat < count; mat++) {

      /* each matrix: */
      for (col = 0; col < cols; col++) {
         GLfloat *v = program->Parameters->ParameterValues[dst];
         for (row = 0; row < rows; row++) {
            if (transpose) {
               v[row] = values[src + row * cols + col];
            }
            else {
               v[row] = values[src + col * rows + row];
            }
         }
         dst++;
      }

      src += rows * cols;  /* next matrix */
   }
}


/**
 * Called by ctx->Driver.UniformMatrix().
 * Note: cols=2, rows=4  ==>  array[2] of vec4
 */
static void
_mesa_uniform_matrix(GLcontext *ctx, GLint cols, GLint rows,
                     GLenum matrixType, GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   struct gl_uniform *uniform;
   GLint offset;

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         "glUniformMatrix(program not linked)");
      return;
   }

   if (location == -1)
      return;   /* The standard specifies this as a no-op */

   if (location < -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniformMatrix(location)");
      return;
   }

   split_location_offset(&location, &offset);

   if (location < 0 || location >= (GLint) shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix(location)");
      return;
   }
   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   uniform = &shProg->Uniforms->Uniforms[location];

   if (shProg->VertexProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx, &shProg->VertexProgram->Base,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   if (shProg->FragmentProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx, &shProg->FragmentProgram->Base,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   uniform->Initialized = GL_TRUE;
}


static void
_mesa_validate_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glValidateProgram");
   if (!shProg) {
      return;
   }

   if (!shProg->LinkStatus) {
      shProg->Validated = GL_FALSE;
      return;
   }

   /* From the GL spec, a program is invalid if any of these are true:

     any two active samplers in the current program object are of
     different types, but refer to the same texture image unit,

     any active sampler in the current program object refers to a texture
     image unit where fixed-function fragment processing accesses a
     texture target that does not match the sampler type, or 

     the sum of the number of active samplers in the program and the
     number of texture image units enabled for fixed-function fragment
     processing exceeds the combined limit on the total number of texture
     image units allowed.
   */

   shProg->Validated = GL_TRUE;
}


/**
 * Plug in Mesa's GLSL functions into the device driver function table.
 */
void
_mesa_init_glsl_driver_functions(struct dd_function_table *driver)
{
   driver->AttachShader = _mesa_attach_shader;
   driver->BindAttribLocation = _mesa_bind_attrib_location;
   driver->CompileShader = _mesa_compile_shader;
   driver->CreateProgram = _mesa_create_program;
   driver->CreateShader = _mesa_create_shader;
   driver->DeleteProgram2 = _mesa_delete_program2;
   driver->DeleteShader = _mesa_delete_shader;
   driver->DetachShader = _mesa_detach_shader;
   driver->GetActiveAttrib = _mesa_get_active_attrib;
   driver->GetActiveUniform = _mesa_get_active_uniform;
   driver->GetAttachedShaders = _mesa_get_attached_shaders;
   driver->GetAttribLocation = _mesa_get_attrib_location;
   driver->GetHandle = _mesa_get_handle;
   driver->GetProgramiv = _mesa_get_programiv;
   driver->GetProgramInfoLog = _mesa_get_program_info_log;
   driver->GetShaderiv = _mesa_get_shaderiv;
   driver->GetShaderInfoLog = _mesa_get_shader_info_log;
   driver->GetShaderSource = _mesa_get_shader_source;
   driver->GetUniformfv = _mesa_get_uniformfv;
   driver->GetUniformiv = _mesa_get_uniformiv;
   driver->GetUniformLocation = _mesa_get_uniform_location;
   driver->IsProgram = _mesa_is_program;
   driver->IsShader = _mesa_is_shader;
   driver->LinkProgram = _mesa_link_program;
   driver->ShaderSource = _mesa_shader_source;
   driver->Uniform = _mesa_uniform;
   driver->UniformMatrix = _mesa_uniform_matrix;
   driver->UseProgram = _mesa_use_program;
   driver->ValidateProgram = _mesa_validate_program;
}
