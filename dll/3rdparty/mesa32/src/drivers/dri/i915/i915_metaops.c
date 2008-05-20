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
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "utils.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "intel_rotate.h"

#include "i915_context.h"
#include "i915_reg.h"

/* A large amount of state doesn't need to be uploaded.
 */
#define ACTIVE (I915_UPLOAD_INVARIENT |         \
		I915_UPLOAD_PROGRAM | 		\
		I915_UPLOAD_STIPPLE |		\
		I915_UPLOAD_CTX |		\
		I915_UPLOAD_BUFFERS |		\
		I915_UPLOAD_TEX(0))		

#define SET_STATE( i915, STATE )			\
do {						\
   i915->current->emitted &= ~ACTIVE;		\
   i915->current = &i915->STATE;			\
   i915->current->emitted &= ~ACTIVE;		\
} while (0)

/* Operations where the 3D engine is decoupled temporarily from the
 * current GL state and used for other purposes than simply rendering
 * incoming triangles.
 */
static void set_initial_state( i915ContextPtr i915 )
{
   memcpy(&i915->meta, &i915->initial, sizeof(i915->meta) );
   i915->meta.active = ACTIVE;
   i915->meta.emitted = 0;
}


static void set_no_depth_stencil_write( i915ContextPtr i915 )
{
   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_FALSE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_TEST_ENABLE | 
				       S5_STENCIL_WRITE_ENABLE);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS6] &= ~(S6_DEPTH_TEST_ENABLE |
				       S6_DEPTH_WRITE_ENABLE);

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}

/* Set stencil unit to replace always with the reference value.
 */
static void set_stencil_replace( i915ContextPtr i915,
				 GLuint s_mask,
				 GLuint s_clear)
{
   GLuint op = STENCILOP_REPLACE;
   GLuint func = COMPAREFUNC_ALWAYS;

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_TRUE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] |= (S5_STENCIL_TEST_ENABLE | 
				      S5_STENCIL_WRITE_ENABLE);


   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS6] &= ~(S6_DEPTH_TEST_ENABLE |
				       S6_DEPTH_WRITE_ENABLE);


   /* ctx->Driver.StencilMask( ctx, s_mask )
    */
   i915->meta.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;

   i915->meta.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
					STENCIL_WRITE_MASK(s_mask));


   /* ctx->Driver.StencilOp( ctx, GL_REPLACE, GL_REPLACE, GL_REPLACE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_FAIL_MASK |
				       S5_STENCIL_PASS_Z_FAIL_MASK |
				       S5_STENCIL_PASS_Z_PASS_MASK);

   i915->meta.Ctx[I915_CTXREG_LIS5] |= ((op << S5_STENCIL_FAIL_SHIFT) |
				      (op << S5_STENCIL_PASS_Z_FAIL_SHIFT) |
				      (op << S5_STENCIL_PASS_Z_PASS_SHIFT));


   /* ctx->Driver.StencilFunc( ctx, GL_ALWAYS, s_ref, ~0 )
    */
   i915->meta.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i915->meta.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
					STENCIL_TEST_MASK(0xff));

   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_REF_MASK |
				       S5_STENCIL_TEST_FUNC_MASK);
					
   i915->meta.Ctx[I915_CTXREG_LIS5] |= ((s_clear << S5_STENCIL_REF_SHIFT) |  
				      (func << S5_STENCIL_TEST_FUNC_SHIFT)); 


   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}


static void set_color_mask( i915ContextPtr i915, GLboolean state )
{
   const GLuint mask = (S5_WRITEDISABLE_RED |
			S5_WRITEDISABLE_GREEN |
			S5_WRITEDISABLE_BLUE |
			S5_WRITEDISABLE_ALPHA);

   /* Copy colormask state from "regular" hw context.
    */
   if (state) {
      i915->meta.Ctx[I915_CTXREG_LIS5] &= ~mask;
      i915->meta.Ctx[I915_CTXREG_LIS5] |= 
	 (i915->state.Ctx[I915_CTXREG_LIS5] & mask);
   }
   else 
      i915->meta.Ctx[I915_CTXREG_LIS5] |= mask;
      
   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}




