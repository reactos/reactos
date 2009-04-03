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
 * \file state.c
 * State management.
 * 
 * This file manages recalculation of derived values in GLcontext.
 */


#include "glheader.h"
#include "mtypes.h"
#include "context.h"
#include "debug.h"
#include "macros.h"
#include "ffvertex_prog.h"
#include "framebuffer.h"
#include "light.h"
#include "matrix.h"
#if FEATURE_pixel_transfer
#include "pixel.h"
#endif
#include "shader/program.h"
#include "state.h"
#include "stencil.h"
#include "texenvprogram.h"
#include "texobj.h"
#include "texstate.h"


static void
update_separate_specular(GLcontext *ctx)
{
   if (NEED_SECONDARY_COLOR(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
   else
      ctx->_TriangleCaps &= ~DD_SEPARATE_SPECULAR;
}


/**
 * Update state dependent on vertex arrays.
 */
static void
update_arrays( GLcontext *ctx )
{
   GLuint i, min;

   /* find min of _MaxElement values for all enabled arrays */

   /* 0 */
   if (ctx->VertexProgram._Current
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      min = ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS]._MaxElement;
   }
   else if (ctx->Array.ArrayObj->Vertex.Enabled) {
      min = ctx->Array.ArrayObj->Vertex._MaxElement;
   }
   else {
      /* can't draw anything without vertex positions! */
      min = 0;
   }

   /* 1 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_WEIGHT].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_WEIGHT]._MaxElement);
   }
   /* no conventional vertex weight array */

   /* 2 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_NORMAL]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Normal.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Normal._MaxElement);
   }

   /* 3 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR0]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Color.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Color._MaxElement);
   }

   /* 4 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR1].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR1]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->SecondaryColor.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->SecondaryColor._MaxElement);
   }

   /* 5 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_FOG].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_FOG]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->FogCoord.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->FogCoord._MaxElement);
   }

   /* 6 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX]._MaxElement);
   }
   else if (ctx->Array.ArrayObj->Index.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->Index._MaxElement);
   }


   /* 7 */
   if (ctx->VertexProgram._Enabled
       && ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG]._MaxElement);
   }

   /* 8..15 */
   for (i = VERT_ATTRIB_TEX0; i <= VERT_ATTRIB_TEX7; i++) {
      if (ctx->VertexProgram._Enabled
          && ctx->Array.ArrayObj->VertexAttrib[i].Enabled) {
         min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[i]._MaxElement);
      }
      else if (i - VERT_ATTRIB_TEX0 < ctx->Const.MaxTextureCoordUnits
               && ctx->Array.ArrayObj->TexCoord[i - VERT_ATTRIB_TEX0].Enabled) {
         min = MIN2(min, ctx->Array.ArrayObj->TexCoord[i - VERT_ATTRIB_TEX0]._MaxElement);
      }
   }

   /* 16..31 */
   if (ctx->VertexProgram._Current) {
      for (i = VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++) {
         if (ctx->Array.ArrayObj->VertexAttrib[i].Enabled) {
            min = MIN2(min, ctx->Array.ArrayObj->VertexAttrib[i]._MaxElement);
         }
      }
   }

   if (ctx->Array.ArrayObj->EdgeFlag.Enabled) {
      min = MIN2(min, ctx->Array.ArrayObj->EdgeFlag._MaxElement);
   }

   /* _MaxElement is one past the last legal array element */
   ctx->Array._MaxElement = min;
}


/**
 * Update the following fields:
 *   ctx->VertexProgram._Enabled
 *   ctx->FragmentProgram._Enabled
 *   ctx->ATIFragmentShader._Enabled
 * This needs to be done before texture state validation.
 */
static void
update_program_enables(GLcontext *ctx)
{
   /* These _Enabled flags indicate if the program is enabled AND valid. */
   ctx->VertexProgram._Enabled = ctx->VertexProgram.Enabled
      && ctx->VertexProgram.Current->Base.Instructions;
   ctx->FragmentProgram._Enabled = ctx->FragmentProgram.Enabled
      && ctx->FragmentProgram.Current->Base.Instructions;
   ctx->ATIFragmentShader._Enabled = ctx->ATIFragmentShader.Enabled
      && ctx->ATIFragmentShader.Current->Instructions[0];
}


