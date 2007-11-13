/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810state.c,v 1.9 2002/10/30 12:51:33 alanh Exp $ */

#include <stdio.h>

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "dd.h"
#include "colormac.h"

#include "texmem.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810state.h"
#include "i810tex.h"
#include "i810vb.h"
#include "i810tris.h"
#include "i810ioctl.h"

#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

static __inline__ GLuint i810PackColor(GLuint format,
				       GLubyte r, GLubyte g,
				       GLubyte b, GLubyte a)
{

   if (I810_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   switch (format) {
   case DV_PF_555:
      return PACK_COLOR_1555( a, r, g, b );
   case DV_PF_565:
      return PACK_COLOR_565( r, g, b );
   default:
      fprintf(stderr, "unknown format %d\n", (int)format);
      return 0;
   }
}


static void i810AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint a = (ZA_UPDATE_ALPHAFUNC|ZA_UPDATE_ALPHAREF);
   GLubyte refByte;

   CLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   switch (ctx->Color.AlphaFunc) {
   case GL_NEVER:    a |= ZA_ALPHA_NEVER;    break;
   case GL_LESS:     a |= ZA_ALPHA_LESS;     break;
   case GL_GEQUAL:   a |= ZA_ALPHA_GEQUAL;   break;
   case GL_LEQUAL:   a |= ZA_ALPHA_LEQUAL;   break;
   case GL_GREATER:  a |= ZA_ALPHA_GREATER;  break;
   case GL_NOTEQUAL: a |= ZA_ALPHA_NOTEQUAL; break;
   case GL_EQUAL:    a |= ZA_ALPHA_EQUAL;    break;
   case GL_ALWAYS:   a |= ZA_ALPHA_ALWAYS;   break;
   default: return;
   }

   a |= ((refByte & 0xfc) << ZA_ALPHAREF_SHIFT);

   I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
   imesa->Setup[I810_CTXREG_ZA] &= ~(ZA_ALPHA_MASK|ZA_ALPHAREF_MASK);
   imesa->Setup[I810_CTXREG_ZA] |= a;
}

static void i810BlendEquationSeparate(GLcontext *ctx,
				      GLenum modeRGB, GLenum modeA)
{
   assert( modeRGB == modeA );

   /* Can only do GL_ADD equation in hardware */
   FALLBACK( I810_CONTEXT(ctx), I810_FALLBACK_BLEND_EQ, 
	     modeRGB != GL_FUNC_ADD);

   /* BlendEquation sets ColorLogicOpEnabled in an unexpected
    * manner.
    */
   FALLBACK( I810_CONTEXT(ctx), I810_FALLBACK_LOGICOP,
	     (ctx->Color.ColorLogicOpEnabled &&
	      ctx->Color.LogicOp != GL_COPY));
}

