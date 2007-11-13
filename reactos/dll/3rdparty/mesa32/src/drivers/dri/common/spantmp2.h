/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file spantmp2.h
 *
 * Template file of span read / write functions.
 *
 * \author Keith Whitwell <keithw@tungstengraphics.com>
 * \author Gareth Hughes <gareth@nvidia.com>
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "colormac.h"
#include "spantmp_common.h"

#ifndef DBG
#define DBG 0
#endif

#ifndef HW_READ_CLIPLOOP
#define HW_READ_CLIPLOOP()	HW_CLIPLOOP()
#endif

#ifndef HW_WRITE_CLIPLOOP
#define HW_WRITE_CLIPLOOP()	HW_CLIPLOOP()
#endif


#if (SPANTMP_PIXEL_FMT == GL_RGB)  && (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_SHORT_5_6_5)

/**
 ** GL_RGB, GL_UNSIGNED_SHORT_5_6_5
 **/

#ifndef GET_PTR
#define GET_PTR(_x, _y) (buf + (_x) * 2 + (_y) * pitch)
#endif

#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    do {                                                                \
       GLshort * _p = (GLshort *) GET_PTR(_x, _y);                      \
       _p[0] = ((((int)r & 0xf8) << 8) | (((int)g & 0xfc) << 3) |	\
		   (((int)b & 0xf8) >> 3));                             \
   } while(0)

#define WRITE_PIXEL( _x, _y, p )					\
   do {                                                                 \
      GLushort * _p = (GLushort *) GET_PTR(_x, _y);                     \
      _p[0] = p;                                                        \
   } while(0)

#define READ_RGBA( rgba, _x, _y )					\
   do {									\
      GLushort p = *(volatile GLshort *) GET_PTR(_x, _y);               \
      rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;				\
      rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;				\
      rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;				\
      rgba[3] = 0xff;							\
   } while (0)

#elif (SPANTMP_PIXEL_FMT == GL_BGRA) && (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)

/**
 ** GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV
 **/

#ifndef GET_PTR
#define GET_PTR(_x, _y) (     buf + (_x) * 4 + (_y) * pitch)
#endif

# define INIT_MONO_PIXEL(p, color)                       \
     p = PACK_COLOR_8888(color[3], color[0], color[1], color[2]) 

# define WRITE_RGBA(_x, _y, r, g, b, a)                                 \
    do {                                                                \
       GLuint * _p = (GLuint *) GET_PTR(_x, _y);                        \
       _p[0] = ((r << 16) | (g << 8) | (b << 0) | (a << 24));           \
    } while(0)

#define WRITE_PIXEL(_x, _y, p)                                          \
    do {                                                                \
       GLuint * _p = (GLuint *) GET_PTR(_x, _y);                        \
       _p[0] = p;                                                       \
    } while(0)

# if defined( USE_X86_ASM )
#  define READ_RGBA(rgba, _x, _y)                                       \
    do {                                                                \
        GLuint p = *(volatile GLuint *) GET_PTR(_x, _y);                \
       __asm__ __volatile__( "bswap	%0; rorl $8, %0"                \
				: "=r" (p) : "r" (p) );                 \
       ((GLuint *)rgba)[0] = p;                                         \
    } while (0)
# elif defined( MESA_BIG_ENDIAN )
    /* On PowerPC with GCC 3.4.2 the shift madness below becomes a single
     * rotlwi instruction.  It also produces good code on SPARC.
     */
#  define READ_RGBA( rgba, _x, _y )				        \
     do {								\
        GLuint p = *(volatile GLuint *) GET_PTR(_x, _y);                \
        GLuint t = p;                                                   \
        *((uint32_t *) rgba) = (t >> 24) | (p << 8);                    \
     } while (0)
# else
#  define READ_RGBA( rgba, _x, _y )				        \
     do {								\
        GLuint p = *(volatile GLuint *) GET_PTR(_x, _y);                \
	rgba[0] = (p >> 16) & 0xff;					\
	rgba[1] = (p >>  8) & 0xff;					\
	rgba[2] = (p >>  0) & 0xff;					\
	rgba[3] = (p >> 24) & 0xff;					\
     } while (0)
# endif

#else
#error SPANTMP_PIXEL_FMT must be set to a valid value!
#endif



/**
 ** Assembly routines.
 **/

#if defined( USE_MMX_ASM ) || defined( USE_SSE_ASM )
#include "x86/read_rgba_span_x86.h"
#include "x86/common_x86_asm.h"
#endif

