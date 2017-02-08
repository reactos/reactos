/**********************************************************
 * Copyright 2010 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#ifndef _ST_API_H_
#define _ST_API_H_

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

/**
 * \file API for communication between state trackers and state tracker
 * managers.
 *
 * While both are state tackers, we use the term state tracker for rendering
 * APIs such as OpenGL or OpenVG, and state tracker manager for window system
 * APIs such as EGL or GLX in this file.
 *
 * This file defines an API to be implemented by both state trackers and state
 * tracker managers.
 */

/**
 * The supported rendering API of a state tracker.
 */
enum st_api_type {
   ST_API_OPENGL,
   ST_API_OPENVG,

   ST_API_COUNT
};

/**
 * The profile of a context.
 */
enum st_profile_type
{
   ST_PROFILE_DEFAULT,			/**< OpenGL compatibility profile */
   ST_PROFILE_OPENGL_CORE,		/**< OpenGL 3.2+ core profile */
   ST_PROFILE_OPENGL_ES1,		/**< OpenGL ES 1.x */
   ST_PROFILE_OPENGL_ES2		/**< OpenGL ES 2.0 */
};

/* for profile_mask in st_api */
#define ST_PROFILE_DEFAULT_MASK      (1 << ST_PROFILE_DEFAULT)
#define ST_PROFILE_OPENGL_CORE_MASK  (1 << ST_PROFILE_OPENGL_CORE)
#define ST_PROFILE_OPENGL_ES1_MASK   (1 << ST_PROFILE_OPENGL_ES1)
#define ST_PROFILE_OPENGL_ES2_MASK   (1 << ST_PROFILE_OPENGL_ES2)

/**
 * New context flags for GL 3.0 and beyond.
 *
 * Profile information (core vs. compatibilty for OpenGL 3.2+) is communicated
 * through the \c st_profile_type, not through flags.
 */
#define ST_CONTEXT_FLAG_DEBUG               (1 << 0)
#define ST_CONTEXT_FLAG_FORWARD_COMPATIBLE  (1 << 1)
#define ST_CONTEXT_FLAG_ROBUST_ACCESS       (1 << 2)

/**
 * Reasons that context creation might fail.
 */
enum st_context_error {
   ST_CONTEXT_SUCCESS = 0,
   ST_CONTEXT_ERROR_NO_MEMORY,
   ST_CONTEXT_ERROR_BAD_API,
   ST_CONTEXT_ERROR_BAD_VERSION,
   ST_CONTEXT_ERROR_BAD_FLAG,
   ST_CONTEXT_ERROR_UNKNOWN_ATTRIBUTE,
   ST_CONTEXT_ERROR_UNKNOWN_FLAG
};

/**
 * Used in st_context_iface->teximage.
 */
enum st_texture_type {
   ST_TEXTURE_1D,
   ST_TEXTURE_2D,
   ST_TEXTURE_3D,
   ST_TEXTURE_RECT
};

/**
 * Available attachments of framebuffer.
 */
enum st_attachment_type {
   ST_ATTACHMENT_FRONT_LEFT,
   ST_ATTACHMENT_BACK_LEFT,
   ST_ATTACHMENT_FRONT_RIGHT,
   ST_ATTACHMENT_BACK_RIGHT,
   ST_ATTACHMENT_DEPTH_STENCIL,
   ST_ATTACHMENT_ACCUM,
   ST_ATTACHMENT_SAMPLE,

   ST_ATTACHMENT_COUNT,
   ST_ATTACHMENT_INVALID = -1
};

/* for buffer_mask in st_visual */
#define ST_ATTACHMENT_FRONT_LEFT_MASK     (1 << ST_ATTACHMENT_FRONT_LEFT)
#define ST_ATTACHMENT_BACK_LEFT_MASK      (1 << ST_ATTACHMENT_BACK_LEFT)
#define ST_ATTACHMENT_FRONT_RIGHT_MASK    (1 << ST_ATTACHMENT_FRONT_RIGHT)
#define ST_ATTACHMENT_BACK_RIGHT_MASK     (1 << ST_ATTACHMENT_BACK_RIGHT)
#define ST_ATTACHMENT_DEPTH_STENCIL_MASK  (1 << ST_ATTACHMENT_DEPTH_STENCIL)
#define ST_ATTACHMENT_ACCUM_MASK          (1 << ST_ATTACHMENT_ACCUM)
#define ST_ATTACHMENT_SAMPLE_MASK         (1 << ST_ATTACHMENT_SAMPLE)

