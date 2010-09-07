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


/**
 * \file context.h
 * Mesa context and visual-related functions.
 *
 * There are three large Mesa data types/classes which are meant to be
 * used by device drivers:
 * - GLcontext: this contains the Mesa rendering state
 * - GLvisual:  this describes the color buffer (RGB vs. ci), whether or not
 *   there's a depth buffer, stencil buffer, etc.
 * - GLframebuffer:  contains pointers to the depth buffer, stencil buffer,
 *   accum buffer and alpha buffers.
 *
 * These types should be encapsulated by corresponding device driver
 * data types.  See xmesa.h and xmesaP.h for an example.
 *
 * In OOP terms, GLcontext, GLvisual, and GLframebuffer are base classes
 * which the device driver must derive from.
 *
 * The following functions create and destroy these data types.
 */


#ifndef CONTEXT_H
#define CONTEXT_H


#include "glapi/glapi.h"
#include "imports.h"
#include "mtypes.h"


/** \name Visual-related functions */
/*@{*/
 
extern GLvisual *
_mesa_create_visual( GLboolean rgbFlag,
                     GLboolean dbFlag,
                     GLboolean stereoFlag,
                     GLint redBits,
                     GLint greenBits,
                     GLint blueBits,
                     GLint alphaBits,
                     GLint indexBits,
                     GLint depthBits,
                     GLint stencilBits,
                     GLint accumRedBits,
                     GLint accumGreenBits,
                     GLint accumBlueBits,
                     GLint accumAlphaBits,
                     GLint numSamples );

extern GLboolean
_mesa_initialize_visual( GLvisual *v,
                         GLboolean rgbFlag,
                         GLboolean dbFlag,
                         GLboolean stereoFlag,
                         GLint redBits,
                         GLint greenBits,
                         GLint blueBits,
                         GLint alphaBits,
                         GLint indexBits,
                         GLint depthBits,
                         GLint stencilBits,
                         GLint accumRedBits,
                         GLint accumGreenBits,
                         GLint accumBlueBits,
                         GLint accumAlphaBits,
                         GLint numSamples );

extern void
_mesa_destroy_visual( GLvisual *vis );

/*@}*/


/** \name Context-related functions */
/*@{*/

extern GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      const struct dd_function_table *driverFunctions,
                      void *driverContext );

extern GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          const GLvisual *visual,
                          GLcontext *share_list,
                          const struct dd_function_table *driverFunctions,
                          void *driverContext );

extern void
_mesa_initialize_context_extra(GLcontext *ctx);

extern void
_mesa_free_context_data( GLcontext *ctx );

extern void
_mesa_destroy_context( GLcontext *ctx );


extern void
_mesa_copy_context(const GLcontext *src, GLcontext *dst, GLuint mask);


extern void
_mesa_make_current( GLcontext *ctx, GLframebuffer *drawBuffer,
                    GLframebuffer *readBuffer );

extern GLboolean
_mesa_share_state(GLcontext *ctx, GLcontext *ctxToShare);

extern GLcontext *
_mesa_get_current_context(void);

/*@}*/


extern void
_mesa_notifySwapBuffers(__GLcontext *gc);


extern struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx);



/** \name Miscellaneous */
/*@{*/

extern void
_mesa_record_error( GLcontext *ctx, GLenum error );

extern void GLAPIENTRY
_mesa_Finish( void );

extern void GLAPIENTRY
_mesa_Flush( void );

/*@}*/



/**
 * \name Macros for flushing buffered rendering commands before state changes,
 * checking if inside glBegin/glEnd, etc.
 */
/*@{*/

/**
 * Flush vertices.
 *
 * \param ctx GL context.
 * \param newstate new state.
 *
 * Checks if dd_function_table::NeedFlush is marked to flush stored vertices,
 * and calls dd_function_table::FlushVertices if so. Marks
 * __GLcontextRec::NewState with \p newstate.
 */
#define FLUSH_VERTICES(ctx, newstate)				\
do {								\
   if (MESA_VERBOSE & VERBOSE_STATE)				\
      _mesa_debug(ctx, "FLUSH_VERTICES in %s\n", MESA_FUNCTION);\
   if (ctx->Driver.NeedFlush & FLUSH_STORED_VERTICES)		\
      ctx->Driver.FlushVertices(ctx, FLUSH_STORED_VERTICES);	\
   ctx->NewState |= newstate;					\
} while (0)

/**
 * Flush current state.
 *
 * \param ctx GL context.
 * \param newstate new state.
 *
 * Checks if dd_function_table::NeedFlush is marked to flush current state,
 * and calls dd_function_table::FlushVertices if so. Marks
 * __GLcontextRec::NewState with \p newstate.
 */
#define FLUSH_CURRENT(ctx, newstate)				\
do {								\
   if (MESA_VERBOSE & VERBOSE_STATE)				\
      _mesa_debug(ctx, "FLUSH_CURRENT in %s\n", MESA_FUNCTION);	\
   if (ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT)		\
      ctx->Driver.FlushVertices(ctx, FLUSH_UPDATE_CURRENT);	\
   ctx->NewState |= newstate;					\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair, with return value.
 * 
 * \param ctx GL context.
 * \param retval value to return value in case the assertion fails.
 */
#define ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, retval)		\
do {									\
   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {	\
      _mesa_error(ctx, GL_INVALID_OPERATION, "Inside glBegin/glEnd");	\
      return retval;							\
   }									\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair.
 * 
 * \param ctx GL context.
 */
#define ASSERT_OUTSIDE_BEGIN_END(ctx)					\
do {									\
   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {	\
      _mesa_error(ctx, GL_INVALID_OPERATION, "Inside glBegin/glEnd");	\
      return;								\
   }									\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair and flush the vertices.
 * 
 * \param ctx GL context.
 */
#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx)				\
do {									\
   ASSERT_OUTSIDE_BEGIN_END(ctx);					\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair and flush the vertices, with return value.
 * 
 * \param ctx GL context.
 * \param retval value to return value in case the assertion fails.
 */
#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval)	\
do {									\
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, retval);			\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)

/*@}*/



/**
 * Is the secondary color needed?
 */
#define NEED_SECONDARY_COLOR(CTX)					\
   (((CTX)->Light.Enabled &&						\
     (CTX)->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)	\
    || (CTX)->Fog.ColorSumEnabled					\
    || ((CTX)->VertexProgram._Current &&				\
        ((CTX)->VertexProgram._Current != (CTX)->VertexProgram._TnlProgram) &&    \
        ((CTX)->VertexProgram._Current->Base.InputsRead & VERT_BIT_COLOR1)) \
    || ((CTX)->FragmentProgram._Current &&				\
        ((CTX)->FragmentProgram._Current != (CTX)->FragmentProgram._TexEnvProgram) &&  \
        ((CTX)->FragmentProgram._Current->Base.InputsRead & FRAG_BIT_COL1)) \
   )


/**
 * Is RGBA LogicOp enabled?
 */
#define RGBA_LOGICOP_ENABLED(CTX) \
  ((CTX)->Color.ColorLogicOpEnabled || \
   ((CTX)->Color.BlendEnabled && (CTX)->Color.BlendEquationRGB == GL_LOGIC_OP))


#endif /* CONTEXT_H */
