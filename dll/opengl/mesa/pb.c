/* $Id: pb.c,v 1.14 1997/11/13 02:16:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: pb.c,v $
 * Revision 1.14  1997/11/13 02:16:48  brianp
 * added lambda array, initialized to zeros
 *
 * Revision 1.13  1997/07/24 01:24:11  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.12  1997/05/28 03:26:02  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.11  1997/05/09 22:40:19  brianp
 * added gl_alloc_pb()
 *
 * Revision 1.10  1997/05/03 00:51:30  brianp
 * new texturing function call: gl_texture_pixels()
 *
 * Revision 1.9  1997/05/01 02:10:33  brianp
 * removed manually unrolled loop to stop Purify uninitialized memory read
 *
 * Revision 1.8  1997/04/16 23:54:11  brianp
 * do per-pixel fog if texturing is enabled
 *
 * Revision 1.7  1997/02/09 19:53:43  brianp
 * now use TEXTURE_xD enable constants
 *
 * Revision 1.6  1997/02/09 18:43:14  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.5  1997/02/03 20:30:54  brianp
 * added a few DEFARRAY macros for BeOS
 *
 * Revision 1.4  1997/01/28 22:17:44  brianp
 * new RGBA mode logic op support
 *
 * Revision 1.3  1996/09/25 03:21:10  brianp
 * added NO_DRAW_BIT support
 *
 * Revision 1.2  1996/09/15 14:18:37  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * Pixel buffer:
 *
 * As fragments are produced (by point, line, and bitmap drawing) they
 * are accumlated in a buffer.  When the buffer is full or has to be
 * flushed (glEnd), we apply all enabled rasterization functions to the
 * pixels and write the results to the display buffer.  The goal is to
 * maximize the number of pixels processed inside loops and to minimize
 * the number of function calls.
 */



#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "alpha.h"
#include "alphabuf.h"
#include "blend.h"
#include "depth.h"
#include "fog.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "pb.h"
#include "scissor.h"
#include "stencil.h"
#include "texture.h"
#include "types.h"
#endif



/*
 * Allocate and initialize a new pixel buffer structure.
 */
struct pixel_buffer *gl_alloc_pb(void)
{
   struct pixel_buffer *pb;
   pb = (struct pixel_buffer *) calloc(sizeof(struct pixel_buffer), 1);
   if (pb) {
      int i;
      /* set non-zero fields */
      pb->primitive = GL_BITMAP;
      /* Set all lambda values to 0.0 since we don't do mipmapping for
       * points or lines and want to use the level 0 texture image.
       */
      for (i=0; i<PB_SIZE; i++) {
         pb->lambda[i] = 0.0;
      }
   }
   return pb;
}




/*
 * When the pixel buffer is full, or needs to be flushed, call this
 * function.  All the pixels in the pixel buffer will be subjected
 * to texturing, scissoring, stippling, alpha testing, stenciling,
 * depth testing, blending, and finally written to the frame buffer.
 */
