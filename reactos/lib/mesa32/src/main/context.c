/**
 * \file context.c
 * Mesa context/visual/framebuffer management functions.
 * \author Brian Paul
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * \mainpage Mesa Core Module
 *
 * \section CoreIntroduction Introduction
 *
 * The Mesa core module consists of all the top-level files in the src
 * directory.  The core module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 * For example, calls to glPolygonMode() are routed to _mesa_PolygonMode()
 * which updates the state related to polygonmode.  Furthermore, dirty
 * state flags related to polygon mode are set and if the device driver
 * implements a special routine for PolygonMode, it will be called.
 *
 *
 * \section AboutDoxygen About Doxygen
 *
 * If you're viewing this information as Doxygen-generated HTML you'll
 * see the documentation index at the top of this page.
 *
 * The first line lists the Mesa source code modules.
 * The second line lists the indexes available for viewing the documentation
 * for each module.
 *
 * Selecting the <b>Main page</b> link will display a summary of the module
 * (this page).
 *
 * Selecting <b>Data Structures</b> will list all C structures.
 *
 * Selecting the <b>File List</b> link will list all the source files in
 * the module.
 * Selecting a filename will show a list of all functions defined in that file.
 *
 * Selecting the <b>Data Fields</b> link will display a list of all
 * documented structure members.
 *
 * Selecting the <b>Globals</b> link will display a list
 * of all functions, structures, global variables and macros in the module.
 *
 */


#include "glheader.h"
#include "imports.h"
#include "accum.h"
#include "attrib.h"
#include "blend.h"
#include "buffers.h"
#include "bufferobj.h"
#include "colortab.h"
#include "context.h"
#include "debug.h"
#include "depth.h"
#include "dlist.h"
#include "eval.h"
#include "enums.h"
#include "extensions.h"
#include "feedback.h"
#include "fog.h"
#include "get.h"
#include "glthread.h"
#include "glapioffsets.h"
#include "histogram.h"
#include "hint.h"
#include "hash.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "occlude.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
#include "program.h"
#endif
#include "rastpos.h"
#include "simple_list.h"
#include "state.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#include "vtxfmt.h"
#if _HAVE_FULL_GL
#include "math/m_translate.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"
#include "math/mathmod.h"
#endif

#ifdef USE_SPARC_ASM
#include "SPARC/sparc.h"
#endif

#ifndef MESA_VERBOSE
int MESA_VERBOSE = 0;
#endif

#ifndef MESA_DEBUG_FLAGS
int MESA_DEBUG_FLAGS = 0;
#endif


/* ubyte -> float conversion */
GLfloat _mesa_ubyte_to_float_color_tab[256];

static void
free_shared_state( GLcontext *ctx, struct gl_shared_state *ss );


/**********************************************************************/
/** \name OpenGL SI-style interface (new in Mesa 3.5)
 *
 * \if subset
 * \note Most of these functions are never called in the Mesa subset.
 * \endif
 */
/*@{*/

/**
 * Destroy context callback.
 * 
 * \param gc context.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 * 
 * \ifnot subset
 * Called by window system/device driver (via __GLexports::destroyCurrent) when
 * the rendering context is to be destroyed.
 * \endif
 *
 * Frees the context data and the context structure.
 */
GLboolean
_mesa_destroyContext(__GLcontext *gc)
{
   if (gc) {
      _mesa_free_context_data(gc);
      _mesa_free(gc);
   }
   return GL_TRUE;
}

/**
 * Unbind context callback.
 * 
 * \param gc context.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \ifnot subset
 * Called by window system/device driver (via __GLexports::loseCurrent)
 * when the rendering context is made non-current.
 * \endif
 *
 * No-op
 */
GLboolean
_mesa_loseCurrent(__GLcontext *gc)
{
   /* XXX unbind context from thread */
   return GL_TRUE;
}

/**
 * Bind context callback.
 * 
 * \param gc context.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \ifnot subset
 * Called by window system/device driver (via __GLexports::makeCurrent)
 * when the rendering context is made current.
 * \endif
 *
 * No-op
 */
GLboolean
_mesa_makeCurrent(__GLcontext *gc)
{
   /* XXX bind context to thread */
   return GL_TRUE;
}

/**
 * Share context callback.
 * 
 * \param gc context.
 * \param gcShare shared context.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \ifnot subset
 * Called by window system/device driver (via __GLexports::shareContext)
 * \endif
 *
 * Update the shared context reference count, gl_shared_state::RefCount.
 */