static void i810BlendFuncSeparate( GLcontext *ctx, GLenum sfactorRGB,
				     GLenum dfactorRGB, GLenum sfactorA,
				     GLenum dfactorA )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint a = SDM_UPDATE_SRC_BLEND | SDM_UPDATE_DST_BLEND;
   GLboolean fallback = GL_FALSE;

   switch (ctx->Color.BlendSrcRGB) {
   case GL_ZERO:                a |= SDM_SRC_ZERO; break;
   case GL_ONE:                 a |= SDM_SRC_ONE; break;
   case GL_SRC_COLOR:           a |= SDM_SRC_SRC_COLOR; break;
   case GL_ONE_MINUS_SRC_COLOR: a |= SDM_SRC_INV_SRC_COLOR; break;
   case GL_SRC_ALPHA:           a |= SDM_SRC_SRC_ALPHA; break;
   case GL_ONE_MINUS_SRC_ALPHA: a |= SDM_SRC_INV_SRC_ALPHA; break;
   case GL_DST_ALPHA:           a |= SDM_SRC_ONE; break;
   case GL_ONE_MINUS_DST_ALPHA: a |= SDM_SRC_ZERO; break;
   case GL_DST_COLOR:           a |= SDM_SRC_DST_COLOR; break;
   case GL_ONE_MINUS_DST_COLOR: a |= SDM_SRC_INV_DST_COLOR; break;

   /* (f, f, f, 1), f = min(As, 1 - Ad) = min(As, 1 - 1) = 0
    * So (f, f, f, 1) = (0, 0, 0, 1).  Since there is no destination alpha and
    * the only supported alpha operation is GL_FUNC_ADD, the result modulating
    * the source alpha with the alpha factor is largely irrelevant.
    */
   case GL_SRC_ALPHA_SATURATE:  a |= SDM_SRC_ZERO; break;

   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      fallback = GL_TRUE;
      break;
   default:
      return;
   }

   switch (ctx->Color.BlendDstRGB) {
   case GL_ZERO:                a |= SDM_DST_ZERO; break;
   case GL_ONE:                 a |= SDM_DST_ONE; break;
   case GL_SRC_COLOR:           a |= SDM_DST_SRC_COLOR; break;
   case GL_ONE_MINUS_SRC_COLOR: a |= SDM_DST_INV_SRC_COLOR; break;
   case GL_SRC_ALPHA:           a |= SDM_DST_SRC_ALPHA; break;
   case GL_ONE_MINUS_SRC_ALPHA: a |= SDM_DST_INV_SRC_ALPHA; break;
   case GL_DST_ALPHA:           a |= SDM_DST_ONE; break;
   case GL_ONE_MINUS_DST_ALPHA: a |= SDM_DST_ZERO; break;
   case GL_DST_COLOR:           a |= SDM_DST_DST_COLOR; break;
   case GL_ONE_MINUS_DST_COLOR: a |= SDM_DST_INV_DST_COLOR; break;

   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      fallback = GL_TRUE;
      break;
   default:
      return;
   }

   FALLBACK( imesa, I810_FALLBACK_BLEND_FUNC, fallback);
   if (!fallback) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_SDM] &= ~(SDM_SRC_MASK|SDM_DST_MASK);
      imesa->Setup[I810_CTXREG_SDM] |= a;
   }
}



static void i810DepthFunc(GLcontext *ctx, GLenum func)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   int zmode;

   switch(func)  {
   case GL_NEVER: zmode = LCS_Z_NEVER; break;
   case GL_ALWAYS: zmode = LCS_Z_ALWAYS; break;
   case GL_LESS: zmode = LCS_Z_LESS; break;
   case GL_LEQUAL: zmode = LCS_Z_LEQUAL; break;
   case GL_EQUAL: zmode = LCS_Z_EQUAL; break;
   case GL_GREATER: zmode = LCS_Z_GREATER; break;
   case GL_GEQUAL: zmode = LCS_Z_GEQUAL; break;
   case GL_NOTEQUAL: zmode = LCS_Z_NOTEQUAL; break;
   default: return;
   }

   I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
   imesa->Setup[I810_CTXREG_LCS] &= ~LCS_Z_MASK;
   imesa->Setup[I810_CTXREG_LCS] |= zmode;
}

static void i810DepthMask(GLcontext *ctx, GLboolean flag)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   I810_STATECHANGE(imesa, I810_UPLOAD_CTX);

   if (flag)
      imesa->Setup[I810_CTXREG_B2] |= B2_ZB_WRITE_ENABLE;
   else
      imesa->Setup[I810_CTXREG_B2] &= ~B2_ZB_WRITE_ENABLE;
}


/* =============================================================
 * Polygon stipple
 *
 * The i810 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 */
static void i810PolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i,j,k;
   int active = (ctx->Polygon.StippleFlag &&
		 imesa->reduced_primitive == GL_TRIANGLES);
   GLuint newMask;

   if (active) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_ST1] &= ~ST1_ENABLE;
   }

   p[0] = mask[12] & 0xf; p[0] |= p[0] << 4;
   p[1] = mask[8] & 0xf; p[1] |= p[1] << 4;
   p[2] = mask[4] & 0xf; p[2] |= p[2] << 4;
   p[3] = mask[0] & 0xf; p[3] |= p[3] << 4;

   for (k = 0 ; k < 8 ; k++)
      for (j = 0 ; j < 4; j++)
	 for (i = 0 ; i < 4 ; i++)
	    if (*m++ != p[j]) {
	       imesa->stipple_in_hw = 0;
	       return;
	    }

   newMask = ((p[0] & 0xf) << 0) |
             ((p[1] & 0xf) << 4) |
             ((p[2] & 0xf) << 8) |
             ((p[3] & 0xf) << 12);

   if (newMask == 0xffff) {
      /* this is needed to make conform pass */
      imesa->stipple_in_hw = 0;
      return;
   }

   imesa->Setup[I810_CTXREG_ST1] &= ~0xffff;
   imesa->Setup[I810_CTXREG_ST1] |= newMask;
   imesa->stipple_in_hw = 1;

   if (active)
      imesa->Setup[I810_CTXREG_ST1] |= ST1_ENABLE;
}



