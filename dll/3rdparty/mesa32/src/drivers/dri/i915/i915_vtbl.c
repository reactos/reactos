/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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



#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "tnl/t_context.h"
#include "tnl/t_vertex.h"

#include "intel_batchbuffer.h"

#include "i915_reg.h"
#include "i915_context.h"

static void i915_render_start( intelContextPtr intel )
{
   GLcontext *ctx = &intel->ctx;
   i915ContextPtr i915 = I915_CONTEXT(intel);

   if (ctx->FragmentProgram._Active) 
      i915ValidateFragmentProgram( i915 );
   else {
      assert(!ctx->FragmentProgram._MaintainTexEnvProgram);
      i915ValidateTextureProgram( i915 );
   }
}


static void i915_reduced_primitive_state( intelContextPtr intel,
					  GLenum rprim )
{
    i915ContextPtr i915 = I915_CONTEXT(intel);
    GLuint st1 = i915->state.Stipple[I915_STPREG_ST1];

    st1 &= ~ST1_ENABLE;

    switch (rprim) {
    case GL_QUADS: /* from RASTERIZE(GL_QUADS) in t_dd_tritemp.h */
    case GL_TRIANGLES:
       if (intel->ctx.Polygon.StippleFlag &&
	   intel->hw_stipple)
	  st1 |= ST1_ENABLE;
       break;
    case GL_LINES:
    case GL_POINTS:
    default:
       break;
    }

    i915->intel.reduced_primitive = rprim;

    if (st1 != i915->state.Stipple[I915_STPREG_ST1]) {
       I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
       i915->state.Stipple[I915_STPREG_ST1] = st1;
    }
}


/* Pull apart the vertex format registers and figure out how large a
 * vertex is supposed to be. 
 */
static GLboolean i915_check_vertex_size( intelContextPtr intel,
					 GLuint expected )
{
   i915ContextPtr i915 = I915_CONTEXT(intel);
   int lis2 = i915->current->Ctx[I915_CTXREG_LIS2];
   int lis4 = i915->current->Ctx[I915_CTXREG_LIS4];
   int i, sz = 0;

   switch (lis4 & S4_VFMT_XYZW_MASK) {
   case S4_VFMT_XY: sz = 2; break;
   case S4_VFMT_XYZ: sz = 3; break;
   case S4_VFMT_XYW: sz = 3; break;
   case S4_VFMT_XYZW: sz = 4; break;
   default: 
      fprintf(stderr, "no xyzw specified\n");
      return 0;
   }

   if (lis4 & S4_VFMT_SPEC_FOG) sz++;
   if (lis4 & S4_VFMT_COLOR) sz++;
   if (lis4 & S4_VFMT_DEPTH_OFFSET) sz++;
   if (lis4 & S4_VFMT_POINT_WIDTH) sz++;
   if (lis4 & S4_VFMT_FOG_PARAM) sz++;
	
   for (i = 0 ; i < 8 ; i++) { 
      switch (lis2 & S2_TEXCOORD_FMT0_MASK) {
      case TEXCOORDFMT_2D: sz += 2; break;
      case TEXCOORDFMT_3D: sz += 3; break;
      case TEXCOORDFMT_4D: sz += 4; break;
      case TEXCOORDFMT_1D: sz += 1; break;
      case TEXCOORDFMT_2D_16: sz += 1; break;
      case TEXCOORDFMT_4D_16: sz += 2; break;
      case TEXCOORDFMT_NOT_PRESENT: break;
      default:
	 fprintf(stderr, "bad texcoord fmt %d\n", i);
	 return GL_FALSE;
      }
      lis2 >>= S2_TEXCOORD_FMT1_SHIFT;
   }
	
   if (sz != expected) 
      fprintf(stderr, "vertex size mismatch %d/%d\n", sz, expected);
   
   return sz == expected;
}


