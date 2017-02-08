/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file feedback.c
 * Selection and feedback modes functions.
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "feedback.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "main/dispatch.h"


#if FEATURE_feedback


#define FB_3D		0x01
#define FB_4D		0x02
#define FB_COLOR	0x04
#define FB_TEXTURE	0X08



static void GLAPIENTRY
_mesa_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode==GL_FEEDBACK) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glFeedbackBuffer" );
      return;
   }
   if (size<0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(size<0)" );
      return;
   }
   if (!buffer && size > 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(buffer==NULL)" );
      ctx->Feedback.BufferSize = 0;
      return;
   }

   switch (type) {
      case GL_2D:
	 ctx->Feedback._Mask = 0;
	 break;
      case GL_3D:
	 ctx->Feedback._Mask = FB_3D;
	 break;
      case GL_3D_COLOR:
	 ctx->Feedback._Mask = (FB_3D | FB_COLOR);
	 break;
      case GL_3D_COLOR_TEXTURE:
	 ctx->Feedback._Mask = (FB_3D | FB_COLOR | FB_TEXTURE);
	 break;
      case GL_4D_COLOR_TEXTURE:
	 ctx->Feedback._Mask = (FB_3D | FB_4D | FB_COLOR | FB_TEXTURE);
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glFeedbackBuffer" );
	 return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE); /* Always flush */
   ctx->Feedback.Type = type;
   ctx->Feedback.BufferSize = size;
   ctx->Feedback.Buffer = buffer;
   ctx->Feedback.Count = 0;	              /* Becaues of this. */
}


static void GLAPIENTRY
_mesa_PassThrough( GLfloat token )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode==GL_FEEDBACK) {
      FLUSH_VERTICES(ctx, 0);
      _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_PASS_THROUGH_TOKEN );
      _mesa_feedback_token( ctx, token );
   }
}


/**
 * Put a vertex into the feedback buffer.
 */
void
_mesa_feedback_vertex(struct gl_context *ctx,
                      const GLfloat win[4],
                      const GLfloat color[4],
                      const GLfloat texcoord[4])
{
   _mesa_feedback_token( ctx, win[0] );
   _mesa_feedback_token( ctx, win[1] );
   if (ctx->Feedback._Mask & FB_3D) {
      _mesa_feedback_token( ctx, win[2] );
   }
   if (ctx->Feedback._Mask & FB_4D) {
      _mesa_feedback_token( ctx, win[3] );
   }
   if (ctx->Feedback._Mask & FB_COLOR) {
      _mesa_feedback_token( ctx, color[0] );
      _mesa_feedback_token( ctx, color[1] );
      _mesa_feedback_token( ctx, color[2] );
      _mesa_feedback_token( ctx, color[3] );
   }
   if (ctx->Feedback._Mask & FB_TEXTURE) {
      _mesa_feedback_token( ctx, texcoord[0] );
      _mesa_feedback_token( ctx, texcoord[1] );
      _mesa_feedback_token( ctx, texcoord[2] );
      _mesa_feedback_token( ctx, texcoord[3] );
   }
}


/**********************************************************************/
/** \name Selection */
/*@{*/

/**
 * Establish a buffer for selection mode values.
 * 
 * \param size buffer size.
 * \param buffer buffer.
 *
 * \sa glSelectBuffer().
 * 
 * \note this function can't be put in a display list.
 * 
 * Verifies we're not in selection mode, flushes the vertices and initialize
 * the fields in __struct gl_contextRec::Select with the given buffer.
 */
static void GLAPIENTRY
_mesa_SelectBuffer( GLsizei size, GLuint *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glSelectBuffer(size)");
      return;
   }

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glSelectBuffer" );
      return;			/* KW: added return */
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE); 
   ctx->Select.Buffer = buffer;
   ctx->Select.BufferSize = size;
   ctx->Select.BufferCount = 0;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = 0.0;
}


/**
 * Write a value of a record into the selection buffer.
 * 
 * \param ctx GL context.
 * \param value value.
 *
 * Verifies there is free space in the buffer to write the value and
 * increments the pointer.
 */
