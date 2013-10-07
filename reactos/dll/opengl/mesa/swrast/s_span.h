/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


#ifndef S_SPAN_H
#define S_SPAN_H


#include "main/config.h"
#include "main/glheader.h"
#include "main/mtypes.h"
#include "swrast/s_chan.h"


struct gl_context;
struct gl_renderbuffer;


/**
 * \defgroup SpanFlags
 * Special bitflags to describe span data.
 *
 * In general, the point/line/triangle functions interpolate/emit the
 * attributes specified by swrast->_ActiveAttribs (i.e. FRAT_BIT_* values).
 * Some things don't fit into that, though, so we have these flags.
 */
/*@{*/
#define SPAN_RGBA       0x01  /**< interpMask and arrayMask */
#define SPAN_Z          0x02  /**< interpMask and arrayMask */
#define SPAN_FLAT       0x04  /**< interpMask: flat shading? */
#define SPAN_XY         0x08  /**< array.x[], y[] valid? */
#define SPAN_MASK       0x10  /**< was array.mask[] filled in by caller? */
#define SPAN_LAMBDA     0x20  /**< array.lambda[] valid? */
#define SPAN_COVERAGE   0x40  /**< array.coverage[] valid? */
/*@}*/


/**
 * \sw_span_arrays 
 * \brief Arrays of fragment values.
 *
 * These will either be computed from the span x/xStep values or
 * filled in by glDraw/CopyPixels, etc.
 * These arrays are separated out of sw_span to conserve memory.
 */
typedef struct sw_span_arrays
{
   /** Per-fragment attributes (indexed by FRAG_ATTRIB_* tokens) */
   /* XXX someday look at transposing first two indexes for better memory
    * access pattern.
    */
   GLfloat attribs[FRAG_ATTRIB_MAX][MAX_WIDTH][4];

   /** This mask indicates which fragments are alive or culled */
   GLubyte mask[MAX_WIDTH];

   GLenum ChanType; /**< Color channel type, GL_UNSIGNED_BYTE, GL_FLOAT */

   /** Attribute arrays that don't fit into attribs[] array above */
   /*@{*/
   GLubyte rgba8[MAX_WIDTH][4];
   GLushort rgba16[MAX_WIDTH][4];
   GLchan (*rgba)[4];  /** either == rgba8 or rgba16 */
   GLint   x[MAX_WIDTH];  /**< fragment X coords */
   GLint   y[MAX_WIDTH];  /**< fragment Y coords */
   GLuint  z[MAX_WIDTH];  /**< fragment Z coords */
   GLuint  index[MAX_WIDTH];  /**< Color indexes */
   GLfloat lambda[MAX_WIDTH]; /**< Texture LOD */
   GLfloat coverage[MAX_WIDTH];  /**< Fragment coverage for AA/smoothing */
   /*@}*/
} SWspanarrays;


/**
 * The SWspan structure describes the colors, Z, fogcoord, texcoords,
 * etc for either a horizontal run or an array of independent pixels.
 * We can either specify a base/step to indicate interpolated values, or
 * fill in explicit arrays of values.  The interpMask and arrayMask bitfields
 * indicate which attributes are active interpolants or arrays, respectively.
 *
 * It would be interesting to experiment with multiprocessor rasterization
 * with this structure.  The triangle rasterizer could simply emit a
 * stream of these structures which would be consumed by one or more
 * span-processing threads which could run in parallel.
 */
typedef struct sw_span
{
   /** Coord of first fragment in horizontal span/run */
   GLint x, y;

   /** Number of fragments in the span */
   GLuint end;

   /** for clipping left edge of spans */
   GLuint leftClip;

   /** This flag indicates that mask[] array is effectively filled with ones */
   GLboolean writeAll;

   /** either GL_POLYGON, GL_LINE, GL_POLYGON, GL_BITMAP */
   GLenum primitive;

   /**
    * This bitmask (of  \link SpanFlags SPAN_* flags\endlink) indicates
    * which of the attrStart/StepX/StepY variables are relevant.
    */
   GLbitfield interpMask;

   /** Fragment attribute interpolants */
   GLfloat attrStart[FRAG_ATTRIB_MAX][4];   /**< initial value */
   GLfloat attrStepX[FRAG_ATTRIB_MAX][4];   /**< dvalue/dx */
   GLfloat attrStepY[FRAG_ATTRIB_MAX][4];   /**< dvalue/dy */

   /* XXX the rest of these will go away eventually... */

   /* For horizontal spans, step is the partial derivative wrt X.
    * For lines, step is the delta from one fragment to the next.
    */
   GLfixed red, redStep;
   GLfixed green, greenStep;
   GLfixed blue, blueStep;
   GLfixed alpha, alphaStep;
   GLfixed index, indexStep;
   GLfixed z, zStep;    /**< XXX z should probably be GLuint */
   GLfixed intTex[2], intTexStep[2];  /**< (s,t) for unit[0] only */

   /**
    * This bitmask (of \link SpanFlags SPAN_* flags\endlink) indicates
    * which of the fragment arrays in the span_arrays struct are relevant.
    */
   GLbitfield arrayMask;

   /** Mask of FRAG_BIT_x bits */
   GLbitfield64 arrayAttribs;

   /**
    * We store the arrays of fragment values in a separate struct so
    * that we can allocate sw_span structs on the stack without using
    * a lot of memory.  The span_arrays struct is about 1.4MB while the
    * sw_span struct is only about 512 bytes.
    */
   SWspanarrays *array;
} SWspan;



#define INIT_SPAN(S, PRIMITIVE)			\
do {						\
   (S).primitive = (PRIMITIVE);			\
   (S).interpMask = 0x0;			\
   (S).arrayMask = 0x0;				\
   (S).arrayAttribs = 0x0;			\
   (S).end = 0;					\
   (S).leftClip = 0;				\
   (S).array = SWRAST_CONTEXT(ctx)->SpanArrays;	\
} while (0)



extern void
_swrast_span_default_attribs(struct gl_context *ctx, SWspan *span);

extern void
_swrast_span_interpolate_z( const struct gl_context *ctx, SWspan *span );

extern GLfloat
_swrast_compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
                       GLfloat dqdx, GLfloat dqdy, GLfloat texW, GLfloat texH,
                       GLfloat s, GLfloat t, GLfloat q, GLfloat invQ);


extern void
_swrast_write_rgba_span( struct gl_context *ctx, SWspan *span);


extern void
_swrast_read_rgba_span(struct gl_context *ctx, struct gl_renderbuffer *rb,
                       GLuint n, GLint x, GLint y, GLvoid *rgba);

extern void
_swrast_put_row(struct gl_context *ctx, struct gl_renderbuffer *rb,
                GLenum datatype,
                GLuint count, GLint x, GLint y,
                const void *values, const GLubyte *mask);

extern void *
_swrast_get_dest_rgba(struct gl_context *ctx, struct gl_renderbuffer *rb,
                      SWspan *span);

#endif