static void i915_emit_invarient_state( intelContextPtr intel )
{
   BATCH_LOCALS;

   BEGIN_BATCH( 20 );

   OUT_BATCH(_3DSTATE_AA_CMD |
	     AA_LINE_ECAAR_WIDTH_ENABLE |
	     AA_LINE_ECAAR_WIDTH_1_0 |
	     AA_LINE_REGION_WIDTH_ENABLE |
	     AA_LINE_REGION_WIDTH_1_0);

   OUT_BATCH(_3DSTATE_DFLT_DIFFUSE_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_SPEC_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_Z_CMD);
   OUT_BATCH(0);

   /* Don't support texture crossbar yet */
   OUT_BATCH(_3DSTATE_COORD_SET_BINDINGS |
	     CSB_TCB(0, 0) |
	     CSB_TCB(1, 1) |
	     CSB_TCB(2, 2) |
	     CSB_TCB(3, 3) |
	     CSB_TCB(4, 4) |
	     CSB_TCB(5, 5) |
	     CSB_TCB(6, 6) |
	     CSB_TCB(7, 7));

   OUT_BATCH(_3DSTATE_RASTER_RULES_CMD |
	     ENABLE_POINT_RASTER_RULE |
	     OGL_POINT_RASTER_RULE |
	     ENABLE_LINE_STRIP_PROVOKE_VRTX |
	     ENABLE_TRI_FAN_PROVOKE_VRTX |
	     LINE_STRIP_PROVOKE_VRTX(1) |
	     TRI_FAN_PROVOKE_VRTX(2) | 
	     ENABLE_TEXKILL_3D_4D |
	     TEXKILL_4D);

   /* Need to initialize this to zero.
    */
   OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 | 
	     I1_LOAD_S(3) |
	     (0));
   OUT_BATCH(0);
 
   /* XXX: Use this */
   OUT_BATCH(_3DSTATE_SCISSOR_ENABLE_CMD | 
	     DISABLE_SCISSOR_RECT);

   OUT_BATCH(_3DSTATE_SCISSOR_RECT_0_CMD);
   OUT_BATCH(0);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DEPTH_SUBRECT_DISABLE);

   OUT_BATCH(_3DSTATE_LOAD_INDIRECT | 0); /* disable indirect state */
   OUT_BATCH(0);


   /* Don't support twosided stencil yet */
   OUT_BATCH(_3DSTATE_BACKFACE_STENCIL_OPS |
	     BFO_ENABLE_STENCIL_TWO_SIDE |
	     0 );
   
   ADVANCE_BATCH();
}


#define emit( intel, state, size )			\
do {							\
   int k;						\
   BEGIN_BATCH( (size) / sizeof(GLuint));		\
   for (k = 0 ; k < (size) / sizeof(GLuint) ; k++)	\
      OUT_BATCH((state)[k]);				\
   ADVANCE_BATCH();					\
} while (0);

static GLuint get_dirty( struct i915_hw_state *state )
{
   GLuint dirty;

   /* Workaround the multitex hang - if one texture unit state is
    * modified, emit all texture units.
    */
   dirty = state->active & ~state->emitted;
   if (dirty & I915_UPLOAD_TEX_ALL)
      state->emitted &= ~I915_UPLOAD_TEX_ALL;
   dirty = state->active & ~state->emitted;

   return dirty;
}


static GLuint get_state_size( struct i915_hw_state *state )
{
   GLuint dirty = get_dirty(state);
   GLuint i;
   GLuint sz = 0;

   if (dirty & I915_UPLOAD_INVARIENT)
      sz += 20 * sizeof(int);

   if (dirty & I915_UPLOAD_CTX)
      sz += sizeof(state->Ctx);

   if (dirty & I915_UPLOAD_BUFFERS) 
      sz += sizeof(state->Buffer);

   if (dirty & I915_UPLOAD_STIPPLE)
      sz += sizeof(state->Stipple);

   if (dirty & I915_UPLOAD_FOG) 
      sz += sizeof(state->Fog);

   if (dirty & I915_UPLOAD_TEX_ALL) {
      int nr = 0;
      for (i = 0; i < I915_TEX_UNITS; i++) 
	 if (dirty & I915_UPLOAD_TEX(i)) 
	    nr++;

      sz += (2+nr*3) * sizeof(GLuint) * 2;
   }

   if (dirty & I915_UPLOAD_CONSTANTS) 
      sz += state->ConstantSize * sizeof(GLuint);

   if (dirty & I915_UPLOAD_PROGRAM) 
      sz += state->ProgramSize * sizeof(GLuint);

   return sz;
}


