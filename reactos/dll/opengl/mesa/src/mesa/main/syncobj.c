/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file syncobj.c
 * Sync object management.
 *
 * Unlike textures and other objects that are shared between contexts, sync
 * objects are not bound to the context.  As a result, the reference counting
 * and delete behavior of sync objects is slightly different.  References to
 * sync objects are added:
 *
 *    - By \c glFencSynce.  This sets the initial reference count to 1.
 *    - At the start of \c glClientWaitSync.  The reference is held for the
 *      duration of the wait call.
 *
 * References are removed:
 *
 *    - By \c glDeleteSync.
 *    - At the end of \c glClientWaitSync.
 *
 * Additionally, drivers may call \c _mesa_ref_sync_object and
 * \c _mesa_unref_sync_object as needed to implement \c ServerWaitSync.
 *
 * As with shader objects, sync object names become invalid as soon as
 * \c glDeleteSync is called.  For this reason \c glDeleteSync sets the
 * \c DeletePending flag.  All functions validate object handles by testing
 * this flag.
 *
 * \note
 * Only \c GL_ARB_sync objects are shared between contexts.  If support is ever
 * added for either \c GL_NV_fence or \c GL_APPLE_fence different semantics
 * will need to be implemented.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "macros.h"
#include "mfeatures.h"
#include "get.h"
#include "dispatch.h"
#include "mtypes.h"

#if FEATURE_ARB_sync
#include "syncobj.h"

static struct gl_sync_object *
_mesa_new_sync_object(struct gl_context *ctx, GLenum type)
{
   struct gl_sync_object *s = MALLOC_STRUCT(gl_sync_object);
   (void) ctx;
   (void) type;

   return s;
}


static void
_mesa_delete_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   (void) ctx;
   free(syncObj);
}


static void
_mesa_fence_sync(struct gl_context *ctx, struct gl_sync_object *syncObj,
		 GLenum condition, GLbitfield flags)
{
   (void) ctx;
   (void) condition;
   (void) flags;

   syncObj->StatusFlag = 1;
}


static void
_mesa_check_sync(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   (void) ctx;
   (void) syncObj;

   /* No-op for software rendering.  Hardware drivers will need to determine
    * whether the state of the sync object has changed.
    */
}


static void
_mesa_wait_sync(struct gl_context *ctx, struct gl_sync_object *syncObj,
		GLbitfield flags, GLuint64 timeout)
{
   (void) ctx;
   (void) syncObj;
   (void) flags;
   (void) timeout;

   /* No-op for software rendering.  Hardware drivers will need to wait until
    * the state of the sync object changes or the timeout expires.
    */
}


void
_mesa_init_sync_object_functions(struct dd_function_table *driver)
{
   driver->NewSyncObject = _mesa_new_sync_object;
   driver->FenceSync = _mesa_fence_sync;
   driver->DeleteSyncObject = _mesa_delete_sync_object;
   driver->CheckSync = _mesa_check_sync;

   /* Use the same no-op wait function for both.
    */
   driver->ClientWaitSync = _mesa_wait_sync;
   driver->ServerWaitSync = _mesa_wait_sync;
}


void
_mesa_init_sync_dispatch(struct _glapi_table *disp)
{
   SET_IsSync(disp, _mesa_IsSync);
   SET_DeleteSync(disp, _mesa_DeleteSync);
   SET_FenceSync(disp, _mesa_FenceSync);
   SET_ClientWaitSync(disp, _mesa_ClientWaitSync);
   SET_WaitSync(disp, _mesa_WaitSync);
   SET_GetInteger64v(disp, _mesa_GetInteger64v);
   SET_GetSynciv(disp, _mesa_GetSynciv);
}


/**
 * Allocate/init the context state related to sync objects.
 */
void
_mesa_init_sync(struct gl_context *ctx)
{
   (void) ctx;
}


/**
 * Free the context state related to sync objects.
 */
void
_mesa_free_sync_data(struct gl_context *ctx)
{
   (void) ctx;
}


static int
_mesa_validate_sync(struct gl_sync_object *syncObj)
{
   return (syncObj != NULL)
      && (syncObj->Type == GL_SYNC_FENCE)
      && !syncObj->DeletePending;
}


void
_mesa_ref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   syncObj->RefCount++;
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


void
_mesa_unref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   syncObj->RefCount--;
   if (syncObj->RefCount == 0) {
      remove_from_list(& syncObj->link);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      ctx->Driver.DeleteSyncObject(ctx, syncObj);
   } else {
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   }
}


