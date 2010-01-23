/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * \file matrix.c
 * Matrix operations.
 *
 * \note
 * -# 4x4 transformation matrices are stored in memory in column major order.
 * -# Points/vertices are to be thought of as column vectors.
 * -# Transformation of a point p by a matrix M is: p' = M * p
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "matrix.h"
#include "mtypes.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"


/**
 * Apply a perspective projection matrix.
 *
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * \sa glFrustum().
 *
 * Flushes vertices and validates parameters. Calls _math_matrix_frustum() with
 * the top matrix of the current matrix stack and sets
 * __GLcontextRec::NewState.
 */
void GLAPIENTRY
_mesa_Frustum( GLdouble left, GLdouble right,
               GLdouble bottom, GLdouble top,
               GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (nearval <= 0.0 ||
       farval <= 0.0 ||
       nearval == farval ||
       left == right ||
       top == bottom)
   {
      _mesa_error( ctx,  GL_INVALID_VALUE, "glFrustum" );
      return;
   }

   _math_matrix_frustum( ctx->CurrentStack->Top,
                         (GLfloat) left, (GLfloat) right, 
			 (GLfloat) bottom, (GLfloat) top, 
			 (GLfloat) nearval, (GLfloat) farval );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Apply an orthographic projection matrix.
 *
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * \sa glOrtho().
 *
 * Flushes vertices and validates parameters. Calls _math_matrix_ortho() with
 * the top matrix of the current matrix stack and sets
 * __GLcontextRec::NewState.
 */
void GLAPIENTRY
_mesa_Ortho( GLdouble left, GLdouble right,
             GLdouble bottom, GLdouble top,
             GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glOrtho(%f, %f, %f, %f, %f, %f)\n",
                  left, right, bottom, top, nearval, farval);

   if (left == right ||
       bottom == top ||
       nearval == farval)
   {
      _mesa_error( ctx,  GL_INVALID_VALUE, "glOrtho" );
      return;
   }

   _math_matrix_ortho( ctx->CurrentStack->Top,
                       (GLfloat) left, (GLfloat) right, 
		       (GLfloat) bottom, (GLfloat) top, 
		       (GLfloat) nearval, (GLfloat) farval );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Set the current matrix stack.
 *
 * \param mode matrix stack.
 *
 * \sa glMatrixMode().
 *
 * Flushes the vertices, validates the parameter and updates
 * __GLcontextRec::CurrentStack and gl_transform_attrib::MatrixMode with the
 * specified matrix stack.
 */
void GLAPIENTRY
_mesa_MatrixMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Transform.MatrixMode == mode && mode != GL_TEXTURE)
      return;
   FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

   switch (mode) {
   case GL_MODELVIEW:
      ctx->CurrentStack = &ctx->ModelviewMatrixStack;
      break;
   case GL_PROJECTION:
      ctx->CurrentStack = &ctx->ProjectionMatrixStack;
      break;
   case GL_TEXTURE:
      if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glMatrixMode(texcoord unit)");
         return;
      }
      ctx->CurrentStack = &ctx->TextureMatrixStack[ctx->Texture.CurrentUnit];
      break;
   case GL_COLOR:
      ctx->CurrentStack = &ctx->ColorMatrixStack;
      break;
   case GL_MATRIX0_NV:
   case GL_MATRIX1_NV:
   case GL_MATRIX2_NV:
   case GL_MATRIX3_NV:
   case GL_MATRIX4_NV:
   case GL_MATRIX5_NV:
   case GL_MATRIX6_NV:
   case GL_MATRIX7_NV:
      if (ctx->Extensions.NV_vertex_program) {
         ctx->CurrentStack = &ctx->ProgramMatrixStack[mode - GL_MATRIX0_NV];
      }
      else {
         _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
         return;
      }
      break;
   case GL_MATRIX0_ARB:
   case GL_MATRIX1_ARB:
   case GL_MATRIX2_ARB:
   case GL_MATRIX3_ARB:
   case GL_MATRIX4_ARB:
   case GL_MATRIX5_ARB:
   case GL_MATRIX6_ARB:
   case GL_MATRIX7_ARB:
      if (ctx->Extensions.ARB_vertex_program ||
          ctx->Extensions.ARB_fragment_program) {
         const GLuint m = mode - GL_MATRIX0_ARB;
         if (m > ctx->Const.MaxProgramMatrices) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glMatrixMode(GL_MATRIX%d_ARB)", m);
            return;
         }
         ctx->CurrentStack = &ctx->ProgramMatrixStack[m];
      }
      else {
         _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
         return;
      }
      break;
   default:
      _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
      return;
   }

   ctx->Transform.MatrixMode = mode;
}