GLboolean
_mesa_shareContext(__GLcontext *gc, __GLcontext *gcShare)
{
   if (gc && gcShare && gc->Shared && gcShare->Shared) {
      gc->Shared->RefCount--;
      if (gc->Shared->RefCount == 0) {
         free_shared_state(gc, gc->Shared);
      }
      gc->Shared = gcShare->Shared;
      gc->Shared->RefCount++;
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


#if _HAVE_FULL_GL
/**
 * Copy context callback.
 */
GLboolean
_mesa_copyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask)
{
   if (dst && src) {
      _mesa_copy_context( src, dst, mask );
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}
#endif

/** No-op */
GLboolean
_mesa_forceCurrent(__GLcontext *gc)
{
   return GL_TRUE;
}

/**
 * Windows/buffer resizing notification callback.
 *
 * \param gc GL context.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 */
GLboolean
_mesa_notifyResize(__GLcontext *gc)
{
   GLint x, y;
   GLuint width, height;
   __GLdrawablePrivate *d = gc->imports.getDrawablePrivate(gc);
   if (!d || !d->getDrawableSize)
      return GL_FALSE;
   d->getDrawableSize( d, &x, &y, &width, &height );
   /* update viewport, resize software buffers, etc. */
   return GL_TRUE;
}

/**
 * Window/buffer destruction notification callback.
 *
 * \param gc GL context.
 * 
 * Called when the context's window/buffer is going to be destroyed. 
 *
 * No-op
 */
void
_mesa_notifyDestroy(__GLcontext *gc)
{
   /* Unbind from it. */
}

/**
 * Swap buffers notification callback.
 * 
 * \param gc GL context.
 *
 * Called by window system just before swapping buffers.
 * We have to finish any pending rendering.
 */
void
_mesa_notifySwapBuffers(__GLcontext *gc)
{
   FLUSH_VERTICES( gc, 0 );
}

/** No-op */
struct __GLdispatchStateRec *
_mesa_dispatchExec(__GLcontext *gc)
{
   return NULL;
}

/** No-op */
void
_mesa_beginDispatchOverride(__GLcontext *gc)
{
}

/** No-op */
void
_mesa_endDispatchOverride(__GLcontext *gc)
{
}

/**
 * \ifnot subset
 * Setup the exports.  
 *
 * The window system will call these functions when it needs Mesa to do
 * something.
 * 
 * \note Device drivers should override these functions!  For example,
 * the Xlib driver should plug in the XMesa*-style functions into this
 * structure.  The XMesa-style functions should then call the _mesa_*
 * version of these functions.  This is an approximation to OO design
 * (inheritance and virtual functions).
 * \endif
 *
 * \if subset
 * No-op.
 * 
 * \endif
 */
static void
_mesa_init_default_exports(__GLexports *exports)
{
#if _HAVE_FULL_GL
    exports->destroyContext = _mesa_destroyContext;
    exports->loseCurrent = _mesa_loseCurrent;
    exports->makeCurrent = _mesa_makeCurrent;
    exports->shareContext = _mesa_shareContext;
    exports->copyContext = _mesa_copyContext;
    exports->forceCurrent = _mesa_forceCurrent;
    exports->notifyResize = _mesa_notifyResize;
    exports->notifyDestroy = _mesa_notifyDestroy;
    exports->notifySwapBuffers = _mesa_notifySwapBuffers;
    exports->dispatchExec = _mesa_dispatchExec;
    exports->beginDispatchOverride = _mesa_beginDispatchOverride;
    exports->endDispatchOverride = _mesa_endDispatchOverride;
#endif
}

/**
 * Exported OpenGL SI interface.
 */
__GLcontext *
__glCoreCreateContext(__GLimports *imports, __GLcontextModes *modes)
{
    GLcontext *ctx;

    ctx = (GLcontext *) (*imports->calloc)(NULL, 1, sizeof(GLcontext));
    if (ctx == NULL) {
	return NULL;
    }

    _mesa_initialize_context(ctx, modes, NULL, imports, GL_FALSE);
    ctx->imports = *imports;

    return ctx;
}

/**
 * Exported OpenGL SI interface.
 */
void
__glCoreNopDispatch(void)
{
#if 0
   /* SI */
   __gl_dispatch = __glNopDispatchState;
#else
   /* Mesa */
   _glapi_set_dispatch(NULL);
#endif
}

/*@}*/


/**********************************************************************/
/** \name GL Visual allocation/destruction                            */
/**********************************************************************/
/*@{*/

/**
 * Allocate a new GLvisual object.
 * 
 * \param rgbFlag GL_TRUE for RGB(A) mode, GL_FALSE for Color Index mode.
 * \param dbFlag double buffering
 * \param stereoFlag stereo buffer
 * \param depthBits requested bits per depth buffer value. Any value in [0, 32]
 * is acceptable but the actual depth type will be GLushort or GLuint as
 * needed.
 * \param stencilBits requested minimum bits per stencil buffer value
 * \param accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits number of bits per color component in accum buffer.
 * \param indexBits number of bits per pixel if \p rgbFlag is GL_FALSE
 * \param redBits number of bits per color component in frame buffer for RGB(A)
 * mode.  We always use 8 in core Mesa though.
 * \param greenBits same as above.
 * \param blueBits same as above.
 * \param alphaBits same as above.
 * \param numSamples not really used.
 * 
 * \return pointer to new GLvisual or NULL if requested parameters can't be
 * met.
 *
 * Allocates a GLvisual structure and initializes it via
 * _mesa_initialize_visual().
 */
GLvisual *
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
                     GLint numSamples )
{
   GLvisual *vis = (GLvisual *) CALLOC( sizeof(GLvisual) );
   if (vis) {
      if (!_mesa_initialize_visual(vis, rgbFlag, dbFlag, stereoFlag,
                                   redBits, greenBits, blueBits, alphaBits,
                                   indexBits, depthBits, stencilBits,
                                   accumRedBits, accumGreenBits,
                                   accumBlueBits, accumAlphaBits,
                                   numSamples)) {
         FREE(vis);
         return NULL;
      }
   }
   return vis;
}

/**
 * Initialize the fields of the given GLvisual.
 * 
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \sa _mesa_create_visual() above for the parameter description.
 *
 * Makes some sanity checks and fills in the fields of the
 * GLvisual structure with the given parameters.
 */
GLboolean
_mesa_initialize_visual( GLvisual *vis,
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
                         GLint numSamples )
{
   (void) numSamples;

   assert(vis);

   /* This is to catch bad values from device drivers not updated for
    * Mesa 3.3.  Some device drivers just passed 1.  That's a REALLY
    * bad value now (a 1-bit depth buffer!?!).
    */
   assert(depthBits == 0 || depthBits > 1);

   if (depthBits < 0 || depthBits > 32) {
      return GL_FALSE;
   }
   if (stencilBits < 0 || stencilBits > (GLint) (8 * sizeof(GLstencil))) {
      return GL_FALSE;
   }
   if (accumRedBits < 0 || accumRedBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumGreenBits < 0 || accumGreenBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumBlueBits < 0 || accumBlueBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumAlphaBits < 0 || accumAlphaBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }

   vis->rgbMode          = rgbFlag;
   vis->doubleBufferMode = dbFlag;
   vis->stereoMode       = stereoFlag;

   vis->redBits          = redBits;
   vis->greenBits        = greenBits;
   vis->blueBits         = blueBits;
   vis->alphaBits        = alphaBits;

   vis->indexBits      = indexBits;
   vis->depthBits      = depthBits;
   vis->accumRedBits   = (accumRedBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumGreenBits = (accumGreenBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumBlueBits  = (accumBlueBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumAlphaBits = (accumAlphaBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->stencilBits    = (stencilBits > 0) ? (8 * sizeof(GLstencil)) : 0;

   vis->haveAccumBuffer   = accumRedBits > 0;
   vis->haveDepthBuffer   = depthBits > 0;
   vis->haveStencilBuffer = stencilBits > 0;

   vis->numAuxBuffers = 0;
   vis->level = 0;
   vis->pixmapMode = 0;

   return GL_TRUE;
}

/**
 * Destroy a visual.
 *
 * \param vis visual.
 * 
 * Frees the visual structure.
 */
void
_mesa_destroy_visual( GLvisual *vis )
{
   FREE(vis);
}

/*@}*/


/**********************************************************************/
/** \name GL Framebuffer allocation/destruction                       */
/**********************************************************************/
/*@{*/

/**
 * Create a new framebuffer.  
 *
 * A GLframebuffer is a structure which encapsulates the depth, stencil and
 * accum buffers and related parameters.
 * 
 * \param visual a GLvisual pointer (we copy the struct contents)
 * \param softwareDepth create/use a software depth buffer?
 * \param softwareStencil create/use a software stencil buffer?
 * \param softwareAccum create/use a software accum buffer?
 * \param softwareAlpha create/use a software alpha buffer?
 *
 * \return pointer to new GLframebuffer struct or NULL if error.
 *
 * Allocate a GLframebuffer structure and initializes it via
 * _mesa_initialize_framebuffer().
 */
GLframebuffer *
_mesa_create_framebuffer( const GLvisual *visual,
                          GLboolean softwareDepth,
                          GLboolean softwareStencil,
                          GLboolean softwareAccum,
                          GLboolean softwareAlpha )
{
   GLframebuffer *buffer = CALLOC_STRUCT(gl_frame_buffer);
   assert(visual);
   if (buffer) {
      _mesa_initialize_framebuffer(buffer, visual,
                                   softwareDepth, softwareStencil,
                                   softwareAccum, softwareAlpha );
   }
   return buffer;
}

/**
 * Initialize a GLframebuffer object.
 * 
 * \sa _mesa_create_framebuffer() above for the parameter description.
 *
 * Makes some sanity checks and fills in the fields of the
 * GLframebuffer structure with the given parameters.
 */
void
_mesa_initialize_framebuffer( GLframebuffer *buffer,
                              const GLvisual *visual,
                              GLboolean softwareDepth,
                              GLboolean softwareStencil,
                              GLboolean softwareAccum,
                              GLboolean softwareAlpha )
{
   assert(buffer);
   assert(visual);

   _mesa_bzero(buffer, sizeof(GLframebuffer));

   /* sanity checks */
   if (softwareDepth ) {
      assert(visual->depthBits > 0);
   }
   if (softwareStencil) {
      assert(visual->stencilBits > 0);
   }
   if (softwareAccum) {
      assert(visual->rgbMode);
      assert(visual->accumRedBits > 0);
      assert(visual->accumGreenBits > 0);
      assert(visual->accumBlueBits > 0);
   }
   if (softwareAlpha) {
      assert(visual->rgbMode);
      assert(visual->alphaBits > 0);
   }

   buffer->Visual = *visual;
   buffer->UseSoftwareDepthBuffer = softwareDepth;
   buffer->UseSoftwareStencilBuffer = softwareStencil;
   buffer->UseSoftwareAccumBuffer = softwareAccum;
   buffer->UseSoftwareAlphaBuffers = softwareAlpha;
}

/**
 * Free a framebuffer struct and its buffers.
 *
 * Calls _mesa_free_framebuffer_data() and frees the structure.
 */
void
_mesa_destroy_framebuffer( GLframebuffer *buffer )
{
   if (buffer) {
      _mesa_free_framebuffer_data(buffer);
      FREE(buffer);
   }
}

/**
 * Free the data hanging off of \p buffer, but not \p buffer itself.
 *
 * \param buffer framebuffer.
 *
 * Frees all the buffers associated with the structure.
 */
void
_mesa_free_framebuffer_data( GLframebuffer *buffer )
{
   if (!buffer)
      return;

   if (buffer->UseSoftwareDepthBuffer && buffer->DepthBuffer) {
      MESA_PBUFFER_FREE( buffer->DepthBuffer );
      buffer->DepthBuffer = NULL;
   }
   if (buffer->UseSoftwareAccumBuffer && buffer->Accum) {
      MESA_PBUFFER_FREE( buffer->Accum );
      buffer->Accum = NULL;
   }
   if (buffer->UseSoftwareStencilBuffer && buffer->Stencil) {
      MESA_PBUFFER_FREE( buffer->Stencil );
      buffer->Stencil = NULL;
   }
   if (buffer->UseSoftwareAlphaBuffers){
      if (buffer->FrontLeftAlpha) {
         MESA_PBUFFER_FREE( buffer->FrontLeftAlpha );
         buffer->FrontLeftAlpha = NULL;
      }
      if (buffer->BackLeftAlpha) {
         MESA_PBUFFER_FREE( buffer->BackLeftAlpha );
         buffer->BackLeftAlpha = NULL;
      }
      if (buffer->FrontRightAlpha) {
         MESA_PBUFFER_FREE( buffer->FrontRightAlpha );
         buffer->FrontRightAlpha = NULL;
      }
      if (buffer->BackRightAlpha) {
         MESA_PBUFFER_FREE( buffer->BackRightAlpha );
         buffer->BackRightAlpha = NULL;
      }
   }
}

/*@}*/


/**********************************************************************/
/** \name Context allocation, initialization, destroying
 *
 * The purpose of the most initialization functions here is to provide the
 * default state values according to the OpenGL specification.
 */
/**********************************************************************/
/*@{*/

/**
 * One-time initialization mutex lock.
 *
 * \sa Used by one_time_init().
 */
_glthread_DECLARE_STATIC_MUTEX(OneTimeLock);

/**
 * Calls all the various one-time-init functions in Mesa.
 *
 * While holding a global mutex lock, calls several initialization functions,
 * and sets the glapi callbacks if the \c MESA_DEBUG environment variable is
 * defined.
 *
 * \sa _mesa_init_lists(), _math_init().
 */
static void
one_time_init( GLcontext *ctx )
{
   static GLboolean alreadyCalled = GL_FALSE;
   _glthread_LOCK_MUTEX(OneTimeLock);
   if (!alreadyCalled) {
      GLuint i;

      /* do some implementation tests */
      assert( sizeof(GLbyte) == 1 );
      assert( sizeof(GLshort) >= 2 );
      assert( sizeof(GLint) >= 4 );
      assert( sizeof(GLubyte) == 1 );
      assert( sizeof(GLushort) >= 2 );
      assert( sizeof(GLuint) >= 4 );

      _mesa_init_lists();

#if _HAVE_FULL_GL
      _math_init();

      for (i = 0; i < 256; i++) {
         _mesa_ubyte_to_float_color_tab[i] = (float) i / 255.0F;
      }
#endif

#ifdef USE_SPARC_ASM
      _mesa_init_sparc_glapi_relocs();
#endif
      if (_mesa_getenv("MESA_DEBUG")) {
         _glapi_noop_enable_warnings(GL_TRUE);
#ifndef GLX_DIRECT_RENDERING
         /* libGL from before 2002/06/28 don't have this function.  Someday,
          * when newer libGL libs are common, remove the #ifdef test.  This
          * only serves to print warnings when calling undefined GL functions.
          */
         _glapi_set_warning_func( (_glapi_warning_func) _mesa_warning );
#endif
      }
      else {
         _glapi_noop_enable_warnings(GL_FALSE);
      }

#if defined(DEBUG) && defined(__DATE__) && defined(__TIME__)
      _mesa_debug(ctx, "Mesa DEBUG build %s %s\n", __DATE__, __TIME__);
#endif

      alreadyCalled = GL_TRUE;
   }
   _glthread_UNLOCK_MUTEX(OneTimeLock);
}

/**
 * Allocate and initialize a shared context state structure.
 *
 * \return pointer to a gl_shared_state structure on success, or NULL on
 * failure.
 *
 * Initializes the display list, texture objects and vertex programs hash
 * tables, allocates the texture objects. If it runs out of memory, frees
 * everything already allocated before returning NULL.
 */
static GLboolean
alloc_shared_state( GLcontext *ctx )
{
   struct gl_shared_state *ss = CALLOC_STRUCT(gl_shared_state);
   if (!ss)
      return GL_FALSE;

   ctx->Shared = ss;

   _glthread_INIT_MUTEX(ss->Mutex);

   ss->DisplayList = _mesa_NewHashTable();
   ss->TexObjects = _mesa_NewHashTable();
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
   ss->Programs = _mesa_NewHashTable();
#endif

#if FEATURE_ARB_vertex_program
   ss->DefaultVertexProgram = _mesa_alloc_program(ctx, GL_VERTEX_PROGRAM_ARB, 0);
   if (!ss->DefaultVertexProgram)
      goto cleanup;
#endif
#if FEATURE_ARB_fragment_program
   ss->DefaultFragmentProgram = _mesa_alloc_program(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!ss->DefaultFragmentProgram)
      goto cleanup;
#endif

   ss->BufferObjects = _mesa_NewHashTable();

   ss->Default1D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_1D);
   if (!ss->Default1D)
      goto cleanup;

   ss->Default2D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_2D);
   if (!ss->Default2D)
      goto cleanup;

   ss->Default3D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_3D);
   if (!ss->Default3D)
      goto cleanup;

   ss->DefaultCubeMap = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_CUBE_MAP_ARB);
   if (!ss->DefaultCubeMap)
      goto cleanup;

   ss->DefaultRect = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_RECTANGLE_NV);
   if (!ss->DefaultRect)
      goto cleanup;

#if 0
   _mesa_save_texture_object(ctx, ss->Default1D);
   _mesa_save_texture_object(ctx, ss->Default2D);
   _mesa_save_texture_object(ctx, ss->Default3D);
   _mesa_save_texture_object(ctx, ss->DefaultCubeMap);
   _mesa_save_texture_object(ctx, ss->DefaultRect);
#endif

   /* Effectively bind the default textures to all texture units */
   ss->Default1D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->Default2D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->Default3D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->DefaultCubeMap->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->DefaultRect->RefCount += MAX_TEXTURE_IMAGE_UNITS;

   return GL_TRUE;

 cleanup:
   /* Ran out of memory at some point.  Free everything and return NULL */
   if (ss->DisplayList)
      _mesa_DeleteHashTable(ss->DisplayList);
   if (ss->TexObjects)
      _mesa_DeleteHashTable(ss->TexObjects);
#if FEATURE_NV_vertex_program
   if (ss->Programs)
      _mesa_DeleteHashTable(ss->Programs);
#endif
#if FEATURE_ARB_vertex_program
   if (ss->DefaultVertexProgram)
      _mesa_delete_program(ctx, ss->DefaultVertexProgram);
#endif
#if FEATURE_ARB_fragment_program
   if (ss->DefaultFragmentProgram)
      _mesa_delete_program(ctx, ss->DefaultFragmentProgram);
#endif
   if (ss->BufferObjects)
      _mesa_DeleteHashTable(ss->BufferObjects);

   if (ss->Default1D)
      (*ctx->Driver.DeleteTexture)(ctx, ss->Default1D);
   if (ss->Default2D)
      (*ctx->Driver.DeleteTexture)(ctx, ss->Default2D);
   if (ss->Default3D)
      (*ctx->Driver.DeleteTexture)(ctx, ss->Default3D);
   if (ss->DefaultCubeMap)
      (*ctx->Driver.DeleteTexture)(ctx, ss->DefaultCubeMap);
   if (ss->DefaultRect)
      (*ctx->Driver.DeleteTexture)(ctx, ss->DefaultRect);
   if (ss)
      _mesa_free(ss);
   return GL_FALSE;
}

