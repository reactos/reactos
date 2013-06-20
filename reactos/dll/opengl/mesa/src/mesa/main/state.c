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


#include "glheader.h"
#include "mtypes.h"
#include "context.h"
#include "debug.h"
#include "macros.h"
#include "ffvertex_prog.h"
#include "framebuffer.h"
#include "light.h"
#include "matrix.h"
#include "pixel.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "shaderobj.h"
#include "state.h"
#include "stencil.h"
#include "texenvprogram.h"
#include "texobj.h"
#include "texstate.h"
#include "varray.h"


static void
update_separate_specular(struct gl_context *ctx)
{
   if (_mesa_need_secondary_color(ctx))
      ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
   else
      ctx->_TriangleCaps &= ~DD_SEPARATE_SPECULAR;
}


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
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   GLuint i, min = ~0;

   /* find min of _MaxElement values for all enabled arrays.
    * Note that the generic arrays always take precedence over
    * the legacy arrays.
    */

   /* 0 */
   if (ctx->VertexProgram._Current
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC0].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC0]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_POS]);
   }

   /* 1 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC1].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC1]);
   }
   /* no conventional vertex weight array */

   /* 2 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC2].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC2]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_NORMAL]);
   }

   /* 3 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC3].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC3]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR0]);
   }

   /* 4 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC4].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC4]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_COLOR1].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR1]);
   }

   /* 5 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC5].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC5]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_FOG].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_FOG]);
   }

   /* 6 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC6].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC6]);
   }
   else if (arrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX]);
   }

   /* 7 */
   if (ctx->VertexProgram._Enabled
       && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC7].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC7]);
   }

   /* 8..15 */
   for (i = 0; i < VERT_ATTRIB_TEX_MAX; i++) {
      if (ctx->VertexProgram._Enabled
          && arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC8 + i].Enabled) {
         min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC8 + i]);
      }
      else if (i < ctx->Const.MaxTextureCoordUnits
               && arrayObj->VertexAttrib[VERT_ATTRIB_TEX(i)].Enabled) {
         min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_TEX(i)]);
      }
   }

   /* 16..31 */
   if (ctx->VertexProgram._Current) {
      for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++) {
         if (arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled) {
            min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(i)]);
         }
      }
   }

   if (arrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled) {
      min = update_min(min, &arrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG]);
   }

   /* _MaxElement is one past the last legal array element */
   arrayObj->_MaxElement = min;
}


/**
 * Update the following fields:
 *   ctx->VertexProgram._Enabled
 *   ctx->FragmentProgram._Enabled
 *   ctx->ATIFragmentShader._Enabled
 * This needs to be done before texture state validation.
 */
static void
update_program_enables(struct gl_context *ctx)
{
   /* These _Enabled flags indicate if the user-defined ARB/NV vertex/fragment
    * program is enabled AND valid.  Similarly for ATI fragment shaders.
    * GLSL shaders not relevant here.
    */
   ctx->VertexProgram._Enabled = ctx->VertexProgram.Enabled
      && ctx->VertexProgram.Current->Base.Instructions;
   ctx->FragmentProgram._Enabled = ctx->FragmentProgram.Enabled
      && ctx->FragmentProgram.Current->Base.Instructions;
   ctx->ATIFragmentShader._Enabled = ctx->ATIFragmentShader.Enabled
      && ctx->ATIFragmentShader.Current->Instructions[0];
}


/**
 * Update the ctx->Vertex/Geometry/FragmentProgram._Current pointers to point
 * to the current/active programs.  Then call ctx->Driver.BindProgram() to
 * tell the driver which programs to use.
 *
 * Programs may come from 3 sources: GLSL shaders, ARB/NV_vertex/fragment
 * programs or programs derived from fixed-function state.
 *
 * This function needs to be called after texture state validation in case
 * we're generating a fragment program from fixed-function texture state.
 *
 * \return bitfield which will indicate _NEW_PROGRAM state if a new vertex
 * or fragment program is being used.
 */
