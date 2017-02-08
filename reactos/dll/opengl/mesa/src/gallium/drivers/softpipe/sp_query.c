/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "os/os_time.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_query.h"
#include "sp_state.h"

struct softpipe_query {
   unsigned type;
   uint64_t start;
   uint64_t end;
   struct pipe_query_data_so_statistics so;
   unsigned num_primitives_generated;
};


static struct softpipe_query *softpipe_query( struct pipe_query *p )
{
   return (struct softpipe_query *)p;
}

static struct pipe_query *
softpipe_create_query(struct pipe_context *pipe, 
		      unsigned type)
{
   struct softpipe_query* sq;

   assert(type == PIPE_QUERY_OCCLUSION_COUNTER ||
          type == PIPE_QUERY_TIME_ELAPSED ||
          type == PIPE_QUERY_SO_STATISTICS ||
          type == PIPE_QUERY_PRIMITIVES_EMITTED ||
          type == PIPE_QUERY_PRIMITIVES_GENERATED ||
          type == PIPE_QUERY_GPU_FINISHED ||
          type == PIPE_QUERY_TIMESTAMP ||
          type == PIPE_QUERY_TIMESTAMP_DISJOINT);
   sq = CALLOC_STRUCT( softpipe_query );
   sq->type = type;

   return (struct pipe_query *)sq;
}


static void
softpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   FREE(q);
}


static void
softpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   struct softpipe_query *sq = softpipe_query(q);

   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sq->start = softpipe->occlusion_count;
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      sq->start = 1000*os_time_get();
      break;
   case PIPE_QUERY_SO_STATISTICS:
      sq->so.primitives_storage_needed = 0;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      sq->so.num_primitives_written = 0;
      softpipe->so_stats.num_primitives_written = 0;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      sq->num_primitives_generated = 0;
      softpipe->num_primitives_generated = 0;
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_GPU_FINISHED:
      break;
   default:
      assert(0);
      break;
   }
   softpipe->active_query_count++;
   softpipe->dirty |= SP_NEW_QUERY;
}


static void
softpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   struct softpipe_query *sq = softpipe_query(q);

   softpipe->active_query_count--;
   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sq->end = softpipe->occlusion_count;
      break;
   case PIPE_QUERY_TIMESTAMP:
      sq->start = 0;
      /* fall through */
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      sq->end = 1000*os_time_get();
      break;
   case PIPE_QUERY_SO_STATISTICS:
      sq->so.primitives_storage_needed =
         softpipe->so_stats.primitives_storage_needed;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      sq->so.num_primitives_written =
         softpipe->so_stats.num_primitives_written;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      sq->num_primitives_generated = softpipe->num_primitives_generated;
      break;
   case PIPE_QUERY_GPU_FINISHED:
      break;
   default:
      assert(0);
      break;
   }
   softpipe->dirty |= SP_NEW_QUERY;
}


static boolean
softpipe_get_query_result(struct pipe_context *pipe, 
			  struct pipe_query *q,
			  boolean wait,
			  void *vresult)
{
   struct softpipe_query *sq = softpipe_query(q);
   uint64_t *result = (uint64_t*)vresult;

   switch (sq->type) {
   case PIPE_QUERY_SO_STATISTICS:
      memcpy(vresult, &sq->so,
             sizeof(struct pipe_query_data_so_statistics));
      break;
   case PIPE_QUERY_GPU_FINISHED:
      *result = TRUE;
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT: {
      struct pipe_query_data_timestamp_disjoint td;
      /*os_get_time is in microseconds*/
      td.frequency = 1000000;
      td.disjoint = sq->end != sq->start;
      memcpy(vresult, &td,
             sizeof(struct pipe_query_data_timestamp_disjoint));
   }
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      *result = sq->so.num_primitives_written;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      *result = sq->num_primitives_generated;
      break;
   default:
      *result = sq->end - sq->start;
      break;
   }
   return TRUE;
}


/**
 * Called by rendering function to check rendering is conditional.
 * \return TRUE if we should render, FALSE if we should skip rendering
 */
boolean
softpipe_check_render_cond(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   boolean b, wait;
   uint64_t result;

   if (!sp->render_cond_query) {
      return TRUE;  /* no query predicate, draw normally */
   }

   wait = (sp->render_cond_mode == PIPE_RENDER_COND_WAIT ||
           sp->render_cond_mode == PIPE_RENDER_COND_BY_REGION_WAIT);

   b = pipe->get_query_result(pipe, sp->render_cond_query, wait, &result);
   if (b)
      return result > 0;
   else
      return TRUE;
}


void softpipe_init_query_funcs(struct softpipe_context *softpipe )
{
   softpipe->pipe.create_query = softpipe_create_query;
   softpipe->pipe.destroy_query = softpipe_destroy_query;
   softpipe->pipe.begin_query = softpipe_begin_query;
   softpipe->pipe.end_query = softpipe_end_query;
   softpipe->pipe.get_query_result = softpipe_get_query_result;
}