/**
 * Deallocate a shared state context and all children structures.
 *
 * \param ctx GL context.
 * \param ss shared state pointer.
 * 
 * Frees the display lists, the texture objects (calling the driver texture
 * deletion callback to free its private data) and the vertex programs, as well
 * as their hash tables.
 *
 * \sa alloc_shared_state().
 */
static void
free_shared_state( GLcontext *ctx, struct gl_shared_state *ss )
{
   /* Free display lists */
   while (1) {
      GLuint list = _mesa_HashFirstEntry(ss->DisplayList);
      if (list) {
         _mesa_destroy_list(ctx, list);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->DisplayList);

   /* Free texture objects */
   ASSERT(ctx->Driver.DeleteTexture);
   while (1) {
      GLuint texName = _mesa_HashFirstEntry(ss->TexObjects);
      if (texName) {
         struct gl_texture_object *texObj = (struct gl_texture_object *)
            _mesa_HashLookup(ss->TexObjects, texName);
         ASSERT(texObj);
         (*ctx->Driver.DeleteTexture)(ctx, texObj);
         _mesa_HashRemove(ss->TexObjects, texName);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->TexObjects);

#if FEATURE_NV_vertex_program
   /* Free vertex programs */
   while (1) {
      GLuint prog = _mesa_HashFirstEntry(ss->Programs);
      if (prog) {
         struct program *p = (struct program *) _mesa_HashLookup(ss->Programs,
                                                                 prog);
         ASSERT(p);
         _mesa_delete_program(ctx, p);
         _mesa_HashRemove(ss->Programs, prog);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->Programs);
#endif

   _mesa_DeleteHashTable(ss->BufferObjects);

   _glthread_DESTROY_MUTEX(ss->Mutex);

   FREE(ss);
}


static void _mesa_init_current( GLcontext *ctx )
{
   int i;

   /* Current group */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      ASSIGN_4V( ctx->Current.Attrib[i], 0.0, 0.0, 0.0, 1.0 );
   }
   /* special cases: */
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_WEIGHT], 1.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_NORMAL], 0.0, 0.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR0], 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR1], 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_FOG], 0.0, 0.0, 0.0, 0.0 );
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_TEX0 + i], 0.0, 0.0, 0.0, 1.0);
   ctx->Current.Index = 1;
   ctx->Current.EdgeFlag = GL_TRUE;
}


