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


#include "i830_context.h"
#include "i830_reg.h"

#include "intel_batchbuffer.h"

#include "tnl/t_context.h"
#include "tnl/t_vertex.h"

static GLboolean i830_check_vertex_size( intelContextPtr intel,
					 GLuint expected );

#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, V0 )					\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = (ATTR);	\
   intel->vertex_attrs[intel->vertex_attr_count].format = (STYLE);	\
   intel->vertex_attr_count++;						\
   v0 |= V0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = 0;		\
   intel->vertex_attrs[intel->vertex_attr_count].format = EMIT_PAD;	\
   intel->vertex_attrs[intel->vertex_attr_count].offset = (N);		\
   intel->vertex_attr_count++;						\
} while (0)


#define VRTX_TEX_SET_FMT(n, x)          ((x)<<((n)*2))
#define TEXBIND_SET(n, x) 		((x)<<((n)*4))

static void i830_render_start( intelContextPtr intel )
{
   GLcontext *ctx = &intel->ctx;
   i830ContextPtr i830 = I830_CONTEXT(intel);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   GLuint v0 = _3DSTATE_VFT0_CMD;
   GLuint v2 = _3DSTATE_VFT1_CMD;
   GLuint mcsb1 = 0;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   intel->vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, VFT0_XYZW );
      intel->coloroffset = 4;
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, VFT0_XYZ );
      intel->coloroffset = 3;
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_POINTSIZE )) {
      EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, VFT0_POINT_WIDTH );
   }

   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, VFT0_DIFFUSE );
      
   intel->specoffset = 0;
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
         intel->specoffset = intel->coloroffset + 1;
         EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, VFT0_SPEC );
      }
      else
         EMIT_PAD( 3 );

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG ))
         EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, VFT0_SPEC );
      else
         EMIT_PAD( 1 );
   }

   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      int i, count = 0;

      for (i = 0; i < I830_TEX_UNITS; i++) {
         if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(i) )) {
            GLuint sz = VB->TexCoordPtr[i]->size;
            GLuint emit;
            GLuint mcs = (i830->state.Tex[i][I830_TEXREG_MCS] & 
                          ~TEXCOORDTYPE_MASK);

	    switch (sz) {
	    case 1: 
	    case 2: 
	       emit = EMIT_2F; 
	       sz = 2; 
	       mcs |= TEXCOORDTYPE_CARTESIAN; 
	       break;
	    case 3:
	       emit = EMIT_3F; 
	       sz = 3;
	       mcs |= TEXCOORDTYPE_VECTOR;
	       break;
	    case 4: 
	       emit = EMIT_3F_XYW; 
	       sz = 3;     
	       mcs |= TEXCOORDTYPE_HOMOGENEOUS;
	       break;
	    default: 
	       continue;
	    };
	      

	    EMIT_ATTR( _TNL_ATTRIB_TEX0+i, emit, 0 );	       
	    v2 |= VRTX_TEX_SET_FMT(count, SZ_TO_HW(sz));
	    mcsb1 |= (count+8)<<(i*4);

	    if (mcs != i830->state.Tex[i][I830_TEXREG_MCS]) {
	       I830_STATECHANGE(i830, I830_UPLOAD_TEX(i));
	       i830->state.Tex[i][I830_TEXREG_MCS] = mcs;
	    }

	    count++;
	 }
      }

      v0 |= VFT0_TEX_COUNT(count);
   }
   
   /* Only need to change the vertex emit code if there has been a
    * statechange to a new hardware vertex format:
    */
   if (v0 != i830->state.Ctx[I830_CTXREG_VF] ||
       v2 != i830->state.Ctx[I830_CTXREG_VF2] ||
       mcsb1 != i830->state.Ctx[I830_CTXREG_MCSB1] ||
       !RENDERINPUTS_EQUAL( index_bitset, i830->last_index_bitset )) {
    
      I830_STATECHANGE( i830, I830_UPLOAD_CTX );

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      intel->vertex_size = 
	 _tnl_install_attrs( ctx, 
			     intel->vertex_attrs, 
			     intel->vertex_attr_count,
			     intel->ViewportMatrix.m, 0 );

      intel->vertex_size >>= 2;

      i830->state.Ctx[I830_CTXREG_VF] = v0;
      i830->state.Ctx[I830_CTXREG_VF2] = v2;
      i830->state.Ctx[I830_CTXREG_MCSB1] = mcsb1;
      RENDERINPUTS_COPY( i830->last_index_bitset, index_bitset );

      assert(i830_check_vertex_size( intel, intel->vertex_size ));
   }
}