/**
 * Push the current matrix stack.
 *
 * \sa glPushMatrix().
 * 
 * Verifies the current matrix stack is not full, and duplicates the top-most
 * matrix in the stack. Marks __GLcontextRec::NewState with the stack dirty
 * flag.
 */
void GLAPIENTRY
_mesa_PushMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_matrix_stack *stack = ctx->CurrentStack;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPushMatrix %s\n",
                  _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   if (stack->Depth + 1 >= stack->MaxDepth) {
      if (ctx->Transform.MatrixMode == GL_TEXTURE) {
         _mesa_error(ctx,  GL_STACK_OVERFLOW,
                     "glPushMatrix(mode=GL_TEXTURE, unit=%d)",
                      ctx->Texture.CurrentUnit);
      }
      else {
         _mesa_error(ctx,  GL_STACK_OVERFLOW, "glPushMatrix(mode=%s)",
                     _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
      }
      return;
   }
   _math_matrix_copy( &stack->Stack[stack->Depth + 1],
                      &stack->Stack[stack->Depth] );
   stack->Depth++;
   stack->Top = &(stack->Stack[stack->Depth]);
   ctx->NewState |= stack->DirtyFlag;
}


/**
 * Pop the current matrix stack.
 *
 * \sa glPopMatrix().
 * 
 * Flushes the vertices, verifies the current matrix stack is not empty, and
 * moves the stack head down. Marks __GLcontextRec::NewState with the dirty
 * stack flag.
 */
void GLAPIENTRY
_mesa_PopMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_matrix_stack *stack = ctx->CurrentStack;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPopMatrix %s\n",
                  _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   if (stack->Depth == 0) {
      if (ctx->Transform.MatrixMode == GL_TEXTURE) {
         _mesa_error(ctx,  GL_STACK_UNDERFLOW,
                     "glPopMatrix(mode=GL_TEXTURE, unit=%d)",
                      ctx->Texture.CurrentUnit);
      }
      else {
         _mesa_error(ctx,  GL_STACK_UNDERFLOW, "glPopMatrix(mode=%s)",
                     _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
      }
      return;
   }
   stack->Depth--;
   stack->Top = &(stack->Stack[stack->Depth]);
   ctx->NewState |= stack->DirtyFlag;
}


/**
 * Replace the current matrix with the identity matrix.
 *
 * \sa glLoadIdentity().
 *
 * Flushes the vertices and calls _math_matrix_set_identity() with the top-most
 * matrix in the current stack. Marks __GLcontextRec::NewState with the stack
 * dirty flag.
 */
void GLAPIENTRY
_mesa_LoadIdentity( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glLoadIdentity()");

   _math_matrix_set_identity( ctx->CurrentStack->Top );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Replace the current matrix with a given matrix.
 *
 * \param m matrix.
 *
 * \sa glLoadMatrixf().
 *
 * Flushes the vertices and calls _math_matrix_loadf() with the top-most matrix
 * in the current stack and the given matrix. Marks __GLcontextRec::NewState
 * with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_LoadMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   if (!m) return;
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
          "glLoadMatrix(%f %f %f %f, %f %f %f %f, %f %f %f %f, %f %f %f %f\n",
          m[0], m[4], m[8], m[12],
          m[1], m[5], m[9], m[13],
          m[2], m[6], m[10], m[14],
          m[3], m[7], m[11], m[15]);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_loadf( ctx->CurrentStack->Top, m );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Multiply the current matrix with a given matrix.
 *
 * \param m matrix.
 *
 * \sa glMultMatrixf().
 *
 * Flushes the vertices and calls _math_matrix_mul_floats() with the top-most
 * matrix in the current stack and the given matrix. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_MultMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   if (!m) return;
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
          "glMultMatrix(%f %f %f %f, %f %f %f %f, %f %f %f %f, %f %f %f %f\n",
          m[0], m[4], m[8], m[12],
          m[1], m[5], m[9], m[13],
          m[2], m[6], m[10], m[14],
          m[3], m[7], m[11], m[15]);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_mul_floats( ctx->CurrentStack->Top, m );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Multiply the current matrix with a rotation matrix.
 *
 * \param angle angle of rotation, in degrees.
 * \param x rotation vector x coordinate.
 * \param y rotation vector y coordinate.
 * \param z rotation vector z coordinate.
 *
 * \sa glRotatef().
 *
 * Flushes the vertices and calls _math_matrix_rotate() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (angle != 0.0F) {
      _math_matrix_rotate( ctx->CurrentStack->Top, angle, x, y, z);
      ctx->NewState |= ctx->CurrentStack->DirtyFlag;
   }
}