static GLbitfield
update_program(struct gl_context *ctx)
{
   const struct gl_shader_program *vsProg = ctx->Shader.CurrentVertexProgram;
   const struct gl_shader_program *gsProg = ctx->Shader.CurrentGeometryProgram;
   struct gl_shader_program *fsProg = ctx->Shader.CurrentFragmentProgram;
   const struct gl_vertex_program *prevVP = ctx->VertexProgram._Current;
   const struct gl_fragment_program *prevFP = ctx->FragmentProgram._Current;
   const struct gl_geometry_program *prevGP = ctx->GeometryProgram._Current;
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

   if (fsProg && fsProg->LinkStatus
       && fsProg->_LinkedShaders[MESA_SHADER_FRAGMENT]) {
      /* Use GLSL fragment shader */
      _mesa_reference_shader_program(ctx,
				     &ctx->Shader._CurrentFragmentProgram,
				     fsProg);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
			       (struct gl_fragment_program *)
			       fsProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram,
			       NULL);
   }
   else if (ctx->FragmentProgram._Enabled) {
      /* Use user-defined fragment program */
      _mesa_reference_shader_program(ctx,
				     &ctx->Shader._CurrentFragmentProgram,
				     NULL);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
                               ctx->FragmentProgram.Current);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram,
			       NULL);
   }
   else if (ctx->FragmentProgram._MaintainTexEnvProgram) {
      /* Use fragment program generated from fixed-function state */
      struct gl_shader_program *f = _mesa_get_fixed_func_fragment_program(ctx);

      _mesa_reference_shader_program(ctx,
				     &ctx->Shader._CurrentFragmentProgram,
				     f);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current,
			       (struct gl_fragment_program *)
                               f->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram,
			       (struct gl_fragment_program *)
                               f->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program);
   }
   else {
      /* No fragment program */
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, NULL);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram,
			       NULL);
   }

   if (gsProg && gsProg->LinkStatus
       && gsProg->_LinkedShaders[MESA_SHADER_GEOMETRY]) {
      /* Use GLSL geometry shader */
      _mesa_reference_geomprog(ctx, &ctx->GeometryProgram._Current,
			       (struct gl_geometry_program *)
			       gsProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program);
   } else {
      /* No geometry program */
      _mesa_reference_geomprog(ctx, &ctx->GeometryProgram._Current, NULL);
   }

   /* Examine vertex program after fragment program as
    * _mesa_get_fixed_func_vertex_program() needs to know active
    * fragprog inputs.
    */
   if (vsProg && vsProg->LinkStatus
       && vsProg->_LinkedShaders[MESA_SHADER_VERTEX]) {
      /* Use GLSL vertex shader */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
			       (struct gl_vertex_program *)
			       vsProg->_LinkedShaders[MESA_SHADER_VERTEX]->Program);
   }
   else if (ctx->VertexProgram._Enabled) {
      /* Use user-defined vertex program */
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current,
                               ctx->VertexProgram.Current);
   }
   else if (ctx->VertexProgram._MaintainTnlProgram) {
      /* Use vertex program generated from fixed-function state */
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

   if (ctx->GeometryProgram._Current != prevGP) {
      new_state |= _NEW_PROGRAM;
      if (ctx->Driver.BindProgram) {
         ctx->Driver.BindProgram(ctx, MESA_GEOMETRY_PROGRAM,
                            (struct gl_program *) ctx->GeometryProgram._Current);
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


/**
 * Examine shader constants and return either _NEW_PROGRAM_CONSTANTS or 0.
 */
static GLbitfield
update_program_constants(struct gl_context *ctx)
{
   GLbitfield new_state = 0x0;

   if (ctx->FragmentProgram._Current) {
      const struct gl_program_parameter_list *params =
         ctx->FragmentProgram._Current->Base.Parameters;
      if (params && params->StateFlags & ctx->NewState) {
         new_state |= _NEW_PROGRAM_CONSTANTS;
      }
   }

   if (ctx->GeometryProgram._Current) {
      const struct gl_program_parameter_list *params =
         ctx->GeometryProgram._Current->Base.Parameters;
      /*FIXME: StateFlags is always 0 because we have unnamed constant
       *       not state changes */
      if (params /*&& params->StateFlags & ctx->NewState*/) {
         new_state |= _NEW_PROGRAM_CONSTANTS;
      }
   }

   if (ctx->VertexProgram._Current) {
      const struct gl_program_parameter_list *params =
         ctx->VertexProgram._Current->Base.Parameters;
      if (params && params->StateFlags & ctx->NewState) {
         new_state |= _NEW_PROGRAM_CONSTANTS;
      }
   }

   return new_state;
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


/**
 * Update derived multisample state.
 */
static void
update_multisample(struct gl_context *ctx)
{
   ctx->Multisample._Enabled = GL_FALSE;
   if (ctx->Multisample.Enabled &&
       ctx->DrawBuffer &&
       ctx->DrawBuffer->Visual.sampleBuffers)
      ctx->Multisample._Enabled = GL_TRUE;
}


/**
 * Update the ctx->Color._ClampFragmentColor field
 */
static void
update_clamp_fragment_color(struct gl_context *ctx)
{
   if (ctx->Color.ClampFragmentColor == GL_FIXED_ONLY_ARB)
      ctx->Color._ClampFragmentColor =
         !ctx->DrawBuffer || !ctx->DrawBuffer->Visual.floatMode;
   else
      ctx->Color._ClampFragmentColor = ctx->Color.ClampFragmentColor;
}


/**
 * Update the ctx->Color._ClampVertexColor field
 */
static void
update_clamp_vertex_color(struct gl_context *ctx)
{
   if (ctx->Light.ClampVertexColor == GL_FIXED_ONLY_ARB)
      ctx->Light._ClampVertexColor =
         !ctx->DrawBuffer || !ctx->DrawBuffer->Visual.floatMode;
   else
      ctx->Light._ClampVertexColor = ctx->Light.ClampVertexColor;
}


/**
 * Update the ctx->Color._ClampReadColor field
 */
static void
update_clamp_read_color(struct gl_context *ctx)
{
   if (ctx->Color.ClampReadColor == GL_FIXED_ONLY_ARB)
      ctx->Color._ClampReadColor =
         !ctx->ReadBuffer || !ctx->ReadBuffer->Visual.floatMode;
   else
      ctx->Color._ClampReadColor = ctx->Color.ClampReadColor;
}

/**
 * Update the ctx->VertexProgram._TwoSideEnabled flag.
 */
static void
update_twoside(struct gl_context *ctx)
{
   if (ctx->Shader.CurrentVertexProgram ||
       ctx->VertexProgram._Enabled) {
      ctx->VertexProgram._TwoSideEnabled = ctx->VertexProgram.TwoSideEnabled;
   } else {
      ctx->VertexProgram._TwoSideEnabled = (ctx->Light.Enabled &&
					    ctx->Light.Model.TwoSide);
   }
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

   /*
    * Stencil
    */
   if (ctx->Stencil._TestTwoSide)
      ctx->_TriangleCaps |= DD_TRI_TWOSTENCIL;
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
   GLbitfield prog_flags = _NEW_PROGRAM;
   GLbitfield new_prog_state = 0x0;

   if (new_state == _NEW_CURRENT_ATTRIB) 
      goto out;

   if (MESA_VERBOSE & VERBOSE_STATE)
      _mesa_print_state("_mesa_update_state", new_state);

   /* Determine which state flags effect vertex/fragment program state */
   if (ctx->FragmentProgram._MaintainTexEnvProgram) {
      prog_flags |= (_NEW_BUFFERS | _NEW_TEXTURE | _NEW_FOG |
		     _NEW_ARRAY | _NEW_LIGHT | _NEW_POINT | _NEW_RENDERMODE |
		     _NEW_PROGRAM | _NEW_FRAG_CLAMP | _NEW_COLOR);
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

   if (new_state & _NEW_BUFFERS)
      _mesa_update_framebuffer(ctx);

   if (new_state & (_NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT))
      _mesa_update_draw_buffer_bounds( ctx );

   if (new_state & _NEW_POLYGON)
      update_polygon( ctx );

   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   if (new_state & (_NEW_LIGHT | _NEW_PROGRAM))
      update_twoside( ctx );

   if (new_state & (_NEW_LIGHT | _NEW_BUFFERS))
      update_clamp_vertex_color(ctx);

   if (new_state & (_NEW_STENCIL | _NEW_BUFFERS))
      _mesa_update_stencil( ctx );

   if (new_state & _NEW_PIXEL)
      _mesa_update_pixel( ctx, new_state );

   if (new_state & _DD_NEW_SEPARATE_SPECULAR)
      update_separate_specular( ctx );

   if (new_state & (_NEW_BUFFERS | _NEW_VIEWPORT))
      update_viewport_matrix(ctx);

   if (new_state & (_NEW_MULTISAMPLE | _NEW_BUFFERS))
      update_multisample( ctx );

   if (new_state & (_NEW_COLOR | _NEW_BUFFERS))
      update_clamp_read_color(ctx);

   if(new_state & (_NEW_FRAG_CLAMP | _NEW_BUFFERS))
      update_clamp_fragment_color(ctx);

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

   if (new_state & (_NEW_ARRAY | _NEW_PROGRAM | _NEW_BUFFER_OBJECT))
      update_arrays( ctx );

 out:
   new_prog_state |= update_program_constants(ctx);

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
   if (!ctx->Array.RebindArrays)
      ctx->Array.RebindArrays = (new_state & (_NEW_ARRAY | _NEW_PROGRAM)) != 0;
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




/**
 * Want to figure out which fragment program inputs are actually
 * constant/current values from ctx->Current.  These should be
 * referenced as a tracked state variable rather than a fragment
 * program input, to save the overhead of putting a constant value in
 * every submitted vertex, transferring it to hardware, interpolating
 * it across the triangle, etc...
 *
 * When there is a VP bound, just use vp->outputs.  But when we're
 * generating vp from fixed function state, basically want to
 * calculate:
 *
 * vp_out_2_fp_in( vp_in_2_vp_out( varying_inputs ) | 
 *                 potential_vp_outputs )
 *
 * Where potential_vp_outputs is calculated by looking at enabled
 * texgen, etc.
 * 
 * The generated fragment program should then only declare inputs that
 * may vary or otherwise differ from the ctx->Current values.
 * Otherwise, the fp should track them as state values instead.
 */
void
_mesa_set_varying_vp_inputs( struct gl_context *ctx,
                             GLbitfield64 varying_inputs )
{
   if (ctx->varying_vp_inputs != varying_inputs) {
      ctx->varying_vp_inputs = varying_inputs;
      ctx->NewState |= _NEW_ARRAY;
      /*printf("%s %x\n", __FUNCTION__, varying_inputs);*/
   }
}


/**
 * Used by drivers to tell core Mesa that the driver is going to
 * install/ use its own vertex program.  In particular, this will
 * prevent generated fragment programs from using state vars instead
 * of ordinary varyings/inputs.
 */
void
_mesa_set_vp_override(struct gl_context *ctx, GLboolean flag)
{
   if (ctx->VertexProgram._Overriden != flag) {
      ctx->VertexProgram._Overriden = flag;

      /* Set one of the bits which will trigger fragment program
       * regeneration:
       */
      ctx->NewState |= _NEW_PROGRAM;
   }
}
