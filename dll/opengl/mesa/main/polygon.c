/**
 * \file polygon.c
 * Polygon operations.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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

#include <precomp.h>

/**
 * Specify whether to cull front- or back-facing facets.
 *
 * \param mode culling mode.
 *
 * \sa glCullFace().
 *
 * Verifies the parameter and updates gl_polygon_attrib::CullFaceMode. On
 * change, flushes the vertices and notifies the driver via
 * the dd_function_table::CullFace callback.
 */
void GLAPIENTRY
_mesa_CullFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glCullFace %s\n", _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_FRONT && mode!=GL_BACK && mode!=GL_FRONT_AND_BACK) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glCullFace" );
      return;
   }

   if (ctx->Polygon.CullFaceMode == mode)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.CullFaceMode = mode;

   if (ctx->Driver.CullFace)
      ctx->Driver.CullFace( ctx, mode );
}


/**
 * Define front- and back-facing 
 *
 * \param mode orientation of front-facing polygons.
 *
 * \sa glFrontFace().
 *
 * Verifies the parameter and updates gl_polygon_attrib::FrontFace. On change
 * flushes the vertices and notifies the driver via
 * the dd_function_table::FrontFace callback.
 */
void GLAPIENTRY
_mesa_FrontFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glFrontFace %s\n", _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_CW && mode!=GL_CCW) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glFrontFace" );
      return;
   }

   if (ctx->Polygon.FrontFace == mode)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.FrontFace = mode;

   ctx->Polygon._FrontBit = (GLboolean) (mode == GL_CW);

   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace( ctx, mode );
}


/**
 * Set the polygon rasterization mode.
 *
 * \param face the polygons which \p mode applies to.
 * \param mode how polygons should be rasterized.
 *
 * \sa glPolygonMode(). 
 * 
 * Verifies the parameters and updates gl_polygon_attrib::FrontMode and
 * gl_polygon_attrib::BackMode. On change flushes the vertices and notifies the
 * driver via the dd_function_table::PolygonMode callback.
 */
void GLAPIENTRY
_mesa_PolygonMode( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonMode %s %s\n",
                  _mesa_lookup_enum_by_nr(face),
                  _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_POINT && mode!=GL_LINE && mode!=GL_FILL) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glPolygonMode(mode)" );
      return;
   }

   switch (face) {
   case GL_FRONT:
      if (ctx->Polygon.FrontMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.FrontMode = mode;
      break;
   case GL_FRONT_AND_BACK:
      if (ctx->Polygon.FrontMode == mode &&
	  ctx->Polygon.BackMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.FrontMode = mode;
      ctx->Polygon.BackMode = mode;
      break;
   case GL_BACK:
      if (ctx->Polygon.BackMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.BackMode = mode;
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glPolygonMode(face)" );
      return;
   }

   if (ctx->Polygon.FrontMode == GL_FILL && ctx->Polygon.BackMode == GL_FILL)
      ctx->_TriangleCaps &= ~DD_TRI_UNFILLED;
   else
      ctx->_TriangleCaps |= DD_TRI_UNFILLED;

   if (ctx->Driver.PolygonMode)
      ctx->Driver.PolygonMode(ctx, face, mode);
}

#if _HAVE_FULL_GL


/**
 * This routine updates the ctx->Polygon.Stipple state.
 * If we're getting the stipple data from a PBO, we map the buffer
 * in order to access the data.
 * In any case, we obey the current pixel unpacking parameters when fetching
 * the stipple data.
 *
 * In the future, this routine should be used as a fallback, called via
 * ctx->Driver.PolygonStipple().  We'll have to update all the DRI drivers
 * too.
 */
void
_mesa_polygon_stipple(struct gl_context *ctx, const GLubyte *pattern)
{
   if (!pattern)
      return;

   _mesa_unpack_polygon_stipple(pattern, ctx->PolygonStipple, &ctx->Unpack);
}


/**
 * Called by glPolygonStipple.
 */
void GLAPIENTRY
_mesa_PolygonStipple( const GLubyte *pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonStipple\n");

   FLUSH_VERTICES(ctx, _NEW_POLYGONSTIPPLE);

   _mesa_polygon_stipple(ctx, pattern);

   if (ctx->Driver.PolygonStipple)
      ctx->Driver.PolygonStipple(ctx, pattern);
}


/**
 * Called by glPolygonStipple.
 */
void GLAPIENTRY
_mesa_GetPolygonStipple( GLubyte *dest )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glGetPolygonStipple\n");
   if (!dest)
      return;

   _mesa_pack_polygon_stipple(ctx->PolygonStipple, dest, &ctx->Pack);
}


void GLAPIENTRY
_mesa_PolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonOffset %f %f\n", factor, units);

   if (ctx->Polygon.OffsetFactor == factor &&
       ctx->Polygon.OffsetUnits == units)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.OffsetFactor = factor;
   ctx->Polygon.OffsetUnits = units;

   if (ctx->Driver.PolygonOffset)
      ctx->Driver.PolygonOffset( ctx, factor, units );
}


void GLAPIENTRY
_mesa_PolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
   GET_CURRENT_CONTEXT(ctx);
   /* XXX mult by DepthMaxF here??? */
   _mesa_PolygonOffset(factor, bias * ctx->DrawBuffer->_DepthMaxF );
}

#endif


/**********************************************************************/
/** \name Initialization */
/*@{*/

/**
 * Initialize the context polygon state.
 *
 * \param ctx GL context.
 *
 * Initializes __struct gl_contextRec::Polygon and __struct gl_contextRec::PolygonStipple
 * attribute groups.
 */
void _mesa_init_polygon( struct gl_context * ctx )
{
   /* Polygon group */
   ctx->Polygon.CullFlag = GL_FALSE;
   ctx->Polygon.CullFaceMode = GL_BACK;
   ctx->Polygon.FrontFace = GL_CCW;
   ctx->Polygon._FrontBit = 0;
   ctx->Polygon.FrontMode = GL_FILL;
   ctx->Polygon.BackMode = GL_FILL;
   ctx->Polygon.SmoothFlag = GL_FALSE;
   ctx->Polygon.StippleFlag = GL_FALSE;
   ctx->Polygon.OffsetFactor = 0.0F;
   ctx->Polygon.OffsetUnits = 0.0F;
   ctx->Polygon.OffsetPoint = GL_FALSE;
   ctx->Polygon.OffsetLine = GL_FALSE;
   ctx->Polygon.OffsetFill = GL_FALSE;


   /* Polygon Stipple group */
   memset( ctx->PolygonStipple, 0xff, 32*sizeof(GLuint) );
}

/*@}*/