/**
 * Multiply the current matrix with a general scaling matrix.
 *
 * \param x x axis scale factor.
 * \param y y axis scale factor.
 * \param z z axis scale factor.
 *
 * \sa glScalef().
 *
 * Flushes the vertices and calls _math_matrix_scale() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_scale( ctx->CurrentStack->Top, x, y, z);
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Multiply the current matrix with a translation matrix.
 *
 * \param x translation vector x coordinate.
 * \param y translation vector y coordinate.
 * \param z translation vector z coordinate.
 *
 * \sa glTranslatef().
 *
 * Flushes the vertices and calls _math_matrix_translate() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_translate( ctx->CurrentStack->Top, x, y, z);
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}

 
#if _HAVE_FULL_GL
void GLAPIENTRY
_mesa_LoadMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   if (!m) return;
   for (i = 0; i < 16; i++)
      f[i] = (GLfloat) m[i];
   _mesa_LoadMatrixf(f);
}

void GLAPIENTRY
_mesa_MultMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   if (!m) return;
   for (i = 0; i < 16; i++)
      f[i] = (GLfloat) m[i];
   _mesa_MultMatrixf( f );
}


void GLAPIENTRY
_mesa_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Rotatef((GLfloat) angle, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}


void GLAPIENTRY
_mesa_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Scalef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}


void GLAPIENTRY
_mesa_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Translatef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}
#endif


#if _HAVE_FULL_GL
void GLAPIENTRY
_mesa_LoadTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposef(tm, m);
   _mesa_LoadMatrixf(tm);
}


void GLAPIENTRY
_mesa_LoadTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposefd(tm, m);
   _mesa_LoadMatrixf(tm);
}


void GLAPIENTRY
_mesa_MultTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposef(tm, m);
   _mesa_MultMatrixf(tm);
}


void GLAPIENTRY
_mesa_MultTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposefd(tm, m);
   _mesa_MultMatrixf(tm);
}
#endif

/**
 * Set the viewport.
 * 
 * \param x, y coordinates of the lower-left corner of the viewport rectangle.
 * \param width width of the viewport rectangle.
 * \param height height of the viewport rectangle.
 *
 * \sa Called via glViewport() or display list execution.
 *
 * Flushes the vertices and calls _mesa_set_viewport() with the given
 * parameters.
 */
void GLAPIENTRY
_mesa_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_set_viewport(ctx, x, y, width, height);
}


/**
 * Set new viewport parameters and update derived state (the _WindowMap
 * matrix).  Usually called from _mesa_Viewport().
 * 
 * \param ctx GL context.
 * \param x, y coordinates of the lower left corner of the viewport rectangle.
 * \param width width of the viewport rectangle.
 * \param height height of the viewport rectangle.
 */
void
_mesa_set_viewport( GLcontext *ctx, GLint x, GLint y,
                    GLsizei width, GLsizei height )
{
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewport %d %d %d %d\n", x, y, width, height);

   if (width < 0 || height < 0) {
      _mesa_error( ctx,  GL_INVALID_VALUE,
                   "glViewport(%d, %d, %d, %d)", x, y, width, height );
      return;
   }

   /* clamp width and height to the implementation dependent range */
   width  = CLAMP(width,  1, (GLsizei) ctx->Const.MaxViewportWidth);
   height = CLAMP(height, 1, (GLsizei) ctx->Const.MaxViewportHeight);

   ctx->Viewport.X = x;
   ctx->Viewport.Width = width;
   ctx->Viewport.Y = y;
   ctx->Viewport.Height = height;
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif

   if (ctx->Driver.Viewport) {
      /* Many drivers will use this call to check for window size changes
       * and reallocate the z/stencil/accum/etc buffers if needed.
       */
      (*ctx->Driver.Viewport)( ctx, x, y, width, height );
   }
}


#if _HAVE_FULL_GL
/**
 * Called by glDepthRange
 *
 * \param nearval  specifies the Z buffer value which should correspond to
 *                 the near clip plane
 * \param farval  specifies the Z buffer value which should correspond to
 *                the far clip plane
 */
void GLAPIENTRY
_mesa_DepthRange( GLclampd nearval, GLclampd farval )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glDepthRange %f %f\n", nearval, farval);

   ctx->Viewport.Near = (GLfloat) CLAMP( nearval, 0.0, 1.0 );
   ctx->Viewport.Far = (GLfloat) CLAMP( farval, 0.0, 1.0 );
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif

   if (ctx->Driver.DepthRange) {
      (*ctx->Driver.DepthRange)( ctx, nearval, farval );
   }
}
#endif



