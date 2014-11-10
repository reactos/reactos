/*
 * Mesa 3-D graphics library
 * Version:  7.3
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
 * \file state.c
 * State management.
 * 
 * This file manages recalculation of derived values in struct gl_context.
 */

#include <precomp.h>


/**
 * Helper for update_arrays().
 * \return  min(current min, array->_MaxElement).
 */
static GLuint
update_min(GLuint min, struct gl_client_array *array)
{
   _mesa_update_array_max_element(array);
   return MIN2(min, array->_MaxElement);
}


/**
 * Update ctx->Array._MaxElement (the max legal index into all enabled arrays).
 * Need to do this upon new array state or new buffer object state.
 */
static void
update_arrays( struct gl_context *ctx )
{
   GLuint min = ~0;

   /* find min of _MaxElement values for all enabled arrays.
    * Note that the generic arrays always take precedence over
    * the legacy arrays.
    */

   /* 0 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_POS]);
   }

   /* 2 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_NORMAL].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_NORMAL]);
   }

   /* 3 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR]);
   }

   /* 5 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_FOG].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_FOG]);
   }

   /* 6 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR_INDEX]);
   }

   /* 8 */
   if (ctx->Array.VertexAttrib[VERT_ATTRIB_TEX].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_TEX]);
   }

   if (ctx->Array.VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled) {
      min = update_min(min, &ctx->Array.VertexAttrib[VERT_ATTRIB_EDGEFLAG]);
   }

   /* _MaxElement is one past the last legal array element */
   ctx->Array._MaxElement = min;
}