/* Push the state into the sarea and/or texture memory.
 */
static void i915_emit_state( intelContextPtr intel )
{
   i915ContextPtr i915 = I915_CONTEXT(intel);
   struct i915_hw_state *state = i915->current;
   int i;
   GLuint dirty = get_dirty(state);
   GLuint counter = intel->batch.counter;
   BATCH_LOCALS;

   if (intel->batch.space < get_state_size(state)) {
      intelFlushBatch(intel, GL_TRUE);
      dirty = get_dirty(state);
      counter = intel->batch.counter;
   }

   if (VERBOSE) 
      fprintf(stderr, "%s dirty: %x\n", __FUNCTION__, dirty);

   if (dirty & I915_UPLOAD_INVARIENT) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_INVARIENT:\n"); 
      i915_emit_invarient_state( intel );
   }

   if (dirty & I915_UPLOAD_CTX) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_CTX:\n"); 
      emit( i915, state->Ctx, sizeof(state->Ctx) );
   }

   if (dirty & I915_UPLOAD_BUFFERS) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_BUFFERS:\n"); 
      emit( i915, state->Buffer, sizeof(state->Buffer) );
   }

   if (dirty & I915_UPLOAD_STIPPLE) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_STIPPLE:\n"); 
      emit( i915, state->Stipple, sizeof(state->Stipple) );
   }

   if (dirty & I915_UPLOAD_FOG) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_FOG:\n"); 
      emit( i915, state->Fog, sizeof(state->Fog) );
   }

   /* Combine all the dirty texture state into a single command to
    * avoid lockups on I915 hardware. 
    */
   if (dirty & I915_UPLOAD_TEX_ALL) {
      int nr = 0;

      for (i = 0; i < I915_TEX_UNITS; i++) 
	 if (dirty & I915_UPLOAD_TEX(i)) 
	    nr++;

      BEGIN_BATCH(2+nr*3);
      OUT_BATCH(_3DSTATE_MAP_STATE | (3*nr));
      OUT_BATCH((dirty & I915_UPLOAD_TEX_ALL) >> I915_UPLOAD_TEX_0_SHIFT);
      for (i = 0 ; i < I915_TEX_UNITS ; i++)
	 if (dirty & I915_UPLOAD_TEX(i)) {
	    OUT_BATCH(state->Tex[i][I915_TEXREG_MS2]);
	    OUT_BATCH(state->Tex[i][I915_TEXREG_MS3]);
	    OUT_BATCH(state->Tex[i][I915_TEXREG_MS4]);
	 }
      ADVANCE_BATCH();

      BEGIN_BATCH(2+nr*3);
      OUT_BATCH(_3DSTATE_SAMPLER_STATE | (3*nr));
      OUT_BATCH((dirty & I915_UPLOAD_TEX_ALL) >> I915_UPLOAD_TEX_0_SHIFT);
      for (i = 0 ; i < I915_TEX_UNITS ; i++)
	 if (dirty & I915_UPLOAD_TEX(i)) {
	    OUT_BATCH(state->Tex[i][I915_TEXREG_SS2]);
	    OUT_BATCH(state->Tex[i][I915_TEXREG_SS3]);
	    OUT_BATCH(state->Tex[i][I915_TEXREG_SS4]);
	 }
      ADVANCE_BATCH();
   }

   if (dirty & I915_UPLOAD_CONSTANTS) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_CONSTANTS:\n"); 
      emit( i915, state->Constant, state->ConstantSize * sizeof(GLuint) );
   }

   if (dirty & I915_UPLOAD_PROGRAM) {
      if (VERBOSE) fprintf(stderr, "I915_UPLOAD_PROGRAM:\n"); 

      assert((state->Program[0] & 0x1ff)+2 == state->ProgramSize);
      
      emit( i915, state->Program, state->ProgramSize * sizeof(GLuint) );
      if (VERBOSE)
	 i915_disassemble_program( state->Program, state->ProgramSize );
   }

   state->emitted |= dirty;
   intel->batch.last_emit_state = counter;
   assert(counter == intel->batch.counter);
}

