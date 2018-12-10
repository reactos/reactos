/* $Id: depth.c,v 1.11 1997/07/24 01:24:45 brianp Exp $ */

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
 * $Log: depth.c,v $
 * Revision 1.11  1997/07/24 01:24:45  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.10  1997/05/28 03:24:22  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.9  1997/04/20 19:54:15  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.8  1997/02/27 19:58:52  brianp
 * don't try to clear depth buffer if there isn't one
 *
 * Revision 1.7  1997/01/31 23:33:08  brianp
 * replaced calloc with malloc in gl_alloc_depth_buffer()
 *
 * Revision 1.6  1996/11/04 01:42:07  brianp
 * multiply Viewport.Sz and .Tz by DEPTH_SCALE
 *
 * Revision 1.5  1996/10/09 03:07:25  brianp
 * replaced malloc with calloc in gl_alloc_depth_buffer()
 *
 * Revision 1.4  1996/09/27 01:24:58  brianp
 * added missing default cases to switches
 *
 * Revision 1.3  1996/09/19 00:54:05  brianp
 * added missing returns after some gl_error() calls
 *
 * Revision 1.2  1996/09/15 14:19:16  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * Depth buffer functions
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "depth.h"
#include "dlist.h"
#include "macros.h"
#include "types.h"
#endif



/**********************************************************************/
/*****                          API Functions                     *****/
/**********************************************************************/



void gl_ClearDepth( GLcontext* ctx, GLclampd depth )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearDepth" );
      return;
   }
   ctx->Depth.Clear = (GLfloat) CLAMP( depth, 0.0, 1.0 );
}



void gl_DepthFunc( GLcontext* ctx, GLenum func )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDepthFunc" );
      return;
   }

   switch (func) {
      case GL_NEVER:
      case GL_LESS:    /* (default) pass if incoming z < stored z */
      case GL_GEQUAL:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_NOTEQUAL:
      case GL_EQUAL:
      case GL_ALWAYS:
         ctx->Depth.Func = func;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDepth.Func" );
   }
}



void gl_DepthMask( GLcontext* ctx, GLboolean flag )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDepthMask" );
      return;
   }

   /*
    * GL_TRUE indicates depth buffer writing is enabled (default)
    * GL_FALSE indicates depth buffer writing is disabled
    */
   ctx->Depth.Mask = flag;
   ctx->NewState |= NEW_RASTER_OPS;
}



void gl_DepthRange( GLcontext* ctx, GLclampd nearval, GLclampd farval )
{
   /*
    * nearval - specifies mapping of the near clipping plane to window
    *   coordinates, default is 0
    * farval - specifies mapping of the far clipping plane to window
    *   coordinates, default is 1
    *
    * After clipping and div by w, z coords are in -1.0 to 1.0,
    * corresponding to near and far clipping planes.  glDepthRange
    * specifies a linear mapping of the normalized z coords in
    * this range to window z coords.
    */

   GLfloat n, f;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDepthRange" );
      return;
   }

   n = (GLfloat) CLAMP( nearval, 0.0, 1.0 );
   f = (GLfloat) CLAMP( farval, 0.0, 1.0 );

   ctx->Viewport.Near = n;
   ctx->Viewport.Far = f;
   ctx->Viewport.Sz = DEPTH_SCALE * ((f - n) / 2.0);
   ctx->Viewport.Tz = DEPTH_SCALE * ((f - n) / 2.0 + n);
}



/**********************************************************************/
/*****                   Depth Testing Functions                  *****/
/**********************************************************************/


/*
 * Depth test horizontal spans of fragments.  These functions are called
 * via ctx->Driver.depth_test_span only.
 *
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span in window coords
 *         z - array [n] of integer depth values
 * In/Out:  mask - array [n] of flags (1=draw pixel, 0=don't draw) 
 * Return:  number of pixels which passed depth test
 */


/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
GLuint gl_depth_test_span_generic( GLcontext* ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
                                   GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLubyte *m = mask;
   GLuint i;
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  *zptr = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
         gl_problem(ctx, "Bad depth func in gl_depth_test_span_generic");
   } /*switch*/

   return passed;
}



/*
 * glDepthFunc(GL_LESS) and glDepthMask(GL_TRUE).
 */