static void 
_mesa_init_constants( GLcontext *ctx )
{
   assert(ctx);

   assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
   assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

   /* Constants, may be overriden by device drivers */
   ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.Max3DTextureLevels = MAX_3D_TEXTURE_LEVELS;
   ctx->Const.MaxCubeTextureLevels = MAX_CUBE_TEXTURE_LEVELS;
   ctx->Const.MaxTextureRectSize = MAX_TEXTURE_RECT_SIZE;
   ctx->Const.MaxTextureUnits = MAX_TEXTURE_UNITS;
   ctx->Const.MaxTextureCoordUnits = MAX_TEXTURE_COORD_UNITS;
   ctx->Const.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxTextureMaxAnisotropy = MAX_TEXTURE_MAX_ANISOTROPY;
   ctx->Const.MaxTextureLodBias = MAX_TEXTURE_LOD_BIAS;
   ctx->Const.MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;
   ctx->Const.SubPixelBits = SUB_PIXEL_BITS;
   ctx->Const.MinPointSize = MIN_POINT_SIZE;
   ctx->Const.MaxPointSize = MAX_POINT_SIZE;
   ctx->Const.MinPointSizeAA = MIN_POINT_SIZE;
   ctx->Const.MaxPointSizeAA = MAX_POINT_SIZE;
   ctx->Const.PointSizeGranularity = (GLfloat) POINT_SIZE_GRANULARITY;
   ctx->Const.MinLineWidth = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidth = MAX_LINE_WIDTH;
   ctx->Const.MinLineWidthAA = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidthAA = MAX_LINE_WIDTH;
   ctx->Const.LineWidthGranularity = (GLfloat) LINE_WIDTH_GRANULARITY;
   ctx->Const.NumAuxBuffers = NUM_AUX_BUFFERS;
   ctx->Const.MaxColorTableSize = MAX_COLOR_TABLE_SIZE;
   ctx->Const.MaxConvolutionWidth = MAX_CONVOLUTION_WIDTH;
   ctx->Const.MaxConvolutionHeight = MAX_CONVOLUTION_HEIGHT;
   ctx->Const.MaxClipPlanes = MAX_CLIP_PLANES;
   ctx->Const.MaxLights = MAX_LIGHTS;
   ctx->Const.MaxSpotExponent = 128.0;
   ctx->Const.MaxShininess = 128.0;
#if FEATURE_ARB_vertex_program
   ctx->Const.MaxVertexProgramInstructions = MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS;
   ctx->Const.MaxVertexProgramAttribs = MAX_NV_VERTEX_PROGRAM_INPUTS;
   ctx->Const.MaxVertexProgramTemps = MAX_NV_VERTEX_PROGRAM_TEMPS;
   ctx->Const.MaxVertexProgramLocalParams = MAX_NV_VERTEX_PROGRAM_PARAMS;
   ctx->Const.MaxVertexProgramEnvParams = MAX_NV_VERTEX_PROGRAM_PARAMS;/*XXX*/
   ctx->Const.MaxVertexProgramAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
#endif
#if FEATURE_ARB_fragment_program
   ctx->Const.MaxFragmentProgramInstructions = MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS;
   ctx->Const.MaxFragmentProgramAttribs = MAX_NV_FRAGMENT_PROGRAM_INPUTS;
   ctx->Const.MaxFragmentProgramTemps = MAX_NV_FRAGMENT_PROGRAM_TEMPS;
   ctx->Const.MaxFragmentProgramLocalParams = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
   ctx->Const.MaxFragmentProgramEnvParams = MAX_NV_FRAGMENT_PROGRAM_PARAMS;/*XXX*/
   ctx->Const.MaxFragmentProgramAddressRegs = MAX_FRAGMENT_PROGRAM_ADDRESS_REGS;
   ctx->Const.MaxFragmentProgramAluInstructions = MAX_FRAGMENT_PROGRAM_ALU_INSTRUCTIONS;
   ctx->Const.MaxFragmentProgramTexInstructions = MAX_FRAGMENT_PROGRAM_TEX_INSTRUCTIONS;
   ctx->Const.MaxFragmentProgramTexIndirections = MAX_FRAGMENT_PROGRAM_TEX_INDIRECTIONS;
#endif
   ctx->Const.MaxProgramMatrices = MAX_PROGRAM_MATRICES;
   ctx->Const.MaxProgramMatrixStackDepth = MAX_PROGRAM_MATRIX_STACK_DEPTH;

   /* If we're running in the X server, do bounds checking to prevent
    * segfaults and server crashes!
    */
#if defined(XFree86LOADER) && defined(IN_MODULE)
   ctx->Const.CheckArrayBounds = GL_TRUE;
#else
   ctx->Const.CheckArrayBounds = GL_FALSE;
#endif

   ASSERT(ctx->Const.MaxTextureUnits == MAX2(ctx->Const.MaxTextureImageUnits, ctx->Const.MaxTextureCoordUnits));
}

/**
 * Initialize the attribute groups in a GL context.
 *
 * \param ctx GL context.
 *
 * Initializes all the attributes, calling the respective <tt>init*</tt>
 * functions for the more complex data structures.
 */
