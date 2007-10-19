/**
 * \file context.c
 * Mesa context/visual/framebuffer management functions.
 * \author Brian Paul
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.4
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * \mainpage Mesa Main Module
 *
 * \section MainIntroduction Introduction
 *
 * The Mesa Main module consists of all the files in the main/ directory.
 * Among the features of this module are:
 * <UL>
 * <LI> Structures to represent most GL state </LI>
 * <LI> State set/get functions </LI>
 * <LI> Display lists </LI>
 * <LI> Texture unit, object and image handling </LI>
 * <LI> Matrix and attribute stacks </LI>
 * </UL>
 *
 * Other modules are responsible for API dispatch, vertex transformation,
 * point/line/triangle setup, rasterization, vertex array caching,
 * vertex/fragment programs/shaders, etc.
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
#include "fbobject.h"
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
#include "texcompress.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#include "version.h"
#include "vtxfmt.h"
#if _HAVE_FULL_GL
#include "math/m_translate.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"
#include "math/mathmod.h"
#endif
#include "shaderobjects.h"

#ifdef USE_SPARC_ASM
#include "sparc/sparc.h"
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
   (void) gc;
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
   (void) gc;
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
   (void) gc;
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
   (void) gc;
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
   (void) gc;
   return NULL;
}

/** No-op */
void
_mesa_beginDispatchOverride(__GLcontext *gc)
{
   (void) gc;
}

/** No-op */
void
_mesa_endDispatchOverride(__GLcontext *gc)
{
   (void) gc;
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
#else
    (void) exports;
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

    /* XXX doesn't work at this time */
    _mesa_initialize_context(ctx, modes, NULL, NULL, NULL);
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
 * Allocates a GLvisual structure and initializes it via
 * _mesa_initialize_visual().
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
 * \note Need to add params for level and numAuxBuffers (at least)
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
 * Makes some sanity checks and fills in the fields of the
 * GLvisual object with the given parameters.  If the caller needs
 * to set additional fields, he should just probably init the whole GLvisual
 * object himself.
 * \return GL_TRUE on success, or GL_FALSE on failure.
 *
 * \sa _mesa_create_visual() above for the parameter description.
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
   assert(vis);

   if (depthBits < 0 || depthBits > 32) {
      return GL_FALSE;
   }
   if (stencilBits < 0 || stencilBits > STENCIL_BITS) {
      return GL_FALSE;
   }
   if (accumRedBits < 0 || accumRedBits > ACCUM_BITS) {
      return GL_FALSE;
   }
   if (accumGreenBits < 0 || accumGreenBits > ACCUM_BITS) {
      return GL_FALSE;
   }
   if (accumBlueBits < 0 || accumBlueBits > ACCUM_BITS) {
      return GL_FALSE;
   }
   if (accumAlphaBits < 0 || accumAlphaBits > ACCUM_BITS) {
      return GL_FALSE;
   }

   vis->rgbMode          = rgbFlag;
   vis->doubleBufferMode = dbFlag;
   vis->stereoMode       = stereoFlag;

   vis->redBits          = redBits;
   vis->greenBits        = greenBits;
   vis->blueBits         = blueBits;
   vis->alphaBits        = alphaBits;
   vis->rgbBits          = redBits + greenBits + blueBits;

   vis->indexBits      = indexBits;
   vis->depthBits      = depthBits;
   vis->stencilBits    = stencilBits;

   vis->accumRedBits   = accumRedBits;
   vis->accumGreenBits = accumGreenBits;
   vis->accumBlueBits  = accumBlueBits;
   vis->accumAlphaBits = accumAlphaBits;

   vis->haveAccumBuffer   = accumRedBits > 0;
   vis->haveDepthBuffer   = depthBits > 0;
   vis->haveStencilBuffer = stencilBits > 0;

   vis->numAuxBuffers = 0;
   vis->level = 0;
   vis->pixmapMode = 0;
   vis->sampleBuffers = numSamples > 0 ? 1 : 0;
   vis->samples = numSamples;

   return GL_TRUE;
}


/**
 * Destroy a visual and free its memory.
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
   (void) ctx;
   _glthread_LOCK_MUTEX(OneTimeLock);
   if (!alreadyCalled) {
      GLuint i;

      /* do some implementation tests */
      assert( sizeof(GLbyte) == 1 );
      assert( sizeof(GLubyte) == 1 );
      assert( sizeof(GLshort) == 2 );
      assert( sizeof(GLushort) == 2 );
      assert( sizeof(GLint) == 4 );
      assert( sizeof(GLuint) == 4 );

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
         _glapi_set_warning_func( (_glapi_warning_func) _mesa_warning );
      }
      else {
         _glapi_noop_enable_warnings(GL_FALSE);
      }

