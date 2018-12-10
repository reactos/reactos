/* $Id: blend.c,v 1.10 1998/01/27 03:42:40 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: blend.c,v $
 * Revision 1.10  1998/01/27 03:42:40  brianp
 * optimized more blending modes (Kai Schuetz)
 *
 * Revision 1.9  1997/07/24 01:24:45  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.8  1997/05/28 03:23:48  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.7  1997/04/20 19:51:57  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.6  1997/02/15 18:27:56  brianp
 * fixed a few error messages
 *
 * Revision 1.5  1997/01/28 22:17:19  brianp
 * moved logic op blending into logic.c
 *
 * Revision 1.4  1997/01/04 00:13:11  brianp
 * was using ! instead of ~ to invert pixel bits (ugh!)
 *
 * Revision 1.3  1996/09/19 00:53:31  brianp
 * added missing returns after some gl_error() calls
 *
 * Revision 1.2  1996/09/15 14:18:10  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdlib.h>
#include "alphabuf.h"
#include "blend.h"
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "pb.h"
#include "span.h"
#include "types.h"
#endif



void gl_BlendFunc( GLcontext* ctx, GLenum sfactor, GLenum dfactor )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBlendFunc" );
      return;
   }

   switch (sfactor) {
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
         ctx->Color.BlendSrc = sfactor;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(sfactor)" );
         return;
   }

   switch (dfactor) {
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
         ctx->Color.BlendDst = dfactor;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(dfactor)" );
   }

   ctx->NewState |= NEW_RASTER_OPS;
}



/*
 * Do the real work of gl_blend_span() and gl_blend_pixels().
 * Input:  n - number of pixels
 *         mask - the usual write mask
 * In/Out:  red, green, blue, alpha - the incoming and modified pixels
 * Input:  rdest, gdest, bdest, adest - the pixels from the dest color buffer
 */
