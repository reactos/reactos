/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
  
#include "shader/prog_parameter.h"
#include "brw_context.h"
#include "brw_aub.h"
#include "brw_util.h"
#include "program.h"
#include "imports.h"
#include "enums.h"
#include "tnl/tnl.h"


static void brwBindProgram( GLcontext *ctx,
			    GLenum target, 
			    struct gl_program *prog )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      break;
   }
}

static struct gl_program *brwNewProgram( GLcontext *ctx,
				      GLenum target, 
				      GLuint id )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct brw_vertex_program *prog = CALLOC_STRUCT(brw_vertex_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_vertex_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   case GL_FRAGMENT_PROGRAM_ARB: {
      struct brw_fragment_program *prog = CALLOC_STRUCT(brw_fragment_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_fragment_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }
}

static void brwDeleteProgram( GLcontext *ctx,
			      struct gl_program *prog )
{
   
   _mesa_delete_program( ctx, prog );
}


static GLboolean brwIsProgramNative( GLcontext *ctx,
				     GLenum target, 
				     struct gl_program *prog )
{
   return GL_TRUE;
}

static void brwProgramStringNotify( GLcontext *ctx,
				    GLenum target,
				    struct gl_program *prog )
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct brw_context *brw = brw_context(ctx);
      struct brw_fragment_program *p = (struct brw_fragment_program *)prog;
      struct brw_fragment_program *fp = (struct brw_fragment_program *)brw->fragment_program;
      if (p == fp)
	 brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      p->id = brw->program_id++;      
      p->param_state = p->program.Base.Parameters->StateFlags;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct brw_context *brw = brw_context(ctx);
      struct brw_vertex_program *p = (struct brw_vertex_program *)prog;
      struct brw_vertex_program *vp = (struct brw_vertex_program *)brw->vertex_program;
      if (p == vp)
	 brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      p->id = brw->program_id++;      
      p->param_state = p->program.Base.Parameters->StateFlags;

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }
}

void brwInitFragProgFuncs( struct dd_function_table *functions )
{
   assert(functions->ProgramStringNotify == _tnl_program_string); 

   functions->BindProgram = brwBindProgram;
   functions->NewProgram = brwNewProgram;
   functions->DeleteProgram = brwDeleteProgram;
   functions->IsProgramNative = brwIsProgramNative;
   functions->ProgramStringNotify = brwProgramStringNotify;
}