#if defined(DEBUG) && defined(__DATE__) && defined(__TIME__)
      _mesa_debug(ctx, "Mesa %s DEBUG build %s %s\n",
                  MESA_VERSION_STRING, __DATE__, __TIME__);
#endif

      alreadyCalled = GL_TRUE;
   }
   _glthread_UNLOCK_MUTEX(OneTimeLock);
}


/**
 * Allocate and initialize a shared context state structure.
 * Initializes the display list, texture objects and vertex programs hash
 * tables, allocates the texture objects. If it runs out of memory, frees
 * everything already allocated before returning NULL.
 *
 * \return pointer to a gl_shared_state structure on success, or NULL on
 * failure.
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
   ss->DefaultVertexProgram = ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
   if (!ss->DefaultVertexProgram)
      goto cleanup;
#endif
#if FEATURE_ARB_fragment_program
   ss->DefaultFragmentProgram = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!ss->DefaultFragmentProgram)
      goto cleanup;
#endif
#if FEATURE_ATI_fragment_shader
   ss->DefaultFragmentShader = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_SHADER_ATI, 0);
   if (!ss->DefaultFragmentShader)
      goto cleanup;
#endif

   ss->BufferObjects = _mesa_NewHashTable();

   ss->GL2Objects = _mesa_NewHashTable ();

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

   /* Effectively bind the default textures to all texture units */
   ss->Default1D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->Default2D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->Default3D->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->DefaultCubeMap->RefCount += MAX_TEXTURE_IMAGE_UNITS;
   ss->DefaultRect->RefCount += MAX_TEXTURE_IMAGE_UNITS;

#if FEATURE_EXT_framebuffer_object
   ss->FrameBuffers = _mesa_NewHashTable();
   if (!ss->FrameBuffers)
      goto cleanup;
   ss->RenderBuffers = _mesa_NewHashTable();
   if (!ss->RenderBuffers)
      goto cleanup;
#endif


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
      ctx->Driver.DeleteProgram(ctx, ss->DefaultVertexProgram);
#endif
#if FEATURE_ARB_fragment_program
   if (ss->DefaultFragmentProgram)
      ctx->Driver.DeleteProgram(ctx, ss->DefaultFragmentProgram);
#endif
#if FEATURE_ATI_fragment_shader
   if (ss->DefaultFragmentShader)
      ctx->Driver.DeleteProgram(ctx, ss->DefaultFragmentShader);
#endif
#if FEATURE_ARB_vertex_buffer_object
   if (ss->BufferObjects)
      _mesa_DeleteHashTable(ss->BufferObjects);
#endif

   if (ss->GL2Objects)
      _mesa_DeleteHashTable (ss->GL2Objects);

#if FEATURE_EXT_framebuffer_object
   if (ss->FrameBuffers)
      _mesa_DeleteHashTable(ss->FrameBuffers);
   if (ss->RenderBuffers)
      _mesa_DeleteHashTable(ss->RenderBuffers);
#endif

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
   /* the default textures */
   (*ctx->Driver.DeleteTexture)(ctx, ss->Default1D);
   (*ctx->Driver.DeleteTexture)(ctx, ss->Default2D);
   (*ctx->Driver.DeleteTexture)(ctx, ss->Default3D);
   (*ctx->Driver.DeleteTexture)(ctx, ss->DefaultCubeMap);
   (*ctx->Driver.DeleteTexture)(ctx, ss->DefaultRect);
   /* all other textures */
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
         ctx->Driver.DeleteProgram(ctx, p);
         _mesa_HashRemove(ss->Programs, prog);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->Programs);
#endif
#if FEATURE_ARB_vertex_program
   _mesa_delete_program(ctx, ss->DefaultVertexProgram);
#endif
#if FEATURE_ARB_fragment_program
   _mesa_delete_program(ctx, ss->DefaultFragmentProgram);
#endif
#if FEATURE_ATI_fragment_shader
   _mesa_delete_program(ctx, ss->DefaultFragmentShader);
#endif

#if FEATURE_ARB_vertex_buffer_object
   _mesa_DeleteHashTable(ss->BufferObjects);
#endif

   _mesa_DeleteHashTable (ss->GL2Objects);

#if FEATURE_EXT_framebuffer_object
   _mesa_DeleteHashTable(ss->FrameBuffers);
   _mesa_DeleteHashTable(ss->RenderBuffers);