void gl_flush_pb( GLcontext *ctx )
{
   struct pixel_buffer* PB = ctx->PB;

   DEFARRAY(GLubyte,mask,PB_SIZE);
   DEFARRAY(GLubyte, rsave, PB_SIZE);
   DEFARRAY(GLubyte, gsave, PB_SIZE);
   DEFARRAY(GLubyte, bsave, PB_SIZE);
   DEFARRAY(GLubyte, asave, PB_SIZE);

   if (PB->count==0)  goto CleanUp;

   /* initialize mask array and clip pixels simultaneously */
   {
      GLint xmin = ctx->Buffer->Xmin;
      GLint xmax = ctx->Buffer->Xmax;
      GLint ymin = ctx->Buffer->Ymin;
      GLint ymax = ctx->Buffer->Ymax;
      GLint *x = PB->x;
      GLint *y = PB->y;
      GLuint i, n = PB->count;
      for (i=0;i<n;i++) {
         mask[i] = (x[i]>=xmin) & (x[i]<=xmax) & (y[i]>=ymin) & (y[i]<=ymax);
      }
   }

   if (ctx->Visual->RGBAflag) {
      /* RGBA COLOR PIXELS */
      if (PB->mono && ctx->MutablePixels) {
	 /* Copy flat color to all pixels */
         MEMSET( PB->r, PB->color[0], PB->count );
         MEMSET( PB->g, PB->color[1], PB->count );
         MEMSET( PB->b, PB->color[2], PB->count );
         MEMSET( PB->a, PB->color[3], PB->count );
      }

      /* If each pixel can be of a different color... */
      if (ctx->MutablePixels || !PB->mono) {

	 if (ctx->Texture.Enabled) {
            /* TODO: need texture lambda valus */
	    gl_texture_pixels( ctx, PB->count, PB->s, PB->t, PB->u,
                               PB->lambda, PB->r, PB->g, PB->b, PB->a);
	 }

	 if (ctx->Fog.Enabled
             && (ctx->Hint.Fog==GL_NICEST || PB->primitive==GL_BITMAP
                 || ctx->Texture.Enabled)) {
	    gl_fog_color_pixels( ctx, PB->count, PB->z,
				 PB->r, PB->g, PB->b, PB->a );
	 }

         /* Scissoring already done above */

	 if (ctx->Color.AlphaEnabled) {
	    if (gl_alpha_test( ctx, PB->count, PB->a, mask )==0) {
	       goto CleanUp;
	    }
	 }

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /* make a copy of the colors */
            MEMCPY( rsave, PB->r, PB->count * sizeof(GLubyte) );
            MEMCPY( gsave, PB->r, PB->count * sizeof(GLubyte) );
            MEMCPY( bsave, PB->r, PB->count * sizeof(GLubyte) );
            MEMCPY( asave, PB->r, PB->count * sizeof(GLubyte) );
         }

         if (ctx->Color.SWLogicOpEnabled) {
            gl_logicop_rgba_pixels( ctx, PB->count, PB->x, PB->y,
                                    PB->r, PB->g, PB->b, PB->a, mask);
         }
         else if (ctx->Color.BlendEnabled) {
            gl_blend_pixels( ctx, PB->count, PB->x, PB->y,
                             PB->r, PB->g, PB->b, PB->a, mask);
         }

         if (ctx->Color.SWmasking) {
            gl_mask_color_pixels( ctx, PB->count, PB->x, PB->y,
                                  PB->r, PB->g, PB->b, PB->a, mask );
         }

         /* write pixels */
         (*ctx->Driver.WriteColorPixels)( ctx, PB->count, PB->x, PB->y,
                                          PB->r, PB->g, PB->b, PB->a, mask );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_alpha_pixels( ctx, PB->count, PB->x, PB->y, PB->a, mask );
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also draw to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            if (ctx->Color.SWLogicOpEnabled) {
               gl_logicop_rgba_pixels( ctx, PB->count, PB->x, PB->y,
                                       PB->r, PB->g, PB->b, PB->a, mask);
            }
            else if (ctx->Color.BlendEnabled) {
               gl_blend_pixels( ctx, PB->count, PB->x, PB->y,
                                rsave, gsave, bsave, asave, mask );
            }
            if (ctx->Color.SWmasking) {
               gl_mask_color_pixels( ctx, PB->count, PB->x, PB->y,
                                     rsave, gsave, bsave, asave, mask);
            }
            (*ctx->Driver.WriteColorPixels)( ctx, PB->count, PB->x, PB->y,
                                             rsave, gsave, bsave, asave, mask);
            if (ctx->RasterMask & ALPHABUF_BIT) {
               ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
               gl_write_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                      asave, mask );
               ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
            }
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
            /*** ALL DONE ***/
         }
      }
      else {
	 /* Same color for all pixels */

         /* Scissoring already done above */

	 if (ctx->Color.AlphaEnabled) {
	    if (gl_alpha_test( ctx, PB->count, PB->a, mask )==0) {
	       goto CleanUp;
	    }
	 }

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         /* write pixels */
         {
            GLubyte red, green, blue, alpha;
            red   = PB->color[0];
            green = PB->color[1];
            blue  = PB->color[2];
            alpha = PB->color[3];
	    (*ctx->Driver.Color)( ctx, red, green, blue, alpha );
         }
         (*ctx->Driver.WriteMonocolorPixels)( ctx, PB->count, PB->x, PB->y, mask );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_mono_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                        PB->color[3], mask );
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also render to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.WriteMonocolorPixels)( ctx, PB->count, PB->x, PB->y, mask );
            if (ctx->RasterMask & ALPHABUF_BIT) {
               ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
               gl_write_mono_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                           PB->color[3], mask );
               ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
            }
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
	 }
         /*** ALL DONE ***/
      }
   }
   else {
      /* COLOR INDEX PIXELS */

      /* If we may be writting pixels with different indexes... */
      if (PB->mono && ctx->MutablePixels) {
	 /* copy index to all pixels */
         GLuint n = PB->count, indx = PB->index;
         GLuint *pbindex = PB->i;
         do {
	    *pbindex++ = indx;
            n--;
	 } while (n);
      }

      if (ctx->MutablePixels || !PB->mono) {
	 /* Pixel color index may be modified */
         GLuint isave[PB_SIZE];

	 if (ctx->Fog.Enabled
             && (ctx->Hint.Fog==GL_NICEST || PB->primitive==GL_BITMAP)) {
	    gl_fog_index_pixels( ctx, PB->count, PB->z, PB->i );
	 }

         /* Scissoring already done above */

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /* make a copy of the indexes */
            MEMCPY( isave, PB->i, PB->count * sizeof(GLuint) );
         }

         if (ctx->Color.SWLogicOpEnabled) {
            gl_logicop_ci_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
         }

         if (ctx->Color.SWmasking) {
            gl_mask_index_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
         }

         /* write pixels */
         (*ctx->Driver.WriteIndexPixels)( ctx, PB->count, PB->x, PB->y,
                                          PB->i, mask );

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also write to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            MEMCPY( PB->i, isave, PB->count * sizeof(GLuint) );
            if (ctx->Color.SWLogicOpEnabled) {
               gl_logicop_ci_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
            }
            if (ctx->Color.SWmasking) {
               gl_mask_index_pixels( ctx, PB->count, PB->x, PB->y,
                                     PB->i, mask );
            }
            (*ctx->Driver.WriteIndexPixels)( ctx, PB->count, PB->x, PB->y,
                                             PB->i, mask );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         }

         /*** ALL DONE ***/
      }
      else {
	 /* Same color index for all pixels */

         /* Scissoring already done above */

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
         
         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         /* write pixels */
         (*ctx->Driver.Index)( ctx, PB->index );
         (*ctx->Driver.WriteMonoindexPixels)( ctx, PB->count, PB->x, PB->y, mask );

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also write to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.WriteMonoindexPixels)( ctx, PB->count, PB->x, PB->y, mask );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         }
         /*** ALL DONE ***/
      }
   }

CleanUp:
   PB->count = 0;
   UNDEFARRAY(mask);
   UNDEFARRAY(rsave);
   UNDEFARRAY(gsave);
   UNDEFARRAY(bsave);
   UNDEFARRAY(asave);
}


