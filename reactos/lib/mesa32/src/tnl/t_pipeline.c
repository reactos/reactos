
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "state.h"
#include "mtypes.h"

#include "math/m_translate.h"
#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"


void _tnl_install_pipeline( GLcontext *ctx,
			    const struct tnl_pipeline_stage **stages )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_pipeline *pipe = &tnl->pipeline;
   GLuint i;

   ASSERT(pipe->nr_stages == 0);

   pipe->run_state_changes = ~0;
   pipe->run_input_changes = ~0;
   pipe->build_state_changes = ~0;
   pipe->build_state_trigger = 0;
   pipe->inputs = 0;

   /* Create a writeable copy of each stage.
    */
   for (i = 0 ; i < MAX_PIPELINE_STAGES && stages[i] ; i++) {
      MEMCPY( &pipe->stages[i], stages[i], sizeof( **stages ));
      pipe->build_state_trigger |= pipe->stages[i].check_state;
   }

   MEMSET( &pipe->stages[i], 0, sizeof( **stages ));

   pipe->nr_stages = i;
}

void _tnl_destroy_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   for (i = 0 ; i < tnl->pipeline.nr_stages ; i++)
      tnl->pipeline.stages[i].destroy( &tnl->pipeline.stages[i] );

   tnl->pipeline.nr_stages = 0;
}

/* TODO: merge validate with run.
 */
void _tnl_validate_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_pipeline *pipe = &tnl->pipeline;
   struct tnl_pipeline_stage *s = pipe->stages;
   GLuint newstate = pipe->build_state_changes;
   GLuint generated = 0;
   GLuint changed_inputs = 0;

   pipe->inputs = 0;
   pipe->build_state_changes = 0;

   for ( ; s->check ; s++) {

      s->changed_inputs |= s->inputs & changed_inputs;

      if (s->check_state & newstate) {
	 if (s->active) {
	    GLuint old_outputs = s->outputs;
	    s->check(ctx, s);
	    if (!s->active)
	       changed_inputs |= old_outputs;
	 }
	 else
	    s->check(ctx, s);
      }

      if (s->active) {
	 pipe->inputs |= s->inputs & ~generated;
	 generated |= s->outputs;
      }
   }
}



void _tnl_run_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_pipeline *pipe = &tnl->pipeline;
   struct tnl_pipeline_stage *s = pipe->stages;
   GLuint changed_state = pipe->run_state_changes;
   GLuint changed_inputs = pipe->run_input_changes;
   GLboolean running = GL_TRUE;
#ifdef HAVE_FAST_MATH
   unsigned short __tmp;
#endif

   if (!tnl->vb.Count)
      return;

   pipe->run_state_changes = 0;
   pipe->run_input_changes = 0;

   /* Done elsewhere.
    */
   ASSERT(pipe->build_state_changes == 0);

#ifdef HAVE_FAST_MATH
   START_FAST_MATH(__tmp);
#endif

   /* If something changes in the pipeline, tag all subsequent stages
    * using this value for recalculation.  Inactive stages have their
    * state and inputs examined to try to keep cached data alive over
    * state-changes.
    */
   for ( ; s->run ; s++) {
      s->changed_inputs |= s->inputs & changed_inputs;

      if (s->run_state & changed_state)
	 s->changed_inputs = s->inputs;

      if (s->active && running) {
	 if (s->changed_inputs)
	    changed_inputs |= s->outputs;

	 running = s->run( ctx, s );

	 s->changed_inputs = 0;
      }
   }

#ifdef HAVE_FAST_MATH
   END_FAST_MATH(__tmp);
#endif
}



/* The default pipeline.  This is useful for software rasterizers, and
 * simple hardware rasterizers.  For customization, I don't recommend
 * tampering with the internals of these stages in the way that
 * drivers did in Mesa 3.4.  These stages are basically black boxes,
 * and should be left intact.
 *
 * To customize the pipeline, consider:
 *
 * - removing redundant stages (making sure that the software rasterizer
 *   can cope with this on fallback paths).  An example is fog
 *   coordinate generation, which is not required in the FX driver.
 *
 * - replacing general-purpose machine-independent stages with
 *   general-purpose machine-specific stages.  There is no example of
 *   this to date, though it must be borne in mind that all subsequent
 *   stages that reference the output of the new stage must cope with
 *   any machine-specific data introduced.  This may not be easy
 *   unless there are no such stages (ie the new stage is the last in
 *   the pipe).
 *
 * - inserting optimized (but specialized) stages ahead of the
 *   general-purpose fallback implementation.  For example, the old
 *   fastpath mechanism, which only works when the VB->Elts input is
 *   available, can be duplicated by placing the fastpath stage at the
 *   head of this pipeline.  Such specialized stages are currently
 *   constrained to have no outputs (ie. they must either finish the *
 *   pipeline by returning GL_FALSE from run(), or do nothing).
 *
 * Some work can be done to lift some of the restrictions in the final
 * case, if it becomes necessary to do so.
 */
const struct tnl_pipeline_stage *_tnl_default_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
#if FEATURE_NV_vertex_program
   &_tnl_vertex_program_stage,
#endif
   &_tnl_render_stage,
   0
};