#endif

   _glthread_DESTROY_MUTEX(ss->Mutex);

   FREE(ss);
}


/**
 * Initialize fields of gl_current_attrib (aka ctx->Current.*)
 */
static void
_mesa_init_current( GLcontext *ctx )
{
   GLuint i;

   /* Current group */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      ASSIGN_4V( ctx->Current.Attrib[i], 0.0, 0.0, 0.0, 1.0 );
   }
   /* special cases: */
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_WEIGHT], 1.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_NORMAL], 0.0, 0.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR0], 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR1], 0.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_FOG], 0.0, 0.0, 0.0, 0.0 );

   ctx->Current.Index = 1;
   ctx->Current.EdgeFlag = GL_TRUE;
}


/**
 * Initialize fields of gl_constants (aka ctx->Const.*).
 * Use defaults from config.h.  The device drivers will often override
 * some of these values (such as number of texture units).
 */
static void
_mesa_init_constants( GLcontext *ctx )
{
   assert(ctx);

   assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
   assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

   /* Constants, may be overriden (usually only reduced) by device drivers */
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
   ctx->Const.MaxColorTableSize = MAX_COLOR_TABLE_SIZE;
   ctx->Const.MaxConvolutionWidth = MAX_CONVOLUTION_WIDTH;
   ctx->Const.MaxConvolutionHeight = MAX_CONVOLUTION_HEIGHT;
   ctx->Const.MaxClipPlanes = MAX_CLIP_PLANES;
   ctx->Const.MaxLights = MAX_LIGHTS;
   ctx->Const.MaxShininess = 128.0;
   ctx->Const.MaxSpotExponent = 128.0;
   ctx->Const.MaxViewportWidth = MAX_WIDTH;
   ctx->Const.MaxViewportHeight = MAX_HEIGHT;
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

   ctx->Const.MaxDrawBuffers = MAX_DRAW_BUFFERS;

   /* GL_OES_read_format */
   ctx->Const.ColorReadFormat = GL_RGBA;
   ctx->Const.ColorReadType = GL_UNSIGNED_BYTE;

#if FEATURE_EXT_framebuffer_object
   ctx->Const.MaxColorAttachments = MAX_COLOR_ATTACHMENTS;
   ctx->Const.MaxRenderbufferSize = MAX_WIDTH;
#endif

   /* sanity checks */
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
   _mesa_init_multisample( ctx );
   _mesa_init_occlude( ctx );
   _mesa_init_pixel( ctx );
   _mesa_init_point( ctx );
   _mesa_init_polygon( ctx );
   _mesa_init_program( ctx );
   _mesa_init_rastpos( ctx );
   _mesa_init_scissor( ctx );
   _mesa_init_shaderobjects (ctx);
   _mesa_init_stencil( ctx );
   _mesa_init_transform( ctx );
   _mesa_init_varray( ctx );
   _mesa_init_viewport( ctx );

   if (!_mesa_init_texture( ctx ))
      return GL_FALSE;

   _mesa_init_texture_s3tc( ctx );
   _mesa_init_texture_fxt1( ctx );

   /* Miscellaneous */
   ctx->NewState = _NEW_ALL;
   ctx->ErrorValue = (GLenum) GL_NO_ERROR;
   ctx->_Facing = 0;
#if CHAN_TYPE == GL_FLOAT
   ctx->ClampFragmentColors = GL_FALSE; /* XXX temporary */
#else
   ctx->ClampFragmentColors = GL_TRUE;
#endif
   ctx->ClampVertexColors = GL_TRUE;

   return GL_TRUE;
}


/**
 * This is the default function we plug into all dispatch table slots
 * This helps prevents a segfault when someone calls a GL function without
 * first checking if the extension's supported.
 */
static int
generic_nop(void)
{
   _mesa_problem(NULL, "User called no-op dispatch function (an unsupported extension function?)");
   return 0;
}


/**
 * Allocate and initialize a new dispatch table.
 */
static struct _glapi_table *
alloc_dispatch_table(void)
{
   /* Find the larger of Mesa's dispatch table and libGL's dispatch table.
    * In practice, this'll be the same for stand-alone Mesa.  But for DRI
    * Mesa we do this to accomodate different versions of libGL and various
    * DRI drivers.
    */
   GLint numEntries = MAX2(_glapi_get_dispatch_table_size(),
                           sizeof(struct _glapi_table) / sizeof(_glapi_proc));
   struct _glapi_table *table =
      (struct _glapi_table *) _mesa_malloc(numEntries * sizeof(_glapi_proc));
   if (table) {
      _glapi_proc *entry = (_glapi_proc *) table;
      GLint i;
      for (i = 0; i < numEntries; i++) {
         entry[i] = (_glapi_proc) generic_nop;
      }
   }
   return table;
}


