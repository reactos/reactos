/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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

#include "main/glheader.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/state.h"
#include "main/mtypes.h"

#include "t_context.h"
#include "t_pipeline.h"
#include "t_vp_build.h"
#include "t_vertex.h"

void _tnl_install_pipeline( GLcontext *ctx,
			    const struct tnl_pipeline_stage **stages )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   tnl->pipeline.new_state = ~0;

   /* Create a writeable copy of each stage.
    */
   for (i = 0 ; i < MAX_PIPELINE_STAGES && stages[i] ; i++) {
      struct tnl_pipeline_stage *s = &tnl->pipeline.stages[i];
      MEMCPY(s, stages[i], sizeof(*s));
      if (s->create)
	 s->create(ctx, s);
   }

   tnl->pipeline.nr_stages = i;
}

void _tnl_destroy_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   for (i = 0 ; i < tnl->pipeline.nr_stages ; i++) {
      struct tnl_pipeline_stage *s = &tnl->pipeline.stages[i];
      if (s->destroy)
	 s->destroy(s);
   }

   tnl->pipeline.nr_stages = 0;
}



static GLuint check_input_changes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;
   
   for (i = 0; i <= _TNL_LAST_MAT; i++) {
      if (tnl->vb.AttribPtr[i]->size != tnl->pipeline.last_attrib_size[i] ||
	  tnl->vb.AttribPtr[i]->stride != tnl->pipeline.last_attrib_stride[i]) {
	 tnl->pipeline.last_attrib_size[i] = tnl->vb.AttribPtr[i]->size;
	 tnl->pipeline.last_attrib_stride[i] = tnl->vb.AttribPtr[i]->stride;
	 tnl->pipeline.input_changes |= 1<<i;
      }
   }

   if (tnl->pipeline.input_changes &&
      tnl->Driver.NotifyInputChanges) 
      tnl->Driver.NotifyInputChanges( ctx, tnl->pipeline.input_changes );

   return tnl->pipeline.input_changes;
}


static GLuint check_output_changes( GLcontext *ctx )
{
#if 0
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (tnl->vb.ResultPtr[i]->size != tnl->last_result_size[i] ||
	  tnl->vb.ResultPtr[i]->stride != tnl->last_result_stride[i]) {
	 tnl->last_result_size[i] = tnl->vb.ResultPtr[i]->size;
	 tnl->last_result_stride[i] = tnl->vb.ResultPtr[i]->stride;
	 tnl->pipeline.output_changes |= 1<<i;
      }
   }

   if (tnl->pipeline.output_changes) 
      tnl->Driver.NotifyOutputChanges( ctx, tnl->pipeline.output_changes );
   
   return tnl->pipeline.output_changes;
#else
   return ~0;
#endif
}


void _tnl_run_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   unsigned short __tmp;
   GLuint i;

   if (!tnl->vb.Count)
      return;

   /* Check for changed input sizes or change in stride to/from zero
    * (ie const or non-const).
    */
   if (check_input_changes( ctx ) || tnl->pipeline.new_state) {
      if (ctx->VertexProgram._MaintainTnlProgram)
	 _tnl_UpdateFixedFunctionProgram( ctx );

      for (i = 0; i < tnl->pipeline.nr_stages ; i++) {
	 struct tnl_pipeline_stage *s = &tnl->pipeline.stages[i];
	 if (s->validate)
	    s->validate( ctx, s );
      }
      
      tnl->pipeline.new_state = 0;
      tnl->pipeline.input_changes = 0;
      
      /* Pipeline can only change its output in response to either a
       * statechange or an input size/stride change.  No other changes
       * are allowed.
       */
      if (check_output_changes( ctx ))
	 _tnl_notify_pipeline_output_change( ctx );
   }

   START_FAST_MATH(__tmp);

   for (i = 0; i < tnl->pipeline.nr_stages ; i++) {
      struct tnl_pipeline_stage *s = &tnl->pipeline.stages[i];
      if (!s->run( ctx, s ))
	 break;
   }

   END_FAST_MATH(__tmp);
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
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage, 
   &_tnl_fog_coordinate_stage,
   &_tnl_render_stage,
   NULL 
};

const struct tnl_pipeline_stage *_tnl_vp_pipeline[] = {
   &_tnl_vertex_program_stage,
   &_tnl_render_stage,
   NULL
};