static void do_blend( GLcontext* ctx, GLuint n, const GLubyte mask[],
                      GLubyte red[], GLubyte green[],
                      GLubyte blue[], GLubyte alpha[],
                      const GLubyte rdest[], const GLubyte gdest[],
                      const GLubyte bdest[], const GLubyte adest[] )
{
   GLuint i;

   if (ctx->Color.BlendSrc==GL_SRC_ALPHA
       && ctx->Color.BlendDst==GL_ONE_MINUS_SRC_ALPHA) {
      /* Alpha blending */
      GLfloat ascale = 256.0f * ctx->Visual->InvAlphaScale;
      GLint rmax = (GLint) ctx->Visual->RedScale;
      GLint gmax = (GLint) ctx->Visual->GreenScale;
      GLint bmax = (GLint) ctx->Visual->BlueScale;
      GLint amax = (GLint) ctx->Visual->AlphaScale;
      for (i=0;i<n;i++) {
	 if (mask[i]) {
	    GLint r, g, b, a;
            GLint t = (GLint) ( alpha[i] * ascale );  /* t in [0,256] */
            GLint s = 256 - t;
	    r = (red[i]   * t + rdest[i] * s) >> 8;
	    g = (green[i] * t + gdest[i] * s) >> 8;
	    b = (blue[i]  * t + bdest[i] * s) >> 8;
	    a = (alpha[i] * t + adest[i] * s) >> 8;

	    /* kai: I think the following clamping is not needed: */

	    red[i]   = MIN2( r, rmax );
	    green[i] = MIN2( g, gmax );
	    blue[i]  = MIN2( b, bmax );
	    alpha[i] = MIN2( a, amax );
	 }
      }
   }
   else {

      /* clipped sum */
      if (ctx->Color.BlendSrc==GL_ONE
	  && ctx->Color.BlendDst==GL_ONE) {
	 GLint rmax = (GLint) ctx->Visual->RedScale;
	 GLint gmax = (GLint) ctx->Visual->GreenScale;
	 GLint bmax = (GLint) ctx->Visual->BlueScale;
	 GLint amax = (GLint) ctx->Visual->AlphaScale;
	 for (i=0; i < n; i++) {
	    if (mask[i]) {
	       red[i]	= MIN2(rmax, red[i]   + rdest[i]);
	       green[i] = MIN2(gmax, green[i] + gdest[i]);
	       blue[i]	= MIN2(bmax, blue[i]  + bdest[i]);
	       alpha[i] = MIN2(amax, alpha[i] + adest[i]);
	    }
	 }
      }

      /* modulation */
      else if ((ctx->Color.BlendSrc==GL_ZERO &&
		    ctx->Color.BlendDst==GL_SRC_COLOR)
	       ||
	       (ctx->Color.BlendSrc==GL_DST_COLOR &&
		    ctx->Color.BlendDst==GL_ZERO)) {
	 if (ctx->Visual->EightBitColor) {
	    for (i=0; i < n; i++) {
	       if (mask[i]) {
		  red[i]   = (red[i]   * rdest[i]) / 255;
		  green[i] = (green[i] * gdest[i]) / 255;
		  blue[i]  = (blue[i]  * bdest[i]) / 255;
		  alpha[i] = (alpha[i] * adest[i]) / 255;
	       }
	    }
	 }
         else {
	    GLint rmax = (GLint) ctx->Visual->RedScale;
	    GLint gmax = (GLint) ctx->Visual->GreenScale;
	    GLint bmax = (GLint) ctx->Visual->BlueScale;
	    GLint amax = (GLint) ctx->Visual->AlphaScale;
	    for (i=0; i < n; i++) {
	       if (mask[i]) {
		  red[i]   = (red[i]   * rdest[i]) / rmax;
		  green[i] = (green[i] * gdest[i]) / gmax;
		  blue[i]  = (blue[i]  * bdest[i]) / bmax;
		  alpha[i] = (alpha[i] * adest[i]) / amax;
	       }
	    }
	 }
      }else{

	 /* General cases: */

	 if (ctx->Visual->EightBitColor) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLint Rs, Gs, Bs, As;	 /* Source colors */
		  GLint Rd, Gd, Bd, Ad;	 /* Dest colors */
		  GLint Rss, Gss, Bss, Ass;  /* Source colors scaled */
		  GLint Rds, Gds, Bds, Ads;  /* Dest colors scaled */

		  /* Source Color */
		  Rs = red[i];
		  Gs = green[i];
		  Bs = blue[i];
		  As = alpha[i];

		  /* Frame buffer color */
		  Rd = rdest[i];
		  Gd = gdest[i];
		  Bd = bdest[i];
		  Ad = adest[i];

		  /* Source scaling */
		  switch (ctx->Color.BlendSrc) {
		     case GL_ZERO:
			Rss = Gss = Bss = Ass = 0;
			break;
		     case GL_ONE:
			Rss = Rs * 255;
			Gss = Gs * 255;
			Bss = Bs * 255;
			Ass = As * 255;
			break;
		     case GL_DST_COLOR:
			Rss = Rs * Rd;
			Gss = Gs * Gd;
			Bss = Bs * Bd;
			Ass = As * Ad;
			break;
		     case GL_ONE_MINUS_DST_COLOR:
			Rss = Rs * (255 - Rd);
			Gss = Gs * (255 - Gd);
			Bss = Bs * (255 - Bd);
			Ass = As * (255 - Ad);
			break;
		     case GL_SRC_ALPHA:
			Rss = Rs * As;
			Gss = Gs * As;
			Bss = Bs * As;
			Ass = As * As;
			break;
		     case GL_ONE_MINUS_SRC_ALPHA:
			Rss = Rs * (255 - As);
			Gss = Gs * (255 - As);
			Bss = Bs * (255 - As);
			Ass = As * (255 - As);
			break;
		     case GL_DST_ALPHA:
			Rss = Rs * Ad;
			Gss = Gs * Ad;
			Bss = Bs * Ad;
			Ass = As * Ad;
			break;
		     case GL_ONE_MINUS_DST_ALPHA:
			Rss = Rs * (255 - Ad);
			Gss = Gs * (255 - Ad);
			Bss = Bs * (255 - Ad);
			Ass = As * (255 - Ad);
			break;
		     case GL_SRC_ALPHA_SATURATE:
		     {
                GLint sA = MIN2(As, 255 - Ad);
                Rss = Rs * sA;
                Gss = Gs * sA;
                Bss = Bs * sA;
                Ass = As * 255;
                break;
		     }
		     default:
			/* this should never happen */
			gl_problem(ctx, "Bad blend source factor in do_blend");
		  }

		  /* Dest scaling */
		  switch (ctx->Color.BlendDst) {
		     case GL_ZERO:
			Rds = Gds = Bds = Ads = 0;
			break;
		     case GL_ONE:
			Rds = Rd * 255;
			Gds = Gd * 255;
			Bds = Bd * 255;
			Ads = Ad * 255;
			break;
		     case GL_SRC_COLOR:
			Rds = Rd * Rs;
			Gds = Gd * Gs;
			Bds = Bd * Bs;
			Ads = Ad * As;
			break;
		     case GL_ONE_MINUS_SRC_COLOR:
			Rds = Rs * (255 - Rs);
			Gds = Gs * (255 - Gs);
			Bds = Bs * (255 - Bs);
			Ads = As * (255 - As);
			break;
		     case GL_SRC_ALPHA:
			Rds = Rd * As;
			Gds = Gd * As;
			Bds = Bd * As;
			Ads = Ad * As;
			break;
		     case GL_ONE_MINUS_SRC_ALPHA:
			Rds = Rd * (255 - As);
			Gds = Gd * (255 - As);
			Bds = Bd * (255 - As);
			Ads = Ad * (255 - As);
			break;
		     case GL_DST_ALPHA:
			Rds = Rd * Ad;
			Gds = Gd * Ad;
			Bds = Bd * Ad;
			Ads = Ad * Ad;
			break;
		     case GL_ONE_MINUS_DST_ALPHA:
			Rds = Rd * (255 - Ad);
			Gds = Gd * (255 - Ad);
			Bds = Bd * (255 - Ad);
			Ads = Ad * (255 - Ad);
			break;
		     default:
			/* this should never happen */
			gl_problem(ctx, "Bad blend dest factor in do_blend");
		  }

		  /* compute blended color */
			red[i]	 = MIN2((Rss + Rds) / 255, 255);
			green[i] = MIN2((Gss + Gds) / 255, 255);
			blue[i]	 = MIN2((Bss + Bds) / 255, 255);
			alpha[i] = MIN2((Ass + Ads) / 255, 255);
	       }
	    }
	 }else{			/* !EightBitColor */
	    GLfloat rmax = ctx->Visual->RedScale;
	    GLfloat gmax = ctx->Visual->GreenScale;
	    GLfloat bmax = ctx->Visual->BlueScale;
	    GLfloat amax = ctx->Visual->AlphaScale;
	    GLfloat rscale = 1.0f / rmax;
	    GLfloat gscale = 1.0f / gmax;
	    GLfloat bscale = 1.0f / bmax;
	    GLfloat ascale = 1.0f / amax;

	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLint Rs, Gs, Bs, As;	 /* Source colors */
		  GLint Rd, Gd, Bd, Ad;	 /* Dest colors */
		  GLfloat sR, sG, sB, sA;  /* Source scaling */
		  GLfloat dR, dG, dB, dA;  /* Dest scaling */
		  GLfloat r, g, b, a;

		  /* Source Color */
		  Rs = red[i];
		  Gs = green[i];
		  Bs = blue[i];
		  As = alpha[i];

		  /* Frame buffer color */
		  Rd = rdest[i];
		  Gd = gdest[i];
		  Bd = bdest[i];
		  Ad = adest[i];

		  /* Source scaling */
		  switch (ctx->Color.BlendSrc) {
		     case GL_ZERO:
			sR = sG = sB = sA = 0.0F;
			break;
		     case GL_ONE:
			sR = sG = sB = sA = 1.0F;
			break;
		     case GL_DST_COLOR:
			sR = (GLfloat) Rd * rscale;
			sG = (GLfloat) Gd * gscale;
			sB = (GLfloat) Bd * bscale;
			sA = (GLfloat) Ad * ascale;
			break;
		     case GL_ONE_MINUS_DST_COLOR:
			sR = 1.0F - (GLfloat) Rd * rscale;
			sG = 1.0F - (GLfloat) Gd * gscale;
			sB = 1.0F - (GLfloat) Bd * bscale;
			sA = 1.0F - (GLfloat) Ad * ascale;
			break;
		     case GL_SRC_ALPHA:
			sR = sG = sB = sA = (GLfloat) As * ascale;
			break;
		     case GL_ONE_MINUS_SRC_ALPHA:
			sR = sG = sB = sA = (GLfloat) 1.0F - (GLfloat) As * ascale;
			break;
		     case GL_DST_ALPHA:
			sR = sG = sB = sA =(GLfloat) Ad * ascale;
			break;
		     case GL_ONE_MINUS_DST_ALPHA:
			sR = sG = sB = sA = 1.0F - (GLfloat) Ad * ascale;
			break;
		     case GL_SRC_ALPHA_SATURATE:
			if (As < 1.0F - (GLfloat) Ad * ascale) {
			   sR = sG = sB = (GLfloat) As * ascale;
			}
			else {
			   sR = sG = sB = 1.0F - (GLfloat) Ad * ascale;
			}
			sA = 1.0;
			break;
		     default:
			/* this should never happen */
			gl_problem(ctx, "Bad blend source factor in do_blend");
		  }

		  /* Dest scaling */
		  switch (ctx->Color.BlendDst) {
		     case GL_ZERO:
			dR = dG = dB = dA = 0.0F;
			break;
		     case GL_ONE:
			dR = dG = dB = dA = 1.0F;
			break;
		     case GL_SRC_COLOR:
			dR = (GLfloat) Rs * rscale;
			dG = (GLfloat) Gs * gscale;
			dB = (GLfloat) Bs * bscale;
			dA = (GLfloat) As * ascale;
			break;
		     case GL_ONE_MINUS_SRC_COLOR:
			dR = 1.0F - (GLfloat) Rs * rscale;
			dG = 1.0F - (GLfloat) Gs * gscale;
			dB = 1.0F - (GLfloat) Bs * bscale;
			dA = 1.0F - (GLfloat) As * ascale;
			break;
		     case GL_SRC_ALPHA:
			dR = dG = dB = dA = (GLfloat) As * ascale;
			break;
		     case GL_ONE_MINUS_SRC_ALPHA:
			dR = dG = dB = dA = (GLfloat) 1.0F - (GLfloat) As * ascale;
			break;
		     case GL_DST_ALPHA:
			dR = dG = dB = dA = (GLfloat) Ad * ascale;
			break;
		     case GL_ONE_MINUS_DST_ALPHA:
			dR = dG = dB = dA = 1.0F - (GLfloat) Ad * ascale;
			break;
		     default:
			/* this should never happen */
			gl_problem(ctx, "Bad blend dest factor in do_blend");
		  }

#ifdef DEBUG
		  assert( sR>= 0.0 && sR<=1.0 );
		  assert( sG>= 0.0 && sG<=1.0 );
		  assert( sB>= 0.0 && sB<=1.0 );
		  assert( sA>= 0.0 && sA<=1.0 );
		  assert( dR>= 0.0 && dR<=1.0 );
		  assert( dG>= 0.0 && dG<=1.0 );
		  assert( dB>= 0.0 && dB<=1.0 );
		  assert( dA>= 0.0 && dA<=1.0 );
#endif

		  /* compute blended color */
		  r = Rs * sR + Rd * dR;
		  g = Gs * sG + Gd * dG;
		  b = Bs * sB + Bd * dB;
		  a = As * sA + Ad * dA;
		  red[i]   = (GLint) CLAMP( r, 0.0F, rmax );
		  green[i] = (GLint) CLAMP( g, 0.0F, gmax );
		  blue[i]  = (GLint) CLAMP( b, 0.0F, bmax );
		  alpha[i] = (GLint) CLAMP( a, 0.0F, amax );
	       }
	    }
	 }
      }
   }

}





