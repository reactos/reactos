/*
 * Mesa 3-D graphics library
 * Version:  6.5
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
 * Initialize the fields of a gl_renderbuffer to default values.
 */
void
_mesa_init_renderbuffer(struct gl_renderbuffer *rb, GLuint name)
{
   _glthread_INIT_MUTEX(rb->Mutex);

   rb->ClassID = 0;
   rb->RefCount = 0;
   rb->Delete = _mesa_delete_renderbuffer;

   /* The rest of these should be set later by the caller of this function or
    * the AllocStorage method:
    */
   rb->AllocStorage = NULL;

   rb->Width = 0;
   rb->Height = 0;
   rb->InternalFormat = GL_RGBA;
   rb->Format = MESA_FORMAT_NONE;
}


/**
 * Delete a gl_framebuffer.
 * This is the default function for renderbuffer->Delete().
 */
void
_mesa_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   /* no-op */
}


/**
 * Attach a renderbuffer to a framebuffer.
 * \param bufferName  one of the BUFFER_x tokens
 */
void
_mesa_add_renderbuffer(struct gl_framebuffer *fb,
                       gl_buffer_index bufferName, struct gl_renderbuffer *rb)
{
   assert(fb);
   assert(rb);
   assert(bufferName < BUFFER_COUNT);

   /* There should be no previous renderbuffer on this attachment point,
    * with the exception of depth/stencil since the same renderbuffer may
    * be used for both.
    */
   assert(bufferName == BUFFER_DEPTH ||
          bufferName == BUFFER_STENCIL ||
          fb->Attachment[bufferName].Renderbuffer == NULL);

   _mesa_reference_renderbuffer(&fb->Attachment[bufferName].Renderbuffer, rb);
}


/**
 * Remove the named renderbuffer from the given framebuffer.
 * \param bufferName  one of the BUFFER_x tokens
 */
void
_mesa_remove_renderbuffer(struct gl_framebuffer *fb,
                          gl_buffer_index bufferName)
{
   assert(bufferName < BUFFER_COUNT);
   _mesa_reference_renderbuffer(&fb->Attachment[bufferName].Renderbuffer,
                                NULL);
}


/**
 * Set *ptr to point to rb.  If *ptr points to another renderbuffer,
 * dereference that buffer first.  The new renderbuffer's refcount will
 * be incremented.  The old renderbuffer's refcount will be decremented.
 * This is normally only called from the _mesa_reference_renderbuffer() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_renderbuffer_(struct gl_renderbuffer **ptr,
                              struct gl_renderbuffer *rb)
{
   if (*ptr) {
      /* Unreference the old renderbuffer */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_renderbuffer *oldRb = *ptr;

      _glthread_LOCK_MUTEX(oldRb->Mutex);
      ASSERT(oldRb->RefCount > 0);
      oldRb->RefCount--;
      /*printf("RB DECR %p (%d) to %d\n", (void*) oldRb, oldRb->Name, oldRb->RefCount);*/
      deleteFlag = (oldRb->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldRb->Mutex);

      if (deleteFlag) {
         oldRb->Delete(oldRb);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (rb) {
      /* reference new renderbuffer */
      _glthread_LOCK_MUTEX(rb->Mutex);
      rb->RefCount++;
      /*printf("RB INCR %p (%d) to %d\n", (void*) rb, rb->Name, rb->RefCount);*/
      _glthread_UNLOCK_MUTEX(rb->Mutex);
      *ptr = rb;
   }
}
