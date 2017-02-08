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

/* This file declares stripped-down versions of functions that
 * normally exist outside of the glsl folder, so that they can be used
 * when running the GLSL compiler standalone (for unit testing or
 * compiling builtins).
 */

#include "standalone_scaffolding.h"

#include <assert.h>
#include <string.h>
#include "ralloc.h"

void
_mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh)
{
   (void) ctx;
   *ptr = sh;
}

struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;

   (void) ctx;

   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = rzalloc(NULL, struct gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}

void initialize_context_to_defaults(struct gl_context *ctx, gl_api api)
{
   memset(ctx, 0, sizeof(*ctx));

   ctx->API = api;

   ctx->Extensions.dummy_false = false;
   ctx->Extensions.dummy_true = true;
   ctx->Extensions.ARB_ES2_compatibility = true;
   ctx->Extensions.ARB_draw_instanced = true;
   ctx->Extensions.ARB_fragment_coord_conventions = true;
   ctx->Extensions.EXT_texture_array = true;
   ctx->Extensions.NV_texture_rectangle = true;
   ctx->Extensions.EXT_texture3D = true;
   ctx->Extensions.OES_EGL_image_external = true;

   ctx->Const.GLSLVersion = 120;

   /* 1.20 minimums. */
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxClipPlanes = 6;
   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxTextureCoordUnits = 2;
   ctx->Const.VertexProgram.MaxAttribs = 16;

   ctx->Const.VertexProgram.MaxUniformComponents = 512;
   ctx->Const.MaxVarying = 8; /* == gl_MaxVaryingFloats / 4 */
   ctx->Const.MaxVertexTextureImageUnits = 0;
   ctx->Const.MaxCombinedTextureImageUnits = 2;
   ctx->Const.MaxTextureImageUnits = 2;
   ctx->Const.FragmentProgram.MaxUniformComponents = 64;

   ctx->Const.MaxDrawBuffers = 1;
}