/*
 * Apply the blending operator to a span of pixels.
 * Input:  n - number of pixels in span
 *         x, y - location of leftmost pixel in span in window coords.
 *         mask - boolean mask indicating which pixels to blend.
 * In/Out:  red, green, blue, alpha - pixel values
 */
void gl_blend_span( GLcontext* ctx, GLuint n, GLint x, GLint y,
		    GLubyte red[], GLubyte green[],
		    GLubyte blue[], GLubyte alpha[],
		    GLubyte mask[] )
{
   GLubyte rdest[MAX_WIDTH], gdest[MAX_WIDTH];
   GLubyte bdest[MAX_WIDTH], adest[MAX_WIDTH];

   /* Read span of current frame buffer pixels */
   gl_read_color_span( ctx, n, x, y, rdest, gdest, bdest, adest );

   do_blend( ctx, n, mask, red, green, blue, alpha, rdest, gdest, bdest, adest );
}





/*
 * Apply the blending operator to an array of pixels.
 * Input:  n - number of pixels in span
 *         x, y - array of pixel locations
 *         mask - boolean mask indicating which pixels to blend.
 * In/Out:  red, green, blue, alpha - array of pixel values
 */
void gl_blend_pixels( GLcontext* ctx,
                      GLuint n, const GLint x[], const GLint y[],
		      GLubyte red[], GLubyte green[],
		      GLubyte blue[], GLubyte alpha[],
		      GLubyte mask[] )
{
   GLubyte rdest[PB_SIZE], gdest[PB_SIZE], bdest[PB_SIZE], adest[PB_SIZE];

   /* Read pixels from current color buffer */
   (*ctx->Driver.ReadColorPixels)( ctx, n, x, y, rdest, gdest, bdest, adest, mask );
   if (ctx->RasterMask & ALPHABUF_BIT) {
      gl_read_alpha_pixels( ctx, n, x, y, adest, mask );
   }

   do_blend( ctx, n, mask, red, green, blue, alpha, rdest, gdest, bdest, adest );
}
