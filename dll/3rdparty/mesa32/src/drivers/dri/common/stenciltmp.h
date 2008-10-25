/* $XFree86: xc/lib/GL/mesa/src/drv/common/stenciltmp.h,v 1.3 2001/03/21 16:14:20 dawes Exp $ */

#include "spantmp_common.h"

#ifndef DBG
#define DBG 0
#endif

#ifndef HAVE_HW_STENCIL_SPANS
#define HAVE_HW_STENCIL_SPANS 0
#endif

#ifndef HAVE_HW_STENCIL_PIXELS
#define HAVE_HW_STENCIL_PIXELS 0
#endif

static void TAG(WriteStencilSpan)( GLcontext *ctx,
                                   struct gl_renderbuffer *rb,
				   GLuint n, GLint x, GLint y,
				   const void *values, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte *stencil = (const GLubyte *) values;
	 GLint x1;
	 GLint n1;
	 LOCAL_STENCIL_VARS;

	 y = Y_FLIP(y);

#if HAVE_HW_STENCIL_SPANS
	 (void) x1; (void) n1;

	 if (DBG) fprintf(stderr, "WriteStencilSpan 0..%d (x1 %d)\n",
			  (int)n1, (int)x1);

	 WRITE_STENCIL_SPAN();
#else /* HAVE_HW_STENCIL_SPANS */
	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);

	       if (DBG) fprintf(stderr, "WriteStencilSpan %d..%d (x1 %d)\n",
				(int)i, (int)n1, (int)x1);

	       if (mask)
	       {
		  for (;n1>0;i++,x1++,n1--)
		     if (mask[i])
			WRITE_STENCIL( x1, y, stencil[i] );
	       }
	       else
	       {
		  for (;n1>0;i++,x1++,n1--)
		     WRITE_STENCIL( x1, y, stencil[i] );
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif /* !HAVE_HW_STENCIL_SPANS */
      }
   HW_WRITE_UNLOCK();
}

#if HAVE_HW_STENCIL_SPANS
/* implement MonoWriteDepthSpan() in terms of WriteDepthSpan() */
static void
TAG(WriteMonoStencilSpan)( GLcontext *ctx, struct gl_renderbuffer *rb,
                           GLuint n, GLint x, GLint y,
                           const void *value, const GLubyte mask[] )
{
   const GLuint stenVal = *((GLuint *) value);
   GLuint stens[MAX_WIDTH];
   GLuint i;
   for (i = 0; i < n; i++)
      stens[i] = stenVal;
   TAG(WriteStencilSpan)(ctx, rb, n, x, y, stens, mask);
}
#else /* HAVE_HW_STENCIL_SPANS */
static void TAG(WriteMonoStencilSpan)( GLcontext *ctx,
                                       struct gl_renderbuffer *rb,
                                       GLuint n, GLint x, GLint y,
                                       const void *value,
                                       const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte stencil = *((const GLubyte *) value);
	 GLint x1;
	 GLint n1;
	 LOCAL_STENCIL_VARS;

	 y = Y_FLIP(y);

	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);

	       if (DBG) fprintf(stderr, "WriteStencilSpan %d..%d (x1 %d)\n",
				(int)i, (int)n1, (int)x1);

	       if (mask)
	       {
		  for (;n1>0;i++,x1++,n1--)
		     if (mask[i])
			WRITE_STENCIL( x1, y, stencil );
	       }
	       else
	       {
		  for (;n1>0;i++,x1++,n1--)
		     WRITE_STENCIL( x1, y, stencil );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}
#endif /* !HAVE_HW_STENCIL_SPANS */


static void TAG(WriteStencilPixels)( GLcontext *ctx,
                                     struct gl_renderbuffer *rb,
				     GLuint n,
				     const GLint x[], const GLint y[],
				     const void *values, const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLubyte *stencil = (const GLubyte *) values;
	 GLuint i;
	 LOCAL_STENCIL_VARS;

	 if (DBG) fprintf(stderr, "WriteStencilPixels\n");

#if HAVE_HW_STENCIL_PIXELS
	 (void) i;

	 WRITE_STENCIL_PIXELS();
#else /* HAVE_HW_STENCIL_PIXELS */
	 HW_CLIPLOOP()
	    {
	       for (i=0;i<n;i++)
	       {
		  if (mask[i]) {
		     const int fy = Y_FLIP(y[i]);
		     if (CLIPPIXEL(x[i],fy))
			WRITE_STENCIL( x[i], fy, stencil[i] );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif /* !HAVE_HW_STENCIL_PIXELS */
      }
   HW_WRITE_UNLOCK();
}


/* Read stencil spans and pixels
 */
static void TAG(ReadStencilSpan)( GLcontext *ctx,
                                  struct gl_renderbuffer *rb,
				  GLuint n, GLint x, GLint y,
				  void *values)
{
   HW_READ_LOCK()
      {
         GLubyte *stencil = (GLubyte *) values;
	 GLint x1,n1;
	 LOCAL_STENCIL_VARS;

	 y = Y_FLIP(y);

	 if (DBG) fprintf(stderr, "ReadStencilSpan\n");

#if HAVE_HW_STENCIL_SPANS
	 (void) x1; (void) n1;

	 READ_STENCIL_SPAN();
#else /* HAVE_HW_STENCIL_SPANS */
	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);
	       for (;n1>0;i++,n1--)
		  READ_STENCIL( stencil[i], (x+i), y );
	    }
	 HW_ENDCLIPLOOP();
#endif /* !HAVE_HW_STENCIL_SPANS */
      }
   HW_READ_UNLOCK();
}

static void TAG(ReadStencilPixels)( GLcontext *ctx,
                                    struct gl_renderbuffer *rb,
                                    GLuint n, const GLint x[], const GLint y[],
				    void *values )
{
   HW_READ_LOCK()
      {
         GLubyte *stencil = (GLubyte *) values;
	 GLuint i;
	 LOCAL_STENCIL_VARS;

	 if (DBG) fprintf(stderr, "ReadStencilPixels\n");
 
#if HAVE_HW_STENCIL_PIXELS
	 (void) i;

	 READ_STENCIL_PIXELS();
#else /* HAVE_HW_STENCIL_PIXELS */
	 HW_CLIPLOOP()
	    {
	       for (i=0;i<n;i++) {
		  int fy = Y_FLIP( y[i] );
		  if (CLIPPIXEL( x[i], fy ))
		     READ_STENCIL( stencil[i], x[i], fy );
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif /* !HAVE_HW_STENCIL_PIXELS */
      }
   HW_READ_UNLOCK();
}



/**
 * Initialize the given renderbuffer's span routines to point to
 * the stencil functions we generated above.
 */
static void TAG(InitStencilPointers)(struct gl_renderbuffer *rb)
{
   rb->GetRow = TAG(ReadStencilSpan);
   rb->GetValues = TAG(ReadStencilPixels);
   rb->PutRow = TAG(WriteStencilSpan);
   rb->PutRowRGB = NULL;
   rb->PutMonoRow = TAG(WriteMonoStencilSpan);
   rb->PutValues = TAG(WriteStencilPixels);
   rb->PutMonoValues = NULL;
}


#undef WRITE_STENCIL
#undef READ_STENCIL
#undef TAG