static void TAG(WriteRGBASpan)( GLcontext *ctx,
                                struct gl_renderbuffer *rb,
				GLuint n, GLint x, GLint y,
				const void *values, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte (*rgba)[4] = (const GLubyte (*)[4]) values;
	 GLint x1;
	 GLint n1;
	 LOCAL_VARS;

	 y = Y_FLIP(y);

	 HW_WRITE_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);

	       if (DBG) fprintf(stderr, "WriteRGBASpan %d..%d (x1 %d)\n",
				(int)i, (int)n1, (int)x1);

	       if (mask)
	       {
		  for (;n1>0;i++,x1++,n1--)
		     if (mask[i])
			WRITE_RGBA( x1, y,
				    rgba[i][0], rgba[i][1],
				    rgba[i][2], rgba[i][3] );
	       }
	       else
	       {
		  for (;n1>0;i++,x1++,n1--)
		     WRITE_RGBA( x1, y,
				 rgba[i][0], rgba[i][1],
				 rgba[i][2], rgba[i][3] );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}

static void TAG(WriteRGBSpan)( GLcontext *ctx,
                               struct gl_renderbuffer *rb,
			       GLuint n, GLint x, GLint y,
			       const void *values, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte (*rgb)[3] = (const GLubyte (*)[3]) values;
	 GLint x1;
	 GLint n1;
	 LOCAL_VARS;

	 y = Y_FLIP(y);

	 HW_WRITE_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);

	       if (DBG) fprintf(stderr, "WriteRGBSpan %d..%d (x1 %d)\n",
				(int)i, (int)n1, (int)x1);

	       if (mask)
	       {
		  for (;n1>0;i++,x1++,n1--)
		     if (mask[i])
			WRITE_RGBA( x1, y, rgb[i][0], rgb[i][1], rgb[i][2], 255 );
	       }
	       else
	       {
		  for (;n1>0;i++,x1++,n1--)
		     WRITE_RGBA( x1, y, rgb[i][0], rgb[i][1], rgb[i][2], 255 );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}

