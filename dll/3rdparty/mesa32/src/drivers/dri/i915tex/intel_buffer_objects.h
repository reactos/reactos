 /**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTEL_BUFFEROBJ_H
#define INTEL_BUFFEROBJ_H

#include "mtypes.h"

struct intel_context;
struct intel_region;
struct gl_buffer_object;


/**
 * Intel vertex/pixel buffer object, derived from Mesa's gl_buffer_object.
 */
struct intel_buffer_object
{
   struct gl_buffer_object Base;
   struct _DriBufferObject *buffer;     /* the low-level buffer manager's buffer handle */

   struct intel_region *region; /* Is there a zero-copy texture
                                   associated with this (pixel)
                                   buffer object? */
};


/* Get the bm buffer associated with a GL bufferobject:
 */
struct _DriBufferObject *intel_bufferobj_buffer(struct intel_context *intel,
                                                struct intel_buffer_object
                                                *obj, GLuint flag);

/* Hook the bufferobject implementation into mesa: 
 */
void intel_bufferobj_init(struct intel_context *intel);



/* Are the obj->Name tests necessary?  Unfortunately yes, mesa
 * allocates a couple of gl_buffer_object structs statically, and
 * the Name == 0 test is the only way to identify them and avoid
 * casting them erroneously to our structs.
 */
static INLINE struct intel_buffer_object *
intel_buffer_object(struct gl_buffer_object *obj)
{
   if (obj->Name)
      return (struct intel_buffer_object *) obj;
   else
      return NULL;
}

/* Helpers for zerocopy image uploads.  See also intel_regions.h:
 */
void intel_bufferobj_cow(struct intel_context *intel,
                         struct intel_buffer_object *intel_obj);
void intel_bufferobj_release_region(struct intel_context *intel,
                                    struct intel_buffer_object *intel_obj);


#endif
