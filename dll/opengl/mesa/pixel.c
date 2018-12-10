/* $Id: pixel.c,v 1.8 1997/07/24 01:23:44 brianp Exp $ */

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
 * $Log: pixel.c,v $
 * Revision 1.8  1997/07/24 01:23:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.7  1997/05/28 03:26:02  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.6  1997/02/09 20:05:03  brianp
 * new arguments for gl_pixel_addr_in_image()
 *
 * Revision 1.5  1997/02/09 18:52:37  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.4  1996/11/06 04:09:37  brianp
 * added a missing return after a gl_error() call
 *
 * Revision 1.3  1996/09/26 22:35:10  brianp
 * fixed a few compiler warnings from IRIX 6 -n32 and -64 compiler
 *
 * Revision 1.2  1996/09/15 14:18:55  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * glPixelStore, glPixelTransfer, glPixelMap, glPixelZoom, etc.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "pixel.h"
#include "image.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#endif



/*
 * Determine if we can use the optimized glDrawPixels function.
 */
static void update_drawpixels_state( GLcontext *ctx )
{
   if (ctx->Visual->RGBAflag==GL_TRUE &&
       ctx->Visual->EightBitColor &&
       ctx->Pixel.RedBias==0.0   && ctx->Pixel.RedScale==1.0 &&
       ctx->Pixel.GreenBias==0.0 && ctx->Pixel.GreenScale==1.0 &&
       ctx->Pixel.BlueBias==0.0  && ctx->Pixel.BlueScale==1.0 &&
       ctx->Pixel.AlphaBias==0.0 && ctx->Pixel.AlphaScale==1.0 &&
       ctx->Pixel.MapColorFlag==GL_FALSE &&
       ctx->Pixel.ZoomX==1.0 && ctx->Pixel.ZoomY==1.0 &&
/*       ctx->Unpack.Alignment==4 &&*/
       ctx->Unpack.RowLength==0 &&
       ctx->Unpack.SkipPixels==0 &&
       ctx->Unpack.SkipRows==0 &&
       ctx->Unpack.SwapBytes==0 &&
       ctx->Unpack.LsbFirst==0) {
      ctx->FastDrawPixels = GL_TRUE;
   }
   else {
      ctx->FastDrawPixels = GL_FALSE;
   }
}




/**********************************************************************/
/*****                    glPixelZoom                             *****/
/**********************************************************************/



void gl_PixelZoom( GLcontext *ctx, GLfloat xfactor, GLfloat yfactor )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPixelZoom" );
      return;
   }
   ctx->Pixel.ZoomX = xfactor;
   ctx->Pixel.ZoomY = yfactor;
   update_drawpixels_state( ctx );
}




/*
 * Write a span of pixels to the frame buffer while applying a pixel zoom.
 * This is only used by glDrawPixels and glCopyPixels.
 * Input:  n - number of pixels in input row
 *         x, y - destination of the span
 *         z - depth values for the span
 *         red, green, blue, alpha - array of colors
 *         y0 - location of first row in the image we're drawing.
 */
void
gl_write_zoomed_color_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            const GLubyte red[], const GLubyte green[],
                            const GLubyte blue[], const GLubyte alpha[],
                            GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLubyte zred[MAX_WIDTH], zgreen[MAX_WIDTH];  /* zoomed pixel colors */
   GLubyte zblue[MAX_WIDTH], zalpha[MAX_WIDTH];
   GLdepth zdepth[MAX_WIDTH];  /* zoomed depth values */
   GLint maxwidth = MIN2( ctx->Buffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->Buffer->Height && r1>=ctx->Buffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zred[j]   = red[i];
         zgreen[j] = green[i];
         zblue[j]  = blue[i];
         zalpha[j] = alpha[i];
         zdepth[j] = z[i];
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (j+skipcol) * xscale;
         if (i<0)  i = n + i - 1;
         zred[j]   = red[i];
         zgreen[j] = green[i];
         zblue[j]  = blue[i];
         zalpha[j] = alpha[i];
         zdepth[j] = z[i];
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      gl_write_color_span( ctx, m, x+skipcol, r, zdepth,
                           zred, zgreen, zblue, zalpha, GL_BITMAP );
   }
}



