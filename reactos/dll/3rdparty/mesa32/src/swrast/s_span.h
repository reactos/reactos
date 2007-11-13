/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#include "mtypes.h"
#include "swrast.h"


/**
 * \defgroup SpanFlags
 * Bitflags used for interpMask and arrayMask fields below to indicate
 * which interpolant values and fragment arrays are in use, respectively.
 *
 * XXX We should replace these flags with the FRAG_BIT_ values someday...
 */
/*@{*/
#define SPAN_RGBA         0x001
#define SPAN_SPEC         0x002
#define SPAN_INDEX        0x004
#define SPAN_Z            0x008
#define SPAN_W            0x010
#define SPAN_FOG          0x020
#define SPAN_TEXTURE      0x040
#define SPAN_INT_TEXTURE  0x080
#define SPAN_LAMBDA       0x100
#define SPAN_COVERAGE     0x200
#define SPAN_FLAT         0x400  /**< flat shading? */
#define SPAN_XY           0x800
#define SPAN_MASK        0x1000
#define SPAN_VARYING     0x2000
/*@}*/


#if 0
/* alternate arrangement for code below */
struct arrays2 {
   union {
      GLubyte  sz1[MAX_WIDTH][4]; /* primary color */
      GLushort sz2[MAX_WIDTH][4];
   } rgba;
   union {
      GLubyte  sz1[MAX_WIDTH][4]; /* specular color and temp storage */
      GLushort sz2[MAX_WIDTH][4];
   } spec;
};
#endif



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
   union {
      struct {
         GLubyte rgba[MAX_WIDTH][4]; /**< primary color */
         GLubyte spec[MAX_WIDTH][4]; /**< specular color and temp storage */
      } sz1;
      struct {
         GLushort rgba[MAX_WIDTH][4];
         GLushort spec[MAX_WIDTH][4];
      } sz2;
   } color;
   /** XXX these are temporary fields, pointing into above color arrays */
   GLchan (*rgba)[4];
   GLchan (*spec)[4];

   GLint   x[MAX_WIDTH];  /**< fragment X coords */
   GLint   y[MAX_WIDTH];  /**< fragment Y coords */
   GLuint  z[MAX_WIDTH];  /**< fragment Z coords */
   GLuint  index[MAX_WIDTH];  /**< Color indexes */
   GLfloat lambda[MAX_TEXTURE_COORD_UNITS][MAX_WIDTH]; /**< Texture LOD */
   GLfloat coverage[MAX_WIDTH];  /**< Fragment coverage for AA/smoothing */
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

   /** This flag indicates that mask[] array is effectively filled with ones */
   GLboolean writeAll;

   /** either GL_POLYGON, GL_LINE, GL_POLYGON, GL_BITMAP */
   GLenum primitive;

   /** 0 = front-facing span, 1 = back-facing span (for two-sided stencil) */
   GLuint facing;

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
#if CHAN_TYPE == GL_FLOAT
   GLfloat red, redStep;
   GLfloat green, greenStep;
   GLfloat blue, blueStep;
   GLfloat alpha, alphaStep;
   GLfloat specRed, specRedStep;
   GLfloat specGreen, specGreenStep;
   GLfloat specBlue, specBlueStep;
#else /* CHAN_TYPE == GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT */
   GLfixed red, redStep;
   GLfixed green, greenStep;
   GLfixed blue, blueStep;
   GLfixed alpha, alphaStep;
   GLfixed specRed, specRedStep;
   GLfixed specGreen, specGreenStep;
   GLfixed specBlue, specBlueStep;
#endif
   GLfixed index, indexStep;
   GLfixed z, zStep;    /* XXX z should probably be GLuint */
   GLfixed intTex[2], intTexStep[2];  /* s, t only */

   /**
    * This bitmask (of \link SpanFlags SPAN_* flags\endlink) indicates
    * which of the fragment arrays in the span_arrays struct are relevant.
    */
   GLbitfield arrayMask;

   /**
    * We store the arrays of fragment values in a separate struct so
    * that we can allocate sw_span structs on the stack without using
    * a lot of memory.  The span_arrays struct is about 1.4MB while the
    * sw_span struct is only about 512 bytes.
    */
   SWspanarrays *array;
} SWspan;



#define INIT_SPAN(S, PRIMITIVE, END, INTERP_MASK, ARRAY_MASK)	\
do {								\
   (S).primitive = (PRIMITIVE);					\
   (S).interpMask = (INTERP_MASK);				\
   (S).arrayMask = (ARRAY_MASK);				\
   (S).end = (END);						\
   (S).facing = 0;						\
   (S).array = SWRAST_CONTEXT(ctx)->SpanArrays;			\
} while (0)



extern void
_swrast_span_default_z( GLcontext *ctx, SWspan *span );

extern void
_swrast_span_interpolate_z( const GLcontext *ctx, SWspan *span );

extern void
_swrast_span_default_fog( GLcontext *ctx, SWspan *span );

extern void
_swrast_span_default_color( GLcontext *ctx, SWspan *span );

extern void
_swrast_span_default_secondary_color(GLcontext *ctx, SWspan *span);

extern void
_swrast_span_default_texcoords( GLcontext *ctx, SWspan *span );

extern GLfloat
_swrast_compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
                       GLfloat dqdx, GLfloat dqdy, GLfloat texW, GLfloat texH,
                       GLfloat s, GLfloat t, GLfloat q, GLfloat invQ);

extern void
_swrast_write_index_span( GLcontext *ctx, SWspan *span);


extern void
_swrast_write_rgba_span( GLcontext *ctx, SWspan *span);


extern void
_swrast_read_rgba_span(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint n, GLint x, GLint y, GLenum type, GLvoid *rgba);

extern void
_swrast_read_index_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y, GLuint indx[] );

extern void
_swrast_get_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, const GLint x[], const GLint y[],
                   void *values, GLuint valueSize);

extern void
_swrast_put_row(GLcontext *ctx, struct gl_renderbuffer *rb,
                GLuint count, GLint x, GLint y,
                const GLvoid *values, GLuint valueSize);

extern void
_swrast_get_row(GLcontext *ctx, struct gl_renderbuffer *rb,
                GLuint count, GLint x, GLint y,
                GLvoid *values, GLuint valueSize);


extern void *
_swrast_get_dest_rgba(GLcontext *ctx, struct gl_renderbuffer *rb,
                      SWspan *span);

#endif
