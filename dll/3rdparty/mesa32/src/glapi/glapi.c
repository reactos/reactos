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


/*
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 *
 * It's intended that this file and the other glapi*.[ch] files are
 * flexible enough to be reused in several places:  XFree86, DRI-
 * based libGL.so, and perhaps the SGI SI.
 *
 * NOTE: There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 *   2002/06/28  - added _glapi_set_warning_func(), Mesa 4.1.
 *   2002/10/01  - _glapi_get_proc_address() will now generate new entrypoints
 *                 itself (using offset ~0).  _glapi_add_entrypoint() can be
 *                 called afterward and it'll fill in the correct dispatch
 *                 offset.  This allows DRI libGL to avoid probing for DRI
 *                 drivers!  No changes to the public glapi interface.
 */



#ifdef HAVE_DIX_CONFIG_H

#include <dix-config.h>
#define PUBLIC

#else

#include "main/glheader.h"

#endif

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <assert.h>
#endif

#include "glapi.h"
#include "glapioffsets.h"
#include "glapitable.h"


/***** BEGIN NO-OP DISPATCH *****/

static GLboolean WarnFlag = GL_FALSE;
static _glapi_warning_func warning_func;

/*
 * Enable/disable printing of warning messages.
 */
PUBLIC void
_glapi_noop_enable_warnings(GLboolean enable)
{
   WarnFlag = enable;
}

/*
 * Register a callback function for reporting errors.
 */
PUBLIC void
_glapi_set_warning_func( _glapi_warning_func func )
{
   warning_func = func;
}