static GLboolean
init_attrib_groups( GLcontext *ctx )
{
   assert(ctx);

   /* Constants */
   _mesa_init_constants( ctx );

   /* Extensions */
   _mesa_init_extensions( ctx );

   /* Attribute Groups */
   _mesa_init_accum( ctx );
   _mesa_init_attrib( ctx );
   _mesa_init_buffers( ctx );
   _mesa_init_buffer_objects( ctx );
   _mesa_init_color( ctx );
   _mesa_init_colortables( ctx );
   _mesa_init_current( ctx );
   _mesa_init_depth( ctx );
   _mesa_init_debug( ctx );
   _mesa_init_display_list( ctx );
   _mesa_init_eval( ctx );
   _mesa_init_feedback( ctx );
   _mesa_init_fog( ctx );
   _mesa_init_histogram( ctx );
   _mesa_init_hint( ctx );
   _mesa_init_line( ctx );
   _mesa_init_lighting( ctx );
   _mesa_init_matrix( ctx );
   _mesa_init_occlude( ctx );
   _mesa_init_pixel( ctx );
   _mesa_init_point( ctx );
   _mesa_init_polygon( ctx );
   _mesa_init_program( ctx );
   _mesa_init_rastpos( ctx );
   _mesa_init_stencil( ctx );
   _mesa_init_transform( ctx );
   _mesa_init_varray( ctx );
   _mesa_init_viewport( ctx );

   if (!_mesa_init_texture( ctx ))
      return GL_FALSE;

   /* Miscellaneous */
   ctx->NewState = _NEW_ALL;
   ctx->ErrorValue = (GLenum) GL_NO_ERROR;
   ctx->CatchSignals = GL_TRUE;
   ctx->_Facing = 0;

   return GL_TRUE;
}


/**
 * If the DRI libGL.so library is old, it may not have the entrypoints for
 * some recent OpenGL extensions.  Dynamically add them now.
 * If we're building stand-alone Mesa where libGL.so has both the dispatcher
 * and driver code, this won't be an issue (and calling this function won't
 * do any harm).
 */
static void
add_newer_entrypoints(void)
{
   unsigned   i;
   static const struct {
      const char * const name;
      unsigned  offset;
   }
   newer_entrypoints[] = {
      /* GL_ARB_window_pos aliases with GL_MESA_window_pos */
      { "glWindowPos2dARB", 513 },
      { "glWindowPos2dvARB", 514 },
      { "glWindowPos2fARB", 515 },
      { "glWindowPos2fvARB", 516 },
      { "glWindowPos2iARB", 517 },
      { "glWindowPos2ivARB", 518 },
      { "glWindowPos2sARB", 519 },
      { "glWindowPos2svARB", 520 },
      { "glWindowPos3dARB", 521 },
      { "glWindowPos3dvARB", 522 },
      { "glWindowPos3fARB", 523 },
      { "glWindowPos3fvARB", 524 },
      { "glWindowPos3iARB", 525 },
      { "glWindowPos3ivARB", 526 },
      { "glWindowPos3sARB", 527 },
      { "glWindowPos3svARB", 528 },
#if FEATURE_NV_vertex_program
      { "glAreProgramsResidentNV", 578 },
      { "glBindProgramNV", 579 },
      { "glDeleteProgramsNV", 580 },
      { "glExecuteProgramNV", 581 },
      { "glGenProgramsNV", 582 },
      { "glGetProgramParameterdvNV", 583 },
      { "glGetProgramParameterfvNV", 584 },
      { "glGetProgramivNV", 585 },
      { "glGetProgramStringNV", 586 },
      { "glGetTrackMatrixivNV", 587 },
      { "glGetVertexAttribdvNV", 588 },
      { "glGetVertexAttribfvNV", 589 },
      { "glGetVertexAttribivNV", 590 },
      { "glGetVertexAttribPointervNV", 591 },
      { "glIsProgramNV", 592 },
      { "glLoadProgramNV", 593 },
      { "glProgramParameter4dNV", 594 },
      { "glProgramParameter4dvNV", 595 },
      { "glProgramParameter4fNV", 596 },
      { "glProgramParameter4fvNV", 597 },
      { "glProgramParameters4dvNV", 598 },
      { "glProgramParameters4fvNV", 599 },
      { "glRequestResidentProgramsNV", 600 },
      { "glTrackMatrixNV", 601 },
      { "glVertexAttribPointerNV", 602 },
      { "glVertexAttrib1dNV", 603 },
      { "glVertexAttrib1dvNV", 604 },
      { "glVertexAttrib1fNV", 605 },
      { "glVertexAttrib1fvNV", 606 },
      { "glVertexAttrib1sNV", 607 },
      { "glVertexAttrib1svNV", 608 },
      { "glVertexAttrib2dNV", 609 },
      { "glVertexAttrib2dvNV", 610 },
      { "glVertexAttrib2fNV", 611 },
      { "glVertexAttrib2fvNV", 612 },
      { "glVertexAttrib2sNV", 613 },
      { "glVertexAttrib2svNV", 614 },
      { "glVertexAttrib3dNV", 615 },
      { "glVertexAttrib3dvNV", 616 },
      { "glVertexAttrib3fNV", 617 },
      { "glVertexAttrib3fvNV", 618 },
      { "glVertexAttrib3sNV", 619 },
      { "glVertexAttrib3svNV", 620 },
      { "glVertexAttrib4dNV", 621 },
      { "glVertexAttrib4dvNV", 622 },
      { "glVertexAttrib4fNV", 623 },
      { "glVertexAttrib4fvNV", 624 },
      { "glVertexAttrib4sNV", 625 },
      { "glVertexAttrib4svNV", 626 },
      { "glVertexAttrib4ubNV", 627 },
      { "glVertexAttrib4ubvNV", 628 },
      { "glVertexAttribs1dvNV", 629 },
      { "glVertexAttribs1fvNV", 630 },
      { "glVertexAttribs1svNV", 631 },
      { "glVertexAttribs2dvNV", 632 },
      { "glVertexAttribs2fvNV", 633 },
      { "glVertexAttribs2svNV", 634 },
      { "glVertexAttribs3dvNV", 635 },
      { "glVertexAttribs3fvNV", 636 },
      { "glVertexAttribs3svNV", 637 },
      { "glVertexAttribs4dvNV", 638 },
      { "glVertexAttribs4fvNV", 639 },
      { "glVertexAttribs4svNV", 640 },
      { "glVertexAttribs4ubvNV", 641 },
#endif
      { "glPointParameteriNV", 642 },
      { "glPointParameterivNV", 643 },
      { "glMultiDrawArraysEXT", 644 },
      { "glMultiDrawElementsEXT", 645 },
      { "glMultiDrawArraysSUN", _gloffset_MultiDrawArraysEXT  },
      { "glMultiDrawElementsSUN", _gloffset_MultiDrawElementsEXT  },
      { "glActiveStencilFaceEXT", 646 },
#if FEATURE_NV_fence
      { "glDeleteFencesNV", 647 },
      { "glGenFencesNV", 648 },
      { "glIsFenceNV", 649 },
      { "glTestFenceNV", 650 },
      { "glGetFenceivNV", 651 },
      { "glFinishFenceNV", 652 },
      { "glSetFenceNV", 653 },
#endif
#if FEATURE_NV_fragment_program
      { "glProgramNamedParameter4fNV", 682 },
      { "glProgramNamedParameter4dNV", 683 },
      { "glProgramNamedParameter4fvNV", 683 },
      { "glProgramNamedParameter4dvNV", 684 },
      { "glGetProgramNamedParameterfvNV", 685 },
      { "glGetProgramNamedParameterdvNV", 686 },
#endif
#if FEATURE_ARB_vertex_program
      { "glVertexAttrib1sARB", _gloffset_VertexAttrib1sNV },
      { "glVertexAttrib1fARB", _gloffset_VertexAttrib1fNV },
      { "glVertexAttrib1dARB", _gloffset_VertexAttrib1dNV },
      { "glVertexAttrib2sARB", _gloffset_VertexAttrib2sNV },
      { "glVertexAttrib2fARB", _gloffset_VertexAttrib2fNV },
      { "glVertexAttrib2dARB", _gloffset_VertexAttrib2dNV },
      { "glVertexAttrib3sARB", _gloffset_VertexAttrib3sNV },
      { "glVertexAttrib3fARB", _gloffset_VertexAttrib3fNV },
      { "glVertexAttrib3dARB", _gloffset_VertexAttrib3dNV },
      { "glVertexAttrib4sARB", _gloffset_VertexAttrib4sNV },
      { "glVertexAttrib4fARB", _gloffset_VertexAttrib4fNV },
      { "glVertexAttrib4dARB", _gloffset_VertexAttrib4dNV },
      { "glVertexAttrib4NubARB", _gloffset_VertexAttrib4ubNV },
      { "glVertexAttrib1svARB", _gloffset_VertexAttrib1svNV },
      { "glVertexAttrib1fvARB", _gloffset_VertexAttrib1fvNV },
      { "glVertexAttrib1dvARB", _gloffset_VertexAttrib1dvNV },
      { "glVertexAttrib2svARB", _gloffset_VertexAttrib2svNV },
      { "glVertexAttrib2fvARB", _gloffset_VertexAttrib2fvNV },
      { "glVertexAttrib2dvARB", _gloffset_VertexAttrib2dvNV },
      { "glVertexAttrib3svARB", _gloffset_VertexAttrib3svNV },
      { "glVertexAttrib3fvARB", _gloffset_VertexAttrib3fvNV },
      { "glVertexAttrib3dvARB", _gloffset_VertexAttrib3dvNV },
      { "glVertexAttrib4bvARB", _gloffset_VertexAttrib4bvARB },
      { "glVertexAttrib4svARB", _gloffset_VertexAttrib4svNV },
      { "glVertexAttrib4ivARB", _gloffset_VertexAttrib4ivARB },
      { "glVertexAttrib4ubvARB", _gloffset_VertexAttrib4ubvARB },
      { "glVertexAttrib4usvARB", _gloffset_VertexAttrib4usvARB },
      { "glVertexAttrib4uivARB", _gloffset_VertexAttrib4uivARB },
      { "glVertexAttrib4fvARB", _gloffset_VertexAttrib4fvNV },
      { "glVertexAttrib4dvARB", _gloffset_VertexAttrib4dvNV },
      { "glVertexAttrib4NbvARB", _gloffset_VertexAttrib4NbvARB },
      { "glVertexAttrib4NsvARB", _gloffset_VertexAttrib4NsvARB },
      { "glVertexAttrib4NivARB", _gloffset_VertexAttrib4NivARB },
      { "glVertexAttrib4NubvARB", _gloffset_VertexAttrib4ubvNV },
      { "glVertexAttrib4NusvARB", _gloffset_VertexAttrib4NusvARB },
      { "glVertexAttrib4NuivARB", _gloffset_VertexAttrib4NuivARB },
      { "glVertexAttribPointerARB", _gloffset_VertexAttribPointerARB },
      { "glEnableVertexAttribArrayARB", _gloffset_EnableVertexAttribArrayARB },
      { "glDisableVertexAttribArrayARB", _gloffset_DisableVertexAttribArrayARB },
      { "glProgramStringARB", _gloffset_ProgramStringARB },
      { "glBindProgramARB", _gloffset_BindProgramNV },
      { "glDeleteProgramsARB", _gloffset_DeleteProgramsNV },
      { "glGenProgramsARB", _gloffset_GenProgramsNV },
      { "glIsProgramARB", _gloffset_IsProgramNV },
      { "glProgramEnvParameter4dARB", _gloffset_ProgramEnvParameter4dARB },
      { "glProgramEnvParameter4dvARB", _gloffset_ProgramEnvParameter4dvARB },
      { "glProgramEnvParameter4fARB", _gloffset_ProgramEnvParameter4fARB },
      { "glProgramEnvParameter4fvARB", _gloffset_ProgramEnvParameter4fvARB },
      { "glProgramLocalParameter4dARB", _gloffset_ProgramLocalParameter4dARB },
      { "glProgramLocalParameter4dvARB", _gloffset_ProgramLocalParameter4dvARB },
      { "glProgramLocalParameter4fARB", _gloffset_ProgramLocalParameter4fARB },
      { "glProgramLocalParameter4fvARB", _gloffset_ProgramLocalParameter4fvARB },
      { "glGetProgramEnvParameterdvARB", _gloffset_GetProgramEnvParameterdvARB },
      { "glGetProgramEnvParameterfvARB", _gloffset_GetProgramEnvParameterfvARB },
      { "glGetProgramLocalParameterdvARB", _gloffset_GetProgramLocalParameterdvARB },
      { "glGetProgramLocalParameterfvARB", _gloffset_GetProgramLocalParameterfvARB },
      { "glGetProgramivARB", _gloffset_GetProgramivARB },
      { "glGetProgramStringARB", _gloffset_GetProgramStringARB },
      { "glGetVertexAttribdvARB", _gloffset_GetVertexAttribdvNV },
      { "glGetVertexAttribfvARB", _gloffset_GetVertexAttribfvNV },
      { "glGetVertexAttribivARB", _gloffset_GetVertexAttribivNV },
      { "glGetVertexAttribPointervARB", _gloffset_GetVertexAttribPointervNV },
#endif
      { "glMultiModeDrawArraysIBM", _gloffset_MultiModeDrawArraysIBM },
      { "glMultiModeDrawElementsIBM", _gloffset_MultiModeDrawElementsIBM },
   };
   
   for ( i = 0 ; i < Elements(newer_entrypoints) ; i++ ) {
      _glapi_add_entrypoint( newer_entrypoints[i].name,
			     newer_entrypoints[i].offset );
   }
}