/* =============================================================
 * Hardware clipping
 */


static void i810Scissor( GLcontext *ctx, GLint x, GLint y,
			 GLsizei w, GLsizei h )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   if (ctx->Scissor.Enabled) {
      I810_FIREVERTICES(imesa);	/* don't pipeline cliprect changes */
      imesa->upload_cliprects = GL_TRUE;
   }

   imesa->scissor_rect.x1 = x;
   imesa->scissor_rect.y1 = imesa->driDrawable->h - (y + h);
   imesa->scissor_rect.x2 = x + w;
   imesa->scissor_rect.y2 = imesa->driDrawable->h - y;
}


static void i810LogicOp( GLcontext *ctx, GLenum opcode )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   FALLBACK( imesa, I810_FALLBACK_LOGICOP,
	     (ctx->Color.ColorLogicOpEnabled && opcode != GL_COPY) );
}

/* Fallback to swrast for select and feedback.
 */
static void i810RenderMode( GLcontext *ctx, GLenum mode )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   FALLBACK( imesa, I810_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}


void i810DrawBuffer(GLcontext *ctx, GLenum mode )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   int front = 0;
  
   /*
    * _DrawDestMask is easier to cope with than <mode>.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0]) {
   case BUFFER_BIT_FRONT_LEFT:
     front = 1;
     break;
   case BUFFER_BIT_BACK_LEFT:
     front = 0;
     break;
   default:
      /* GL_NONE or GL_FRONT_AND_BACK or stereo left&right, etc */
      FALLBACK( imesa, I810_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   if ( imesa->sarea->pf_current_page == 1 ) 
     front ^= 1;
 
   FALLBACK( imesa, I810_FALLBACK_DRAW_BUFFER, GL_FALSE );
   I810_FIREVERTICES(imesa);
   I810_STATECHANGE(imesa, I810_UPLOAD_BUFFERS);

   if (front)
   {
     imesa->BufferSetup[I810_DESTREG_DI1] = (imesa->i810Screen->fbOffset |
					     imesa->i810Screen->backPitchBits);
     i810XMesaSetFrontClipRects( imesa );
   }
   else
   {
     imesa->BufferSetup[I810_DESTREG_DI1] = (imesa->i810Screen->backOffset |
					     imesa->i810Screen->backPitchBits);
     i810XMesaSetBackClipRects( imesa );
   }
}


static void i810ReadBuffer(GLcontext *ctx, GLenum mode )
{
   /* XXX anything? */
}


static void i810ClearColor(GLcontext *ctx, const GLfloat color[4] )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLubyte c[4];
   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);
   imesa->ClearColor = i810PackColor( imesa->i810Screen->fbFormat,
				      c[0], c[1], c[2], c[3] );
}


/* =============================================================
 * Culling - the i810 isn't quite as clean here as the rest of
 *           its interfaces, but it's not bad.
 */
static void i810CullFaceFrontFace(GLcontext *ctx, GLenum unused)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint mode = LCS_CULL_BOTH;

   if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = LCS_CULL_CW;
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (LCS_CULL_CW ^ LCS_CULL_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (LCS_CULL_CW ^ LCS_CULL_CCW);
   }

   imesa->LcsCullMode = mode;

   if (ctx->Polygon.CullFlag)
   {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_CULL_MASK;
      imesa->Setup[I810_CTXREG_LCS] |= mode;
   }
}


