/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/shaderapi.h"
#include "program/prog_instruction.h"
#include "program/program.h"

#include "cso_cache/cso_context.h"
#include "draw/draw_context.h"

#include "st_context.h"
#include "st_program.h"
#include "st_mesa_to_tgsi.h"
#include "st_cb_program.h"
#include "st_glsl_to_tgsi.h"



/**
 * Called via ctx->Driver.BindProgram() to bind an ARB vertex or
 * fragment program.
 */
static void
st_bind_program(struct gl_context *ctx, GLenum target, struct gl_program *prog)
{
   struct st_context *st = st_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
      break;
   case MESA_GEOMETRY_PROGRAM:
      st->dirty.st |= ST_NEW_GEOMETRY_PROGRAM;
      break;
   }
}


/**
 * Called via ctx->Driver.UseProgram() to bind a linked GLSL program
 * (vertex shader + fragment shader).
 */
static void
st_use_program(struct gl_context *ctx, struct gl_shader_program *shProg)
{
   struct st_context *st = st_context(ctx);

   st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   st->dirty.st |= ST_NEW_GEOMETRY_PROGRAM;
}


/**
 * Called via ctx->Driver.NewProgram() to allocate a new vertex or
 * fragment program.
 */
static struct gl_program *
st_new_program(struct gl_context *ctx, GLenum target, GLuint id)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = ST_CALLOC_STRUCT(st_vertex_program);
      return _mesa_init_vertex_program(ctx, &prog->Base, target, id);
   }

   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV: {
      struct st_fragment_program *prog = ST_CALLOC_STRUCT(st_fragment_program);
      return _mesa_init_fragment_program(ctx, &prog->Base, target, id);
   }

   case MESA_GEOMETRY_PROGRAM: {
      struct st_geometry_program *prog = ST_CALLOC_STRUCT(st_geometry_program);
      return _mesa_init_geometry_program(ctx, &prog->Base, target, id);
   }

   default:
      assert(0);
      return NULL;
   }
}


/**
 * Called via ctx->Driver.DeleteProgram()
 */
static void
st_delete_program(struct gl_context *ctx, struct gl_program *prog)
{
   struct st_context *st = st_context(ctx);

   switch( prog->Target ) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         struct st_vertex_program *stvp = (struct st_vertex_program *) prog;
         st_release_vp_variants( st, stvp );
         
         if (stvp->glsl_to_tgsi)
            free_glsl_to_tgsi_visitor(stvp->glsl_to_tgsi);
      }
      break;
   case MESA_GEOMETRY_PROGRAM:
      {
         struct st_geometry_program *stgp =
            (struct st_geometry_program *) prog;

         st_release_gp_variants(st, stgp);
         
         if (stgp->glsl_to_tgsi)
            free_glsl_to_tgsi_visitor(stgp->glsl_to_tgsi);

         if (stgp->tgsi.tokens) {
            st_free_tokens((void *) stgp->tgsi.tokens);
            stgp->tgsi.tokens = NULL;
         }
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         struct st_fragment_program *stfp =
            (struct st_fragment_program *) prog;

         st_release_fp_variants(st, stfp);
         
         if (stfp->glsl_to_tgsi)
            free_glsl_to_tgsi_visitor(stfp->glsl_to_tgsi);
         
         if (stfp->tgsi.tokens) {
            st_free_tokens(stfp->tgsi.tokens);
            stfp->tgsi.tokens = NULL;
         }
      }
      break;
   default:
      assert(0); /* problem */
   }

   /* delete base class */
   _mesa_delete_program( ctx, prog );
}


/**
 * Called via ctx->Driver.IsProgramNative()
 */
static GLboolean
st_is_program_native(struct gl_context *ctx,
                     GLenum target, 
                     struct gl_program *prog)
{
   return GL_TRUE;
}


/**
 * Called via ctx->Driver.ProgramStringNotify()
 * Called when the program's text/code is changed.  We have to free
 * all shader variants and corresponding gallium shaders when this happens.
 */
static GLboolean
st_program_string_notify( struct gl_context *ctx,
                                           GLenum target,
                                           struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct st_fragment_program *stfp = (struct st_fragment_program *) prog;

      st_release_fp_variants(st, stfp);

      if (stfp->tgsi.tokens) {
         st_free_tokens(stfp->tgsi.tokens);
         stfp->tgsi.tokens = NULL;
      }

      if (st->fp == stfp)
	 st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   }
   else if (target == MESA_GEOMETRY_PROGRAM) {
      struct st_geometry_program *stgp = (struct st_geometry_program *) prog;

      st_release_gp_variants(st, stgp);

      if (stgp->tgsi.tokens) {
         st_free_tokens((void *) stgp->tgsi.tokens);
         stgp->tgsi.tokens = NULL;
      }

      if (st->gp == stgp)
	 st->dirty.st |= ST_NEW_GEOMETRY_PROGRAM;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *stvp = (struct st_vertex_program *) prog;

      st_release_vp_variants( st, stvp );

      if (st->vp == stvp)
	 st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   }

   /* XXX check if program is legal, within limits */
   return GL_TRUE;
}


/**
 * Plug in the program and shader-related device driver functions.
 */
void
st_init_program_functions(struct dd_function_table *functions)
{
   functions->BindProgram = st_bind_program;
   functions->UseProgram = st_use_program;
   functions->NewProgram = st_new_program;
   functions->DeleteProgram = st_delete_program;
   functions->IsProgramNative = st_is_program_native;
   functions->ProgramStringNotify = st_program_string_notify;
   
   functions->NewShader = st_new_shader;
   functions->NewShaderProgram = st_new_shader_program;
   functions->LinkShader = st_link_shader;
}