/**
 * Initialize a GLcontext struct. 
 *
 * This includes allocating all the other structs and arrays which hang off of
 * the context by pointers.
 * 
 * \sa _mesa_create_context() for the parameter description.
 *
 * Performs the imports and exports callback tables initialization, and
 * miscellaneous one-time initializations. If no shared context is supplied one
 * is allocated, and increase its reference count.  Setups the GL API dispatch
 * tables.  Initialize the TNL module. Sets the maximum Z buffer depth.
 * Finally queries the \c MESA_DEBUG and \c MESA_VERBOSE environment variables
 * for debug flags.
 *
 * \note the direct parameter is ignored (obsolete).
 */
GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          const GLvisual *visual,
                          GLcontext *share_list,
                          void *driver_ctx,
                          GLboolean direct )
{
   GLuint dispatchSize;

   ASSERT(driver_ctx);

   /* If the driver wants core Mesa to use special imports, it'll have to
    * override these defaults.
    */
   _mesa_init_default_imports( &(ctx->imports), driver_ctx );

   /* initialize the exports (Mesa functions called by the window system) */
   _mesa_init_default_exports( &(ctx->exports) );

   /* misc one-time initializations */
   one_time_init(ctx);

   ctx->DriverCtx = driver_ctx;
   ctx->Visual = *visual;
   ctx->DrawBuffer = NULL;
   ctx->ReadBuffer = NULL;

   /* Set these pointers to defaults now in case they're not set since
    * we need them while creating the default textures.
    */
   if (!ctx->Driver.NewTextureObject)
      ctx->Driver.NewTextureObject = _mesa_new_texture_object;
   if (!ctx->Driver.DeleteTexture)
      ctx->Driver.DeleteTexture = _mesa_delete_texture_object;
   if (!ctx->Driver.NewTextureImage)
      ctx->Driver.NewTextureImage = _mesa_new_texture_image;

   if (share_list) {
      /* share state with another context */
      ctx->Shared = share_list->Shared;
   }
   else {
      /* allocate new, unshared state */
      if (!alloc_shared_state( ctx )) {
         return GL_FALSE;
      }
   }
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount++;
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   if (!init_attrib_groups( ctx )) {
      free_shared_state(ctx, ctx->Shared);
      return GL_FALSE;
   }

   /* libGL ABI coordination */
   add_newer_entrypoints();

   /* Find the larger of Mesa's dispatch table and libGL's dispatch table.
    * In practice, this'll be the same for stand-alone Mesa.  But for DRI
    * Mesa we do this to accomodate different versions of libGL and various
    * DRI drivers.
    */
   dispatchSize = MAX2(_glapi_get_dispatch_table_size(),
                       sizeof(struct _glapi_table) / sizeof(void *));

   /* setup API dispatch tables */
   ctx->Exec = (struct _glapi_table *) CALLOC(dispatchSize * sizeof(void*));
   ctx->Save = (struct _glapi_table *) CALLOC(dispatchSize * sizeof(void*));
   if (!ctx->Exec || !ctx->Save) {
      free_shared_state(ctx, ctx->Shared);
      if (ctx->Exec)
         FREE( ctx->Exec );
   }
   _mesa_init_exec_table(ctx->Exec, dispatchSize);
   ctx->CurrentDispatch = ctx->Exec;

#if _HAVE_FULL_GL
   _mesa_init_dlist_table(ctx->Save, dispatchSize);
   _mesa_install_save_vtxfmt( ctx, &ctx->ListState.ListVtxfmt );


   /* Neutral tnl module stuff */
   _mesa_init_exec_vtxfmt( ctx ); 
   ctx->TnlModule.Current = NULL;
   ctx->TnlModule.SwapCount = 0;
#endif

   return GL_TRUE;
}

