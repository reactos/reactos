/* $Id: accum.c,v 1.5 1997/07/24 01:24:28 brianp Exp $ */

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
 * $Log: accum.c,v $
 * Revision 1.5  1997/07/24 01:24:28  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:23:09  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1997/04/30 01:54:48  brianp
 * call gl_warning() if calling gl_Accum w/out accum buffer
 *
 * Revision 1.2  1996/09/15 14:19:44  brianp
 * now use GLframebuffer and GLvisual
 * added gl_alloc_accum_buffer()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "accum.h"
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "types.h"
#endif


void gl_alloc_accum_buffer( GLcontext *ctx )
{
   GLint n;

   if (ctx->Buffer->Accum) {
      free( ctx->Buffer->Accum );
      ctx->Buffer->Accum = NULL;
   }

   /* allocate accumulation buffer if not already present */
   n = ctx->Buffer->Width * ctx->Buffer->Height * 4 * sizeof(GLaccum);
   ctx->Buffer->Accum = (GLaccum *) malloc( n );
   if (!ctx->Buffer->Accum) {
      /* unable to setup accumulation buffer */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glAccum" );
   }
}



void gl_ClearAccum( GLcontext *ctx,
                    GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAccum" );
      return;
   }
   ctx->Accum.ClearColor[0] = CLAMP( red, -1.0, 1.0 );
   ctx->Accum.ClearColor[1] = CLAMP( green, -1.0, 1.0 );
   ctx->Accum.ClearColor[2] = CLAMP( blue, -1.0, 1.0 );
   ctx->Accum.ClearColor[3] = CLAMP( alpha, -1.0, 1.0 );
}




void gl_Accum( GLcontext *ctx, GLenum op, GLfloat value )
{
   GLuint xpos, ypos, width, height;
   GLfloat acc_scale;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAccum" );
      return;
   }

   if (ctx->Visual->AccumBits==0 || !ctx->Buffer->Accum) {
      /* No accumulation buffer! */
      gl_warning(ctx, "Calling glAccum() without an accumulation buffer");
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   /* Determine region to operate upon. */
   if (ctx->Scissor.Enabled) {
      xpos = ctx->Scissor.X;
      ypos = ctx->Scissor.Y;
      width = ctx->Scissor.Width;
      height = ctx->Scissor.Height;
   }
   else {
      /* whole window */
      xpos = 0;
      ypos = 0;
      width = ctx->Buffer->Width;
      height = ctx->Buffer->Height;
   }

   switch (op) {
      case GL_ADD:
         {
	    GLaccum ival, *acc;
	    GLuint i, j;

	    ival = (GLaccum) (value * acc_scale);
	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc += ival;	  acc++;   /* red */
		  *acc += ival;	  acc++;   /* green */
		  *acc += ival;	  acc++;   /* blue */
		  *acc += ival;	  acc++;   /* alpha */
	       }
	       ypos++;
	    }
	 }
	 break;
      case GL_MULT:
	 {
	    GLaccum *acc;
	    GLuint i, j;

	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*r*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*g*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*g*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*a*/
	       }
	       ypos++;
	    }
	 }
	 break;
      case GL_ACCUM:
	 {
	    GLaccum *acc;
	    GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
	    GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
	    GLfloat rscale, gscale, bscale, ascale;
	    GLuint i, j;

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );

	    /* Accumulate */
	    rscale = value * acc_scale * ctx->Visual->InvRedScale;
	    gscale = value * acc_scale * ctx->Visual->InvGreenScale;
	    bscale = value * acc_scale * ctx->Visual->InvBlueScale;
	    ascale = value * acc_scale * ctx->Visual->InvAlphaScale;
	    for (j=0;j<height;j++) {
	       (*ctx->Driver.ReadColorSpan)( ctx, width, xpos, ypos,
                                             red, green, blue, alpha);
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc += (GLaccum) ( (GLfloat) red[i]   * rscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) green[i] * gscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) blue[i]  * bscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) alpha[i] * ascale );  acc++;
	       }
	       ypos++;
	    }

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
	 }
	 break;
      case GL_LOAD:
	 {
	    GLaccum *acc;
	    GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
	    GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
	    GLfloat rscale, gscale, bscale, ascale;
	    GLuint i, j;

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );

	    /* Load accumulation buffer */
	    rscale = value * acc_scale * ctx->Visual->InvRedScale;
	    gscale = value * acc_scale * ctx->Visual->InvGreenScale;
	    bscale = value * acc_scale * ctx->Visual->InvBlueScale;
	    ascale = value * acc_scale * ctx->Visual->InvAlphaScale;
	    for (j=0;j<height;j++) {
	       (*ctx->Driver.ReadColorSpan)( ctx, width, xpos, ypos,
                                             red, green, blue, alpha);
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc++ = (GLaccum) ( (GLfloat) red[i]   * rscale );
		  *acc++ = (GLaccum) ( (GLfloat) green[i] * gscale );
		  *acc++ = (GLaccum) ( (GLfloat) blue[i]  * bscale );
		  *acc++ = (GLaccum) ( (GLfloat) alpha[i] * ascale );
	       }
	       ypos++;
	    }

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
	 }
	 break;
      case GL_RETURN:
	 {
	    GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
	    GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
	    GLaccum *acc;
	    GLfloat rscale, gscale, bscale, ascale;
	    GLint rmax, gmax, bmax, amax;
	    GLuint i, j;

	    rscale = value / acc_scale * ctx->Visual->RedScale;
	    gscale = value / acc_scale * ctx->Visual->GreenScale;
	    bscale = value / acc_scale * ctx->Visual->BlueScale;
	    ascale = value / acc_scale * ctx->Visual->AlphaScale;
	    rmax = (GLint) ctx->Visual->RedScale;
	    gmax = (GLint) ctx->Visual->GreenScale;
	    bmax = (GLint) ctx->Visual->BlueScale;
	    amax = (GLint) ctx->Visual->AlphaScale;
	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  GLint r, g, b, a;
		  r = (GLint) ( (GLfloat) (*acc++) * rscale + 0.5F );
		  g = (GLint) ( (GLfloat) (*acc++) * gscale + 0.5F );
		  b = (GLint) ( (GLfloat) (*acc++) * bscale + 0.5F );
		  a = (GLint) ( (GLfloat) (*acc++) * ascale + 0.5F );
		  red[i]   = CLAMP( r, 0, rmax );
		  green[i] = CLAMP( g, 0, gmax );
		  blue[i]  = CLAMP( b, 0, bmax );
		  alpha[i] = CLAMP( a, 0, amax );
	       }
	       (*ctx->Driver.WriteColorSpan)( ctx, width, xpos, ypos,
                                              red, green, blue, alpha, NULL );
	       ypos++;
	    }
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glAccum" );
   }
}