/**
 * Initialize a GLcontext struct (rendering context).
 *
 * This includes allocating all the other structs and arrays which hang off of
 * the context by pointers.
 * Note that the driver needs to pass in its dd_function_table here since
 * we need to at least call driverFunctions->NewTextureObject to create the
 * default texture objects.
 *
 * Called by _mesa_create_context().
 *
 * Performs the imports and exports callback tables initialization, and
 * miscellaneous one-time initializations. If no shared context is supplied one
 * is allocated, and increase its reference count.  Setups the GL API dispatch
 * tables.  Initialize the TNL module. Sets the maximum Z buffer depth.
 * Finally queries the \c MESA_DEBUG and \c MESA_VERBOSE environment variables
 * for debug flags.
 *
 * \param ctx the context to initialize
 * \param visual describes the visual attributes for this context
 * \param share_list points to context to share textures, display lists,
 *        etc with, or NULL
 * \param driverFunctions table of device driver functions for this context
 *        to use
 * \param driverContext pointer to driver-specific context data
 */
GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          const GLvisual *visual,
                          GLcontext *share_list,
                          const struct dd_function_table *driverFunctions,
                          void *driverContext )
{
   ASSERT(driverContext);
   assert(driverFunctions->NewTextureObject);
   assert(driverFunctions->FreeTexImageData);

   /* If the driver wants core Mesa to use special imports, it'll have to
    * override these defaults.
    */
   _mesa_init_default_imports( &(ctx->imports), driverContext );

   /* initialize the exports (Mesa functions called by the window system) */
   _mesa_init_default_exports( &(ctx->exports) );

   /* misc one-time initializations */
   one_time_init(ctx);

   ctx->Visual = *visual;
   ctx->DrawBuffer = NULL;
   ctx->ReadBuffer = NULL;
   ctx->WinSysDrawBuffer = NULL;
   ctx->WinSysReadBuffer = NULL;

   /* Plug in driver functions and context pointer here.
    * This is important because when we call alloc_shared_state() below
    * we'll call ctx->Driver.NewTextureObject() to create the default
    * textures.
    */
   ctx->Driver = *driverFunctions;
   ctx->DriverCtx = driverContext;

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

   /* setup the API dispatch tables */
   ctx->Exec = alloc_dispatch_table();
   ctx->Save = alloc_dispatch_table();
   if (!ctx->Exec || !ctx->Save) {
      free_shared_state(ctx, ctx->Shared);
      if (ctx->Exec)
         _mesa_free(ctx->Exec);
   }
   _mesa_init_exec_table(ctx->Exec);
   ctx->CurrentDispatch = ctx->Exec;
#if _HAVE_FULL_GL
   _mesa_init_dlist_table(ctx->Save);
   _mesa_install_save_vtxfmt( ctx, &ctx->ListState.ListVtxfmt );
   /* Neutral tnl module stuff */
   _mesa_init_exec_vtxfmt( ctx );
   ctx->TnlModule.Current = NULL;
   ctx->TnlModule.SwapCount = 0;
#endif

   ctx->_MaintainTexEnvProgram = (_mesa_getenv("MESA_TEX_PROG") != NULL);
   ctx->_MaintainTnlProgram = (_mesa_getenv("MESA_TNL_PROG") != NULL);

   return GL_TRUE;
}


/**
 * Allocate and initialize a GLcontext structure.
 * Note that the driver needs to pass in its dd_function_table here since
 * we need to at least call driverFunctions->NewTextureObject to initialize
 * the rendering context.
 *
 * \param visual a GLvisual pointer (we copy the struct contents)
 * \param share_list another context to share display lists with or NULL
 * \param driverFunctions points to the dd_function_table into which the
 *        driver has plugged in all its special functions.
 * \param driverCtx points to the device driver's private context state
 *
 * \return pointer to a new __GLcontextRec or NULL if error.
 */
GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      const struct dd_function_table *driverFunctions,
                      void *driverContext )