/**
 * Allocate and initialize a GLcontext structure.
 *
 * \param visual a GLvisual pointer (we copy the struct contents)
 * \param share_list another context to share display lists with or NULL
 * \param driver_ctx pointer to device driver's context state struct
 * \param direct obsolete, ignored
 * 
 * \return pointer to a new __GLcontextRec or NULL if error.
 */
GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      void *driver_ctx,
                      GLboolean direct )

{
   GLcontext *ctx;

   ASSERT(visual);
   ASSERT(driver_ctx);

   ctx = (GLcontext *) _mesa_calloc(sizeof(GLcontext));
   if (!ctx)
      return NULL;

   if (_mesa_initialize_context(ctx, visual, share_list, driver_ctx, direct)) {
      return ctx;
   }
   else {
      _mesa_free(ctx);
      return NULL;
   }
}

/**
 * Free the data associated with the given context.
 * 
 * But doesn't free the GLcontext struct itself.
 *
 * \sa _mesa_initialize_context() and init_attrib_groups().
 */
void
_mesa_free_context_data( GLcontext *ctx )
{
   /* if we're destroying the current context, unbind it first */
   if (ctx == _mesa_get_current_context()) {
      _mesa_make_current(NULL, NULL);
   }

   _mesa_free_lighting_data( ctx );
   _mesa_free_eval_data( ctx );
   _mesa_free_texture_data( ctx );
   _mesa_free_matrix_data( ctx );
   _mesa_free_viewport_data( ctx );
   _mesa_free_colortables_data( ctx );
#if FEATURE_NV_vertex_program
   if (ctx->VertexProgram.Current) {
      ctx->VertexProgram.Current->Base.RefCount--;
      if (ctx->VertexProgram.Current->Base.RefCount <= 0)
         _mesa_delete_program(ctx, &(ctx->VertexProgram.Current->Base));
   }
#endif
#if FEATURE_NV_fragment_program
   if (ctx->FragmentProgram.Current) {
      ctx->FragmentProgram.Current->Base.RefCount--;
      if (ctx->FragmentProgram.Current->Base.RefCount <= 0)
         _mesa_delete_program(ctx, &(ctx->FragmentProgram.Current->Base));
   }
#endif

   /* Shared context state (display lists, textures, etc) */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount--;
   assert(ctx->Shared->RefCount >= 0);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   if (ctx->Shared->RefCount == 0) {
      /* free shared state */
      free_shared_state( ctx, ctx->Shared );
   }

   if (ctx->Extensions.String)
      FREE((void *) ctx->Extensions.String);

   FREE(ctx->Exec);
   FREE(ctx->Save);
}

/**
 * Destroy a GLcontext structure.
 *
 * \param ctx GL context.
 * 
 * Calls _mesa_free_context_data() and free the structure.
 */
void
_mesa_destroy_context( GLcontext *ctx )
{
   if (ctx) {
      _mesa_free_context_data(ctx);
      FREE( (void *) ctx );
   }
}

#if _HAVE_FULL_GL
/**
 * Copy attribute groups from one context to another.
 * 
 * \param src source context
 * \param dst destination context
 * \param mask bitwise OR of GL_*_BIT flags
 *
 * According to the bits specified in \p mask, copies the corresponding
 * attributes from \p src into \dst.  For many of the attributes a simple \c
 * memcpy is not enough due to the existence of internal pointers in their data
 * structures.
 */
void
_mesa_copy_context( const GLcontext *src, GLcontext *dst, GLuint mask )
{
   if (mask & GL_ACCUM_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Accum = src->Accum;
   }
   if (mask & GL_COLOR_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Color = src->Color;
   }
   if (mask & GL_CURRENT_BIT) {
      /* OK to memcpy */
      dst->Current = src->Current;
   }
   if (mask & GL_DEPTH_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Depth = src->Depth;
   }
   if (mask & GL_ENABLE_BIT) {
      /* no op */
   }
   if (mask & GL_EVAL_BIT) {
      /* OK to memcpy */
      dst->Eval = src->Eval;
   }
   if (mask & GL_FOG_BIT) {
      /* OK to memcpy */
      dst->Fog = src->Fog;
   }
   if (mask & GL_HINT_BIT) {
      /* OK to memcpy */
      dst->Hint = src->Hint;
   }
   if (mask & GL_LIGHTING_BIT) {
      GLuint i;
      /* begin with memcpy */
      MEMCPY( &dst->Light, &src->Light, sizeof(struct gl_light) );
      /* fixup linked lists to prevent pointer insanity */
      make_empty_list( &(dst->Light.EnabledList) );
      for (i = 0; i < MAX_LIGHTS; i++) {
         if (dst->Light.Light[i].Enabled) {
            insert_at_tail(&(dst->Light.EnabledList), &(dst->Light.Light[i]));
         }
      }
   }
   if (mask & GL_LINE_BIT) {
      /* OK to memcpy */
      dst->Line = src->Line;
   }
   if (mask & GL_LIST_BIT) {
      /* OK to memcpy */
      dst->List = src->List;
   }
   if (mask & GL_PIXEL_MODE_BIT) {
      /* OK to memcpy */
      dst->Pixel = src->Pixel;
   }
   if (mask & GL_POINT_BIT) {
      /* OK to memcpy */
      dst->Point = src->Point;
   }
   if (mask & GL_POLYGON_BIT) {
      /* OK to memcpy */
      dst->Polygon = src->Polygon;
   }
   if (mask & GL_POLYGON_STIPPLE_BIT) {
      /* Use loop instead of MEMCPY due to problem with Portland Group's
       * C compiler.  Reported by John Stone.
       */
      GLuint i;
      for (i = 0; i < 32; i++) {
         dst->PolygonStipple[i] = src->PolygonStipple[i];
      }
   }
   if (mask & GL_SCISSOR_BIT) {
      /* OK to memcpy */
      dst->Scissor = src->Scissor;
   }
   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Stencil = src->Stencil;
   }
   if (mask & GL_TEXTURE_BIT) {
      /* Cannot memcpy because of pointers */
      _mesa_copy_texture_state(src, dst);
   }
   if (mask & GL_TRANSFORM_BIT) {
      /* OK to memcpy */
      dst->Transform = src->Transform;
   }
   if (mask & GL_VIEWPORT_BIT) {
      /* Cannot use memcpy, because of pointers in GLmatrix _WindowMap */
      dst->Viewport.X = src->Viewport.X;
      dst->Viewport.Y = src->Viewport.Y;
      dst->Viewport.Width = src->Viewport.Width;
      dst->Viewport.Height = src->Viewport.Height;
      dst->Viewport.Near = src->Viewport.Near;
      dst->Viewport.Far = src->Viewport.Far;
      _math_matrix_copy(&dst->Viewport._WindowMap, &src->Viewport._WindowMap);
   }

   /* XXX FIXME:  Call callbacks?
    */
   dst->NewState = _NEW_ALL;
}
#endif


/**
 * Check if the given context can render into the given framebuffer
 * by checking visual attributes.
 * \return GL_TRUE if compatible, GL_FALSE otherwise.
 */