static void i810LineWidth( GLcontext *ctx, GLfloat widthf )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   int width = (int)ctx->Line._Width;

   imesa->LcsLineWidth = 0;
   if (width & 1) imesa->LcsLineWidth |= LCS_LINEWIDTH_1_0;
   if (width & 2) imesa->LcsLineWidth |= LCS_LINEWIDTH_2_0;

   if (imesa->reduced_primitive == GL_LINES) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_LINEWIDTH_3_0;
      imesa->Setup[I810_CTXREG_LCS] |= imesa->LcsLineWidth;
   }
}

static void i810PointSize( GLcontext *ctx, GLfloat sz )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   int size = (int)ctx->Point._Size;

   imesa->LcsPointSize = 0;
   if (size & 1) imesa->LcsPointSize |= LCS_LINEWIDTH_1_0;
   if (size & 2) imesa->LcsPointSize |= LCS_LINEWIDTH_2_0;

   if (imesa->reduced_primitive == GL_POINTS) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_LINEWIDTH_3_0;
      imesa->Setup[I810_CTXREG_LCS] |= imesa->LcsPointSize;
   }
}

/* =============================================================
 * Color masks
 */

static void i810ColorMask(GLcontext *ctx,
			  GLboolean r, GLboolean g,
			  GLboolean b, GLboolean a )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint tmp = 0;

   if (r && g && b) {
      tmp = imesa->Setup[I810_CTXREG_B2] | B2_FB_WRITE_ENABLE;
      FALLBACK( imesa, I810_FALLBACK_COLORMASK, GL_FALSE );
   } else if (!r && !g && !b) {
      tmp = imesa->Setup[I810_CTXREG_B2] & ~B2_FB_WRITE_ENABLE;
      FALLBACK( imesa, I810_FALLBACK_COLORMASK, GL_FALSE );
   } else {
      FALLBACK( imesa, I810_FALLBACK_COLORMASK, GL_TRUE );
      return;
   }

   if (tmp != imesa->Setup[I810_CTXREG_B2]) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_B2] = tmp;
      imesa->dirty |= I810_UPLOAD_CTX;
   }
}

/* Seperate specular not fully implemented on the i810.
 */
static void i810LightModelfv(GLcontext *ctx, GLenum pname,
			       const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL)
   {
      i810ContextPtr imesa = I810_CONTEXT( ctx );
      FALLBACK( imesa, I810_FALLBACK_SPECULAR,
		(ctx->Light.Enabled &&
		 ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR));
   }
}

/* But the 815 has it...
 */
static void i810LightModelfv_i815(GLcontext *ctx, GLenum pname,
				    const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL)
   {
      i810ContextPtr imesa = I810_CONTEXT( ctx );

      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	 imesa->Setup[I810_CTXREG_B1] |= B1_SPEC_ENABLE;
      else
	 imesa->Setup[I810_CTXREG_B1] &= ~B1_SPEC_ENABLE;
   }
}

/* In Mesa 3.5 we can reliably do native flatshading.
 */
static void i810ShadeModel(GLcontext *ctx, GLenum mode)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
   if (mode == GL_FLAT)
      imesa->Setup[I810_CTXREG_LCS] |= LCS_INTERP_FLAT;
   else
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_INTERP_FLAT;
}



/* =============================================================
 * Fog
 */
static void i810Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   if (pname == GL_FOG_COLOR) {
      GLuint color = (((GLubyte)(ctx->Fog.Color[0]*255.0F) << 16) |
		      ((GLubyte)(ctx->Fog.Color[1]*255.0F) << 8) |
		      ((GLubyte)(ctx->Fog.Color[2]*255.0F) << 0));

      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_FOG] = ((GFX_OP_FOG_COLOR | color) &
				      ~FOG_RESERVED_MASK);
   }
}


/* =============================================================
 */