/**
 * Enumerations of state tracker context resources.
 */
enum st_context_resource_type {
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_2D,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_3D,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_POSITIVE_X,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_NEGATIVE_X,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_POSITIVE_Y,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_POSITIVE_Z,
   ST_CONTEXT_RESOURCE_OPENGL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
   ST_CONTEXT_RESOURCE_OPENGL_RENDERBUFFER,
   ST_CONTEXT_RESOURCE_OPENVG_PARENT_IMAGE
};

/**
 * Flush flags.
 */
#define ST_FLUSH_FRONT                    (1 << 0)

/**
 * Value to st_manager->get_param function.
 */
enum st_manager_param {
   /**
    * The dri state tracker on old libGL's doesn't do the right thing
    * with regards to invalidating the framebuffers.
    *
    * For the mesa state tracker that means that it needs to invalidate
    * the framebuffer in glViewport itself.
    */
   ST_MANAGER_BROKEN_INVALIDATE
};

/**
 * The return type of st_api->get_proc_address.
 */
typedef void (*st_proc_t)(void);

struct pipe_context;
struct pipe_resource;
struct pipe_fence_handle;

/**
 * Used in st_context_iface->get_resource_for_egl_image.
 */
struct st_context_resource
{
   /* these fields are filled by the caller */
   enum st_context_resource_type type;
   void *resource;

   /* this is owned by the caller */
   struct pipe_resource *texture;
};

/**
 * Used in st_manager_iface->get_egl_image.
 */
struct st_egl_image
{
   /* this is owned by the caller */
   struct pipe_resource *texture;

   unsigned level;
   unsigned layer;
};

/**
 * Represent the visual of a framebuffer.
 */
struct st_visual
{
   /**
    * Available buffers.  Tested with ST_FRAMEBUFFER_*_MASK.
    */
   unsigned buffer_mask;

   /**
    * Buffer formats.  The formats are always set even when the buffer is
    * not available.
    */
   enum pipe_format color_format;
   enum pipe_format depth_stencil_format;
   enum pipe_format accum_format;
   int samples;

   /**
    * Desired render buffer.
    */
   enum st_attachment_type render_buffer;
};

/**
 * Represent the attributes of a context.
 */
struct st_context_attribs
{
   /**
    * The profile and minimal version to support.
    *
    * The valid profiles and versions are rendering API dependent.  The latest
    * version satisfying the request should be returned, unless the
    * ST_CONTEXT_FLAG_FORWARD_COMPATIBLE bit is set.
    */
   enum st_profile_type profile;
   int major, minor;

   /** Mask of ST_CONTEXT_FLAG_x bits */
   unsigned flags;

   /**
    * The visual of the framebuffers the context will be bound to.
    */
   struct st_visual visual;
};

/**
 * Represent a windowing system drawable.
 *
 * The framebuffer is implemented by the state tracker manager and
 * used by the state trackers.
 *
 * Instead of the winsys pokeing into the API context to figure
 * out what buffers that might be needed in the future by the API
 * context, it calls into the framebuffer to get the textures.
 *
 * This structure along with the notify_invalid_framebuffer
 * allows framebuffers to be shared between different threads
 * but at the same make the API context free from thread
 * syncronisation primitves, with the exception of a small
 * atomic flag used for notification of framebuffer dirty status.
 *
 * The thread syncronisation is put inside the framebuffer
 * and only called once the framebuffer has become dirty.
 */
struct st_framebuffer_iface
{
   /**
    * Atomic stamp which changes when framebuffers need to be updated.
    */

   int32_t stamp;

   /**
    * Available for the state tracker manager to use.
    */
   void *st_manager_private;

   /**
    * The visual of a framebuffer.
    */
   const struct st_visual *visual;

   /**
    * Flush the front buffer.
    *
    * On some window systems, changes to the front buffers are not immediately
    * visible.  They need to be flushed.
    *
    * @att is one of the front buffer attachments.
    */
   boolean (*flush_front)(struct st_framebuffer_iface *stfbi,
                          enum st_attachment_type statt);

   /**
    * The state tracker asks for the textures it needs.
    *
    * It should try to only ask for attachments that it currently renders
    * to, thus allowing the winsys to delay the allocation of textures not
    * needed. For example front buffer attachments are not needed if you
    * only do back buffer rendering.
    *
    * The implementor of this function needs to also ensure
    * thread safty as this call might be done from multiple threads.
    *
    * The returned textures are owned by the caller.  They should be
    * unreferenced when no longer used.  If this function is called multiple
    * times with different sets of attachments, those buffers not included in
    * the last call might be destroyed.  This behavior might change in the
    * future.
    */
   boolean (*validate)(struct st_framebuffer_iface *stfbi,
                       const enum st_attachment_type *statts,
                       unsigned count,
                       struct pipe_resource **out);
};