GLboolean GLAPIENTRY
_mesa_IsSync(GLsync sync)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) sync;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   return _mesa_validate_sync(syncObj) ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_DeleteSync(GLsync sync)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) sync;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* From the GL_ARB_sync spec:
    *
    *    DeleteSync will silently ignore a <sync> value of zero. An
    *    INVALID_VALUE error is generated if <sync> is neither zero nor the
    *    name of a sync object.
    */
   if (sync == 0) {
      return;
   }

   if (!_mesa_validate_sync(syncObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteSync");
      return;
   }

   /* If there are no client-waits or server-waits pending on this sync, delete
    * the underlying object.
    */
   syncObj->DeletePending = GL_TRUE;
   _mesa_unref_sync_object(ctx, syncObj);
}


GLsync GLAPIENTRY
_mesa_FenceSync(GLenum condition, GLbitfield flags)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *syncObj;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (condition != GL_SYNC_GPU_COMMANDS_COMPLETE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glFenceSync(condition=0x%x)",
		  condition);
      return 0;
   }

   if (flags != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glFenceSync(flags=0x%x)",
		  condition);
      return 0;
   }

   syncObj = ctx->Driver.NewSyncObject(ctx, GL_SYNC_FENCE);
   if (syncObj != NULL) {
      syncObj->Type = GL_SYNC_FENCE;
      /* The name is not currently used, and it is never visible to
       * applications.  If sync support is extended to provide support for
       * NV_fence, this field will be used.  We'll also need to add an object
       * ID hashtable.
       */
      syncObj->Name = 1;
      syncObj->RefCount = 1;
      syncObj->DeletePending = GL_FALSE;
      syncObj->SyncCondition = condition;
      syncObj->Flags = flags;
      syncObj->StatusFlag = 0;

      ctx->Driver.FenceSync(ctx, syncObj, condition, flags);

      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      insert_at_tail(& ctx->Shared->SyncObjects, & syncObj->link);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      return (GLsync) syncObj;
   }

   return NULL;
}


GLenum GLAPIENTRY
_mesa_ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) sync;
   GLenum ret;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_WAIT_FAILED);

   if (!_mesa_validate_sync(syncObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glClientWaitSync");
      return GL_WAIT_FAILED;
   }

   if ((flags & ~GL_SYNC_FLUSH_COMMANDS_BIT) != 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClientWaitSync(flags=0x%x)", flags);
      return GL_WAIT_FAILED;
   }

   _mesa_ref_sync_object(ctx, syncObj);

   /* From the GL_ARB_sync spec:
    *
    *    ClientWaitSync returns one of four status values. A return value of
    *    ALREADY_SIGNALED indicates that <sync> was signaled at the time
    *    ClientWaitSync was called. ALREADY_SIGNALED will always be returned
    *    if <sync> was signaled, even if the value of <timeout> is zero.
    */
   ctx->Driver.CheckSync(ctx, syncObj);
   if (syncObj->StatusFlag) {
      ret = GL_ALREADY_SIGNALED;
   } else {
      ctx->Driver.ClientWaitSync(ctx, syncObj, flags, timeout);

      ret = syncObj->StatusFlag ? GL_CONDITION_SATISFIED : GL_TIMEOUT_EXPIRED;
   }

   _mesa_unref_sync_object(ctx, syncObj);
   return ret;
}


void GLAPIENTRY
_mesa_WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) sync;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!_mesa_validate_sync(syncObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glWaitSync");
      return;
   }

   if (flags != 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glWaitSync(flags=0x%x)", flags);
      return;
   }

   /* From the GL_ARB_sync spec:
    *
    *     If the value of <timeout> is zero, then WaitSync does nothing.
    */
   if (timeout == 0) {
      return;
   }

   ctx->Driver.ServerWaitSync(ctx, syncObj, flags, timeout);
}


void GLAPIENTRY
_mesa_GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length,
		GLint *values)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) sync;
   GLsizei size = 0;
   GLint v[1];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!_mesa_validate_sync(syncObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetSynciv");
      return;
   }

   switch (pname) {
   case GL_OBJECT_TYPE:
      v[0] = syncObj->Type;
      size = 1;
      break;

   case GL_SYNC_CONDITION:
      v[0] = syncObj->SyncCondition;
      size = 1;
      break;

   case GL_SYNC_STATUS:
      /* Update the state of the sync by dipping into the driver.  Note that
       * this call won't block.  It just updates state in the common object
       * data from the current driver state.
       */
      ctx->Driver.CheckSync(ctx, syncObj);

      v[0] = (syncObj->StatusFlag) ? GL_SIGNALED : GL_UNSIGNALED;
      size = 1;
      break;

   case GL_SYNC_FLAGS:
      v[0] = syncObj->Flags;
      size = 1;
      break;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSynciv(pname=0x%x)\n", pname);
      return;
   }

   if (size > 0) {
      const GLsizei copy_count = MIN2(size, bufSize);

      memcpy(values, v, sizeof(GLint) * copy_count);
   }

   if (length != NULL) {
      *length = size;
   }
}

#endif /* FEATURE_ARB_sync */