GLuint gl_depth_test_span_less( GLcontext* ctx,
                                GLuint n, GLint x, GLint y, const GLdepth z[],
                                GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLuint i;
   GLuint passed = 0;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         if (z[i] < zptr[i]) {
            /* pass */
            zptr[i] = z[i];
            passed++;
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
   return passed;
}


/*
 * glDepthFunc(GL_GREATER) and glDepthMask(GL_TRUE).
 */
GLuint gl_depth_test_span_greater( GLcontext* ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
                                   GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLuint i;
   GLuint passed = 0;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         if (z[i] > zptr[i]) {
            /* pass */
            zptr[i] = z[i];
            passed++;
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
   return passed;
}



/*
 * Depth test an array of randomly positioned fragments.
 */


#define ZADDR_SETUP   GLdepth *depthbuffer = ctx->Buffer->Depth; \
                      GLint width = ctx->Buffer->Width;

#define ZADDR( X, Y )   (depthbuffer + (Y) * width + (X) )



/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
void gl_depth_test_pixels_generic( GLcontext* ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLdepth z[], GLubyte mask[] )
{
   register GLdepth *zptr;
   register GLuint i;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
         gl_problem(ctx, "Bad depth func in gl_depth_test_pixels_generic");
   } /*switch*/
}



/*
 * glDepthFunc( GL_LESS ) and glDepthMask( GL_TRUE ).
 */
void gl_depth_test_pixels_less( GLcontext* ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLdepth z[], GLubyte mask[] )
{
   GLdepth *zptr;
   GLuint i;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         zptr = Z_ADDRESS(ctx,x[i],y[i]);
         if (z[i] < *zptr) {
            /* pass */
            *zptr = z[i];
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
}


/*
 * glDepthFunc( GL_GREATER ) and glDepthMask( GL_TRUE ).
 */
void gl_depth_test_pixels_greater( GLcontext* ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLdepth z[], GLubyte mask[] )
{
   GLdepth *zptr;
   GLuint i;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         zptr = Z_ADDRESS(ctx,x[i],y[i]);
         if (z[i] > *zptr) {
            /* pass */
            *zptr = z[i];
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
}




/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/*
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This function is only called through Driver.read_depth_span_float()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void gl_read_depth_span_float( GLcontext* ctx,
                               GLuint n, GLint x, GLint y, GLfloat depth[] )
{
   GLdepth *zptr;
   GLfloat scale;
   GLuint i;

   scale = 1.0F / DEPTH_SCALE;

   if (ctx->Buffer->Depth) {
      zptr = Z_ADDRESS( ctx, x, y );
      for (i=0;i<n;i++) {
	 depth[i] = (GLfloat) zptr[i] * scale;
      }
   }
   else {
      for (i=0;i<n;i++) {
	 depth[i] = 0.0F;
      }
   }
}


/*
 * Return a span of depth values from the depth buffer as integers in
 * [0,MAX_DEPTH].
 * This function is only called through Driver.read_depth_span_int()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void gl_read_depth_span_int( GLcontext* ctx,
                             GLuint n, GLint x, GLint y, GLdepth depth[] )
{
   if (ctx->Buffer->Depth) {
      GLdepth *zptr = Z_ADDRESS( ctx, x, y );
      MEMCPY( depth, zptr, n * sizeof(GLdepth) );
   }
   else {
      GLuint i;
      for (i=0;i<n;i++) {
	 depth[i] = 0.0;
      }
   }
}



/**********************************************************************/
/*****                Allocate and Clear Depth Buffer             *****/
/**********************************************************************/



/*
 * Allocate a new depth buffer.  If there's already a depth buffer allocated
 * it will be free()'d.  The new depth buffer will be uniniitalized.
 * This function is only called through Driver.alloc_depth_buffer.
 */
void gl_alloc_depth_buffer( GLcontext* ctx )
{
   /* deallocate current depth buffer if present */
   if (ctx->Buffer->Depth) {
      free(ctx->Buffer->Depth);
      ctx->Buffer->Depth = NULL;
   }

   /* allocate new depth buffer, but don't initialize it */
   ctx->Buffer->Depth = (GLdepth *) malloc( ctx->Buffer->Width
                                            * ctx->Buffer->Height
                                            * sizeof(GLdepth) );
   if (!ctx->Buffer->Depth) {
      /* out of memory */
      ctx->Depth.Test = GL_FALSE;
      gl_error( ctx, GL_OUT_OF_MEMORY, "Couldn't allocate depth buffer" );
   }
}




/*
 * Clear the depth buffer.  If the depth buffer doesn't exist yet we'll
 * allocate it now.
 * This function is only called through Driver.clear_depth_buffer.
 */
void gl_clear_depth_buffer( GLcontext* ctx )
{
   GLdepth clear_value = (GLdepth) (ctx->Depth.Clear * DEPTH_SCALE);

   if (ctx->Visual->DepthBits==0 || !ctx->Buffer->Depth) {
      /* no depth buffer */
      return;
   }

   /* The loops in this function have been written so the IRIX 5.3
    * C compiler can unroll them.  Hopefully other compilers can too!
    */

   if (ctx->Scissor.Enabled) {
      /* only clear scissor region */
      GLint y;
      for (y=ctx->Buffer->Ymin; y<=ctx->Buffer->Ymax; y++) {
         GLdepth *d = Z_ADDRESS( ctx, ctx->Buffer->Xmin, y );
         GLint n = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
         do {
            *d++ = clear_value;
            n--;
         } while (n);
      }
   }
   else {
      /* clear whole buffer */
      GLdepth *d = ctx->Buffer->Depth;
      GLint n = ctx->Buffer->Width * ctx->Buffer->Height;
      while (n>=16) {
         d[0] = clear_value;    d[1] = clear_value;
         d[2] = clear_value;    d[3] = clear_value;
         d[4] = clear_value;    d[5] = clear_value;
         d[6] = clear_value;    d[7] = clear_value;
         d[8] = clear_value;    d[9] = clear_value;
         d[10] = clear_value;   d[11] = clear_value;
         d[12] = clear_value;   d[13] = clear_value;
         d[14] = clear_value;   d[15] = clear_value;
         d += 16;
         n -= 16;
      }
      while (n>0) {
         *d++ = clear_value;
         n--;
      }
   }
}



