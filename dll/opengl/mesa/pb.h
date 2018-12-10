/* $Id: pb.h,v 1.4 1997/11/13 02:16:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
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
 * $Log: pb.h,v $
 * Revision 1.4  1997/11/13 02:16:48  brianp
 * added lambda array, initialized to zeros
 *
 * Revision 1.3  1997/05/09 22:40:19  brianp
 * added gl_alloc_pb()
 *
 * Revision 1.2  1997/02/09 18:43:14  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef PB_H
#define PB_H


#include "types.h"



/*
 * Pixel buffer size, must be larger than MAX_WIDTH.
 */
#define PB_SIZE (3*MAX_WIDTH)


struct pixel_buffer {
	GLint x[PB_SIZE];	/* X window coord in [0,MAX_WIDTH) */
	GLint y[PB_SIZE];	/* Y window coord in [0,MAX_HEIGHT) */
	GLdepth z[PB_SIZE];	/* Z window coord in [0,MAX_DEPTH] */
	GLubyte r[PB_SIZE];	/* Red */
	GLubyte g[PB_SIZE];	/* Green */
	GLubyte b[PB_SIZE];	/* Blue */
	GLubyte a[PB_SIZE];	/* Alpha */
	GLuint i[PB_SIZE];	/* Index */
	GLfloat s[PB_SIZE];	/* Texture S coordinate */
	GLfloat t[PB_SIZE];	/* Texture T coordinate */
	GLfloat u[PB_SIZE];	/* Texture R coordinate */
	GLfloat lambda[PB_SIZE];/* Texture lambda value */
	GLint color[4];		/* Mono color, integers! */
	GLuint index;		/* Mono index */
	GLuint count;		/* Number of pixels in buffer */
	GLboolean mono;		/* Same color or index for all pixels? */
	GLenum primitive;	/* GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP*/
};


/*
 * Initialize the Pixel Buffer, specifying the type of primitive being drawn.
 */
#define PB_INIT( PB, PRIM )		\
	(PB)->count = 0;		\
	(PB)->mono = GL_FALSE;		\
	(PB)->primitive = (PRIM);



/*
 * Set the color used for all subsequent pixels in the buffer.
 */
#define PB_SET_COLOR( CTX, PB, R, G, B, A )		\
	if ((PB)->color[0]!=(R) || (PB)->color[1]!=(G)	\
	 || (PB)->color[2]!=(B) || (PB)->color[3]!=(A)	\
	 || !(PB)->mono) {				\
		gl_flush_pb( ctx );			\
	}						\
	(PB)->color[0] = R;				\
	(PB)->color[1] = G;				\
	(PB)->color[2] = B;				\
	(PB)->color[3] = A;				\
	(PB)->mono = GL_TRUE;


/*
 * Set the color index used for all subsequent pixels in the buffer.
 */
#define PB_SET_INDEX( CTX, PB, I )		\
	if ((PB)->index!=(I) || !(PB)->mono) {	\
		gl_flush_pb( CTX );		\
	}					\
	(PB)->index = I;			\
	(PB)->mono = GL_TRUE;


/*
 * "write" a pixel using current color or index
 */
#define PB_WRITE_PIXEL( PB, X, Y, Z )		\
	(PB)->x[(PB)->count] = X;		\
	(PB)->y[(PB)->count] = Y;		\
	(PB)->z[(PB)->count] = Z;		\
	(PB)->count++;


/*
 * "write" an RGBA pixel
 */
#define PB_WRITE_RGBA_PIXEL( PB, X, Y, Z, R, G, B, A )	\
	(PB)->x[(PB)->count] = X;			\
	(PB)->y[(PB)->count] = Y;			\
	(PB)->z[(PB)->count] = Z;			\
	(PB)->r[(PB)->count] = R;			\
	(PB)->g[(PB)->count] = G;			\
	(PB)->b[(PB)->count] = B;			\
	(PB)->a[(PB)->count] = A;			\
	(PB)->count++;

/*
 * "write" a color-index pixel
 */
#define PB_WRITE_CI_PIXEL( PB, X, Y, Z, I )	\
	(PB)->x[(PB)->count] = X;		\
	(PB)->y[(PB)->count] = Y;		\
	(PB)->z[(PB)->count] = Z;		\
	(PB)->i[(PB)->count] = I;		\
	(PB)->count++;


/*
 * "write" an RGBA pixel with texture coordinates
 */
#define PB_WRITE_TEX_PIXEL( PB, X, Y, Z, R, G, B, A, S, T, U )	\
	(PB)->x[(PB)->count] = X;				\
	(PB)->y[(PB)->count] = Y;				\
	(PB)->z[(PB)->count] = Z;				\
	(PB)->r[(PB)->count] = R;				\
	(PB)->g[(PB)->count] = G;				\
	(PB)->b[(PB)->count] = B;				\
	(PB)->a[(PB)->count] = A;				\
	(PB)->s[(PB)->count] = S;				\
	(PB)->t[(PB)->count] = T;				\
	(PB)->u[(PB)->count] = U;				\
	(PB)->count++;


/*
 * Call this function at least every MAX_WIDTH pixels:
 */
#define PB_CHECK_FLUSH( CTX, PB )		\
	if ((PB)->count>=PB_SIZE-MAX_WIDTH) {	\
	   gl_flush_pb( CTX );			\
	}


extern struct pixel_buffer *gl_alloc_pb(void);

extern void gl_flush_pb( GLcontext *ctx );

#endif
