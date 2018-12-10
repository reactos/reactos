/* $Id: vb.h,v 1.12 1997/08/13 02:07:17 brianp Exp $ */

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
 * $Log: vb.h,v $
 * Revision 1.12  1997/08/13 02:07:17  brianp
 * set VB_MAX to 72 is using Glide for better performance (David Bucciarelli)
 *
 * Revision 1.11  1997/06/21 21:30:59  brianp
 * updated many comments
 *
 * Revision 1.10  1997/06/20 02:08:23  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.9  1997/05/09 22:41:55  brianp
 * replaced gl_init_vb() with gl_alloc_vb()
 *
 * Revision 1.8  1997/04/24 00:29:36  brianp
 * added TexCoordSize
 *
 * Revision 1.7  1997/04/20 15:59:30  brianp
 * removed VERTEX2_BIT stuff
 *
 * Revision 1.6  1997/04/12 16:21:35  brianp
 * added CLIP_* flags and gl_init_vb()
 *
 * Revision 1.5  1997/04/07 02:59:59  brianp
 * added VertexSizeMask
 *
 * Revision 1.4  1997/04/02 03:13:12  brianp
 * removed Unclipped[], added ClipMask[], ClipOrMask, ClipAndMask
 *
 * Revision 1.3  1996/12/18 19:59:44  brianp
 * removed the material bitmask constants
 *
 * Revision 1.2  1996/09/27 01:31:17  brianp
 * added gl_init_vb() prototype
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * OVERVIEW:  The vertices defined between glBegin() and glEnd()
 * are accumulated in the vertex buffer.  When either the
 * vertex buffer becomes filled or glEnd() is called, we must flush
 * the buffer.  That is, we apply the vertex transformations, compute
 * lighting, fog, texture coordinates etc.  Then, we can render the
 * vertices as points, lines or polygons by calling the gl_render_vb()
 * function in render.c
 *
 */


#ifndef VB_H
#define VB_H


#include "types.h"



/* Flush VB when this number of vertices is accumulated:  (a multiple of 12) */
#define VB_MAX 480

/* Arrays must also accomodate new vertices from clipping: */
#define VB_SIZE  (VB_MAX + 2 * (6 + MAX_CLIP_PLANES))


/* Bit values for VertexSizeMask, below. */
/*#define VERTEX2_BIT  1*/
#define VERTEX3_BIT  2
#define VERTEX4_BIT  4



struct vertex_buffer {
   GLfloat Obj[VB_SIZE][4];	/* Object coords */
   GLfloat Eye[VB_SIZE][4];	/* Eye coords */
   GLfloat Clip[VB_SIZE][4];	/* Clip coords */
   GLfloat Win[VB_SIZE][3];	/* Window coords */

   GLfloat Normal[VB_SIZE][3];	/* Normal vectors */

   GLubyte Fcolor[VB_SIZE][4];	/* Front colors (RGBA) */
   GLubyte Bcolor[VB_SIZE][4];	/* Back colors (RGBA) */
   GLubyte (*Color)[4];		/* == Fcolor or Bcolor */

   GLuint Findex[VB_SIZE];	/* Front color indexes */
   GLuint Bindex[VB_SIZE];	/* Back color indexes */
   GLuint *Index;		/* == Findex or Bindex */

   GLboolean Edgeflag[VB_SIZE];	/* Polygon edge flags */

   GLfloat TexCoord[VB_SIZE][4];/* Texture coords */

   GLubyte ClipMask[VB_SIZE];	/* bitwise-OR of CLIP_* values, below */
   GLubyte ClipOrMask;		/* bitwise-OR of all ClipMask[] values */
   GLubyte ClipAndMask;		/* bitwise-AND of all ClipMask[] values */

   GLuint Start;		/* First vertex to process */
   GLuint Count;		/* Number of vertexes in buffer */
   GLuint Free;			/* Next empty position for clipping */

   GLuint VertexSizeMask;	/* Bitwise-or of VERTEX[234]_BIT */
   GLuint TexCoordSize;		/* Either 2 or 4 */
   GLboolean MonoColor;		/* Do all vertices have same color? */
   GLboolean MonoNormal;	/* Do all vertices have same normal? */
   GLboolean MonoMaterial;	/* Do all vertices have same material? */

   /* to handle glMaterial calls inside glBegin/glEnd: */
   GLuint MaterialMask[VB_SIZE];	/* Which material values to change */
   struct gl_material Material[VB_SIZE][2]; /* New material values */
};



/* Vertex buffer clipping flags */
#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_ALL_BITS    0x3f

#define CLIP_ALL   1
#define CLIP_NONE  2
#define CLIP_SOME  3


extern struct vertex_buffer *gl_alloc_vb(void);


#endif