/**
 * Update vertex/fragment program state.  In particular, update these fields:
 *   ctx->VertexProgram._Current
 *   ctx->VertexProgram._TnlProgram,
 * These point to the highest priority enabled vertex/fragment program or are
 * NULL if fixed-function processing is to be done.
 *
 * This function needs to be called after texture state validation in case
 * we're generating a fragment program from fixed-function texture state.
 *
 * \return bitfield which will indicate _NEW_PROGRAM state if a new vertex
 * or fragment program is being used.
 */
static GLbitfield
update_program(GLcontext *ctx)
{
   const struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   const struct gl_vertex_program *prevVP = ctx->VertexProgram._Current;
   const struct gl_fragment_program *prevFP = ctx->FragmentProgram._Current;
   GLbitfield new_state = 0x0;

   /*
    * Set the ctx->VertexProgram._Current and ctx->FragmentProgram._Current
    * pointers to the programs that should be used for rendering.  If either
    * is NULL, use fixed-function code paths.
    *
    * These programs may come from several sources.  The priority is as
    * follows:
    *   1. OpenGL 2.0/ARB vertex/fragment shaders
    *   2. ARB/NV vertex/fragment programs
    *   3. Programs derived from fixed-function state.
    *
    * Note: it's possible for a vertex shader to get used with a fragment
    * program (and vice versa) here, but in practice that shouldn't ever
    * come up, or matter.
    */

   if (shProg && shProg->LinkStatus && shProg->FragmentProgram) {
      /* Use shader programs */
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                               shProg->FragmentProgram);
   }
   else if (ctx->FragmentProgram._Enabled) {
      /* use user-defined vertex program */
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                               ctx->FragmentProgram.Current);
   }
   else if (ctx->FragmentProgram._MaintainTexEnvProgram) {
      /* Use fragment program generated from fixed-function state.
       */
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                               _mesa_get_fixed_func_fragment_program(ctx));
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram,
                               ctx->FragmentProgram._Current);
   }
   else {
      /* no fragment program */
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, NULL);
   }

   /* Examine vertex program after fragment program as
    * _mesa_get_fixed_func_vertex_program() needs to know active
    * fragprog inputs.
    */
   if (shProg && shProg->LinkStatus && shProg->VertexProgram) {
      /* Use shader programs */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                            shProg->VertexProgram);
   }
   else if (ctx->VertexProgram._Enabled) {
      /* use user-defined vertex program */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                               ctx->VertexProgram.Current);
   }
   else if (ctx->VertexProgram._MaintainTnlProgram) {
      /* Use vertex program generated from fixed-function state.
       */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                               _mesa_get_fixed_func_vertex_program(ctx));
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._TnlProgram,
                               ctx->VertexProgram._Current);
   }
   else {
      /* no vertex program */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current, NULL);
   }

   /* Let the driver know what's happening:
    */
   if (ctx->FragmentProgram._Current != prevFP) {
      new_state |= _NEW_PROGRAM;
      if (ctx->Driver.BindProgram) {
         ctx->Driver.BindProgram(ctx, GL_FRAGMENT_PROGRAM_ARB,
                          (struct gl_program *) ctx->FragmentProgram._Current);
      }
   }
   
   if (ctx->VertexProgram._Current != prevVP) {
      new_state |= _NEW_PROGRAM;
      if (ctx->Driver.BindProgram) {
         ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
                            (struct gl_program *) ctx->VertexProgram._Current);
      }
   }

   return new_state;
}


static void
update_viewport_matrix(GLcontext *ctx)
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


/**
 * Update derived multisample state.
 */
static void
update_multisample(GLcontext *ctx)
{
   ctx->Multisample._Enabled = GL_FALSE;
   if (ctx->Multisample.Enabled &&
       ctx->DrawBuffer &&
       ctx->DrawBuffer->Visual.sampleBuffers)
      ctx->Multisample._Enabled = GL_TRUE;
}


/**
 * Update derived color/blend/logicop state.
 */
static void
update_color(GLcontext *ctx)
{
   /* This is needed to support 1.1's RGB logic ops AND
    * 1.0's blending logicops.
    */
   ctx->Color._LogicOpEnabled = RGBA_LOGICOP_ENABLED(ctx);
}


/*
 * Check polygon state and set DD_TRI_CULL_FRONT_BACK and/or DD_TRI_OFFSET
 * in ctx->_TriangleCaps if needed.
 */