static void i915_destroy_context( intelContextPtr intel )
{
   _tnl_free_vertices(&intel->ctx);
}


/**
 * Set the color buffer drawing region.
 */
static void
i915_set_color_region( intelContextPtr intel, const intelRegion *region)
{
   i915ContextPtr i915 = I915_CONTEXT(intel);
   I915_STATECHANGE( i915, I915_UPLOAD_BUFFERS );
   i915->state.Buffer[I915_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i915->state.Buffer[I915_DESTREG_CBUFADDR2] = region->offset;
}


/**
 * specify the z-buffer/stencil region
 */
static void
i915_set_z_region( intelContextPtr intel, const intelRegion *region)
{
   i915ContextPtr i915 = I915_CONTEXT(intel);
   I915_STATECHANGE( i915, I915_UPLOAD_BUFFERS );
   i915->state.Buffer[I915_DESTREG_DBUFADDR1] =
      (BUF_3D_ID_DEPTH | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i915->state.Buffer[I915_DESTREG_DBUFADDR2] = region->offset;
}


/**
 * Set both the color and Z/stencil drawing regions.
 * Similar to two previous functions, but don't use I915_STATECHANGE()
 */
static void
i915_update_color_z_regions(intelContextPtr intel,
                            const intelRegion *colorRegion,
                            const intelRegion *depthRegion)
{
   i915ContextPtr i915 = I915_CONTEXT(intel);

   i915->state.Buffer[I915_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(colorRegion->pitch) | BUF_3D_USE_FENCE);
   i915->state.Buffer[I915_DESTREG_CBUFADDR2] = colorRegion->offset;

   i915->state.Buffer[I915_DESTREG_DBUFADDR1] =
      (BUF_3D_ID_DEPTH |
       BUF_3D_PITCH(depthRegion->pitch) |  /* pitch in bytes */
       BUF_3D_USE_FENCE);
   i915->state.Buffer[I915_DESTREG_DBUFADDR2] = depthRegion->offset;
}


static void i915_lost_hardware( intelContextPtr intel )
{
   I915_CONTEXT(intel)->state.emitted = 0;
}

static void i915_emit_flush( intelContextPtr intel )
{
   BATCH_LOCALS;

   BEGIN_BATCH(2);
   OUT_BATCH( MI_FLUSH | FLUSH_MAP_CACHE | FLUSH_RENDER_CACHE ); 
   OUT_BATCH( 0 );
   ADVANCE_BATCH();
}


void i915InitVtbl( i915ContextPtr i915 )
{
   i915->intel.vtbl.alloc_tex_obj = i915AllocTexObj;
   i915->intel.vtbl.check_vertex_size = i915_check_vertex_size;
   i915->intel.vtbl.clear_with_tris = i915ClearWithTris;
   i915->intel.vtbl.rotate_window = i915RotateWindow;
   i915->intel.vtbl.destroy = i915_destroy_context;
   i915->intel.vtbl.emit_state = i915_emit_state;
   i915->intel.vtbl.lost_hardware = i915_lost_hardware;
   i915->intel.vtbl.reduced_primitive_state = i915_reduced_primitive_state;
   i915->intel.vtbl.render_start = i915_render_start;
   i915->intel.vtbl.set_color_region = i915_set_color_region;
   i915->intel.vtbl.set_z_region = i915_set_z_region;
   i915->intel.vtbl.update_color_z_regions = i915_update_color_z_regions;
   i915->intel.vtbl.update_texture_state = i915UpdateTextureState;
   i915->intel.vtbl.emit_flush = i915_emit_flush;
}