static GLboolean
warn(void)
{
   if ((WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
       && warning_func) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


#define KEYWORD1 static
#define KEYWORD1_ALT static
#define KEYWORD2 GLAPIENTRY
#define NAME(func)  NoOp##func

#define F NULL

#define DISPATCH(func, args, msg)					      \
   if (warn()) {							      \
      warning_func(NULL, "GL User Error: called without context: %s", #func); \
   }

#define RETURN_DISPATCH(func, args, msg)				      \
   if (warn()) {							      \
      warning_func(NULL, "GL User Error: called without context: %s", #func); \
   }									      \
   return 0

#define DISPATCH_TABLE_NAME __glapi_noop_table
#define UNUSED_TABLE_NAME __unused_noop_functions

#define TABLE_ENTRY(name) (_glapi_proc) NoOp##name

static GLint NoOpUnused(void)
{
   if (warn()) {
      warning_func(NULL, "GL User Error: calling extension function without a current context\n");
   }
   return 0;
}

#include "glapitemp.h"

/***** END NO-OP DISPATCH *****/



/**
 * \name Current dispatch and current context control variables
 *
 * Depending on whether or not multithreading is support, and the type of
 * support available, several variables are used to store the current context
 * pointer and the current dispatch table pointer.  In the non-threaded case,
 * the variables \c _glapi_Dispatch and \c _glapi_Context are used for this
 * purpose.
 *
 * In the "normal" threaded case, the variables \c _glapi_Dispatch and
 * \c _glapi_Context will be \c NULL if an application is detected as being
 * multithreaded.  Single-threaded applications will use \c _glapi_Dispatch
 * and \c _glapi_Context just like the case without any threading support.
 * When \c _glapi_Dispatch and \c _glapi_Context are \c NULL, the thread state
 * data \c _gl_DispatchTSD and \c ContextTSD are used.  Drivers and the
 * static dispatch functions access these variables via \c _glapi_get_dispatch
 * and \c _glapi_get_context.
 *
 * There is a race condition in setting \c _glapi_Dispatch to \c NULL.  It is
 * possible for the original thread to be setting it at the same instant a new
 * thread, perhaps running on a different processor, is clearing it.  Because
 * of that, \c ThreadSafe, which can only ever be changed to \c GL_TRUE, is
 * used to determine whether or not the application is multithreaded.
 * 
 * In the TLS case, the variables \c _glapi_Dispatch and \c _glapi_Context are
 * hardcoded to \c NULL.  Instead the TLS variables \c _glapi_tls_Dispatch and
 * \c _glapi_tls_Context are used.  Having \c _glapi_Dispatch and
 * \c _glapi_Context be hardcoded to \c NULL maintains binary compatability
 * between TLS enabled loaders and non-TLS DRI drivers.
 */
/*@{*/
#if defined(GLX_USE_TLS)

PUBLIC __thread struct _glapi_table * _glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")))
    = (struct _glapi_table *) __glapi_noop_table;

PUBLIC __thread void * _glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

PUBLIC const struct _glapi_table *_glapi_Dispatch = NULL;
PUBLIC const void *_glapi_Context = NULL;

#else

#if defined(THREADS)

static GLboolean ThreadSafe = GL_FALSE;  /**< In thread-safe mode? */
_glthread_TSD _gl_DispatchTSD;           /**< Per-thread dispatch pointer */
static _glthread_TSD ContextTSD;         /**< Per-thread context pointer */

#if defined(WIN32_THREADS)
void FreeTSD(_glthread_TSD *p);
void FreeAllTSD(void)
{
   FreeTSD(&_gl_DispatchTSD);
   FreeTSD(&ContextTSD);
}
#endif /* defined(WIN32_THREADS) */

#endif /* defined(THREADS) */

PUBLIC struct _glapi_table *_glapi_Dispatch = 
  (struct _glapi_table *) __glapi_noop_table;
PUBLIC void *_glapi_Context = NULL;

#endif /* defined(GLX_USE_TLS) */
/*@}*/




/**
 * We should call this periodically from a function such as glXMakeCurrent
 * in order to test if multiple threads are being used.
 */
void
_glapi_check_multithread(void)
{
#if defined(THREADS) && !defined(GLX_USE_TLS)
   if (!ThreadSafe) {
      static unsigned long knownID;
      static GLboolean firstCall = GL_TRUE;
      if (firstCall) {
         knownID = _glthread_GetID();
         firstCall = GL_FALSE;
      }
      else if (knownID != _glthread_GetID()) {
         ThreadSafe = GL_TRUE;
         _glapi_set_dispatch(NULL);
         _glapi_set_context(NULL);
      }
   }
   else if (!_glapi_get_dispatch()) {
      /* make sure that this thread's dispatch pointer isn't null */
      _glapi_set_dispatch(NULL);
   }
#endif
}



/**
 * Set the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast to
 * void from the real context pointer type.
 */
PUBLIC void
_glapi_set_context(void *context)
{
   (void) __unused_noop_functions; /* silence a warning */
#if defined(GLX_USE_TLS)
   _glapi_tls_Context = context;
#elif defined(THREADS)
   _glthread_SetTSD(&ContextTSD, context);
   _glapi_Context = (ThreadSafe) ? NULL : context;
#else
   _glapi_Context = context;
#endif
}



/**
 * Get the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast from
 * void to the real context pointer type.
 */
PUBLIC void *
_glapi_get_context(void)
{
#if defined(GLX_USE_TLS)
   return _glapi_tls_Context;
#elif defined(THREADS)
   if (ThreadSafe) {
      return _glthread_GetTSD(&ContextTSD);
   }
   else {
      return _glapi_Context;
   }
#else
   return _glapi_Context;
#endif
}

#ifdef USE_X86_ASM

#if defined( GLX_USE_TLS )
extern       GLubyte gl_dispatch_functions_start[];
extern       GLubyte gl_dispatch_functions_end[];
#else
extern const GLubyte gl_dispatch_functions_start[];
#endif

#endif /* USE_X86_ASM */


#if defined(USE_X64_64_ASM) && defined(GLX_USE_TLS)
# define DISPATCH_FUNCTION_SIZE  16
#elif defined(USE_X86_ASM)
# if defined(THREADS) && !defined(GLX_USE_TLS)
#  define DISPATCH_FUNCTION_SIZE  32
# else
#  define DISPATCH_FUNCTION_SIZE  16
# endif
#endif

#if !defined(DISPATCH_FUNCTION_SIZE) && !defined(XFree86Server) && !defined(XGLServer)
# define NEED_FUNCTION_POINTER
#endif

#if defined(PTHREADS) || defined(GLX_USE_TLS)
/**
 * Perform platform-specific GL API entry-point fixups.
 */
static void
init_glapi_relocs( void )
{
#if defined(USE_X86_ASM) && defined(GLX_USE_TLS) && !defined(GLX_X86_READONLY_TEXT)
    extern unsigned long _x86_get_dispatch(void);
    char run_time_patch[] = {
       0x65, 0xa1, 0, 0, 0, 0 /* movl %gs:0,%eax */
    };
    GLuint *offset = (GLuint *) &run_time_patch[2]; /* 32-bits for x86/32 */
    const GLubyte * const get_disp = (const GLubyte *) run_time_patch;
    GLubyte * curr_func = (GLubyte *) gl_dispatch_functions_start;

    *offset = _x86_get_dispatch();
    while ( curr_func != (GLubyte *) gl_dispatch_functions_end ) {
	(void) memcpy( curr_func, get_disp, sizeof(run_time_patch));
	curr_func += DISPATCH_FUNCTION_SIZE;
    }
#endif
}
#endif /* defined(PTHREADS) || defined(GLX_USE_TLS) */


/**
 * Set the global or per-thread dispatch table pointer.
 * If the dispatch parameter is NULL we'll plug in the no-op dispatch
 * table (__glapi_noop_table).
 */
PUBLIC void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
#if defined(PTHREADS) || defined(GLX_USE_TLS)
   static pthread_once_t once_control = PTHREAD_ONCE_INIT;
   pthread_once( & once_control, init_glapi_relocs );
#endif

   if (!dispatch) {
      /* use the no-op functions */
      dispatch = (struct _glapi_table *) __glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(GLX_USE_TLS)
   _glapi_tls_Dispatch = dispatch;
#elif defined(THREADS)
   _glthread_SetTSD(&_gl_DispatchTSD, (void *) dispatch);
   _glapi_Dispatch = (ThreadSafe) ? NULL : dispatch;
#else /*THREADS*/
   _glapi_Dispatch = dispatch;
#endif /*THREADS*/
}



/**
 * Return pointer to current dispatch table for calling thread.
 */
PUBLIC struct _glapi_table *
_glapi_get_dispatch(void)
{
   struct _glapi_table * api;
#if defined(GLX_USE_TLS)
   api = _glapi_tls_Dispatch;
#elif defined(THREADS)
   api = (ThreadSafe)
     ? (struct _glapi_table *) _glthread_GetTSD(&_gl_DispatchTSD)
     : _glapi_Dispatch;
#else
   api = _glapi_Dispatch;
#endif
   return api;
}




/*
 * The dispatch table size (number of entries) is the size of the
 * _glapi_table struct plus the number of dynamic entries we can add.
 * The extra slots can be filled in by DRI drivers that register new extension
 * functions.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *) + MAX_EXTENSION_FUNCS)


/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
PUBLIC GLuint
_glapi_get_dispatch_table_size(void)
{
   return DISPATCH_TABLE_SIZE;
}


/**
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intended for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
#ifdef EXTRA_DEBUG
   const GLuint entries = _glapi_get_dispatch_table_size();
   const void **tab = (const void **) table;
   GLuint i;
   for (i = 1; i < entries; i++) {
      assert(tab[i]);
   }

   /* Do some spot checks to be sure that the dispatch table
    * slots are assigned correctly.
    */
   {
      GLuint BeginOffset = _glapi_get_proc_offset("glBegin");
      char *BeginFunc = (char*) &table->Begin;
      GLuint offset = (BeginFunc - (char *) table) / sizeof(void *);
      assert(BeginOffset == _gloffset_Begin);
      assert(BeginOffset == offset);
   }
   {
      GLuint viewportOffset = _glapi_get_proc_offset("glViewport");
      char *viewportFunc = (char*) &table->Viewport;
      GLuint offset = (viewportFunc - (char *) table) / sizeof(void *);
      assert(viewportOffset == _gloffset_Viewport);
      assert(viewportOffset == offset);
   }
   {
      GLuint VertexPointerOffset = _glapi_get_proc_offset("glVertexPointer");
      char *VertexPointerFunc = (char*) &table->VertexPointer;
      GLuint offset = (VertexPointerFunc - (char *) table) / sizeof(void *);
      assert(VertexPointerOffset == _gloffset_VertexPointer);
      assert(VertexPointerOffset == offset);
   }
   {
      GLuint ResetMinMaxOffset = _glapi_get_proc_offset("glResetMinmax");
      char *ResetMinMaxFunc = (char*) &table->ResetMinmax;
      GLuint offset = (ResetMinMaxFunc - (char *) table) / sizeof(void *);
      assert(ResetMinMaxOffset == _gloffset_ResetMinmax);
      assert(ResetMinMaxOffset == offset);
   }
   {
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColor");
      char *blendColorFunc = (char*) &table->BlendColor;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColor);
      assert(blendColorOffset == offset);
   }
   {
      GLuint secondaryColor3fOffset = _glapi_get_proc_offset("glSecondaryColor3fEXT");
      char *secondaryColor3fFunc = (char*) &table->SecondaryColor3fEXT;
      GLuint offset = (secondaryColor3fFunc - (char *) table) / sizeof(void *);
      assert(secondaryColor3fOffset == _gloffset_SecondaryColor3fEXT);
      assert(secondaryColor3fOffset == offset);
   }
   {
      GLuint pointParameterivOffset = _glapi_get_proc_offset("glPointParameterivNV");
      char *pointParameterivFunc = (char*) &table->PointParameterivNV;
      GLuint offset = (pointParameterivFunc - (char *) table) / sizeof(void *);
      assert(pointParameterivOffset == _gloffset_PointParameterivNV);
      assert(pointParameterivOffset == offset);
   }
   {
      GLuint setFenceOffset = _glapi_get_proc_offset("glSetFenceNV");
      char *setFenceFunc = (char*) &table->SetFenceNV;
      GLuint offset = (setFenceFunc - (char *) table) / sizeof(void *);
      assert(setFenceOffset == _gloffset_SetFenceNV);
      assert(setFenceOffset == offset);
   }
#else
   (void) table;
#endif
}
