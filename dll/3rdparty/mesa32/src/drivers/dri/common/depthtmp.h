
/*
 * Notes:
 * 1. These functions plug into the gl_renderbuffer structure.
 * 2. The 'values' parameter always points to GLuint values, regardless of
 *    the actual Z buffer depth.
 */


#include "spantmp_common.h"

#ifndef DBG
#define DBG 0
#endif

#ifndef HAVE_HW_DEPTH_SPANS
#define HAVE_HW_DEPTH_SPANS 0
#endif

#ifndef HAVE_HW_DEPTH_PIXELS
#define HAVE_HW_DEPTH_PIXELS 0
#endif

static void TAG(WriteDepthSpan)( GLcontext *ctx,
                                 struct gl_renderbuffer *rb,
                                 GLuint n, GLint x, GLint y,
				 const void *values,
				 const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLuint *depth = (const GLuint *) values;
	 GLint x1;
	 GLint n1;
	 LOCAL_DEPTH_VARS;

	 y = Y_FLIP( y );

#if HAVE_HW_DEPTH_SPANS
	 (void) x1; (void) n1;

	 if ( DBG ) fprintf( stderr, "WriteDepthSpan 0..%d (x1 %d)\n",
			     (int)n, (int)x );

	 WRITE_DEPTH_SPAN();
#else
	 HW_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN( x, y, n, x1, n1, i );

	       if ( DBG ) fprintf( stderr, "WriteDepthSpan %d..%d (x1 %d) (mask %p)\n",
				   (int)i, (int)n1, (int)x1, mask );

	       if ( mask ) {
		  for ( ; n1>0 ; i++, x1++, n1-- ) {
		     if ( mask[i] ) WRITE_DEPTH( x1, y, depth[i] );
		  }
	       } else {
		  for ( ; n1>0 ; i++, x1++, n1-- ) {
		     WRITE_DEPTH( x1, y, depth[i] );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_WRITE_UNLOCK();
}


#if HAVE_HW_DEPTH_SPANS
/* implement MonoWriteDepthSpan() in terms of WriteDepthSpan() */
static void
TAG(WriteMonoDepthSpan)( GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y,
                         const void *value, const GLubyte mask[] )
{
   const GLuint depthVal = *((GLuint *) value);
   GLuint depths[MAX_WIDTH];
   GLuint i;
   for (i = 0; i < n; i++)
      depths[i] = depthVal;
   TAG(WriteDepthSpan)(ctx, rb, n, x, y, depths, mask);
}
#else
static void TAG(WriteMonoDepthSpan)( GLcontext *ctx,
                                     struct gl_renderbuffer *rb,
                                     GLuint n, GLint x, GLint y,
                                     const void *value,
                                     const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLuint depth = *((GLuint *) value);
	 GLint x1;
	 GLint n1;
	 LOCAL_DEPTH_VARS;

	 y = Y_FLIP( y );

	 HW_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN( x, y, n, x1, n1, i );

	       if ( DBG ) fprintf( stderr, "%s %d..%d (x1 %d) = %u\n",
				   __FUNCTION__, (int)i, (int)n1, (int)x1, (GLuint)depth );

	       if ( mask ) {
		  for ( ; n1>0 ; i++, x1++, n1-- ) {
		     if ( mask[i] ) WRITE_DEPTH( x1, y, depth );
		  }
	       } else {
		  for ( ; n1>0 ; x1++, n1-- ) {
		     WRITE_DEPTH( x1, y, depth );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_WRITE_UNLOCK();
}
#endif


static void TAG(WriteDepthPixels)( GLcontext *ctx,
                                   struct gl_renderbuffer *rb,
				   GLuint n,
				   const GLint x[],
				   const GLint y[],
				   const void *values,
				   const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
         const GLuint *depth = (const GLuint *) values;
	 GLuint i;
	 LOCAL_DEPTH_VARS;

	 if ( DBG ) fprintf( stderr, "WriteDepthPixels\n" );

#if HAVE_HW_DEPTH_PIXELS
	 (void) i;

	 WRITE_DEPTH_PIXELS();
#else
	 HW_CLIPLOOP()
	    {
	       if ( mask ) {
		  for ( i = 0 ; i < n ; i++ ) {
		     if ( mask[i] ) {
			const int fy = Y_FLIP( y[i] );
			if ( CLIPPIXEL( x[i], fy ) )
			   WRITE_DEPTH( x[i], fy, depth[i] );
		     }
		  }
	       }
	       else {
		  for ( i = 0 ; i < n ; i++ ) {
		     const int fy = Y_FLIP( y[i] );
		     if ( CLIPPIXEL( x[i], fy ) )
			WRITE_DEPTH( x[i], fy, depth[i] );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_WRITE_UNLOCK();
}


/* Read depth spans and pixels
 */
static void TAG(ReadDepthSpan)( GLcontext *ctx,
                                struct gl_renderbuffer *rb,
				GLuint n, GLint x, GLint y,
				void *values )
{
   HW_READ_LOCK()
      {
         GLuint *depth = (GLuint *) values;
	 GLint x1, n1;
	 LOCAL_DEPTH_VARS;

	 y = Y_FLIP( y );

	 if ( DBG ) fprintf( stderr, "ReadDepthSpan\n" );

#if HAVE_HW_DEPTH_SPANS
	 (void) x1; (void) n1;

	 READ_DEPTH_SPAN();
#else
	 HW_CLIPLOOP()
	    {
	       GLint i = 0;
	       CLIPSPAN( x, y, n, x1, n1, i );
	       for ( ; n1>0 ; i++, n1-- ) {
		  READ_DEPTH( depth[i], x+i, y );
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_READ_UNLOCK();
}

static void TAG(ReadDepthPixels)( GLcontext *ctx,
                                  struct gl_renderbuffer *rb,
                                  GLuint n,
				  const GLint x[], const GLint y[],
				  void *values )
{
   HW_READ_LOCK()
      {
         GLuint *depth = (GLuint *) values;
	 GLuint i;
	 LOCAL_DEPTH_VARS;

	 if ( DBG ) fprintf( stderr, "ReadDepthPixels\n" );

#if HAVE_HW_DEPTH_PIXELS
	 (void) i;

	 READ_DEPTH_PIXELS();
#else
	 HW_CLIPLOOP()
	    {
	       for ( i = 0 ; i < n ;i++ ) {
		  int fy = Y_FLIP( y[i] );
		  if ( CLIPPIXEL( x[i], fy ) )
		     READ_DEPTH( depth[i], x[i], fy );
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_READ_UNLOCK();
}


/**
 * Initialize the given renderbuffer's span routines to point to
 * the depth/z functions we generated above.
 */
static void TAG(InitDepthPointers)(struct gl_renderbuffer *rb)
{
   rb->GetRow = TAG(ReadDepthSpan);
   rb->GetValues = TAG(ReadDepthPixels);
   rb->PutRow = TAG(WriteDepthSpan);
   rb->PutRowRGB = NULL;
   rb->PutMonoRow = TAG(WriteMonoDepthSpan);
   rb->PutValues = TAG(WriteDepthPixels);
   rb->PutMonoValues = NULL;
}


#if HAVE_HW_DEPTH_SPANS
#undef WRITE_DEPTH_SPAN
#undef WRITE_DEPTH_PIXELS
#undef READ_DEPTH_SPAN
#undef READ_DEPTH_PIXELS
#else
#undef WRITE_DEPTH
#undef READ_DEPTH
#endif
#undef TAG