static inline void
write_record(struct gl_context *ctx, GLuint value)
{
   if (ctx->Select.BufferCount < ctx->Select.BufferSize) {
      ctx->Select.Buffer[ctx->Select.BufferCount] = value;
   }
   ctx->Select.BufferCount++;
}


/**
 * Update the hit flag and the maximum and minimum depth values.
 *
 * \param ctx GL context.
 * \param z depth.
 *
 * Sets gl_selection::HitFlag and updates gl_selection::HitMinZ and
 * gl_selection::HitMaxZ.
 */
void
_mesa_update_hitflag(struct gl_context *ctx, GLfloat z)
{
   ctx->Select.HitFlag = GL_TRUE;
   if (z < ctx->Select.HitMinZ) {
      ctx->Select.HitMinZ = z;
   }
   if (z > ctx->Select.HitMaxZ) {
      ctx->Select.HitMaxZ = z;
   }
}


/**
 * Write the hit record.
 *
 * \param ctx GL context.
 *
 * Write the hit record, i.e., the number of names in the stack, the minimum and
 * maximum depth values and the number of names in the name stack at the time
 * of the event. Resets the hit flag. 
 *
 * \sa gl_selection.
 */
static void
write_hit_record(struct gl_context *ctx)
{
   GLuint i;
   GLuint zmin, zmax, zscale = (~0u);

   /* HitMinZ and HitMaxZ are in [0,1].  Multiply these values by */
   /* 2^32-1 and round to nearest unsigned integer. */

   assert( ctx != NULL ); /* this line magically fixes a SunOS 5.x/gcc bug */
   zmin = (GLuint) ((GLfloat) zscale * ctx->Select.HitMinZ);
   zmax = (GLuint) ((GLfloat) zscale * ctx->Select.HitMaxZ);

   write_record( ctx, ctx->Select.NameStackDepth );
   write_record( ctx, zmin );
   write_record( ctx, zmax );
   for (i = 0; i < ctx->Select.NameStackDepth; i++) {
      write_record( ctx, ctx->Select.NameStack[i] );
   }

   ctx->Select.Hits++;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = -1.0;
}


/**
 * Initialize the name stack.
 *
 * Verifies we are in select mode and resets the name stack depth and resets
 * the hit record data in gl_selection. Marks new render mode in
 * __struct gl_contextRec::NewState.
 */
static void GLAPIENTRY
_mesa_InitNames( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   /* Record the hit before the HitFlag is wiped out again. */
   if (ctx->RenderMode == GL_SELECT) {
      if (ctx->Select.HitFlag) {
         write_hit_record( ctx );
      }
   }
   ctx->Select.NameStackDepth = 0;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = 0.0;
   ctx->NewState |= _NEW_RENDERMODE;
}


/**
 * Load the top-most name of the name stack.
 *
 * \param name name.
 *
 * Verifies we are in selection mode and that the name stack is not empty.
 * Flushes vertices. If there is a hit flag writes it (via write_hit_record()),
 * and replace the top-most name in the stack.
 *
 * sa __struct gl_contextRec::Select.
 */
static void GLAPIENTRY
_mesa_LoadName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }
   if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glLoadName" );
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);

   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth < MAX_NAME_STACK_DEPTH) {
      ctx->Select.NameStack[ctx->Select.NameStackDepth-1] = name;
   }
   else {
      ctx->Select.NameStack[MAX_NAME_STACK_DEPTH-1] = name;
   }
}


/**
 * Push a name into the name stack.
 *
 * \param name name.
 *
 * Verifies we are in selection mode and that the name stack is not full.
 * Flushes vertices. If there is a hit flag writes it (via write_hit_record()),
 * and adds the name to the top of the name stack.
 *
 * sa __struct gl_contextRec::Select.
 */
static void GLAPIENTRY
_mesa_PushName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth >= MAX_NAME_STACK_DEPTH) {
      _mesa_error( ctx, GL_STACK_OVERFLOW, "glPushName" );
   }
   else
      ctx->Select.NameStack[ctx->Select.NameStackDepth++] = name;
}