static void i810Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   switch(cap) {
   case GL_ALPHA_TEST:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_B1] &= ~B1_ALPHA_TEST_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_ALPHA_TEST_ENABLE;
      break;
   case GL_BLEND:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_B1] &= ~B1_BLEND_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_BLEND_ENABLE;

      /* For some reason enable(GL_BLEND) affects ColorLogicOpEnabled.
       */
      FALLBACK( imesa, I810_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled &&
		 ctx->Color.LogicOp != GL_COPY));
      break;
   case GL_DEPTH_TEST:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_B1] &= ~B1_Z_TEST_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_Z_TEST_ENABLE;
      break;
   case GL_SCISSOR_TEST:
      /* XXX without these next two lines, conform's scissor test fails */
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      I810_STATECHANGE(imesa, I810_UPLOAD_BUFFERS);
      I810_FIREVERTICES(imesa);	/* don't pipeline cliprect changes */
      imesa->upload_cliprects = GL_TRUE;
      imesa->scissor = state;
      break;
   case GL_POLYGON_STIPPLE:
      if (imesa->stipple_in_hw && imesa->reduced_primitive == GL_TRIANGLES)
      {
	 I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
	 imesa->Setup[I810_CTXREG_ST1] &= ~ST1_ENABLE;
	 if (state)
	    imesa->Setup[I810_CTXREG_ST1] |= ST1_ENABLE;
      }
      break;
   case GL_LINE_SMOOTH:
      /* Need to fatten the lines by .5, or they disappear...
       */
      if (imesa->reduced_primitive == GL_LINES) {
	 I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
	 imesa->Setup[I810_CTXREG_AA] &= ~AA_ENABLE;
	 imesa->Setup[I810_CTXREG_LCS] &= ~LCS_LINEWIDTH_0_5;
	 if (state) {
	    imesa->Setup[I810_CTXREG_AA] |= AA_ENABLE;
	    imesa->Setup[I810_CTXREG_LCS] |= LCS_LINEWIDTH_0_5;
	 }
      }
      break;
   case GL_POINT_SMOOTH:
      if (imesa->reduced_primitive == GL_POINTS) {
	 I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
	 imesa->Setup[I810_CTXREG_AA] &= ~AA_ENABLE;
	 imesa->Setup[I810_CTXREG_LCS] &= ~LCS_LINEWIDTH_0_5;
	 if (state) {
	    imesa->Setup[I810_CTXREG_AA] |= AA_ENABLE;
	    imesa->Setup[I810_CTXREG_LCS] |= LCS_LINEWIDTH_0_5;
	 }
      }
      break;
   case GL_POLYGON_SMOOTH:
      if (imesa->reduced_primitive == GL_TRIANGLES) {
	 I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
	 imesa->Setup[I810_CTXREG_AA] &= ~AA_ENABLE;
	 if (state)
	    imesa->Setup[I810_CTXREG_AA] |= AA_ENABLE;
      }
      break;
   case GL_FOG:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_B1] &= ~B1_FOG_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_FOG_ENABLE;
      break;
   case GL_CULL_FACE:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_CULL_MASK;
      if (state)
	 imesa->Setup[I810_CTXREG_LCS] |= imesa->LcsCullMode;
      else
	 imesa->Setup[I810_CTXREG_LCS] |= LCS_CULL_DISABLE;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE_NV:
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      if (ctx->Texture.CurrentUnit == 0) {
	 imesa->Setup[I810_CTXREG_MT] &= ~MT_TEXEL0_ENABLE;
	 if (state)
	    imesa->Setup[I810_CTXREG_MT] |= MT_TEXEL0_ENABLE;
      } else {
	 imesa->Setup[I810_CTXREG_MT] &= ~MT_TEXEL1_ENABLE;
	 if (state)
	    imesa->Setup[I810_CTXREG_MT] |= MT_TEXEL1_ENABLE;
      }
      break;
   case GL_COLOR_LOGIC_OP:
      FALLBACK( imesa, I810_FALLBACK_LOGICOP,
		(state && ctx->Color.LogicOp != GL_COPY));
      break;
   case GL_STENCIL_TEST:
      FALLBACK( imesa, I810_FALLBACK_STENCIL, state );
      break;
   default:
      ;
   }
}







/* =============================================================
 */




