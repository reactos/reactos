/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (C) Copyright IBM Corporation 2002, 2003
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
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 *    Gareth Hughes <gareth@nvidia.com>
 */

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
	 GLuint i;
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
				    const void *value,
				    const GLubyte mask[] )
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
	 GLuint i;
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
	       for (;n1>0;i++,x1++,n1--)
		  READ_RGBA( rgba[i], x1, y );
	    }
         HW_ENDCLIPLOOP();
      }
   HW_READ_UNLOCK();
}


static void TAG(ReadRGBAPixels)( GLcontext *ctx,
                                 struct gl_renderbuffer *rb,
				 GLuint n, const GLint x[], const GLint y[],
				 void *values )
{
   HW_READ_LOCK()
      {
         GLubyte (*rgba)[4] = (GLubyte (*)[4]) values;
         const GLubyte *mask = NULL; /* remove someday */
	 GLuint i;
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
   rb->GetRow = TAG(ReadRGBASpan);
}


#undef WRITE_PIXEL
#undef WRITE_RGBA
#undef READ_RGBA
#undef TAG