/**
 * Pop a name into the name stack.
 *
 * Verifies we are in selection mode and that the name stack is not empty.
 * Flushes vertices. If there is a hit flag writes it (via write_hit_record()),
 * and removes top-most name in the name stack.
 *
 * sa __struct gl_contextRec::Select.
 */
static void GLAPIENTRY
_mesa_PopName( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_STACK_UNDERFLOW, "glPopName" );
   }
   else
      ctx->Select.NameStackDepth--;
}

/*@}*/


/**********************************************************************/
/** \name Render Mode */
/*@{*/

/**
 * Set rasterization mode.
 *
 * \param mode rasterization mode.
 *
 * \note this function can't be put in a display list.
 *
 * \sa glRenderMode().
 * 
 * Flushes the vertices and do the necessary cleanup according to the previous
 * rasterization mode, such as writing the hit record or resent the select
 * buffer index when exiting the select mode. Updates
 * __struct gl_contextRec::RenderMode and notifies the driver via the
 * dd_function_table::RenderMode callback.
 */
GLint GLAPIENTRY
_mesa_RenderMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint result;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glRenderMode %s\n", _mesa_lookup_enum_by_nr(mode));

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);

   switch (ctx->RenderMode) {
      case GL_RENDER:
	 result = 0;
	 break;
      case GL_SELECT:
	 if (ctx->Select.HitFlag) {
	    write_hit_record( ctx );
	 }
	 if (ctx->Select.BufferCount > ctx->Select.BufferSize) {
	    /* overflow */
#ifdef DEBUG
            _mesa_warning(ctx, "Feedback buffer overflow");
#endif
	    result = -1;
	 }
	 else {
	    result = ctx->Select.Hits;
	 }
	 ctx->Select.BufferCount = 0;
	 ctx->Select.Hits = 0;
	 ctx->Select.NameStackDepth = 0;
	 break;
#if _HAVE_FULL_GL
      case GL_FEEDBACK:
	 if (ctx->Feedback.Count > ctx->Feedback.BufferSize) {
	    /* overflow */
	    result = -1;
	 }
	 else {
	    result = ctx->Feedback.Count;
	 }
	 ctx->Feedback.Count = 0;
	 break;
#endif
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }

   switch (mode) {
      case GL_RENDER:
         break;
      case GL_SELECT:
	 if (ctx->Select.BufferSize==0) {
	    /* haven't called glSelectBuffer yet */
	    _mesa_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
#if _HAVE_FULL_GL
      case GL_FEEDBACK:
	 if (ctx->Feedback.BufferSize==0) {
	    /* haven't called glFeedbackBuffer yet */
	    _mesa_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
#endif
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }

   ctx->RenderMode = mode;
   if (ctx->Driver.RenderMode)
      ctx->Driver.RenderMode( ctx, mode );

   return result;
}

/*@}*/


void
_mesa_init_feedback_dispatch(struct _glapi_table *disp)
{
   SET_InitNames(disp, _mesa_InitNames);
   SET_FeedbackBuffer(disp, _mesa_FeedbackBuffer);
   SET_LoadName(disp, _mesa_LoadName);
   SET_PassThrough(disp, _mesa_PassThrough);
   SET_PopName(disp, _mesa_PopName);
   SET_PushName(disp, _mesa_PushName);
   SET_SelectBuffer(disp, _mesa_SelectBuffer);
   SET_RenderMode(disp, _mesa_RenderMode);
}


#endif /* FEATURE_feedback */


/**********************************************************************/
/** \name Initialization */
/*@{*/

/**
 * Initialize context feedback data.
 */
void _mesa_init_feedback( struct gl_context * ctx )
{
   /* Feedback */
   ctx->Feedback.Type = GL_2D;   /* TODO: verify */
   ctx->Feedback.Buffer = NULL;
   ctx->Feedback.BufferSize = 0;
   ctx->Feedback.Count = 0;

   /* Selection/picking */
   ctx->Select.Buffer = NULL;
   ctx->Select.BufferSize = 0;
   ctx->Select.BufferCount = 0;
   ctx->Select.Hits = 0;
   ctx->Select.NameStackDepth = 0;

   /* Miscellaneous */
   ctx->RenderMode = GL_RENDER;
}

/*@}*/
