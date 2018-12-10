/* $Id: config.h,v 1.8 1997/09/27 00:13:44 brianp Exp $ */

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
 * $Log: config.h,v $
 * Revision 1.8  1997/09/27 00:13:44  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.7  1997/09/23 00:57:21  brianp
 * removed MAX_DISPLAYLISTS
 *
 * Revision 1.6  1997/06/17 02:20:25  brianp
 * changed MAX_WIDTH to 1600 and MAX_HEIGHT to 1200
 *
 * Revision 1.5  1997/05/28 03:27:19  brianp
 * set MAX_TEXTURE_LEVELS to 9 if compiling for 3Dfx driver (FX)
 *
 * Revision 1.4  1997/01/08 20:54:02  brianp
 * added DITHER666 option from Michael Pichler
 *
 * Revision 1.3  1996/10/01 03:30:18  brianp
 * changed MAX_DEPTH to 0x00ffffff for 32-bit depth buffer
 *
 * Revision 1.2  1996/09/15 01:49:26  brianp
 * removed some junk
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */



/*
 * Tunable configuration parameters.
 */



#ifndef CONFIG_H
#define CONFIG_H



/* Maximum modelview matrix stack depth: */
#define MAX_MODELVIEW_STACK_DEPTH 32

/* Maximum projection matrix stack depth: */
#define MAX_PROJECTION_STACK_DEPTH 32

/* Maximum texture matrix stack depth: */
#define MAX_TEXTURE_STACK_DEPTH 10

/* Maximum attribute stack depth: */
#define MAX_ATTRIB_STACK_DEPTH 16

/* Maximum client attribute stack depth: */
#define MAX_CLIENT_ATTRIB_STACK_DEPTH 16

/* Maximum recursion depth of display list calls: */
#define MAX_LIST_NESTING 64

/* Maximum number of lights: */
#define MAX_LIGHTS 8

/* Maximum user-defined clipping planes: */
#define MAX_CLIP_PLANES 6

/* Number of texture levels */
#define MAX_TEXTURE_LEVELS 11

/* Max texture size */
#define MAX_TEXTURE_SIZE   (1 << (MAX_TEXTURE_LEVELS-1))

/* Maximum pixel map lookup table size: */
#define MAX_PIXEL_MAP_TABLE 256

/* Number of auxillary color buffers: */
#define NUM_AUX_BUFFERS 0

/* Maximum order (degree) of curves: */
#ifdef AMIGA
#   define MAX_EVAL_ORDER 12
#else
#   define MAX_EVAL_ORDER 30
#endif

/* Maximum Name stack depth */
#define MAX_NAME_STACK_DEPTH 64

/* Min and Max point sizes and granularity */
#define MIN_POINT_SIZE 1.0
#define MAX_POINT_SIZE 10.0
#define POINT_SIZE_GRANULARITY 0.1

/* Min and Max line widths and granularity */
#define MIN_LINE_WIDTH 1.0
#define MAX_LINE_WIDTH 10.0
#define LINE_WIDTH_GRANULARITY 1.0

/* Max texture palette size */
#define MAX_TEXTURE_PALETTE_SIZE 256


/* Maximum viewport size: */
#ifdef AMIGA
#  define MAX_WIDTH 640
#  define MAX_HEIGHT 400
#else
#  define MAX_WIDTH 1600
#  define MAX_HEIGHT 1200
#endif



/*
 * Bits per accumulation buffer color component:  8 or 16
 */
#define ACCUM_BITS 16


/*
 * Bits per depth buffer value:  16 or 32
 */
#define MAX_DEPTH 0x00ffffff
#define DEPTH_SCALE ((GLfloat) 0x00ffffff)


/*
 * Bits per stencil value:  8
 */
#define STENCIL_BITS 8



/***
 *** For X11 driver only:
 ***/

/*
 * When defined, use 6x6x6 dithering instead of 5x9x5.
 * 5x9x5 better for general colors, 6x6x6 better for grayscale.
 */
/*#define DITHER666*/


#endif