static void
update_viewport_matrix(struct gl_context *ctx)
{
   const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;

   ASSERT(depthMax > 0);

   /* Compute scale and bias values. This is really driver-specific
    * and should be maintained elsewhere if at all.
    * NOTE: RasterPos uses this.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         depthMax);
}


/*
 * Check polygon state and set DD_TRI_CULL_FRONT_BACK and/or DD_TRI_OFFSET
 * in ctx->_TriangleCaps if needed.
 */
static void
update_polygon(struct gl_context *ctx)
{
   ctx->_TriangleCaps &= ~(DD_TRI_CULL_FRONT_BACK | DD_TRI_OFFSET);

   if (ctx->Polygon.CullFlag && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
      ctx->_TriangleCaps |= DD_TRI_CULL_FRONT_BACK;

   if (   ctx->Polygon.OffsetPoint
       || ctx->Polygon.OffsetLine
       || ctx->Polygon.OffsetFill)
      ctx->_TriangleCaps |= DD_TRI_OFFSET;
}


/**
 * Update the ctx->_TriangleCaps bitfield.
 * XXX that bitfield should really go away someday!
 * This function must be called after other update_*() functions since
 * there are dependencies on some other derived values.
 */
#if 0
static void
update_tricaps(struct gl_context *ctx, GLbitfield new_state)
{
   ctx->_TriangleCaps = 0;

   /*
    * Points
    */
   if (1/*new_state & _NEW_POINT*/) {
      if (ctx->Point.SmoothFlag)
         ctx->_TriangleCaps |= DD_POINT_SMOOTH;
      if (ctx->Point._Attenuated)
         ctx->_TriangleCaps |= DD_POINT_ATTEN;
   }

   /*
    * Lines
    */
   if (1/*new_state & _NEW_LINE*/) {
      if (ctx->Line.SmoothFlag)
         ctx->_TriangleCaps |= DD_LINE_SMOOTH;
      if (ctx->Line.StippleFlag)
         ctx->_TriangleCaps |= DD_LINE_STIPPLE;
   }

   /*
    * Polygons
    */
   if (1/*new_state & _NEW_POLYGON*/) {
      if (ctx->Polygon.SmoothFlag)
         ctx->_TriangleCaps |= DD_TRI_SMOOTH;
      if (ctx->Polygon.StippleFlag)
         ctx->_TriangleCaps |= DD_TRI_STIPPLE;
      if (ctx->Polygon.FrontMode != GL_FILL
          || ctx->Polygon.BackMode != GL_FILL)
         ctx->_TriangleCaps |= DD_TRI_UNFILLED;
      if (ctx->Polygon.CullFlag
          && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
         ctx->_TriangleCaps |= DD_TRI_CULL_FRONT_BACK;
      if (ctx->Polygon.OffsetPoint ||
          ctx->Polygon.OffsetLine ||
          ctx->Polygon.OffsetFill)
         ctx->_TriangleCaps |= DD_TRI_OFFSET;
   }

   /*
    * Lighting and shading
    */
   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
      ctx->_TriangleCaps |= DD_TRI_LIGHT_TWOSIDE;
   if (ctx->Light.ShadeModel == GL_FLAT)
      ctx->_TriangleCaps |= DD_FLATSHADE;
   if (_mesa_need_secondary_color(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
}
#endif


/**
 * Compute derived GL state.
 * If __struct gl_contextRec::NewState is non-zero then this function \b must
 * be called before rendering anything.
 *
 * Calls dd_function_table::UpdateState to perform any internal state
 * management necessary.
 * 
 * \sa _mesa_update_modelview_project(), _mesa_update_texture(),
 * _mesa_update_buffer_bounds(),
 * _mesa_update_lighting() and _mesa_update_tnl_spaces().
 */
void
_mesa_update_state_locked( struct gl_context *ctx )
{
   GLbitfield new_state = ctx->NewState;

   if (new_state == _NEW_CURRENT_ATTRIB) 
      goto out;

   /*
    * Now update derived state info
    */

   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      _mesa_update_modelview_project( ctx, new_state );

   if (new_state & (_NEW_TEXTURE|_NEW_TEXTURE_MATRIX))
      _mesa_update_texture( ctx, new_state );

   if (new_state & _NEW_BUFFERS)
      _mesa_update_framebuffer(ctx);

   if (new_state & (_NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT))
      _mesa_update_draw_buffer_bounds( ctx );

   if (new_state & _NEW_POLYGON)
      update_polygon( ctx );

   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   if (new_state & (_NEW_STENCIL | _NEW_BUFFERS))
      _mesa_update_stencil( ctx );

   if (new_state & _NEW_PIXEL)
      _mesa_update_pixel( ctx, new_state );

   if (new_state & (_NEW_BUFFERS | _NEW_VIEWPORT))
      update_viewport_matrix(ctx);

#if 0
   if (new_state & (_NEW_POINT | _NEW_LINE | _NEW_POLYGON | _NEW_LIGHT
                    | _NEW_STENCIL | _DD_NEW_SEPARATE_SPECULAR))
      update_tricaps( ctx, new_state );
#endif

   /* ctx->_NeedEyeCoords is now up to date.
    *
    * If the truth value of this variable has changed, update for the
    * new lighting space and recompute the positions of lights and the
    * normal transform.
    *
    * If the lighting space hasn't changed, may still need to recompute
    * light positions & normal transforms for other reasons.
    */
   if (new_state & _MESA_NEW_NEED_EYE_COORDS) 
      _mesa_update_tnl_spaces( ctx, new_state );

   if (new_state & (_NEW_ARRAY | _NEW_BUFFER_OBJECT))
      update_arrays( ctx );

 out:

   /*
    * Give the driver a chance to act upon the new_state flags.
    * The driver might plug in different span functions, for example.
    * Also, this is where the driver can invalidate the state of any
    * active modules (such as swrast_setup, swrast, tnl, etc).
    *
    * Set ctx->NewState to zero to avoid recursion if
    * Driver.UpdateState() has to call FLUSH_VERTICES().  (fixed?)
    */
   new_state = ctx->NewState;
   ctx->NewState = 0;
   ctx->Driver.UpdateState(ctx, new_state);
   ctx->Array.NewState = 0;
   if (!ctx->Array.RebindArrays)
      ctx->Array.RebindArrays = (new_state & _NEW_ARRAY) != 0;
}


/* This is the usual entrypoint for state updates:
 */
void
_mesa_update_state( struct gl_context *ctx )
{
   _mesa_lock_context_textures(ctx);
   _mesa_update_state_locked(ctx);
   _mesa_unlock_context_textures(ctx);
}