#define REG( type, nr ) (((type)<<5)|(nr))

#define REG_R(x)       REG(REG_TYPE_R, x)
#define REG_T(x)       REG(REG_TYPE_T, x)
#define REG_CONST(x)   REG(REG_TYPE_CONST, x)
#define REG_S(x)       REG(REG_TYPE_S, x)
#define REG_OC         REG(REG_TYPE_OC, 0)
#define REG_OD	       REG(REG_TYPE_OD, 0)
#define REG_U(x)       REG(REG_TYPE_U, x)

#define REG_T_DIFFUSE  REG(REG_TYPE_T, T_DIFFUSE)
#define REG_T_SPECULAR REG(REG_TYPE_T, T_SPECULAR)
#define REG_T_FOG_W    REG(REG_TYPE_T, T_FOG_W)
#define REG_T_TEX(x)   REG(REG_TYPE_T, x)


#define A0_DEST_REG( reg ) ( (reg) << A0_DEST_NR_SHIFT )
#define A0_SRC0_REG( reg ) ( (reg) << A0_SRC0_NR_SHIFT )
#define A1_SRC1_REG( reg ) ( (reg) << A1_SRC1_NR_SHIFT )
#define A1_SRC2_REG( reg ) ( (reg) << A1_SRC2_NR_SHIFT )
#define A2_SRC2_REG( reg ) ( (reg) << A2_SRC2_NR_SHIFT )
#define D0_DECL_REG( reg ) ( (reg) << D0_NR_SHIFT )
#define T0_DEST_REG( reg ) ( (reg) << T0_DEST_NR_SHIFT )

#define T0_SAMPLER( unit )     ((unit)<<T0_SAMPLER_NR_SHIFT)

#define T1_ADDRESS_REG( type, nr ) (((type)<<T1_ADDRESS_REG_TYPE_SHIFT)| \
				    ((nr)<<T1_ADDRESS_REG_NR_SHIFT))


#define A1_SRC0_XYZW ((SRC_X << A1_SRC0_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A1_SRC0_CHANNEL_Y_SHIFT) |	\
		      (SRC_Z << A1_SRC0_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A1_SRC0_CHANNEL_W_SHIFT))

#define A1_SRC1_XY   ((SRC_X << A1_SRC1_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A1_SRC1_CHANNEL_Y_SHIFT))

#define A2_SRC1_ZW   ((SRC_Z << A2_SRC1_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A2_SRC1_CHANNEL_W_SHIFT))

#define A2_SRC2_XYZW ((SRC_X << A2_SRC2_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A2_SRC2_CHANNEL_Y_SHIFT) |	\
		      (SRC_Z << A2_SRC2_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A2_SRC2_CHANNEL_W_SHIFT))





static void set_no_texture( i915ContextPtr i915 )
{
   static const GLuint prog[] = {
      _3DSTATE_PIXEL_SHADER_PROGRAM,

      /* Declare incoming diffuse color:
       */
      (D0_DCL |
       D0_DECL_REG( REG_T_DIFFUSE ) |
       D0_CHANNEL_ALL),
      D1_MBZ,
      D2_MBZ,

      /* output-color = mov(t_diffuse)
       */
      (A0_MOV |
       A0_DEST_REG( REG_OC ) |
       A0_DEST_CHANNEL_ALL |
       A0_SRC0_REG( REG_T_DIFFUSE )),
      (A1_SRC0_XYZW),
      0,
   };

   
   memcpy( i915->meta.Program, prog, sizeof(prog) );
   i915->meta.ProgramSize = sizeof(prog) / sizeof(*prog);
   i915->meta.Program[0] |= i915->meta.ProgramSize - 2;
   i915->meta.emitted &= ~I915_UPLOAD_PROGRAM;
}