static void i830_reduced_primitive_state( intelContextPtr intel,
					  GLenum rprim )
{
    i830ContextPtr i830 = I830_CONTEXT(intel);
    GLuint st1 = i830->state.Stipple[I830_STPREG_ST1];

    st1 &= ~ST1_ENABLE;

    switch (rprim) {
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

    i830->intel.reduced_primitive = rprim;

    if (st1 != i830->state.Stipple[I830_STPREG_ST1]) {
       I830_STATECHANGE(i830, I830_UPLOAD_STIPPLE);
       i830->state.Stipple[I830_STPREG_ST1] = st1;
    }
}

/* Pull apart the vertex format registers and figure out how large a
 * vertex is supposed to be. 
 */
static GLboolean i830_check_vertex_size( intelContextPtr intel,
					 GLuint expected )
{
   i830ContextPtr i830 = I830_CONTEXT(intel);
   int vft0 = i830->current->Ctx[I830_CTXREG_VF];
   int vft1 = i830->current->Ctx[I830_CTXREG_VF2];
   int nrtex = (vft0 & VFT0_TEX_COUNT_MASK) >> VFT0_TEX_COUNT_SHIFT;
   int i, sz = 0;

   switch (vft0 & VFT0_XYZW_MASK) {
   case VFT0_XY: sz = 2; break;
   case VFT0_XYZ: sz = 3; break;
   case VFT0_XYW: sz = 3; break;
   case VFT0_XYZW: sz = 4; break;
   default: 
      fprintf(stderr, "no xyzw specified\n");
      return 0;
   }

   if (vft0 & VFT0_SPEC) sz++;
   if (vft0 & VFT0_DIFFUSE) sz++;
   if (vft0 & VFT0_DEPTH_OFFSET) sz++;
   if (vft0 & VFT0_POINT_WIDTH) sz++;
	
   for (i = 0 ; i < nrtex ; i++) { 
      switch (vft1 & VFT1_TEX0_MASK) {
      case TEXCOORDFMT_2D: sz += 2; break;
      case TEXCOORDFMT_3D: sz += 3; break;
      case TEXCOORDFMT_4D: sz += 4; break;
      case TEXCOORDFMT_1D: sz += 1; break;
      }
      vft1 >>= VFT1_TEX1_SHIFT;
   }
	
   if (sz != expected) 
      fprintf(stderr, "vertex size mismatch %d/%d\n", sz, expected);
   
   return sz == expected;
}

static void i830_emit_invarient_state( intelContextPtr intel )
{
   BATCH_LOCALS;

   BEGIN_BATCH( 40 );

   OUT_BATCH(_3DSTATE_MAP_CUBE | MAP_UNIT(0));
   OUT_BATCH(_3DSTATE_MAP_CUBE | MAP_UNIT(1));
   OUT_BATCH(_3DSTATE_MAP_CUBE | MAP_UNIT(2));
   OUT_BATCH(_3DSTATE_MAP_CUBE | MAP_UNIT(3));

   OUT_BATCH(_3DSTATE_DFLT_DIFFUSE_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_SPEC_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_Z_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_FOG_MODE_CMD);
   OUT_BATCH(FOGFUNC_ENABLE |
	     FOG_LINEAR_CONST | 
	     FOGSRC_INDEX_Z | 
	     ENABLE_FOG_DENSITY);
   OUT_BATCH(0);
   OUT_BATCH(0);


   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
	     MAP_UNIT(0) |
	     DISABLE_TEX_STREAM_BUMP |
	     ENABLE_TEX_STREAM_COORD_SET |
	     TEX_STREAM_COORD_SET(0) |
	     ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(0));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
	     MAP_UNIT(1) |
	     DISABLE_TEX_STREAM_BUMP |
	     ENABLE_TEX_STREAM_COORD_SET |
	     TEX_STREAM_COORD_SET(1) |
	     ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(1));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
	     MAP_UNIT(2) |
	     DISABLE_TEX_STREAM_BUMP |
	     ENABLE_TEX_STREAM_COORD_SET |
	     TEX_STREAM_COORD_SET(2) |
	     ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(2));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
	     MAP_UNIT(3) |
	     DISABLE_TEX_STREAM_BUMP |
	     ENABLE_TEX_STREAM_COORD_SET |
	     TEX_STREAM_COORD_SET(3) |
	     ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(3));

   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(0));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(1));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(2));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(3));

   OUT_BATCH(_3DSTATE_RASTER_RULES_CMD |
	     ENABLE_POINT_RASTER_RULE |
	     OGL_POINT_RASTER_RULE |
	     ENABLE_LINE_STRIP_PROVOKE_VRTX |
	     ENABLE_TRI_FAN_PROVOKE_VRTX |
	     ENABLE_TRI_STRIP_PROVOKE_VRTX |
	     LINE_STRIP_PROVOKE_VRTX(1) |
	     TRI_FAN_PROVOKE_VRTX(2) | 
	     TRI_STRIP_PROVOKE_VRTX(2));

   OUT_BATCH(_3DSTATE_SCISSOR_ENABLE_CMD | 
	     DISABLE_SCISSOR_RECT);

   OUT_BATCH(_3DSTATE_SCISSOR_RECT_0_CMD);
   OUT_BATCH(0);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_VERTEX_TRANSFORM);
   OUT_BATCH(DISABLE_VIEWPORT_TRANSFORM | DISABLE_PERSPECTIVE_DIVIDE);

   OUT_BATCH(_3DSTATE_W_STATE_CMD);
   OUT_BATCH(MAGIC_W_STATE_DWORD1);
   OUT_BATCH(0x3f800000 /* 1.0 in IEEE float */ );


   OUT_BATCH(_3DSTATE_COLOR_FACTOR_CMD);
   OUT_BATCH(0x80808080);	/* .5 required in alpha for GL_DOT3_RGBA_EXT */

   ADVANCE_BATCH();
}


