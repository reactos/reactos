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
 * - struct gl_context: this contains the Mesa rendering state
 * - struct gl_config:  this describes the color buffer (RGB vs. ci), whether
 *   or not there's a depth buffer, stencil buffer, etc.
 * - struct gl_framebuffer:  contains pointers to the depth buffer, stencil
 *   buffer, accum buffer and alpha buffers.
 *
 * These types should be encapsulated by corresponding device driver
 * data types.  See xmesa.h and xmesaP.h for an example.
 *
 * In OOP terms, struct gl_context, struct gl_config, and struct gl_framebuffer
 * are base classes which the device driver must derive from.
 *
 * The following functions create and destroy these data types.
 */


#ifndef CONTEXT_H
#define CONTEXT_H


#include "imports.h"
#include "mtypes.h"


#ifdef __cplusplus
extern "C" {
#endif


struct _glapi_table;


/** \name Visual-related functions */
/*@{*/
 
extern struct gl_config *
_mesa_create_visual( GLboolean dbFlag,
                     GLboolean stereoFlag,
                     GLint redBits,
                     GLint greenBits,
                     GLint blueBits,
                     GLint alphaBits,
                     GLint depthBits,
                     GLint stencilBits,
                     GLint accumRedBits,
                     GLint accumGreenBits,
                     GLint accumBlueBits,
                     GLint accumAlphaBits);

extern GLboolean
_mesa_initialize_visual( struct gl_config *v,
                         GLboolean dbFlag,
                         GLboolean stereoFlag,
                         GLint redBits,
                         GLint greenBits,
                         GLint blueBits,
                         GLint alphaBits,
                         GLint depthBits,
                         GLint stencilBits,
                         GLint accumRedBits,
                         GLint accumGreenBits,
                         GLint accumBlueBits,
                         GLint accumAlphaBits);

extern void
_mesa_destroy_visual( struct gl_config *vis );

/*@}*/


/** \name Context-related functions */
/*@{*/

extern GLboolean
_mesa_initialize_context( struct gl_context *ctx,
                          const struct gl_config *visual,
                          struct gl_context *share_list,
                          const struct dd_function_table *driverFunctions,
                          void *driverContext );

extern struct gl_context *
_mesa_create_context(const struct gl_config *visual,
                     struct gl_context *share_list,
                     const struct dd_function_table *driverFunctions,
                     void *driverContext);

extern void
_mesa_free_context_data( struct gl_context *ctx );

extern void
_mesa_destroy_context( struct gl_context *ctx );


extern void
_mesa_copy_context(const struct gl_context *src, struct gl_context *dst, GLuint mask);


extern void
_mesa_check_init_viewport(struct gl_context *ctx, GLuint width, GLuint height);

extern GLboolean
_mesa_make_current( struct gl_context *ctx, struct gl_framebuffer *drawBuffer,
                    struct gl_framebuffer *readBuffer );

extern GLboolean
_mesa_share_state(struct gl_context *ctx, struct gl_context *ctxToShare);

extern struct gl_context *
_mesa_get_current_context(void);

/*@}*/

extern void
_mesa_init_get_hash(struct gl_context *ctx);

extern void
_mesa_notifySwapBuffers(struct gl_context *gc);


extern struct _glapi_table *
_mesa_get_dispatch(struct gl_context *ctx);


void
_mesa_set_mvp_with_dp4( struct gl_context *ctx,
                        GLboolean flag );


extern GLboolean
_mesa_valid_to_render(struct gl_context *ctx, const char *where);



/** \name Miscellaneous */
/*@{*/

extern void
_mesa_record_error( struct gl_context *ctx, GLenum error );


extern void
_mesa_finish(struct gl_context *ctx);

extern void
_mesa_flush(struct gl_context *ctx);


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
 * __struct gl_contextRec::NewState with \p newstate.
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
 * __struct gl_contextRec::NewState with \p newstate.
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
 * \param retval value to return in case the assertion fails.
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
 * \param retval value to return in case the assertion fails.
 */
#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval)	\
do {									\
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, retval);			\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)

/*@}*/


#ifdef __cplusplus
}
#endif


#endif /* CONTEXT_H */