static void enable_texture_blend_replace( i915ContextPtr i915 )
{
   static const GLuint prog[] = {
      _3DSTATE_PIXEL_SHADER_PROGRAM,

      /* Declare the sampler:
       */
      (D0_DCL |
       D0_DECL_REG( REG_S(0) ) |
       D0_SAMPLE_TYPE_2D |
       D0_CHANNEL_NONE),
      D1_MBZ,
      D2_MBZ,

      /* Declare the interpolated texture coordinate:
       */
      (D0_DCL |
       D0_DECL_REG( REG_T_TEX(0) ) |
       D0_CHANNEL_ALL),
      D1_MBZ,
      D2_MBZ,

      /* output-color = texld(sample0, texcoord0) 
       */
      (T0_TEXLD | 
       T0_DEST_REG( REG_OC ) |
       T0_SAMPLER( 0 )),
      T1_ADDRESS_REG(REG_TYPE_T, 0),
      T2_MBZ
   };

   memcpy( i915->meta.Program, prog, sizeof(prog) );
   i915->meta.ProgramSize = sizeof(prog) / sizeof(*prog);
   i915->meta.Program[0] |= i915->meta.ProgramSize - 2;
   i915->meta.emitted &= ~I915_UPLOAD_PROGRAM;
}





/* Set up an arbitary piece of memory as a rectangular texture
 * (including the front or back buffer).
 */
static void set_tex_rect_source( i915ContextPtr i915,
				 GLuint offset,
				 GLuint width, 
				 GLuint height,
				 GLuint pitch, /* in bytes! */
				 GLuint textureFormat )
{
   GLuint unit = 0;
   GLint numLevels = 1;
   GLuint *state = i915->meta.Tex[0];

#if 0
   printf("TexRect source offset 0x%x  pitch %d\n", offset, pitch);
#endif

/*    fprintf(stderr, "%s: offset: %x w: %d h: %d pitch %d format %x\n", */
/* 	   __FUNCTION__, offset, width, height, pitch, textureFormat ); */

   state[I915_TEXREG_MS2] = offset;
   state[I915_TEXREG_MS3] = (((height - 1) << MS3_HEIGHT_SHIFT) |
			    ((width - 1) << MS3_WIDTH_SHIFT) |
			    textureFormat |
			    MS3_USE_FENCE_REGS);

   state[I915_TEXREG_MS4] = ((((pitch / 4) - 1) << MS4_PITCH_SHIFT) | 
			    ((((numLevels-1) * 4)) << MS4_MAX_LOD_SHIFT));

   state[I915_TEXREG_SS2] = ((FILTER_NEAREST << SS2_MIN_FILTER_SHIFT) |
			    (MIPFILTER_NONE << SS2_MIP_FILTER_SHIFT) |
			    (FILTER_NEAREST << SS2_MAG_FILTER_SHIFT));
   state[I915_TEXREG_SS3] = ((TEXCOORDMODE_WRAP << SS3_TCX_ADDR_MODE_SHIFT) |
			    (TEXCOORDMODE_WRAP << SS3_TCY_ADDR_MODE_SHIFT) |
			    (TEXCOORDMODE_WRAP << SS3_TCZ_ADDR_MODE_SHIFT) |
			    (unit<<SS3_TEXTUREMAP_INDEX_SHIFT));

   state[I915_TEXREG_SS4] = 0;

   i915->meta.emitted &= ~I915_UPLOAD_TEX(0);
}


/* Select between front and back draw buffers.
 */
static void set_draw_region( i915ContextPtr i915, const intelRegion *region )
{
#if 0
   printf("Rotate into region: offset 0x%x  pitch %d\n",
          region->offset, region->pitch);
#endif
   i915->meta.Buffer[I915_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i915->meta.Buffer[I915_DESTREG_CBUFADDR2] = region->offset;
   i915->meta.emitted &= ~I915_UPLOAD_BUFFERS;
}


#if 0
/* Setup an arbitary draw format, useful for targeting texture or agp
 * memory.
 */
static void set_draw_format( i915ContextPtr i915,
			     GLuint format,
			     GLuint depth_format)
{
   i915->meta.Buffer[I915_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) | /* .5 */
					DSTORG_VERT_BIAS(0x8) | /* .5 */
					format |
					LOD_PRECLAMP_OGL |
					TEX_DEFAULT_COLOR_OGL |
					depth_format);

   i915->meta.emitted &= ~I915_UPLOAD_BUFFERS;
/*    fprintf(stderr, "%s: DV1: %x\n",  */
/* 	   __FUNCTION__, i915->meta.Buffer[I915_DESTREG_DV1]); */
}
#endif

