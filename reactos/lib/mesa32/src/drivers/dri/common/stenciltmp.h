/* $XFree86: xc/lib/GL/mesa/src/drv/common/stenciltmp.h,v 1.3 2001/03/21 16:14:20 dawes Exp $ */

#include "spantmp_common.h"

#ifndef DBG
#define DBG 0
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
      }
   HW_WRITE_UNLOCK();
}


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

	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);
	       for (;n1>0;i++,n1--)
		  READ_STENCIL( stencil[i], (x+i), y );
	    }
	 HW_ENDCLIPLOOP();
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
 
	 HW_CLIPLOOP()
	    {
	       for (i=0;i<n;i++) {
		  int fy = Y_FLIP( y[i] );
		  if (CLIPPIXEL( x[i], fy ))
		     READ_STENCIL( stencil[i], x[i], fy );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_READ_UNLOCK();
}




#undef WRITE_STENCIL
#undef READ_STENCIL
#undef TAG