static GLboolean 
check_compatible(const GLcontext *ctx, const GLframebuffer *buffer)
{
   const GLvisual *ctxvis = &ctx->Visual;
   const GLvisual *bufvis = &buffer->Visual;

   if (ctxvis == bufvis)
      return GL_TRUE;

   if (ctxvis->rgbMode != bufvis->rgbMode)
      return GL_FALSE;
   if (ctxvis->doubleBufferMode && !bufvis->doubleBufferMode)
      return GL_FALSE;
   if (ctxvis->stereoMode && !bufvis->stereoMode)
      return GL_FALSE;
   if (ctxvis->haveAccumBuffer && !bufvis->haveAccumBuffer)
      return GL_FALSE;
   if (ctxvis->haveDepthBuffer && !bufvis->haveDepthBuffer)
      return GL_FALSE;
   if (ctxvis->haveStencilBuffer && !bufvis->haveStencilBuffer)
      return GL_FALSE;
   if (ctxvis->redMask && ctxvis->redMask != bufvis->redMask)
      return GL_FALSE;
   if (ctxvis->greenMask && ctxvis->greenMask != bufvis->greenMask)
      return GL_FALSE;
   if (ctxvis->blueMask && ctxvis->blueMask != bufvis->blueMask)
      return GL_FALSE;
   if (ctxvis->depthBits && ctxvis->depthBits != bufvis->depthBits)
      return GL_FALSE;
   if (ctxvis->stencilBits && ctxvis->stencilBits != bufvis->stencilBits)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Set the current context, binding the given frame buffer to the context.
 *
 * \param newCtx new GL context.
 * \param buffer framebuffer.
 * 
 * Calls _mesa_make_current2() with \p buffer as read and write framebuffer.
 */
void
_mesa_make_current( GLcontext *newCtx, GLframebuffer *buffer )
{
   _mesa_make_current2( newCtx, buffer, buffer );
}

/**
 * Bind the given context to the given draw-buffer and read-buffer and
 * make it the current context for this thread.
 *
 * \param newCtx new GL context. If NULL then there will be no current GL
 * context.
 * \param drawBuffer draw framebuffer.
 * \param readBuffer read framebuffer.
 * 
 * Check that the context's and framebuffer's visuals are compatible, returning
 * immediately otherwise. Sets the glapi current context via
 * _glapi_set_context(). If \p newCtx is not NULL, associates \p drawBuffer and
 * \p readBuffer with it and calls dd_function_table::ResizeBuffers if the buffers size has changed. 
 * Calls dd_function_table::MakeCurrent callback if defined.
 *
 * When a context is bound by the first time and the \c MESA_INFO environment
 * variable is set it calls print_info() as an aid for remote user
 * troubleshooting.
 */
void
_mesa_make_current2( GLcontext *newCtx, GLframebuffer *drawBuffer,
                     GLframebuffer *readBuffer )
{
   if (MESA_VERBOSE)
      _mesa_debug(newCtx, "_mesa_make_current2()\n");

   /* Check that the context's and framebuffer's visuals are compatible.
    */
   if (newCtx && drawBuffer && newCtx->DrawBuffer != drawBuffer) {
      if (!check_compatible(newCtx, drawBuffer))
         return;
   }
   if (newCtx && readBuffer && newCtx->ReadBuffer != readBuffer) {
      if (!check_compatible(newCtx, readBuffer))
         return;
   }

   /* We call this function periodically (just here for now) in
    * order to detect when multithreading has begun.
    */
   _glapi_check_multithread();

   _glapi_set_context((void *) newCtx);
   ASSERT(_mesa_get_current_context() == newCtx);


   if (!newCtx) {
      _glapi_set_dispatch(NULL);  /* none current */
   }
   else {
      _glapi_set_dispatch(newCtx->CurrentDispatch);

      if (drawBuffer && readBuffer) {
	 /* TODO: check if newCtx and buffer's visual match??? */
	 newCtx->DrawBuffer = drawBuffer;
	 newCtx->ReadBuffer = readBuffer;
	 newCtx->NewState |= _NEW_BUFFERS;

#if _HAVE_FULL_GL
         if (drawBuffer->Width == 0 && drawBuffer->Height == 0) {
            /* get initial window size */
            GLuint bufWidth, bufHeight;

            /* ask device driver for size of output buffer */
            (*newCtx->Driver.GetBufferSize)( drawBuffer, &bufWidth, &bufHeight );

            if (drawBuffer->Width != bufWidth || 
		drawBuffer->Height != bufHeight) {

	       drawBuffer->Width = bufWidth;
	       drawBuffer->Height = bufHeight;

 	       newCtx->Driver.ResizeBuffers( drawBuffer );
	    }
         }

         if (readBuffer != drawBuffer &&
             readBuffer->Width == 0 && readBuffer->Height == 0) {
            /* get initial window size */
            GLuint bufWidth, bufHeight;

            /* ask device driver for size of output buffer */
            (*newCtx->Driver.GetBufferSize)( readBuffer, &bufWidth, &bufHeight );

            if (readBuffer->Width != bufWidth ||
		readBuffer->Height != bufHeight) {

	       readBuffer->Width = bufWidth;
	       readBuffer->Height = bufHeight;

	       newCtx->Driver.ResizeBuffers( readBuffer );
	    }
         }
#endif
      }

      /* Alert the driver - usually passed on to the sw t&l module,
       * but also used to detect threaded cases in the radeon codegen
       * hw t&l module.
       */
      if (newCtx->Driver.MakeCurrent)
	 newCtx->Driver.MakeCurrent( newCtx, drawBuffer, readBuffer );

      /* We can use this to help debug user's problems.  Tell them to set
       * the MESA_INFO env variable before running their app.  Then the
       * first time each context is made current we'll print some useful
       * information.
       */
      if (newCtx->FirstTimeCurrent) {
	 if (_mesa_getenv("MESA_INFO")) {
	    _mesa_print_info();
	 }
	 newCtx->FirstTimeCurrent = GL_FALSE;
      }
   }
}

/**
 * Get current context for the calling thread.
 * 
 * \return pointer to the current GL context.
 * 
 * Calls _glapi_get_context(). This isn't the fastest way to get the current
 * context.  If you need speed, see the #GET_CURRENT_CONTEXT macro in context.h.
 */
GLcontext *
_mesa_get_current_context( void )
{
   return (GLcontext *) _glapi_get_context();
}

/**
 * Get context's current API dispatch table.
 *
 * It'll either be the immediate-mode execute dispatcher or the display list
 * compile dispatcher.
 * 
 * \param ctx GL context.
 *
 * \return pointer to dispatch_table.
 *
 * Simply returns __GLcontextRec::CurrentDispatch.
 */
struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx)
{
   return ctx->CurrentDispatch;
}

/*@}*/


/**********************************************************************/
/** \name Miscellaneous functions                                     */
/**********************************************************************/
/*@{*/

/**
 * Record an error.
 *
 * \param ctx GL context.
 * \param error error code.
 * 
 * Records the given error code and call the driver's dd_function_table::Error
 * function if defined.
 *
 * \sa
 * This is called via _mesa_error().
 */
void
_mesa_record_error( GLcontext *ctx, GLenum error )
{
   if (!ctx)
      return;

   if (ctx->ErrorValue == GL_NO_ERROR) {
      ctx->ErrorValue = error;
   }

   /* Call device driver's error handler, if any.  This is used on the Mac. */
   if (ctx->Driver.Error) {
      (*ctx->Driver.Error)( ctx );
   }
}

/**
 * Execute glFinish().
 *
 * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
 * dd_function_table::Finish driver callback, if not NULL.
 */
void GLAPIENTRY
_mesa_Finish( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (ctx->Driver.Finish) {
      (*ctx->Driver.Finish)( ctx );
   }
}

/**
 * Execute glFlush().
 *
 * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
 * dd_function_table::Flush driver callback, if not NULL.
 */
void GLAPIENTRY
_mesa_Flush( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (ctx->Driver.Flush) {
      (*ctx->Driver.Flush)( ctx );
   }
}


/*@}*/
