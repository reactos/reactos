/* $XFree86: xc/lib/GL/mesa/src/drv/common/depthtmp.h,v 1.5 2001/03/21 16:14:20 dawes Exp $ */

#ifndef DBG
#define DBG 0
#endif


#ifndef HAVE_HW_DEPTH_SPANS
#define HAVE_HW_DEPTH_SPANS 0
#endif
#ifndef HAVE_HW_DEPTH_PIXELS
#define HAVE_HW_DEPTH_PIXELS 0
#endif

#ifndef HW_READ_LOCK
#define HW_READ_LOCK()		HW_LOCK()
#endif
#ifndef HW_READ_UNLOCK
#define HW_READ_UNLOCK()	HW_UNLOCK()
#endif

static void TAG(WriteDepthSpan)( GLcontext *ctx,
                             GLuint n, GLint x, GLint y,
				 const GLdepth *depth,
				 const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
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

	       if ( DBG ) fprintf( stderr, "WriteDepthSpan %d..%d (x1 %d)\n",
				   (int)i, (int)n1, (int)x1 );

	       if ( mask ) {
		  for ( ; i < n1 ; i++, x1++ ) {
		     if ( mask[i] ) WRITE_DEPTH( x1, y, depth[i] );
		  }
	       } else {
		  for ( ; i < n1 ; i++, x1++ ) {
		     WRITE_DEPTH( x1, y, depth[i] );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_WRITE_UNLOCK();
}

static void TAG(WriteDepthPixels)( GLcontext *ctx,
				   GLuint n,
				   const GLint x[],
				   const GLint y[],
				   const GLdepth depth[],
				   const GLubyte mask[] )
{
   HW_WRITE_LOCK()
      {
	 GLint i;
	 LOCAL_DEPTH_VARS;

	 if ( DBG ) fprintf( stderr, "WriteDepthPixels\n" );

#if HAVE_HW_DEPTH_PIXELS
	 (void) i;

	 WRITE_DEPTH_PIXELS();
#else
	 HW_CLIPLOOP()
	    {
	       for ( i = 0 ; i < n ; i++ ) {
		  if ( mask[i] ) {
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
				GLuint n, GLint x, GLint y,
				GLdepth depth[] )
{
   HW_READ_LOCK()
      {
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
	       for ( ; i < n1 ; i++ )
		  READ_DEPTH( depth[i], (x1+i), y );
	    }
	 HW_ENDCLIPLOOP();
#endif
      }
   HW_READ_UNLOCK();
}

static void TAG(ReadDepthPixels)( GLcontext *ctx, GLuint n,
				  const GLint x[], const GLint y[],
				  GLdepth depth[] )
{
   HW_READ_LOCK()
      {
	 GLint i;
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