void i810EmitDrawingRectangle( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i810ScreenPrivate *i810Screen = imesa->i810Screen;
   int x0 = imesa->drawX;
   int y0 = imesa->drawY;
   int x1 = x0 + dPriv->w;
   int y1 = y0 + dPriv->h;
   GLuint dr2, dr3, dr4;


   /* Coordinate origin of the window - may be offscreen.
    */
   dr4 = imesa->BufferSetup[I810_DESTREG_DR4] = ((y0<<16) |
						 (((unsigned)x0)&0xFFFF));

   /* Clip to screen.
    */
   if (x0 < 0) x0 = 0;
   if (y0 < 0) y0 = 0;
   if (x1 > i810Screen->width-1) x1 = i810Screen->width-1;
   if (y1 > i810Screen->height-1) y1 = i810Screen->height-1;


   /* Onscreen drawing rectangle.
    */
   dr2 = imesa->BufferSetup[I810_DESTREG_DR2] = ((y0<<16) | x0);
   dr3 = imesa->BufferSetup[I810_DESTREG_DR3] = (((y1+1)<<16) | (x1+1));


   imesa->dirty |= I810_UPLOAD_BUFFERS;
}



static void i810CalcViewport( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = imesa->ViewportMatrix.m;

   /* See also i810_translate_vertex.  SUBPIXEL adjustments can be done
    * via state vars, too.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + imesa->driDrawable->h + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * (1.0 / 0xffff);
   m[MAT_TZ] =   v[MAT_TZ] * (1.0 / 0xffff);
}

static void i810Viewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   i810CalcViewport( ctx );
}

static void i810DepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   i810CalcViewport( ctx );
}



void i810PrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s\n",
	   msg,
	   (unsigned int) state,
	   (state & I810_UPLOAD_TEX0)  ? "upload-tex0, " : "",
	   (state & I810_UPLOAD_TEX1)  ? "upload-tex1, " : "",
	   (state & I810_UPLOAD_CTX)        ? "upload-ctx, " : "",
	   (state & I810_UPLOAD_BUFFERS)    ? "upload-bufs, " : ""
	   );
}



void i810InitState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   i810ScreenPrivate *i810Screen = imesa->i810Screen;

   memset(imesa->Setup, 0, sizeof(imesa->Setup));

   imesa->Setup[I810_CTXREG_VF] = 0;

   imesa->Setup[I810_CTXREG_MT] = (GFX_OP_MAP_TEXELS |
				   MT_UPDATE_TEXEL1_STATE |
				   MT_TEXEL1_COORD1 |
				   MT_TEXEL1_MAP1 |
				   MT_TEXEL1_DISABLE |
				   MT_UPDATE_TEXEL0_STATE |
				   MT_TEXEL0_COORD0 |
				   MT_TEXEL0_MAP0 |
				   MT_TEXEL0_DISABLE);

   imesa->Setup[I810_CTXREG_MC0] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_0 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     ((MC_ARG_ITERATED_COLOR |
				       MC_ARG_DONT_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG1_SHIFT) |
				     MC_UPDATE_ARG2 |
				     ((MC_ARG_ONE |
				       MC_ARG_DONT_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG2_SHIFT) |
				     MC_UPDATE_OP |
				     MC_OP_ARG1 );

   imesa->Setup[I810_CTXREG_MC1] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_1 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     ((MC_ARG_ONE |
				       MC_ARG_DONT_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG1_SHIFT) |
				     MC_UPDATE_ARG2 |
				     ((MC_ARG_ONE |
				       MC_ARG_DONT_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG2_SHIFT) |
				     MC_UPDATE_OP |
				     MC_OP_DISABLE );


   imesa->Setup[I810_CTXREG_MC2] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_2 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     ((MC_ARG_CURRENT_COLOR |
				       MC_ARG_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG1_SHIFT) |
				     MC_UPDATE_ARG2 |
				     ((MC_ARG_ONE |
				       MC_ARG_DONT_REPLICATE_ALPHA |
				       MC_ARG_DONT_INVERT) << MC_ARG2_SHIFT) |
				     MC_UPDATE_OP |
				     MC_OP_DISABLE );


   imesa->Setup[I810_CTXREG_MA0] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_0 |
				     MA_UPDATE_ARG1 |
				     ((MA_ARG_ITERATED_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG1_SHIFT) |
				     MA_UPDATE_ARG2 |
				     ((MA_ARG_CURRENT_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG2_SHIFT) |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_MA1] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_1 |
				     MA_UPDATE_ARG1 |
				     ((MA_ARG_CURRENT_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG1_SHIFT) |
				     MA_UPDATE_ARG2 |
				     ((MA_ARG_CURRENT_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG2_SHIFT) |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_MA2] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_2 |
				     MA_UPDATE_ARG1 |
				     ((MA_ARG_CURRENT_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG1_SHIFT) |
				     MA_UPDATE_ARG2 |
				     ((MA_ARG_CURRENT_ALPHA |
				       MA_ARG_DONT_INVERT) << MA_ARG2_SHIFT) |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_SDM] = ( GFX_OP_SRC_DEST_MONO |
				     SDM_UPDATE_MONO_ENABLE |
				     0 |
				     SDM_UPDATE_SRC_BLEND |
				     SDM_SRC_ONE |
				     SDM_UPDATE_DST_BLEND |
				     SDM_DST_ZERO );

   /* Use for colormask:
    */
   imesa->Setup[I810_CTXREG_CF0] = GFX_OP_COLOR_FACTOR;
   imesa->Setup[I810_CTXREG_CF1] = 0xffffffff;

   imesa->Setup[I810_CTXREG_ZA] = (GFX_OP_ZBIAS_ALPHAFUNC |
				   ZA_UPDATE_ALPHAFUNC |
				   ZA_ALPHA_ALWAYS |
				   ZA_UPDATE_ZBIAS |
				   0 |
				   ZA_UPDATE_ALPHAREF |
				   0x0);

   imesa->Setup[I810_CTXREG_FOG] = (GFX_OP_FOG_COLOR |
				    (0xffffff & ~FOG_RESERVED_MASK));

   /* Choose a pipe
    */
   imesa->Setup[I810_CTXREG_B1] = ( GFX_OP_BOOL_1 |
				    B1_UPDATE_SPEC_SETUP_ENABLE |
				    0 |
				    B1_UPDATE_ALPHA_SETUP_ENABLE |
				    B1_ALPHA_SETUP_ENABLE |
				    B1_UPDATE_CI_KEY_ENABLE |
				    0 |
				    B1_UPDATE_CHROMAKEY_ENABLE |
				    0 |
				    B1_UPDATE_Z_BIAS_ENABLE |
				    0 |
				    B1_UPDATE_SPEC_ENABLE |
				    0 |
				    B1_UPDATE_FOG_ENABLE |
				    0 |
				    B1_UPDATE_ALPHA_TEST_ENABLE |
				    0 |
				    B1_UPDATE_BLEND_ENABLE |
				    0 |
				    B1_UPDATE_Z_TEST_ENABLE |
				    0 );

   imesa->Setup[I810_CTXREG_B2] = ( GFX_OP_BOOL_2 |
				    B2_UPDATE_MAP_CACHE_ENABLE |
				    B2_MAP_CACHE_ENABLE |
				    B2_UPDATE_ALPHA_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_FOG_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_SPEC_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_RGB_DITHER_ENABLE |
				    B2_RGB_DITHER_ENABLE |
				    B2_UPDATE_FB_WRITE_ENABLE |
				    B2_FB_WRITE_ENABLE |
				    B2_UPDATE_ZB_WRITE_ENABLE |
				    B2_ZB_WRITE_ENABLE );

   imesa->Setup[I810_CTXREG_LCS] = ( GFX_OP_LINEWIDTH_CULL_SHADE_MODE |
				     LCS_UPDATE_ZMODE |
				     LCS_Z_LESS |
				     LCS_UPDATE_LINEWIDTH |
				     LCS_LINEWIDTH_1_0 |
				     LCS_UPDATE_ALPHA_INTERP |
				     LCS_ALPHA_INTERP |
				     LCS_UPDATE_FOG_INTERP |
				     0 |
				     LCS_UPDATE_SPEC_INTERP |
				     0 |
				     LCS_UPDATE_RGB_INTERP |
				     LCS_RGB_INTERP |
				     LCS_UPDATE_CULL_MODE |
				     LCS_CULL_DISABLE);

   imesa->LcsCullMode = LCS_CULL_CW;
   imesa->LcsLineWidth = LCS_LINEWIDTH_1_0;
   imesa->LcsPointSize = LCS_LINEWIDTH_1_0;

   imesa->Setup[I810_CTXREG_PV] = ( GFX_OP_PV_RULE |
				    PV_UPDATE_PIXRULE |
				    PV_PIXRULE_ENABLE |
				    PV_UPDATE_LINELIST |
				    PV_LINELIST_PV1 |
				    PV_UPDATE_TRIFAN |
				    PV_TRIFAN_PV2 |
				    PV_UPDATE_TRISTRIP |
				    PV_TRISTRIP_PV2 );


   imesa->Setup[I810_CTXREG_ST0] = GFX_OP_STIPPLE;
   imesa->Setup[I810_CTXREG_ST1] = 0;

   imesa->Setup[I810_CTXREG_AA] = ( GFX_OP_ANTIALIAS |
				    AA_UPDATE_EDGEFLAG |
				    0 |
				    AA_UPDATE_POLYWIDTH |
				    AA_POLYWIDTH_05 |
				    AA_UPDATE_LINEWIDTH |
				    AA_LINEWIDTH_05 |
				    AA_UPDATE_BB_EXPANSION |
				    0 |
				    AA_UPDATE_AA_ENABLE |
				    0 );

   memset(imesa->BufferSetup, 0, sizeof(imesa->BufferSetup));
   imesa->BufferSetup[I810_DESTREG_DI0] = CMD_OP_DESTBUFFER_INFO;

   if (imesa->glCtx->Visual.doubleBufferMode && imesa->sarea->pf_current_page == 0) {
      /* use back buffer by default */
      imesa->BufferSetup[I810_DESTREG_DI1] = (i810Screen->backOffset |
					      i810Screen->backPitchBits);
   } else {
      /* use front buffer by default */
      imesa->BufferSetup[I810_DESTREG_DI1] = (i810Screen->fbOffset |
					      i810Screen->backPitchBits);
   }

   imesa->BufferSetup[I810_DESTREG_DV0] = GFX_OP_DESTBUFFER_VARS;
   imesa->BufferSetup[I810_DESTREG_DV1] = (DV_HORG_BIAS_OGL |
					   DV_VORG_BIAS_OGL |
					   i810Screen->fbFormat);

   imesa->BufferSetup[I810_DESTREG_DR0] = GFX_OP_DRAWRECT_INFO;
   imesa->BufferSetup[I810_DESTREG_DR1] = DR1_RECT_CLIP_ENABLE;
}