/**
 * Represent a rendering context.
 *
 * This entity is created from st_api and used by the state tracker manager.
 */
struct st_context_iface
{
   /**
    * Available for the state tracker and the manager to use.
    */
   void *st_context_private;
   void *st_manager_private;

   /**
    * Destroy the context.
    */
   void (*destroy)(struct st_context_iface *stctxi);

   /**
    * Flush all drawing from context to the pipe also flushes the pipe.
    */
   void (*flush)(struct st_context_iface *stctxi, unsigned flags,
                 struct pipe_fence_handle **fence);

   /**
    * Replace the texture image of a texture object at the specified level.
    *
    * This function is optional.
    */
   boolean (*teximage)(struct st_context_iface *stctxi, enum st_texture_type target,
                       int level, enum pipe_format internal_format,
                       struct pipe_resource *tex, boolean mipmap);

   /**
    * Used to implement glXCopyContext.
    */
   void (*copy)(struct st_context_iface *stctxi,
                struct st_context_iface *stsrci, unsigned mask);

   /**
    * Used to implement wglShareLists.
    */
   boolean (*share)(struct st_context_iface *stctxi,
                    struct st_context_iface *stsrci);

   /**
    * Look up and return the info of a resource for EGLImage.
    *
    * This function is optional.
    */
   boolean (*get_resource_for_egl_image)(struct st_context_iface *stctxi,
                                         struct st_context_resource *stres);
};


/**
 * Represent a state tracker manager.
 *
 * This interface is implemented by the state tracker manager.  It corresponds
 * to a "display" in the window system.
 */
struct st_manager
{
   struct pipe_screen *screen;

   /**
    * Look up and return the info of an EGLImage.
    *
    * This is used to implement for example EGLImageTargetTexture2DOES.
    * The GLeglImageOES agrument of that call is passed directly to this
    * function call and the information needed to access this is returned
    * in the given struct out.
    *
    * @smapi: manager owning the caller context
    * @stctx: caller context
    * @egl_image: EGLImage that caller recived
    * @out: return struct filled out with access information.
    *
    * This function is optional.
    */
   boolean (*get_egl_image)(struct st_manager *smapi,
                            void *egl_image,
                            struct st_egl_image *out);

   /**
    * Query an manager param.
    */
   int (*get_param)(struct st_manager *smapi,
                    enum st_manager_param param);
};

/**
 * Represent a rendering API such as OpenGL or OpenVG.
 *
 * Implemented by the state tracker and used by the state tracker manager.
 */
struct st_api
{
   /**
    * The name of the rendering API.  This is informative.
    */
   const char *name;

   /**
    * The supported rendering API.
    */
   enum st_api_type api;

   /**
    * The supported profiles.  Tested with ST_PROFILE_*_MASK.
    */
   unsigned profile_mask;

   /**
    * Destroy the API.
    */
   void (*destroy)(struct st_api *stapi);

   /**
    * Return an API entry point.
    *
    * For GL this is the same as _glapi_get_proc_address.
    */
   st_proc_t (*get_proc_address)(struct st_api *stapi, const char *procname);

   /**
    * Create a rendering context.
    */
   struct st_context_iface *(*create_context)(struct st_api *stapi,
                                              struct st_manager *smapi,
                                              const struct st_context_attribs *attribs,
                                              enum st_context_error *error,
                                              struct st_context_iface *stsharei);

   /**
    * Bind the context to the calling thread with draw and read as drawables.
    *
    * The framebuffers might be NULL, or might have different visuals than the
    * context does.
    */
   boolean (*make_current)(struct st_api *stapi,
                           struct st_context_iface *stctxi,
                           struct st_framebuffer_iface *stdrawi,
                           struct st_framebuffer_iface *streadi);

   /**
    * Get the currently bound context in the calling thread.
    */
   struct st_context_iface *(*get_current)(struct st_api *stapi);
};

/**
 * Return true if the visual has the specified buffers.
 */
static INLINE boolean
st_visual_have_buffers(const struct st_visual *visual, unsigned mask)
{
   return ((visual->buffer_mask & mask) == mask);
}

#endif /* _ST_API_H_ */