static void TAG(WriteRGBAPixels)( GLcontext *ctx,
                                  struct gl_renderbuffer *rb,
                                  GLuint n, const GLint x[], const GLint y[],
                                  const void *values, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte (*rgba)[4] = (const GLubyte (*)[4]) values;
	 GLint i;
	 LOCAL_VARS;

	 if (DBG) fprintf(stderr, "WriteRGBAPixels\n");

	 HW_WRITE_CLIPLOOP()
	    {
	       if (mask)
	       {
	          for (i=0;i<n;i++)
	          {
		     if (mask[i]) {
		        const int fy = Y_FLIP(y[i]);
		        if (CLIPPIXEL(x[i],fy))
			   WRITE_RGBA( x[i], fy,
				       rgba[i][0], rgba[i][1],
				       rgba[i][2], rgba[i][3] );
		     }
	          }
	       }
	       else
	       {
	          for (i=0;i<n;i++)
	          {
		     const int fy = Y_FLIP(y[i]);
		     if (CLIPPIXEL(x[i],fy))
			WRITE_RGBA( x[i], fy,
				    rgba[i][0], rgba[i][1],
				    rgba[i][2], rgba[i][3] );
	          }
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}


static void TAG(WriteMonoRGBASpan)( GLcontext *ctx,	
                                    struct gl_renderbuffer *rb,
				    GLuint n, GLint x, GLint y, 
				    const void *value, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte *color = (const GLubyte *) value;
	 GLint x1;
	 GLint n1;
	 LOCAL_VARS;
	 INIT_MONO_PIXEL(p, color);

	 y = Y_FLIP( y );

	 if (DBG) fprintf(stderr, "WriteMonoRGBASpan\n");

	 HW_WRITE_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);
	       if (mask)
	       {
	          for (;n1>0;i++,x1++,n1--)
		     if (mask[i])
		        WRITE_PIXEL( x1, y, p );
	       }
	       else
	       {
	          for (;n1>0;i++,x1++,n1--)
		     WRITE_PIXEL( x1, y, p );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}


static void TAG(WriteMonoRGBAPixels)( GLcontext *ctx,
                                      struct gl_renderbuffer *rb,
				      GLuint n,
				      const GLint x[], const GLint y[],
				      const void *value,
				      const GLubyte mask[] ) 
{
   HW_WRITE_LOCK()
      {
         const GLubyte *color = (const GLubyte *) value;
	 GLint i;
	 LOCAL_VARS;
	 INIT_MONO_PIXEL(p, color);

	 if (DBG) fprintf(stderr, "WriteMonoRGBAPixels\n");

	 HW_WRITE_CLIPLOOP()
	    {
	       if (mask)
	       {
		  for (i=0;i<n;i++)
		     if (mask[i]) {
			int fy = Y_FLIP(y[i]);
			if (CLIPPIXEL( x[i], fy ))
			   WRITE_PIXEL( x[i], fy, p );
		     }
	       }
	       else
	       {
		  for (i=0;i<n;i++) {
		     int fy = Y_FLIP(y[i]);
		     if (CLIPPIXEL( x[i], fy ))
			WRITE_PIXEL( x[i], fy, p );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}


static void TAG(ReadRGBASpan)( GLcontext *ctx,
                               struct gl_renderbuffer *rb,
			       GLuint n, GLint x, GLint y, void *values)
{
   HW_READ_LOCK()
      {
         GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
	 GLint x1,n1;
	 LOCAL_VARS;

	 y = Y_FLIP(y);

	 if (DBG) fprintf(stderr, "ReadRGBASpan\n");

	 HW_READ_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);
	       for (;n1>0;i++,x1++,n1--)
		  READ_RGBA( rgba[i], x1, y );
	    }
         HW_ENDCLIPLOOP();
      }
   HW_READ_UNLOCK();
}


#if defined(USE_MMX_ASM) && \
   (((SPANTMP_PIXEL_FMT == GL_BGRA) && \
	(SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)) || \
    ((SPANTMP_PIXEL_FMT == GL_RGB) && \
	(SPANTMP_PIXEL_TYPE == GL_UNSIGNED_SHORT_5_6_5)))
static void TAG2(ReadRGBASpan,_MMX)( GLcontext *ctx,
                                     struct gl_renderbuffer *rb,
                                     GLuint n, GLint x, GLint y, void *values)
{
#ifndef USE_INNER_EMMS
   /* The EMMS instruction is directly in-lined here because using GCC's
    * built-in _mm_empty function was found to utterly destroy performance.
    */
   __asm__ __volatile__( "emms" );
#endif

   HW_READ_LOCK()
     {
        GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
	GLint x1,n1;
	LOCAL_VARS;

	y = Y_FLIP(y);

	if (DBG) fprintf(stderr, "ReadRGBASpan\n");

	HW_READ_CLIPLOOP()
	  {
	     GLint i = 0;
	     CLIPSPAN(x,y,n,x1,n1,i);

	       {
		  const void * src = GET_PTR( x1, y );
#if (SPANTMP_PIXEL_FMT == GL_RGB) && \
		  (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_SHORT_5_6_5)
		  _generic_read_RGBA_span_RGB565_MMX( src, rgba[i], n1 );
#else
		  _generic_read_RGBA_span_BGRA8888_REV_MMX( src, rgba[i], n1 );
#endif
	       }
	  }
	HW_ENDCLIPLOOP();
     }
   HW_READ_UNLOCK();
#ifndef USE_INNER_EMMS
   __asm__ __volatile__( "emms" );
#endif
}
#endif


#if defined(USE_SSE_ASM) && \
   (SPANTMP_PIXEL_FMT == GL_BGRA) && \
     (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)
static void TAG2(ReadRGBASpan,_SSE2)( GLcontext *ctx,
                                      struct gl_renderbuffer *rb,
                                      GLuint n, GLint x, GLint y,
                                      void *values)
{
   HW_READ_LOCK()
     {
        GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
	GLint x1,n1;
	LOCAL_VARS;

	y = Y_FLIP(y);

	if (DBG) fprintf(stderr, "ReadRGBASpan\n");

	HW_READ_CLIPLOOP()
	  {
	     GLint i = 0;
	     CLIPSPAN(x,y,n,x1,n1,i);

	       {
		  const void * src = GET_PTR( x1, y );
		  _generic_read_RGBA_span_BGRA8888_REV_SSE2( src, rgba[i], n1 );
	       }
	  }
	HW_ENDCLIPLOOP();
     }
   HW_READ_UNLOCK();
}
#endif

#if defined(USE_SSE_ASM) && \
   (SPANTMP_PIXEL_FMT == GL_BGRA) && \
     (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)
static void TAG2(ReadRGBASpan,_SSE)( GLcontext *ctx,
                                     struct gl_renderbuffer *rb,
                                     GLuint n, GLint x, GLint y,
                                     void *values)
{
#ifndef USE_INNER_EMMS
   /* The EMMS instruction is directly in-lined here because using GCC's
    * built-in _mm_empty function was found to utterly destroy performance.
    */
   __asm__ __volatile__( "emms" );
#endif

   HW_READ_LOCK()
     {
        GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
	GLint x1,n1;
	LOCAL_VARS;

	y = Y_FLIP(y);

	if (DBG) fprintf(stderr, "ReadRGBASpan\n");

	HW_READ_CLIPLOOP()
	  {
	     GLint i = 0;
	     CLIPSPAN(x,y,n,x1,n1,i);

	       {
		  const void * src = GET_PTR( x1, y );
		  _generic_read_RGBA_span_BGRA8888_REV_SSE( src, rgba[i], n1 );
	       }
	  }
	HW_ENDCLIPLOOP();
     }
   HW_READ_UNLOCK();
#ifndef USE_INNER_EMMS
   __asm__ __volatile__( "emms" );
#endif
}
#endif


static void TAG(ReadRGBAPixels)( GLcontext *ctx,
                                 struct gl_renderbuffer *rb,
				 GLuint n, const GLint x[], const GLint y[],
				 void *values )
{
   HW_READ_LOCK()
      {
         GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
         GLubyte *mask = NULL; /* remove someday */
	 GLint i;
	 LOCAL_VARS;

	 if (DBG) fprintf(stderr, "ReadRGBAPixels\n");

	 HW_READ_CLIPLOOP()
	    {
	       if (mask)
	       {
		  for (i=0;i<n;i++)
		     if (mask[i]) {
			int fy = Y_FLIP( y[i] );
			if (CLIPPIXEL( x[i], fy ))
			   READ_RGBA( rgba[i], x[i], fy );
		     }
	       }
	       else
	       {
		  for (i=0;i<n;i++) {
		     int fy = Y_FLIP( y[i] );
		     if (CLIPPIXEL( x[i], fy ))
			READ_RGBA( rgba[i], x[i], fy );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_READ_UNLOCK();
}

static void TAG(InitPointers)(struct gl_renderbuffer *rb)
{
   rb->PutRow = TAG(WriteRGBASpan);
   rb->PutRowRGB = TAG(WriteRGBSpan);
   rb->PutMonoRow = TAG(WriteMonoRGBASpan);
   rb->PutValues = TAG(WriteRGBAPixels);
   rb->PutMonoValues = TAG(WriteMonoRGBAPixels);
   rb->GetValues = TAG(ReadRGBAPixels);

#if defined(USE_SSE_ASM) && \
   (SPANTMP_PIXEL_FMT == GL_BGRA) && \
     (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)
   if ( cpu_has_xmm2 ) {
      if (DBG) fprintf( stderr, "Using %s version of GetRow\n", "SSE2" );
      rb->GetRow = TAG2(ReadRGBASpan, _SSE2);
   }
   else
#endif
#if defined(USE_SSE_ASM) && \
   (SPANTMP_PIXEL_FMT == GL_BGRA) && \
     (SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)
   if ( cpu_has_xmm ) {
      if (DBG) fprintf( stderr, "Using %s version of GetRow\n", "SSE" );
      rb->GetRow = TAG2(ReadRGBASpan, _SSE);
   }
   else
#endif
#if defined(USE_MMX_ASM) && \
   (((SPANTMP_PIXEL_FMT == GL_BGRA) && \
	(SPANTMP_PIXEL_TYPE == GL_UNSIGNED_INT_8_8_8_8_REV)) || \
    ((SPANTMP_PIXEL_FMT == GL_RGB) && \
	(SPANTMP_PIXEL_TYPE == GL_UNSIGNED_SHORT_5_6_5)))
   if ( cpu_has_mmx ) {
      if (DBG) fprintf( stderr, "Using %s version of GetRow\n", "MMX" );
      rb->GetRow = TAG2(ReadRGBASpan, _MMX);
   }
   else
#endif
   {
      if (DBG) fprintf( stderr, "Using %s version of GetRow\n", "C" );
      rb->GetRow = TAG(ReadRGBASpan);
   }

}


#undef INIT_MONO_PIXEL
#undef WRITE_PIXEL
#undef WRITE_RGBA
#undef READ_RGBA
#undef TAG
#undef TAG2
#undef GET_PTR
#undef SPANTMP_PIXEL_FMT
#undef SPANTMP_PIXEL_TYPE