#define emit( intel, state, size )			\
do {							\
   int k;						\
   BEGIN_BATCH( size / sizeof(GLuint));			\
   for (k = 0 ; k < size / sizeof(GLuint) ; k++)	\
      OUT_BATCH(state[k]);				\
   ADVANCE_BATCH();					\
} while (0);

static GLuint get_state_size( struct i830_hw_state *state )
{
   GLuint dirty = state->active & ~state->emitted;
   GLuint sz = 0;
   GLuint i;

   if (dirty & I830_UPLOAD_INVARIENT)
      sz += 40 * sizeof(int);

   if (dirty & I830_UPLOAD_CTX) 
      sz += sizeof(state->Ctx);

   if (dirty & I830_UPLOAD_BUFFERS) 
      sz += sizeof(state->Buffer);

   if (dirty & I830_UPLOAD_STIPPLE) 
      sz += sizeof(state->Stipple);

   for (i = 0; i < I830_TEX_UNITS; i++) {
      if ((dirty & I830_UPLOAD_TEX(i)))  
	 sz += sizeof(state->Tex[i]); 

      if (dirty & I830_UPLOAD_TEXBLEND(i)) 
	 sz += state->TexBlendWordsUsed[i] * 4;
   }

   return sz;
}


/* Push the state into the sarea and/or texture memory.
 */
static void i830_emit_state( intelContextPtr intel )
{
   i830ContextPtr i830 = I830_CONTEXT(intel);
   struct i830_hw_state *state = i830->current;
   int i;
   GLuint dirty = state->active & ~state->emitted;
   GLuint counter = intel->batch.counter;
   BATCH_LOCALS;

   if (intel->batch.space < get_state_size(state)) {
      intelFlushBatch(intel, GL_TRUE);
      dirty = state->active & ~state->emitted;
      counter = intel->batch.counter;
   }

   if (dirty & I830_UPLOAD_INVARIENT) {
      if (VERBOSE) fprintf(stderr, "I830_UPLOAD_INVARIENT:\n"); 
      i830_emit_invarient_state( intel );
   }

   if (dirty & I830_UPLOAD_CTX) {
      if (VERBOSE) fprintf(stderr, "I830_UPLOAD_CTX:\n"); 
      emit( i830, state->Ctx, sizeof(state->Ctx) );
   }

   if (dirty & I830_UPLOAD_BUFFERS) {
      if (VERBOSE) fprintf(stderr, "I830_UPLOAD_BUFFERS:\n"); 
      emit( i830, state->Buffer, sizeof(state->Buffer) );
   }

   if (dirty & I830_UPLOAD_STIPPLE) {
      if (VERBOSE) fprintf(stderr, "I830_UPLOAD_STIPPLE:\n"); 
      emit( i830, state->Stipple, sizeof(state->Stipple) );
   }

   for (i = 0; i < I830_TEX_UNITS; i++) {
      if ((dirty & I830_UPLOAD_TEX(i))) { 
 	 if (VERBOSE) fprintf(stderr, "I830_UPLOAD_TEX(%d):\n", i); 
	 emit( i830, state->Tex[i], sizeof(state->Tex[i])); 
      } 

      if (dirty & I830_UPLOAD_TEXBLEND(i)) {
	 if (VERBOSE) fprintf(stderr, "I830_UPLOAD_TEXBLEND(%d):\n", i); 
	 emit( i830, state->TexBlend[i], 
	       state->TexBlendWordsUsed[i] * 4 );
      }
   }

   state->emitted |= dirty;
   intel->batch.last_emit_state = counter;
   assert(counter == intel->batch.counter);
}

