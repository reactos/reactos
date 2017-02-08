/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Transform feedback functions.
 *
 * \author Brian Paul
 *         Marek Olšák
 */


#include "main/bufferobj.h"
#include "main/context.h"
#include "main/mfeatures.h"
#include "main/transformfeedback.h"

#include "st_cb_bufferobjects.h"
#include "st_cb_xformfb.h"
#include "st_context.h"

#include "pipe/p_context.h"
#include "util/u_draw.h"
#include "util/u_inlines.h"
#include "cso_cache/cso_context.h"

#if FEATURE_EXT_transform_feedback

struct st_transform_feedback_object {
   struct gl_transform_feedback_object base;

   unsigned num_targets;
   struct pipe_stream_output_target *targets[PIPE_MAX_SO_BUFFERS];

   /* This encapsulates the count that can be used as a source for draw_vbo.
    * It contains a stream output target from the last call of
    * EndTransformFeedback. */
   struct pipe_stream_output_target *draw_count;
};

static INLINE struct st_transform_feedback_object *
st_transform_feedback_object(struct gl_transform_feedback_object *obj)
{
   return (struct st_transform_feedback_object *) obj;
}

static struct gl_transform_feedback_object *
st_new_transform_feedback(struct gl_context *ctx, GLuint name)
{
   struct st_transform_feedback_object *obj;

   obj = CALLOC_STRUCT(st_transform_feedback_object);
   if (!obj)
      return NULL;

   obj->base.Name = name;
   obj->base.RefCount = 1;
   return &obj->base;
}


static void
st_delete_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj)
{
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);
   unsigned i;

   pipe_so_target_reference(&sobj->draw_count, NULL);

   /* Unreference targets. */
   for (i = 0; i < sobj->num_targets; i++) {
      pipe_so_target_reference(&sobj->targets[i], NULL);
   }

   for (i = 0; i < Elements(sobj->base.Buffers); i++) {
      _mesa_reference_buffer_object(ctx, &sobj->base.Buffers[i], NULL);
   }

   free(obj);
}


/* XXX Do we really need the mode? */
static void
st_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                            struct gl_transform_feedback_object *obj)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);
   unsigned i, max_num_targets;

   max_num_targets = MIN2(Elements(sobj->base.Buffers),
                          Elements(sobj->targets));

   /* Convert the transform feedback state into the gallium representation. */
   for (i = 0; i < max_num_targets; i++) {
      struct st_buffer_object *bo = st_buffer_object(sobj->base.Buffers[i]);

      if (bo) {
         /* Check whether we need to recreate the target. */
         if (!sobj->targets[i] ||
             sobj->targets[i] == sobj->draw_count ||
             sobj->targets[i]->buffer != bo->buffer ||
             sobj->targets[i]->buffer_offset != sobj->base.Offset[i] ||
             sobj->targets[i]->buffer_size != sobj->base.Size[i]) {
            /* Create a new target. */
            struct pipe_stream_output_target *so_target =
                  pipe->create_stream_output_target(pipe, bo->buffer,
                                                    sobj->base.Offset[i],
                                                    sobj->base.Size[i]);

            pipe_so_target_reference(&sobj->targets[i], NULL);
            sobj->targets[i] = so_target;
         }

         sobj->num_targets = i+1;
      } else {
         pipe_so_target_reference(&sobj->targets[i], NULL);
      }
   }

   /* Start writing at the beginning of each target. */
   cso_set_stream_outputs(st->cso_context, sobj->num_targets, sobj->targets,
                          0);
}


static void
st_pause_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj)
{
   struct st_context *st = st_context(ctx);
   cso_set_stream_outputs(st->cso_context, 0, NULL, 0);
}


static void
st_resume_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj)
{
   struct st_context *st = st_context(ctx);
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);

   cso_set_stream_outputs(st->cso_context, sobj->num_targets, sobj->targets,
                          ~0);
}


static struct pipe_stream_output_target *
st_transform_feedback_get_draw_target(struct gl_transform_feedback_object *obj)
{
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);
   unsigned i;

   for (i = 0; i < Elements(sobj->targets); i++) {
      if (sobj->targets[i]) {
         return sobj->targets[i];
      }
   }

   assert(0);
   return NULL;
}


static void
st_end_transform_feedback(struct gl_context *ctx,
                          struct gl_transform_feedback_object *obj)
{
   struct st_context *st = st_context(ctx);
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);

   cso_set_stream_outputs(st->cso_context, 0, NULL, 0);

   pipe_so_target_reference(&sobj->draw_count,
                            st_transform_feedback_get_draw_target(obj));
}


void
st_transform_feedback_draw_init(struct gl_transform_feedback_object *obj,
                                struct pipe_draw_info *out)
{
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);

   out->count_from_stream_output = sobj->draw_count;
}


void
st_init_xformfb_functions(struct dd_function_table *functions)
{
   functions->NewTransformFeedback = st_new_transform_feedback;
   functions->DeleteTransformFeedback = st_delete_transform_feedback;
   functions->BeginTransformFeedback = st_begin_transform_feedback;
   functions->EndTransformFeedback = st_end_transform_feedback;
   functions->PauseTransformFeedback = st_pause_transform_feedback;
   functions->ResumeTransformFeedback = st_resume_transform_feedback;
}

#endif /* FEATURE_EXT_transform_feedback */