{
   GLcontext *ctx;

   ASSERT(visual);
   ASSERT(driverContext);

   ctx = (GLcontext *) _mesa_calloc(sizeof(GLcontext));
   if (!ctx)
      return NULL;

   if (_mesa_initialize_context(ctx, visual, share_list,
                                driverFunctions, driverContext)) {
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
      _mesa_make_current(NULL, NULL, NULL);
   }

   _mesa_free_lighting_data( ctx );
   _mesa_free_eval_data( ctx );
   _mesa_free_texture_data( ctx );
   _mesa_free_matrix_data( ctx );
   _mesa_free_viewport_data( ctx );
   _mesa_free_colortables_data( ctx );
   _mesa_free_program_data(ctx);
   _mesa_free_occlude_data(ctx);

#if FEATURE_ARB_vertex_buffer_object
   _mesa_delete_buffer_object(ctx, ctx->Array.NullBufferObj);
#endif

   /* free dispatch tables */
   _mesa_free(ctx->Exec);
   _mesa_free(ctx->Save);

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
}


/**
 * Destroy a GLcontext structure.
 *
 * \param ctx GL context.
 *
 * Calls _mesa_free_context_data() and frees the GLcontext structure itself.
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
 * attributes from \p src into \p dst.  For many of the attributes a simple \c
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
_mesa_make_current( GLcontext *newCtx, GLframebuffer *drawBuffer,
                    GLframebuffer *readBuffer )
{
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(newCtx, "_mesa_make_current()\n");

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

#if !defined(IN_DRI_DRIVER)
   /* We call this function periodically (just here for now) in
    * order to detect when multithreading has begun.  In a DRI driver, this
    * step is done by the driver loader (e.g., libGL).
    */
   _glapi_check_multithread();
#endif /* !defined(IN_DRI_DRIVER) */

   _glapi_set_context((void *) newCtx);
   ASSERT(_mesa_get_current_context() == newCtx);

   if (!newCtx) {
      _glapi_set_dispatch(NULL);  /* none current */
   }
   else {
      _glapi_set_dispatch(newCtx->CurrentDispatch);

      if (drawBuffer && readBuffer) {
	 /* TODO: check if newCtx and buffer's visual match??? */

         ASSERT(drawBuffer->Name == 0);
         ASSERT(readBuffer->Name == 0);
         newCtx->WinSysDrawBuffer = drawBuffer;
         newCtx->WinSysReadBuffer = readBuffer;
         /* don't replace user-buffer bindings with window system buffer */
         if (!newCtx->DrawBuffer || newCtx->DrawBuffer->Name == 0) {
            newCtx->DrawBuffer = drawBuffer;
            newCtx->ReadBuffer = readBuffer;
         }

	 newCtx->NewState |= _NEW_BUFFERS;

#if _HAVE_FULL_GL
         if (!drawBuffer->Initialized) {
            /* get initial window size */
            GLuint bufWidth, bufHeight;
            /* ask device driver for size of the buffer */
            (*newCtx->Driver.GetBufferSize)(drawBuffer, &bufWidth, &bufHeight);
            /* set initial buffer size */
            if (newCtx->Driver.ResizeBuffers)
               newCtx->Driver.ResizeBuffers(newCtx, drawBuffer,
                                            bufWidth, bufHeight);
            drawBuffer->Initialized = GL_TRUE;
         }

         if (readBuffer != drawBuffer && !readBuffer->Initialized) {
            /* get initial window size */
            GLuint bufWidth, bufHeight;
            /* ask device driver for size of the buffer */
            (*newCtx->Driver.GetBufferSize)(readBuffer, &bufWidth, &bufHeight);
            /* set initial buffer size */
            if (newCtx->Driver.ResizeBuffers)
               newCtx->Driver.ResizeBuffers(newCtx, readBuffer,
                                            bufWidth, bufHeight);
            readBuffer->Initialized = GL_TRUE;
         }
#endif
         if (newCtx->FirstTimeCurrent) {
            /* set initial viewport and scissor size now */
            _mesa_set_viewport(newCtx, 0, 0, drawBuffer->Width, drawBuffer->Height);
            newCtx->Scissor.Width = drawBuffer->Width;
            newCtx->Scissor.Height = drawBuffer->Height;
         }
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
 * Make context 'ctx' share the display lists, textures and programs
 * that are associated with 'ctxToShare'.
 * Any display lists, textures or programs associated with 'ctx' will
 * be deleted if nobody else is sharing them.
 */
GLboolean
_mesa_share_state(GLcontext *ctx, GLcontext *ctxToShare)
{
   if (ctx && ctxToShare && ctx->Shared && ctxToShare->Shared) {
      ctx->Shared->RefCount--;
      if (ctx->Shared->RefCount == 0) {
         free_shared_state(ctx, ctx->Shared);
      }
      ctx->Shared = ctxToShare->Shared;
      ctx->Shared->RefCount++;
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
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