/**********************************************************************/
/** \name State management */
/*@{*/


/**
 * Update the projection matrix stack.
 *
 * \param ctx GL context.
 *
 * Calls _math_matrix_analyse() with the top-matrix of the projection matrix
 * stack, and recomputes user clip positions if necessary.
 * 
 * \note This routine references __GLcontextRec::Tranform attribute values to
 * compute userclip positions in clip space, but is only called on
 * _NEW_PROJECTION.  The _mesa_ClipPlane() function keeps these values up to
 * date across changes to the __GLcontextRec::Transform attributes.
 */
static void
update_projection( GLcontext *ctx )
{
   _math_matrix_analyse( ctx->ProjectionMatrixStack.Top );

#if FEATURE_userclip
   /* Recompute clip plane positions in clipspace.  This is also done
    * in _mesa_ClipPlane().
    */
   if (ctx->Transform.ClipPlanesEnabled) {
      GLuint p;
      for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
	 if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	    _mesa_transform_vector( ctx->Transform._ClipUserPlane[p],
				 ctx->Transform.EyeUserPlane[p],
				 ctx->ProjectionMatrixStack.Top->inv );
	 }
      }
   }
#endif
}


/**
 * Calculate the combined modelview-projection matrix.
 *
 * \param ctx GL context.
 *
 * Multiplies the top matrices of the projection and model view stacks into
 * __GLcontextRec::_ModelProjectMatrix via _math_matrix_mul_matrix() and
 * analyzes the resulting matrix via _math_matrix_analyse().
 */
static void
calculate_model_project_matrix( GLcontext *ctx )
{
   _math_matrix_mul_matrix( &ctx->_ModelProjectMatrix,
                            ctx->ProjectionMatrixStack.Top,
                            ctx->ModelviewMatrixStack.Top );

   _math_matrix_analyse( &ctx->_ModelProjectMatrix );
}


/**
 * Updates the combined modelview-projection matrix.
 *
 * \param ctx GL context.
 * \param new_state new state bit mask.
 *
 * If there is a new model view matrix then analyzes it. If there is a new
 * projection matrix, updates it. Finally calls
 * calculate_model_project_matrix() to recalculate the modelview-projection
 * matrix.
 */
void _mesa_update_modelview_project( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_MODELVIEW) {
      _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
    
      /* Bring cull position uptodate.
       */
      TRANSFORM_POINT3( ctx->Transform.CullObjPos, 
			ctx->ModelviewMatrixStack.Top->inv,
			ctx->Transform.CullEyePos );
   }


   if (new_state & _NEW_PROJECTION)
      update_projection( ctx );

   /* Keep ModelviewProject uptodate always to allow tnl
    * implementations that go model->clip even when eye is required.
    */
   calculate_model_project_matrix(ctx);
}

/*@}*/


/**********************************************************************/
/** Matrix stack initialization */
/*@{*/


/**
 * Initialize a matrix stack.
 *
 * \param stack matrix stack.
 * \param maxDepth maximum stack depth.
 * \param dirtyFlag dirty flag.
 * 
 * Allocates an array of \p maxDepth elements for the matrix stack and calls
 * _math_matrix_ctr() and _math_matrix_alloc_inv() for each element to
 * initialize it.
 */
static void
init_matrix_stack( struct gl_matrix_stack *stack,
                   GLuint maxDepth, GLuint dirtyFlag )
{
   GLuint i;

   stack->Depth = 0;
   stack->MaxDepth = maxDepth;
   stack->DirtyFlag = dirtyFlag;
   /* The stack */
   stack->Stack = (GLmatrix *) CALLOC(maxDepth * sizeof(GLmatrix));
   for (i = 0; i < maxDepth; i++) {
      _math_matrix_ctr(&stack->Stack[i]);
      _math_matrix_alloc_inv(&stack->Stack[i]);
   }
   stack->Top = stack->Stack;
}

/**
 * Free matrix stack.
 * 
 * \param stack matrix stack.
 * 
 * Calls _math_matrix_dtr() for each element of the matrix stack and
 * frees the array.
 */
static void
free_matrix_stack( struct gl_matrix_stack *stack )
{
   GLuint i;
   for (i = 0; i < stack->MaxDepth; i++) {
      _math_matrix_dtr(&stack->Stack[i]);
   }
   FREE(stack->Stack);
   stack->Stack = stack->Top = NULL;
}