static void i830_destroy_context( intelContextPtr intel )
{
   _tnl_free_vertices(&intel->ctx);
}

static void
i830_set_color_region(intelContextPtr intel, const intelRegion *region)
{
   i830ContextPtr i830 = I830_CONTEXT(intel);
   I830_STATECHANGE( i830, I830_UPLOAD_BUFFERS );
   i830->state.Buffer[I830_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i830->state.Buffer[I830_DESTREG_CBUFADDR2] = region->offset;
}


static void
i830_set_z_region(intelContextPtr intel, const intelRegion *region)
{
   i830ContextPtr i830 = I830_CONTEXT(intel);
   I830_STATECHANGE( i830, I830_UPLOAD_BUFFERS );
   i830->state.Buffer[I830_DESTREG_DBUFADDR1] =
      (BUF_3D_ID_DEPTH | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i830->state.Buffer[I830_DESTREG_DBUFADDR2] = region->offset;
}


static void
i830_update_color_z_regions(intelContextPtr intel,
                            const intelRegion *colorRegion,
                            const intelRegion *depthRegion)
{
   i830ContextPtr i830 = I830_CONTEXT(intel);

   i830->state.Buffer[I830_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(colorRegion->pitch) | BUF_3D_USE_FENCE);
   i830->state.Buffer[I830_DESTREG_CBUFADDR2] = colorRegion->offset;

   i830->state.Buffer[I830_DESTREG_DBUFADDR1] =
      (BUF_3D_ID_DEPTH | BUF_3D_PITCH(depthRegion->pitch) | BUF_3D_USE_FENCE);
   i830->state.Buffer[I830_DESTREG_DBUFADDR2] = depthRegion->offset;
}


/* This isn't really handled at the moment.
 */
static void i830_lost_hardware( intelContextPtr intel )
{
   I830_CONTEXT(intel)->state.emitted = 0;
}



static void i830_emit_flush( intelContextPtr intel )
{
   BATCH_LOCALS;

   BEGIN_BATCH(2);
   OUT_BATCH( MI_FLUSH | FLUSH_MAP_CACHE ); 
   OUT_BATCH( 0 );
   ADVANCE_BATCH();
}




void i830InitVtbl( i830ContextPtr i830 )
{
   i830->intel.vtbl.alloc_tex_obj = i830AllocTexObj;
   i830->intel.vtbl.check_vertex_size = i830_check_vertex_size;
   i830->intel.vtbl.clear_with_tris = i830ClearWithTris;
   i830->intel.vtbl.rotate_window = i830RotateWindow;
   i830->intel.vtbl.destroy = i830_destroy_context;
   i830->intel.vtbl.emit_state = i830_emit_state;
   i830->intel.vtbl.lost_hardware = i830_lost_hardware;
   i830->intel.vtbl.reduced_primitive_state = i830_reduced_primitive_state;
   i830->intel.vtbl.set_color_region = i830_set_color_region;
   i830->intel.vtbl.set_z_region = i830_set_z_region;
   i830->intel.vtbl.update_color_z_regions = i830_update_color_z_regions;
   i830->intel.vtbl.update_texture_state = i830UpdateTextureState;
   i830->intel.vtbl.emit_flush = i830_emit_flush;
   i830->intel.vtbl.render_start = i830_render_start;
}
