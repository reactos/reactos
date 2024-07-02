/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file config.h
 * Tunable configuration parameters.
 */

#ifndef MESA_CONFIG_H_INCLUDED
#define MESA_CONFIG_H_INCLUDED


/**
 * \name OpenGL implementation limits
 */
/*@{*/

/** Maximum modelview matrix stack depth */
#define MAX_MODELVIEW_STACK_DEPTH 32

/** Maximum projection matrix stack depth */
#define MAX_PROJECTION_STACK_DEPTH 32

/** Maximum texture matrix stack depth */
#define MAX_TEXTURE_STACK_DEPTH 10

/** Maximum color matrix stack depth */
#define MAX_COLOR_STACK_DEPTH 4

/** Maximum attribute stack depth */
#define MAX_ATTRIB_STACK_DEPTH 16

/** Maximum client attribute stack depth */
#define MAX_CLIENT_ATTRIB_STACK_DEPTH 16

/** Maximum recursion depth of display list calls */
#define MAX_LIST_NESTING 64

/** Maximum number of lights */
#define MAX_LIGHTS 8

/**
 * Maximum number of user-defined clipping planes supported by any driver in
 * Mesa.  This is used to size arrays.
 */
#define MAX_CLIP_PLANES 8

/** Maximum pixel map lookup table size */
#define MAX_PIXEL_MAP_TABLE 256

/** Maximum number of auxillary color buffers */
#define MAX_AUX_BUFFERS 1

/** Maximum order (degree) of curves */
#ifdef AMIGA
#   define MAX_EVAL_ORDER 12
#else
#   define MAX_EVAL_ORDER 30
#endif

/** Maximum Name stack depth */
#define MAX_NAME_STACK_DEPTH 64

/** Minimum point size */
#define MIN_POINT_SIZE 1.0
/** Maximum point size */
#define MAX_POINT_SIZE 60.0
/** Point size granularity */
#define POINT_SIZE_GRANULARITY 0.1

/** Minimum line width */
#define MIN_LINE_WIDTH 1.0
/** Maximum line width */
#define MAX_LINE_WIDTH 10.0
/** Line width granularity */
#define LINE_WIDTH_GRANULARITY 0.1

/** Max memory to allow for a single texture image (in megabytes) */
#define MAX_TEXTURE_MBYTES 1024

/** Number of 1D/2D texture mipmap levels */
#define MAX_TEXTURE_LEVELS 15

/** Number of 3D texture mipmap levels */
#define MAX_3D_TEXTURE_LEVELS 15

/** Number of cube texture mipmap levels - GL_ARB_texture_cube_map */
#define MAX_CUBE_TEXTURE_LEVELS 15


/** 
 * Maximum viewport/image width. Must accomodate all texture sizes too. 
 */

#ifndef MAX_WIDTH
#   define MAX_WIDTH 16384
#endif
/** Maximum viewport/image height */
#ifndef MAX_HEIGHT
#   define MAX_HEIGHT 16384
#endif

/* XXX: hack to prevent stack overflow on windows until all temporary arrays
 * [MAX_WIDTH] are allocated from the heap */
#ifdef WIN32
#undef MAX_TEXTURE_LEVELS
#undef MAX_3D_TEXTURE_LEVELS
#undef MAX_CUBE_TEXTURE_LEVELS
#undef MAX_TEXTURE_RECT_SIZE
#undef MAX_WIDTH
#undef MAX_HEIGHT
#define MAX_TEXTURE_LEVELS 13
#define MAX_CUBE_TEXTURE_LEVELS 13
#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096
#endif

/** Maxmimum size for CVA.  May be overridden by the drivers.  */
#define MAX_ARRAY_LOCK_SIZE 3000

/** Subpixel precision for antialiasing, window coordinate snapping */
#define SUB_PIXEL_BITS 4

/** Size of histogram tables */
#define HISTOGRAM_TABLE_SIZE 256

/** Max convolution filter width */
#define MAX_CONVOLUTION_WIDTH 9
/** Max convolution filter height */
#define MAX_CONVOLUTION_HEIGHT 9

/** For GL_EXT_texture_filter_anisotropic */
#define MAX_TEXTURE_MAX_ANISOTROPY 16.0

#define MAX_NV_VERTEX_PROGRAM_INPUTS 16


/** For GL_EXT_framebuffer_object */
/*@{*/
#define MAX_COLOR_ATTACHMENTS 8
/*@}*/

/** For GL_ATI_envmap_bump - support bump mapping on first 8 units */
#define SUPPORTED_ATI_BUMP_UNITS 0xff


/**
 * \name Mesa-specific parameters
 */
/*@{*/


/**
 * If non-zero use GLdouble for walking triangle edges, for better accuracy.
 */
#define TRIANGLE_WALK_DOUBLE 0


/**
 * Bits per depth buffer value (max is 32).
 */
#ifndef DEFAULT_SOFTWARE_DEPTH_BITS
#define DEFAULT_SOFTWARE_DEPTH_BITS 16
#endif
/** Depth buffer data type */
#if DEFAULT_SOFTWARE_DEPTH_BITS <= 16
#define DEFAULT_SOFTWARE_DEPTH_TYPE GLushort
#else
#define DEFAULT_SOFTWARE_DEPTH_TYPE GLuint
#endif


/**
 * Bits per stencil value: 8
 */
#define STENCIL_BITS 8


/**
 * For swrast, bits per color channel:  8, 16 or 32
 */
#ifndef CHAN_BITS
#define CHAN_BITS 8
#endif


/*
 * Color channel component order
 * 
 * \note Changes will almost certainly cause problems at this time.
 */
#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3


/**
 * Maximum number of temporary vertices required for clipping.  
 *
 * Used in array_cache and tnl modules.
 */
#define MAX_CLIPPED_VERTICES ((2 * (6 + MAX_CLIP_PLANES))+1)


#endif /* MESA_CONFIG_H_INCLUDED */