/*@}*/


/**********************************************************************/
/** \name Initialization */
/*@{*/


/**
 * Initialize the context matrix data.
 *
 * \param ctx GL context.
 *
 * Initializes each of the matrix stacks and the combined modelview-projection
 * matrix.
 */
void _mesa_init_matrix( GLcontext * ctx )
{
   GLint i;

   /* Initialize matrix stacks */
   init_matrix_stack(&ctx->ModelviewMatrixStack, MAX_MODELVIEW_STACK_DEPTH,
                     _NEW_MODELVIEW);
   init_matrix_stack(&ctx->ProjectionMatrixStack, MAX_PROJECTION_STACK_DEPTH,
                     _NEW_PROJECTION);
   init_matrix_stack(&ctx->ColorMatrixStack, MAX_COLOR_STACK_DEPTH,
                     _NEW_COLOR_MATRIX);
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      init_matrix_stack(&ctx->TextureMatrixStack[i], MAX_TEXTURE_STACK_DEPTH,
                        _NEW_TEXTURE_MATRIX);
   for (i = 0; i < MAX_PROGRAM_MATRICES; i++)
      init_matrix_stack(&ctx->ProgramMatrixStack[i], 
		        MAX_PROGRAM_MATRIX_STACK_DEPTH, _NEW_TRACK_MATRIX);
   ctx->CurrentStack = &ctx->ModelviewMatrixStack;

   /* Init combined Modelview*Projection matrix */
   _math_matrix_ctr( &ctx->_ModelProjectMatrix );
}


/**
 * Free the context matrix data.
 * 
 * \param ctx GL context.
 *
 * Frees each of the matrix stacks and the combined modelview-projection
 * matrix.
 */
void _mesa_free_matrix_data( GLcontext *ctx )
{
   GLint i;

   free_matrix_stack(&ctx->ModelviewMatrixStack);
   free_matrix_stack(&ctx->ProjectionMatrixStack);
   free_matrix_stack(&ctx->ColorMatrixStack);
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      free_matrix_stack(&ctx->TextureMatrixStack[i]);
   for (i = 0; i < MAX_PROGRAM_MATRICES; i++)
      free_matrix_stack(&ctx->ProgramMatrixStack[i]);
   /* combined Modelview*Projection matrix */
   _math_matrix_dtr( &ctx->_ModelProjectMatrix );

}


/** 
 * Initialize the context transform attribute group.
 *
 * \param ctx GL context.
 *
 * \todo Move this to a new file with other 'transform' routines.
 */
void _mesa_init_transform( GLcontext *ctx )
{
   GLint i;

   /* Transformation group */
   ctx->Transform.MatrixMode = GL_MODELVIEW;
   ctx->Transform.Normalize = GL_FALSE;
   ctx->Transform.RescaleNormals = GL_FALSE;
   ctx->Transform.RasterPositionUnclipped = GL_FALSE;
   for (i=0;i<MAX_CLIP_PLANES;i++) {
      ASSIGN_4V( ctx->Transform.EyeUserPlane[i], 0.0, 0.0, 0.0, 0.0 );
   }
   ctx->Transform.ClipPlanesEnabled = 0;

   ASSIGN_4V( ctx->Transform.CullObjPos, 0.0, 0.0, 1.0, 0.0 );
   ASSIGN_4V( ctx->Transform.CullEyePos, 0.0, 0.0, 1.0, 0.0 );
}


/** 
 * Initialize the context viewport attribute group.
 *
 * \param ctx GL context.
 * 
 * \todo Move this to a new file with other 'viewport' routines.
 */
void _mesa_init_viewport( GLcontext *ctx )
{
   GLfloat depthMax = 65535.0F; /* sorf of arbitrary */

   /* Viewport group */
   ctx->Viewport.X = 0;
   ctx->Viewport.Y = 0;
   ctx->Viewport.Width = 0;
   ctx->Viewport.Height = 0;
   ctx->Viewport.Near = 0.0;
   ctx->Viewport.Far = 1.0;
   _math_matrix_ctr(&ctx->Viewport._WindowMap);

   _math_matrix_viewport(&ctx->Viewport._WindowMap, 0, 0, 0, 0,
                         0.0F, 1.0F, depthMax);
}


/** 
 * Free the context viewport attribute group data.
 *
 * \param ctx GL context.
 * 
 * \todo Move this to a new file with other 'viewport' routines.
 */
void _mesa_free_viewport_data( GLcontext *ctx )
{
   _math_matrix_dtr(&ctx->Viewport._WindowMap);
}

/*@}*/