static void
update_polygon(GLcontext *ctx)
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
update_tricaps(GLcontext *ctx, GLbitfield new_state)
{
   ctx->_TriangleCaps = 0;

   /*
    * Points
    */
   if (1/*new_state & _NEW_POINT*/) {
      if (ctx->Point.SmoothFlag)
         ctx->_TriangleCaps |= DD_POINT_SMOOTH;
      if (ctx->Point.Size != 1.0F)
         ctx->_TriangleCaps |= DD_POINT_SIZE;
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
      if (ctx->Line.Width != 1.0)
         ctx->_TriangleCaps |= DD_LINE_WIDTH;
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
   if (NEED_SECONDARY_COLOR(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;

   /*
    * Stencil
    */
   if (ctx->Stencil._TestTwoSide)
      ctx->_TriangleCaps |= DD_TRI_TWOSTENCIL;
}
#endif


/**
 * Compute derived GL state.
 * If __GLcontextRec::NewState is non-zero then this function \b must
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
_mesa_update_state_locked( GLcontext *ctx )
{
   GLbitfield new_state = ctx->NewState;
   GLbitfield prog_flags = _NEW_PROGRAM;
   GLbitfield new_prog_state = 0x0;

   if (MESA_VERBOSE & VERBOSE_STATE)
      _mesa_print_state("_mesa_update_state", new_state);

   /* Determine which state flags effect vertex/fragment program state */
   if (ctx->FragmentProgram._MaintainTexEnvProgram) {
      prog_flags |= (_NEW_TEXTURE | _NEW_FOG | _DD_NEW_SEPARATE_SPECULAR);
   }
   if (ctx->VertexProgram._MaintainTnlProgram) {
      prog_flags |= (_NEW_ARRAY | _NEW_TEXTURE | _NEW_TEXTURE_MATRIX |
                     _NEW_TRANSFORM | _NEW_POINT |
                     _NEW_FOG | _NEW_LIGHT |
                     _MESA_NEW_NEED_EYE_COORDS);
   }

   /*
    * Now update derived state info
    */

   if (new_state & prog_flags)
      update_program_enables( ctx );

   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      _mesa_update_modelview_project( ctx, new_state );

   if (new_state & (_NEW_PROGRAM|_NEW_TEXTURE|_NEW_TEXTURE_MATRIX))
      _mesa_update_texture( ctx, new_state );

   if (new_state & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL))
      _mesa_update_framebuffer(ctx);

   if (new_state & (_NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT))
      _mesa_update_draw_buffer_bounds( ctx );

   if (new_state & _NEW_POLYGON)
      update_polygon( ctx );

   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   if (new_state & _NEW_STENCIL)
      _mesa_update_stencil( ctx );

#if FEATURE_pixel_transfer
   if (new_state & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_pixel( ctx, new_state );
#endif

   if (new_state & _DD_NEW_SEPARATE_SPECULAR)
      update_separate_specular( ctx );

   if (new_state & (_NEW_ARRAY | _NEW_PROGRAM))
      update_arrays( ctx );

   if (new_state & (_NEW_BUFFERS | _NEW_VIEWPORT))
      update_viewport_matrix(ctx);

   if (new_state & _NEW_MULTISAMPLE)
      update_multisample( ctx );

   if (new_state & _NEW_COLOR)
      update_color( ctx );

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

   if (new_state & prog_flags) {
      /* When we generate programs from fixed-function vertex/fragment state
       * this call may generate/bind a new program.  If so, we need to
       * propogate the _NEW_PROGRAM flag to the driver.
       */
      new_prog_state |= update_program( ctx );
   }

   /*
    * Give the driver a chance to act upon the new_state flags.
    * The driver might plug in different span functions, for example.
    * Also, this is where the driver can invalidate the state of any
    * active modules (such as swrast_setup, swrast, tnl, etc).
    *
    * Set ctx->NewState to zero to avoid recursion if
    * Driver.UpdateState() has to call FLUSH_VERTICES().  (fixed?)
    */
   new_state = ctx->NewState | new_prog_state;
   ctx->NewState = 0;
   ctx->Driver.UpdateState(ctx, new_state);
   ctx->Array.NewState = 0;
}


/* This is the usual entrypoint for state updates:
 */
void
_mesa_update_state( GLcontext *ctx )
{
   _mesa_lock_context_textures(ctx);
   _mesa_update_state_locked(ctx);
   _mesa_unlock_context_textures(ctx);
}