/*
 * As above, but write CI pixels.
 */
void
gl_write_zoomed_index_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            const GLuint indexes[], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLuint zindexes[MAX_WIDTH];  /* zoomed color indexes */
   GLdepth zdepth[MAX_WIDTH];  /* zoomed depth values */
   GLint maxwidth = MIN2( ctx->Buffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->Buffer->Height && r1>=ctx->Buffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zindexes[j] = indexes[i];
         zdepth[j]   = z[i];
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (j+skipcol) * xscale;
         if (i<0)  i = n + i - 1;
         zindexes[j] = indexes[i];
         zdepth[j] = z[i];
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      gl_write_index_span( ctx, m, x+skipcol, r, zdepth, zindexes, GL_BITMAP );
   }
}



/*
 * As above, but write stencil values.
 */
void
gl_write_zoomed_stencil_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y,
                              const GLubyte stencil[], GLint y0 )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLubyte zstencil[MAX_WIDTH];  /* zoomed stencil values */
   GLint maxwidth = MIN2( ctx->Buffer->Width, MAX_WIDTH );

   /* compute width of output row */
   m = (GLint) ABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y-y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0>=ctx->Buffer->Height && r1>=ctx->Buffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   assert( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zstencil[j] = stencil[i];
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (j+skipcol) * xscale;
         if (i<0)  i = n + i - 1;
         zstencil[j] = stencil[i];
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      gl_write_stencil_span( ctx, m, x+skipcol, r, zstencil );
   }
}




/**********************************************************************/
/*****                    glPixelStore                            *****/
/**********************************************************************/


void gl_PixelStorei( GLcontext *ctx, GLenum pname, GLint param )
{
   /* NOTE: this call can't be compiled into the display list */

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPixelStore" );
      return;
   }

   switch (pname) {
      case GL_PACK_SWAP_BYTES:
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_LSB_FIRST:
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.RowLength = param;
	 }
	 break;
      case GL_PACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.SkipPixels = param;
	 }
	 break;
      case GL_PACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.SkipRows = param;
	 }
	 break;
      case GL_PACK_ALIGNMENT:
         if (param==1 || param==2 || param==4 || param==8) {
	    ctx->Pack.Alignment = param;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
         break;
      case GL_UNPACK_LSB_FIRST:
	 ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.RowLength = param;
	 }
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.SkipPixels = param;
	 }
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.SkipRows = param;
	 }
	 break;
      case GL_UNPACK_ALIGNMENT:
         if (param==1 || param==2 || param==4 || param==8) {
	    ctx->Unpack.Alignment = param;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore" );
	 }
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glPixelStore" );
   }
   update_drawpixels_state( ctx );
}





/**********************************************************************/
/*****                         glPixelMap                         *****/
/**********************************************************************/



