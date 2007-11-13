/* $Id: svgapix.h,v 1.5 2002/11/11 18:42:44 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * Copyright (C) 1995-2002  Brian Paul
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
 * SVGA driver for Mesa.
 * Original author:  Brian Paul
 * Additional authors:  Slawomir Szczyrba <steev@hot.pl>  (Mesa 3.2)
 */


#ifndef SVGAPIX_H
#define SVGAPIX_H

#include "GL/gl.h"
#include "GL/svgamesa.h"
#include "context.h"
#include "colormac.h"
#include "vga.h"

struct svgamesa_context {
   GLcontext *gl_ctx;		/* the core Mesa context */
   GLvisual *gl_vis;		/* describes the color buffer */
   GLframebuffer *gl_buffer;	/* the ancillary buffers */
   GLuint clear_index;		/* current clear index */
   GLint clear_red, 
         clear_green, 
	 clear_blue;		/* current clear rgb color */
   GLuint clear_truecolor;	/* current clear rgb color */
   GLushort hicolor;		/* current hicolor */
   GLushort clear_hicolor;	/* current clear hicolor */
   GLint width, height;		/* size of color buffer */
   GLint depth;			/* bits per pixel (8,16,24 or 32) */
};

typedef struct { GLubyte b,g,r; } _RGB;

struct svga_buffer {   
   GLint     Depth;
   GLint     BufferSize;
   GLubyte   * FrontBuffer;
   GLubyte   * BackBuffer;
   GLubyte   * VideoRam;
   GLubyte   * DrawBuffer;  /* == FrontBuffer or BackBuffer */
   GLubyte   * ReadBuffer;  /* == FrontBuffer or BackBuffer */
};

extern struct svga_buffer SVGABuffer;
extern vga_modeinfo * SVGAInfo;
extern SVGAMesaContext SVGAMesa;    /* the current context */

#endif /* SVGAPIX_H */