/*
 * Clear the accumulation Buffer->
 */
void gl_clear_accum_buffer( GLcontext *ctx )
{
   GLuint buffersize;
   GLfloat acc_scale;

   if (ctx->Visual->AccumBits==0) {
      /* No accumulation buffer! */
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   /* number of pixels */
   buffersize = ctx->Buffer->Width * ctx->Buffer->Height;

   if (!ctx->Buffer->Accum) {
      /* try to alloc accumulation buffer */
      ctx->Buffer->Accum = (GLaccum *)
	                   malloc( buffersize * 4 * sizeof(GLaccum) );
   }

   if (ctx->Buffer->Accum) {
      if (ctx->Scissor.Enabled) {
	 /* Limit clear to scissor box */
	 GLaccum r, g, b, a;
	 GLint i, j;
         GLint width, height;
         GLaccum *row;
	 r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	 g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	 b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	 a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
         /* size of region to clear */
         width = 4 * (ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1);
         height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
         /* ptr to first element to clear */
         row = ctx->Buffer->Accum
               + 4 * (ctx->Buffer->Ymin * ctx->Buffer->Width
                      + ctx->Buffer->Xmin);
         for (j=0;j<height;j++) {
            for (i=0;i<width;i+=4) {
               row[i+0] = r;
               row[i+1] = g;
               row[i+2] = b;
               row[i+3] = a;
	    }
            row += 4 * ctx->Buffer->Width;
	 }
      }
      else {
	 /* clear whole buffer */
	 if (ctx->Accum.ClearColor[0]==0.0 &&
	     ctx->Accum.ClearColor[1]==0.0 &&
	     ctx->Accum.ClearColor[2]==0.0 &&
	     ctx->Accum.ClearColor[3]==0.0) {
	    /* Black */
	    MEMSET( ctx->Buffer->Accum, 0, buffersize * 4 * sizeof(GLaccum) );
	 }
	 else {
	    /* Not black */
	    GLaccum *acc, r, g, b, a;
	    GLuint i;

	    acc = ctx->Buffer->Accum;
	    r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	    g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	    b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	    a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
	    for (i=0;i<buffersize;i++) {
	       *acc++ = r;
	       *acc++ = g;
	       *acc++ = b;
	       *acc++ = a;
	    }
	 }
      }
   }
}