static void set_vertex_format( i915ContextPtr i915 )
{
   i915->meta.Ctx[I915_CTXREG_LIS2] = 
      (S2_TEXCOORD_FMT(0, TEXCOORDFMT_2D) |
       S2_TEXCOORD_FMT(1, TEXCOORDFMT_NOT_PRESENT) | 
       S2_TEXCOORD_FMT(2, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(3, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(4, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(5, TEXCOORDFMT_NOT_PRESENT) | 
       S2_TEXCOORD_FMT(6, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(7, TEXCOORDFMT_NOT_PRESENT));

   i915->meta.Ctx[I915_CTXREG_LIS4] &= ~S4_VFMT_MASK;

   i915->meta.Ctx[I915_CTXREG_LIS4] |= 
      (S4_VFMT_COLOR |
       S4_VFMT_SPEC_FOG |
       S4_VFMT_XYZW);

   i915->meta.emitted &= ~I915_UPLOAD_CTX;

}


static void draw_quad(i915ContextPtr i915, 
		      GLfloat x0, GLfloat x1,
		      GLfloat y0, GLfloat y1, 
		      GLubyte red, GLubyte green,
		      GLubyte blue, GLubyte alpha,
		      GLfloat s0, GLfloat s1,
		      GLfloat t0, GLfloat t1 )
{
   GLuint vertex_size = 8;
   GLuint *vb = intelEmitInlinePrimitiveLocked( &i915->intel, 
						PRIM3D_TRIFAN, 
						4 * vertex_size,
						vertex_size );
   intelVertex tmp;
   int i;

   if (0)
      fprintf(stderr, "%s: %f,%f-%f,%f 0x%x%x%x%x %f,%f-%f,%f\n",
	      __FUNCTION__,
	      x0,y0,x1,y1,red,green,blue,alpha,s0,t0,s1,t1);


   /* initial vertex, left bottom */
   tmp.v.x = x0;
   tmp.v.y = y0;
   tmp.v.z = 1.0;
   tmp.v.w = 1.0; 
   tmp.v.color.red = red;
   tmp.v.color.green = green;
   tmp.v.color.blue = blue;
   tmp.v.color.alpha = alpha;
   tmp.v.specular.red = 0;
   tmp.v.specular.green = 0;
   tmp.v.specular.blue = 0;
   tmp.v.specular.alpha = 0;
   tmp.v.u0 = s0;
   tmp.v.v0 = t0;

   for (i = 0 ; i < vertex_size ; i++)
      vb[i] = tmp.ui[i];

   /* right bottom */
   vb += vertex_size;
   tmp.v.x = x1;
   tmp.v.u0 = s1;
   for (i = 0 ; i < vertex_size ; i++)
      vb[i] = tmp.ui[i];

   /* right top */
   vb += vertex_size;
   tmp.v.y = y1;
   tmp.v.v0 = t1;
   for (i = 0 ; i < vertex_size ; i++)
      vb[i] = tmp.ui[i];

   /* left top */
   vb += vertex_size;
   tmp.v.x = x0;
   tmp.v.u0 = s0;
   for (i = 0 ; i < vertex_size ; i++)
      vb[i] = tmp.ui[i];
}


static void draw_poly(i915ContextPtr i915, 
		      GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha,
                      GLuint numVerts,
                      /*const*/ GLfloat verts[][2],
                      /*const*/ GLfloat texcoords[][2])
{
   GLuint vertex_size = 8;
   GLuint *vb = intelEmitInlinePrimitiveLocked( &i915->intel, 
						PRIM3D_TRIFAN, 
						numVerts * vertex_size,
						vertex_size );
   intelVertex tmp;
   int i, k;

   /* initial constant vertex fields */
   tmp.v.z = 1.0;
   tmp.v.w = 1.0; 
   tmp.v.color.red = red;
   tmp.v.color.green = green;
   tmp.v.color.blue = blue;
   tmp.v.color.alpha = alpha;
   tmp.v.specular.red = 0;
   tmp.v.specular.green = 0;
   tmp.v.specular.blue = 0;
   tmp.v.specular.alpha = 0;

   for (k = 0; k < numVerts; k++) {
      tmp.v.x = verts[k][0];
      tmp.v.y = verts[k][1];
      tmp.v.u0 = texcoords[k][0];
      tmp.v.v0 = texcoords[k][1];

      for (i = 0 ; i < vertex_size ; i++)
         vb[i] = tmp.ui[i];

      vb += vertex_size;
   }
}


void 
i915ClearWithTris(intelContextPtr intel, GLbitfield buffers,
		  GLboolean allFoo,
		  GLint cxFoo, GLint cyFoo, GLint cwFoo, GLint chFoo)
{
   i915ContextPtr i915 = I915_CONTEXT( intel );
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   intelScreenPrivate *screen = intel->intelScreen;
   int x0, y0, x1, y1;
   GLint cx, cy, cw, ch;
   GLboolean all;

   SET_STATE( i915, meta ); 
   set_initial_state( i915 ); 
   set_no_texture( i915 ); 
   set_vertex_format( i915 ); 

   LOCK_HARDWARE(intel);

   /* get clear bounds after locking */
   cx = intel->ctx.DrawBuffer->_Xmin;
   cy = intel->ctx.DrawBuffer->_Ymin;
   cw = intel->ctx.DrawBuffer->_Xmax - cx;
   ch = intel->ctx.DrawBuffer->_Ymax - cy;
   all = (cw == intel->ctx.DrawBuffer->Width &&
          ch == intel->ctx.DrawBuffer->Height);

   if (!all) {
      x0 = cx;
      y0 = cy;
      x1 = x0 + cw;
      y1 = y0 + ch;
   } else {
      x0 = 0;
      y0 = 0;
      x1 = x0 + dPriv->w;
      y1 = y0 + dPriv->h;
   }

   /* Don't do any clipping to screen - these are window coordinates.
    * The active cliprects will be applied as for any other geometry.
    */

   if (buffers & BUFFER_BIT_FRONT_LEFT) { 
      set_no_depth_stencil_write( i915 );
      set_color_mask( i915, GL_TRUE );
      set_draw_region( i915, &screen->front );

      draw_quad(i915, x0, x1, y0, y1,
		intel->clear_red, intel->clear_green, 
 		intel->clear_blue, intel->clear_alpha, 
		0, 0, 0, 0);
   }

   if (buffers & BUFFER_BIT_BACK_LEFT) {
      set_no_depth_stencil_write( i915 );
      set_color_mask( i915, GL_TRUE );
      set_draw_region( i915, &screen->back );

      draw_quad(i915, x0, x1, y0, y1,
		intel->clear_red, intel->clear_green,
		intel->clear_blue, intel->clear_alpha,
		0, 0, 0, 0);
   }

   if (buffers & BUFFER_BIT_STENCIL) {
      set_stencil_replace( i915, 
			   intel->ctx.Stencil.WriteMask[0], 
			   intel->ctx.Stencil.Clear);
      
      set_color_mask( i915, GL_FALSE );
      set_draw_region( i915, &screen->front ); /* could be either? */

      draw_quad( i915, x0, x1, y0, y1, 0, 0, 0, 0, 0, 0, 0, 0 );
   }

   UNLOCK_HARDWARE(intel);

   SET_STATE( i915, state );
}


/**
 * Copy the window contents named by dPriv to the rotated (or reflected)
 * color buffer.
 * srcBuf is BUFFER_BIT_FRONT_LEFT or BUFFER_BIT_BACK_LEFT to indicate the source.
 */
void
i915RotateWindow(intelContextPtr intel, __DRIdrawablePrivate *dPriv,
                 GLuint srcBuf)
{
   i915ContextPtr i915 = I915_CONTEXT( intel );
   intelScreenPrivate *screen = intel->intelScreen;
   const GLuint cpp = screen->cpp;
   drm_clip_rect_t fullRect;
   GLuint textureFormat, srcOffset, srcPitch;
   const drm_clip_rect_t *clipRects;
   int numClipRects;
   int i;

   int xOrig, yOrig;
   int origNumClipRects;
   drm_clip_rect_t *origRects;

   /*
    * set up hardware state
    */
   intelFlush( &intel->ctx );

   SET_STATE( i915, meta ); 
   set_initial_state( i915 ); 
   set_no_texture( i915 ); 
   set_vertex_format( i915 ); 
   set_no_depth_stencil_write( i915 );
   set_color_mask( i915, GL_TRUE );

   LOCK_HARDWARE(intel);

   /* save current drawing origin and cliprects (restored at end) */
   xOrig = intel->drawX;
   yOrig = intel->drawY;
   origNumClipRects = intel->numClipRects;
   origRects = intel->pClipRects;

   if (!intel->numClipRects)
      goto done;

   /*
    * set drawing origin, cliprects for full-screen access to rotated screen
    */
   fullRect.x1 = 0;
   fullRect.y1 = 0;
   fullRect.x2 = screen->rotatedWidth;
   fullRect.y2 = screen->rotatedHeight;
   intel->drawX = 0;
   intel->drawY = 0;
   intel->numClipRects = 1;
   intel->pClipRects = &fullRect;

   set_draw_region( i915, &screen->rotated );

   if (cpp == 4)
      textureFormat = MAPSURF_32BIT | MT_32BIT_ARGB8888;
   else
      textureFormat = MAPSURF_16BIT | MT_16BIT_RGB565;

   if (srcBuf == BUFFER_BIT_FRONT_LEFT) {
      srcPitch = screen->front.pitch;   /* in bytes */
      srcOffset = screen->front.offset; /* bytes */
      clipRects = dPriv->pClipRects;
      numClipRects = dPriv->numClipRects;
   }
   else {
      srcPitch = screen->back.pitch;   /* in bytes */
      srcOffset = screen->back.offset; /* bytes */
      clipRects = dPriv->pBackClipRects;
      numClipRects = dPriv->numBackClipRects;
   }

   /* set the whole screen up as a texture to avoid alignment issues */
   set_tex_rect_source(i915,
                       srcOffset,
                       screen->width,
		       screen->height,
                       srcPitch,
                       textureFormat);

   enable_texture_blend_replace(i915);

   /*
    * loop over the source window's cliprects
    */
   for (i = 0; i < numClipRects; i++) {
      int srcX0 = clipRects[i].x1;
      int srcY0 = clipRects[i].y1;
      int srcX1 = clipRects[i].x2;
      int srcY1 = clipRects[i].y2;
      GLfloat verts[4][2], tex[4][2];
      int j;

      /* build vertices for four corners of clip rect */
      verts[0][0] = srcX0;  verts[0][1] = srcY0;
      verts[1][0] = srcX1;  verts[1][1] = srcY0;
      verts[2][0] = srcX1;  verts[2][1] = srcY1;
      verts[3][0] = srcX0;  verts[3][1] = srcY1;

      /* .. and texcoords */
      tex[0][0] = srcX0;  tex[0][1] = srcY0;
      tex[1][0] = srcX1;  tex[1][1] = srcY0;
      tex[2][0] = srcX1;  tex[2][1] = srcY1;
      tex[3][0] = srcX0;  tex[3][1] = srcY1;

      /* transform coords to rotated screen coords */
      for (j = 0; j < 4; j++) {
         matrix23TransformCoordf(&screen->rotMatrix,
                                 &verts[j][0], &verts[j][1]);
      }

      /* draw polygon to map source image to dest region */
      draw_poly(i915, 255, 255, 255, 255, 4, verts, tex);

   } /* cliprect loop */

   intelFlushBatchLocked( intel, GL_FALSE, GL_FALSE, GL_FALSE );

 done:
   /* restore original drawing origin and cliprects */
   intel->drawX = xOrig;
   intel->drawY = yOrig;
   intel->numClipRects = origNumClipRects;
   intel->pClipRects = origRects;

   UNLOCK_HARDWARE(intel);

   SET_STATE( i915, state );
}

