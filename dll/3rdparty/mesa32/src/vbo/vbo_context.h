/*
 * mesa 3-D graphics library
 * Version:  6.5
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
 * \file vbo_context.h
 * \brief VBO builder module datatypes and definitions.
 * \author Keith Whitwell
 */


/**
 * \mainpage The VBO builder module
 *
 * This module hooks into the GL dispatch table and catches all vertex
 * building and drawing commands, such as glVertex3f, glBegin and
 * glDrawArrays.  The module stores all incoming vertex data as arrays
 * in GL vertex buffer objects (VBOs), and translates all drawing
 * commands into calls to a driver supplied DrawPrimitives() callback.
 *
 * The module captures both immediate mode and display list drawing,
 * and manages the allocation, reference counting and deallocation of
 * vertex buffer objects itself.
 * 
 * The DrawPrimitives() callback can be either implemented by the
 * driver itself or hooked to the tnl module's _tnl_draw_primitives()
 * function for hardware without tnl capablilties or during fallbacks.
 */


#ifndef _VBO_CONTEXT_H
#define _VBO_CONTEXT_H

#include "vbo.h"
#include "vbo_attrib.h"
#include "vbo_exec.h"
#if FEATURE_dlist
#include "vbo_save.h"
#endif


struct vbo_context {
   struct gl_client_array currval[VBO_ATTRIB_MAX];
   
   /* These point into the above.  TODO: remove. 
    */
   struct gl_client_array *legacy_currval;
   struct gl_client_array *generic_currval;
   struct gl_client_array *mat_currval;

   GLuint map_vp_none[32];
   GLuint map_vp_arb[32];

   GLfloat *current[VBO_ATTRIB_MAX]; /* points into ctx->Current, ctx->Light.Material */
   GLfloat CurrentFloatEdgeFlag;


   struct vbo_exec_context exec;
#if FEATURE_dlist
   struct vbo_save_context save;
#endif

   /* Callback into the driver.  This must always succeed, the driver
    * is responsible for initiating any fallback actions required:
    */
   vbo_draw_func draw_prims;
};


static INLINE struct vbo_context *vbo_context(GLcontext *ctx) 
{
   return (struct vbo_context *)(ctx->swtnl_im);
}

enum {
   VP_NONE = 1,
   VP_NV,
   VP_ARB
};

static INLINE GLuint get_program_mode( GLcontext *ctx )
{
   if (!ctx->VertexProgram._Current)
      return VP_NONE;
   else if (ctx->VertexProgram._Current->IsNVProgram)
      return VP_NV;
   else
      return VP_ARB;
}


#endif