static void i810InvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   I810_CONTEXT(ctx)->new_state |= new_state;
}


void i810InitStateFuncs(GLcontext *ctx)
{
   /* Callbacks for internal Mesa events.
    */
   ctx->Driver.UpdateState = i810InvalidateState;

   /* API callbacks
    */
   ctx->Driver.AlphaFunc = i810AlphaFunc;
   ctx->Driver.BlendEquationSeparate = i810BlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate = i810BlendFuncSeparate;
   ctx->Driver.ClearColor = i810ClearColor;
   ctx->Driver.ColorMask = i810ColorMask;
   ctx->Driver.CullFace = i810CullFaceFrontFace;
   ctx->Driver.DepthFunc = i810DepthFunc;
   ctx->Driver.DepthMask = i810DepthMask;
   ctx->Driver.Enable = i810Enable;
   ctx->Driver.Fogfv = i810Fogfv;
   ctx->Driver.FrontFace = i810CullFaceFrontFace;
   ctx->Driver.LineWidth = i810LineWidth;
   ctx->Driver.LogicOpcode = i810LogicOp;
   ctx->Driver.PolygonStipple = i810PolygonStipple;
   ctx->Driver.RenderMode = i810RenderMode;
   ctx->Driver.Scissor = i810Scissor;
   ctx->Driver.DrawBuffer = i810DrawBuffer;
   ctx->Driver.ReadBuffer = i810ReadBuffer;
   ctx->Driver.ShadeModel = i810ShadeModel;
   ctx->Driver.DepthRange = i810DepthRange;
   ctx->Driver.Viewport = i810Viewport;
   ctx->Driver.PointSize = i810PointSize;

   if (IS_I815(I810_CONTEXT(ctx))) {
      ctx->Driver.LightModelfv = i810LightModelfv_i815;
   } else {
      ctx->Driver.LightModelfv = i810LightModelfv;
   }
}