void gl_PixelMapfv( GLcontext *ctx,
                    GLenum map, GLint mapsize, const GLfloat *values )
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPixelMapfv" );
      return;
   }

   if (mapsize<0 || mapsize>MAX_PIXEL_MAP_TABLE) {
      gl_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
      return;
   }

   if (map>=GL_PIXEL_MAP_S_TO_S && map<=GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      GLuint p;
      GLboolean ok = GL_FALSE;
      for (p=1; p<=MAX_PIXEL_MAP_TABLE; p=p<<1) {
	 if ( (p&mapsize) == p ) {
	    ok = GL_TRUE;
	    break;
	 }
      }
      if (!ok) {
	 gl_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
         return;
      }
   }

   switch (map) {
      case GL_PIXEL_MAP_S_TO_S:
         ctx->Pixel.MapStoSsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapStoS[i] = (GLint) values[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_I:
         ctx->Pixel.MapItoIsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoI[i] = (GLint) values[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         ctx->Pixel.MapItoRsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoR[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         ctx->Pixel.MapItoGsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoG[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         ctx->Pixel.MapItoBsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoB[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         ctx->Pixel.MapItoAsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoA[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         ctx->Pixel.MapRtoRsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapRtoR[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         ctx->Pixel.MapGtoGsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapGtoG[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         ctx->Pixel.MapBtoBsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapBtoB[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         ctx->Pixel.MapAtoAsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapAtoA[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPixelMapfv(map)" );
   }
}





void gl_GetPixelMapfv( GLcontext *ctx, GLenum map, GLfloat *values )
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetPixelMapfv" );
      return;
   }
   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         for (i=0;i<ctx->Pixel.MapItoIsize;i++) {
	    values[i] = (GLfloat) ctx->Pixel.MapItoI[i];
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         for (i=0;i<ctx->Pixel.MapStoSsize;i++) {
	    values[i] = (GLfloat) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         MEMCPY(values,ctx->Pixel.MapItoR,ctx->Pixel.MapItoRsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         MEMCPY(values,ctx->Pixel.MapItoG,ctx->Pixel.MapItoGsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         MEMCPY(values,ctx->Pixel.MapItoB,ctx->Pixel.MapItoBsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         MEMCPY(values,ctx->Pixel.MapItoA,ctx->Pixel.MapItoAsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         MEMCPY(values,ctx->Pixel.MapRtoR,ctx->Pixel.MapRtoRsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         MEMCPY(values,ctx->Pixel.MapGtoG,ctx->Pixel.MapGtoGsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         MEMCPY(values,ctx->Pixel.MapBtoB,ctx->Pixel.MapBtoBsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         MEMCPY(values,ctx->Pixel.MapAtoA,ctx->Pixel.MapAtoAsize*sizeof(GLfloat));
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}


void gl_GetPixelMapuiv( GLcontext *ctx, GLenum map, GLuint *values )
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetPixelMapfv" );
      return;
   }
   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         MEMCPY(values, ctx->Pixel.MapItoI, ctx->Pixel.MapItoIsize*sizeof(GLint));
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         MEMCPY(values, ctx->Pixel.MapStoS, ctx->Pixel.MapStoSsize*sizeof(GLint));
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i=0;i<ctx->Pixel.MapItoRsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i=0;i<ctx->Pixel.MapItoGsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i=0;i<ctx->Pixel.MapItoBsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i=0;i<ctx->Pixel.MapItoAsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i=0;i<ctx->Pixel.MapRtoRsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i=0;i<ctx->Pixel.MapGtoGsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i=0;i<ctx->Pixel.MapBtoBsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i=0;i<ctx->Pixel.MapAtoAsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}


void gl_GetPixelMapusv( GLcontext *ctx, GLenum map, GLushort *values )
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetPixelMapfv" );
      return;
   }
   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
	 for (i=0;i<ctx->Pixel.MapItoIsize;i++) {
	    values[i] = (GLushort) ctx->Pixel.MapItoI[i];
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
	 for (i=0;i<ctx->Pixel.MapStoSsize;i++) {
	    values[i] = (GLushort) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i=0;i<ctx->Pixel.MapItoRsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i=0;i<ctx->Pixel.MapItoGsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i=0;i<ctx->Pixel.MapItoBsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i=0;i<ctx->Pixel.MapItoAsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i=0;i<ctx->Pixel.MapRtoRsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i=0;i<ctx->Pixel.MapGtoGsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i=0;i<ctx->Pixel.MapBtoBsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i=0;i<ctx->Pixel.MapAtoAsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}



/**********************************************************************/
/*****                       glPixelTransfer                      *****/
/**********************************************************************/


/*
 * Implements glPixelTransfer[fi] whether called immediately or from a
 * display list.
 */
void gl_PixelTransferf( GLcontext *ctx, GLenum pname, GLfloat param )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPixelTransfer" );
      return;
   }

   switch (pname) {
      case GL_MAP_COLOR:
         ctx->Pixel.MapColorFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_MAP_STENCIL:
         ctx->Pixel.MapStencilFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_INDEX_SHIFT:
         ctx->Pixel.IndexShift = (GLint) param;
	 break;
      case GL_INDEX_OFFSET:
         ctx->Pixel.IndexOffset = (GLint) param;
	 break;
      case GL_RED_SCALE:
         ctx->Pixel.RedScale = param;
	 break;
      case GL_RED_BIAS:
         ctx->Pixel.RedBias = param;
	 break;
      case GL_GREEN_SCALE:
         ctx->Pixel.GreenScale = param;
	 break;
      case GL_GREEN_BIAS:
         ctx->Pixel.GreenBias = param;
	 break;
      case GL_BLUE_SCALE:
         ctx->Pixel.BlueScale = param;
	 break;
      case GL_BLUE_BIAS:
         ctx->Pixel.BlueBias = param;
	 break;
      case GL_ALPHA_SCALE:
         ctx->Pixel.AlphaScale = param;
	 break;
      case GL_ALPHA_BIAS:
         ctx->Pixel.AlphaBias = param;
	 break;
      case GL_DEPTH_SCALE:
         ctx->Pixel.DepthScale = param;
	 break;
      case GL_DEPTH_BIAS:
         ctx->Pixel.DepthBias = param;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
         return;
   }
   update_drawpixels_state( ctx );
}





/**********************************************************************/
/*****                 Pixel packing/unpacking                    *****/
/**********************************************************************/



/*
 * Unpack a 2-D pixel array/image.  The unpacked format will be con-
 * tiguous (no "empty" bytes) with byte/bit swapping applied as needed.
 * Input:  same as glDrawPixels
 * Output:  pointer to block of pixel data in same format and type as input
 *          or NULL if error.
 */
GLvoid *gl_unpack_pixels( GLcontext *ctx,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          const GLvoid *pixels )
{
   GLint s, n;

   s = gl_sizeof_type( type );
   if (s<0) {
      gl_error( ctx, GL_INVALID_ENUM, "internal error in gl_unpack(type)" );
      return NULL;
   }

   n = gl_components_in_format( format );
   if (n<0) {
      gl_error( ctx, GL_INVALID_ENUM, "gl_unpack_pixels(format)" );
      return NULL;
   }

   if (type==GL_BITMAP) {
      /* BITMAP data */
      GLint bytes, i, width_in_bytes;
      GLubyte *buffer, *dst;
      GLvoid *src;

      /* Alloc dest storage */
      bytes = CEILING( width * height , 8 );
      buffer = (GLubyte *) malloc( bytes );
      if (!buffer) {
	 return NULL;
      }

      /* Copy/unpack pixel data to buffer */
      width_in_bytes = CEILING( width, 8 );
      dst = buffer;
      for (i=0;i<height;i++) {
         src = gl_pixel_addr_in_image( &ctx->Unpack, pixels, width, height,
                                       format, type, i);
         if (!src) {
            free(buffer);
            return NULL;
         }
	 MEMCPY( dst, src, width_in_bytes );
	 dst += width_in_bytes;
      }

      /* Bit flipping */
      if (ctx->Unpack.LsbFirst) {
	 gl_flip_bytes( buffer, bytes );
      }
      return (GLvoid *) buffer;
   }
   else {
      /* Non-BITMAP data */
      GLint width_in_bytes, bytes, i;
      GLubyte *buffer, *dst;
      GLvoid *src;

      width_in_bytes = width * n * s;

      /* Alloc dest storage */
      bytes = height * width_in_bytes;
      buffer = (GLubyte *) malloc( bytes );
      if (!buffer) {
	 return NULL;
      }

      /* Copy/unpack pixel data to buffer */
      dst = buffer;
      for (i=0;i<height;i++) {
         src = gl_pixel_addr_in_image( &ctx->Unpack, pixels, width, height,
                                       format, type, i);
         if (!src) {
            free(buffer);
            return NULL;
         }
	 MEMCPY( dst, src, width_in_bytes );
	 dst += width_in_bytes;
      }

      /* Byte swapping */
      if (ctx->Unpack.SwapBytes && s>1) {
	 if (s==2) {
	    gl_swap2( (GLushort *) buffer, bytes/2 );
	 }
	 else if (s==4) {
	    gl_swap4( (GLuint *) buffer, bytes/4 );
	 }
      }
      return (GLvoid *) buffer;
   }
}




/*
   if (s>=a) {
      k = n * l;
   }
   else {  *s<a*
      k = (a/s) * ceil( s*n*l / a );
   }

   s = size in bytes of a single component
   a = alignment
   n = number of components in a pixel
   l = number of pixels in a row

   k = number of components or indices between first pixel in each row in mem.
*/

