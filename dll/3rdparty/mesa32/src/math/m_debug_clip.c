/*
 * Mesa 3-D graphics library
 * Version:  6.1
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
 *
 * Authors:
 *    Gareth Hughes
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"

#include "m_matrix.h"
#include "m_xform.h"

#include "m_debug.h"
#include "m_debug_util.h"

#ifdef __UNIXOS2__
/* The linker doesn't like empty files */
static char dummy;
#endif

#ifdef DEBUG_MATH  /* This code only used for debugging */

static clip_func *clip_tab[2] = {
   _mesa_clip_tab,
   _mesa_clip_np_tab
};
static char *cnames[2] = {
   "_mesa_clip_tab",
   "_mesa_clip_np_tab"
};
#ifdef RUN_DEBUG_BENCHMARK
static char *cstrings[2] = {
   "clip, perspective divide",
   "clip, no divide"
};
#endif


/* =============================================================
 * Reference cliptests
 */

static GLvector4f *ref_cliptest_points4( GLvector4f *clip_vec,
					 GLvector4f *proj_vec,
					 GLubyte clipMask[],
					 GLubyte *orMask,
					 GLubyte *andMask )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   GLuint c = 0;
   GLfloat (*vProj)[4] = (GLfloat (*)[4])proj_vec->start;
   GLubyte tmpAndMask = *andMask;
   GLubyte tmpOrMask = *orMask;
   GLuint i;
   for ( i = 0 ; i < count ; i++, STRIDE_F(from, stride) ) {
      const GLfloat cx = from[0];
      const GLfloat cy = from[1];
      const GLfloat cz = from[2];
      const GLfloat cw = from[3];
      GLubyte mask = 0;
      if ( -cx + cw < 0 ) mask |= CLIP_RIGHT_BIT;
      if (  cx + cw < 0 ) mask |= CLIP_LEFT_BIT;
      if ( -cy + cw < 0 ) mask |= CLIP_TOP_BIT;
      if (  cy + cw < 0 ) mask |= CLIP_BOTTOM_BIT;
      if ( -cz + cw < 0 ) mask |= CLIP_FAR_BIT;
      if (  cz + cw < 0 ) mask |= CLIP_NEAR_BIT;
      clipMask[i] = mask;
      if ( mask ) {
	 c++;
	 tmpAndMask &= mask;
	 tmpOrMask |= mask;
	 vProj[i][0] = 0;
	 vProj[i][1] = 0;
	 vProj[i][2] = 0;
	 vProj[i][3] = 1;
      } else {
	 GLfloat oow = 1.0F / cw;
	 vProj[i][0] = cx * oow;
	 vProj[i][1] = cy * oow;
	 vProj[i][2] = cz * oow;
	 vProj[i][3] = oow;
      }
   }

   *orMask = tmpOrMask;
   *andMask = (GLubyte) (c < count ? 0 : tmpAndMask);

   proj_vec->flags |= VEC_SIZE_4;
   proj_vec->size = 4;
   proj_vec->count = clip_vec->count;
   return proj_vec;
}

/* Keep these here for now, even though we don't use them...
 */
static GLvector4f *ref_cliptest_points3( GLvector4f *clip_vec,
					 GLvector4f *proj_vec,
					 GLubyte clipMask[],
					 GLubyte *orMask,
					 GLubyte *andMask )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;

   GLubyte tmpOrMask = *orMask;
   GLubyte tmpAndMask = *andMask;
   GLuint i;
   for ( i = 0 ; i < count ; i++, STRIDE_F(from, stride) ) {
      const GLfloat cx = from[0], cy = from[1], cz = from[2];
      GLubyte mask = 0;
      if ( cx >  1.0 )		mask |= CLIP_RIGHT_BIT;
      else if ( cx < -1.0 )	mask |= CLIP_LEFT_BIT;
      if ( cy >  1.0 )		mask |= CLIP_TOP_BIT;
      else if ( cy < -1.0 )	mask |= CLIP_BOTTOM_BIT;
      if ( cz >  1.0 )		mask |= CLIP_FAR_BIT;
      else if ( cz < -1.0 )	mask |= CLIP_NEAR_BIT;
      clipMask[i] = mask;
      tmpOrMask |= mask;
      tmpAndMask &= mask;
   }

   *orMask = tmpOrMask;
   *andMask = tmpAndMask;
   return clip_vec;
}

static GLvector4f * ref_cliptest_points2( GLvector4f *clip_vec,
					  GLvector4f *proj_vec,
					  GLubyte clipMask[],
					  GLubyte *orMask,
					  GLubyte *andMask )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;

   GLubyte tmpOrMask = *orMask;
   GLubyte tmpAndMask = *andMask;
   GLuint i;
   for ( i = 0 ; i < count ; i++, STRIDE_F(from, stride) ) {
      const GLfloat cx = from[0], cy = from[1];
      GLubyte mask = 0;
      if ( cx >  1.0 )		mask |= CLIP_RIGHT_BIT;
      else if ( cx < -1.0 )	mask |= CLIP_LEFT_BIT;
      if ( cy >  1.0 )		mask |= CLIP_TOP_BIT;
      else if ( cy < -1.0 )	mask |= CLIP_BOTTOM_BIT;
      clipMask[i] = mask;
      tmpOrMask |= mask;
      tmpAndMask &= mask;
   }

   *orMask = tmpOrMask;
   *andMask = tmpAndMask;
   return clip_vec;
}

static clip_func ref_cliptest[5] = {
   0,
   0,
   ref_cliptest_points2,
   ref_cliptest_points3,
   ref_cliptest_points4
};


/* =============================================================
 * Cliptest tests
 */

ALIGN16(static GLfloat, s[TEST_COUNT][4]);
ALIGN16(static GLfloat, d[TEST_COUNT][4]);
ALIGN16(static GLfloat, r[TEST_COUNT][4]);


static int test_cliptest_function( clip_func func, int np,
				   int psize, long *cycles )
{
   GLvector4f source[1], dest[1], ref[1];
   GLubyte dm[TEST_COUNT], dco, dca;
   GLubyte rm[TEST_COUNT], rco, rca;
   int i, j;
#ifdef  RUN_DEBUG_BENCHMARK
   int cycle_i;                /* the counter for the benchmarks we run */
#endif

   (void) cycles;

   if ( psize > 4 ) {
      _mesa_problem( NULL, "test_cliptest_function called with psize > 4\n" );
      return 0;
   }

   for ( i = 0 ; i < TEST_COUNT ; i++) {
      ASSIGN_4V( d[i], 0.0, 0.0, 0.0, 1.0 );
      ASSIGN_4V( s[i], 0.0, 0.0, 0.0, 1.0 );
      for ( j = 0 ; j < psize ; j++ )
         s[i][j] = rnd();
   }

   source->data = (GLfloat(*)[4])s;
   source->start = (GLfloat *)s;
   source->count = TEST_COUNT;
   source->stride = sizeof(s[0]);
   source->size = 4;
   source->flags = 0;

   dest->data = (GLfloat(*)[4])d;
   dest->start = (GLfloat *)d;
   dest->count = TEST_COUNT;
   dest->stride = sizeof(float[4]);
   dest->size = 0;
   dest->flags = 0;

   ref->data = (GLfloat(*)[4])r;
   ref->start = (GLfloat *)r;
   ref->count = TEST_COUNT;
   ref->stride = sizeof(float[4]);
   ref->size = 0;
   ref->flags = 0;

   dco = rco = 0;
   dca = rca = CLIP_FRUSTUM_BITS;

   ref_cliptest[psize]( source, ref, rm, &rco, &rca );

   if ( mesa_profile ) {
      BEGIN_RACE( *cycles );
      func( source, dest, dm, &dco, &dca );
      END_RACE( *cycles );
   }
   else {
      func( source, dest, dm, &dco, &dca );
   }

   if ( dco != rco ) {
      _mesa_printf( "\n-----------------------------\n" );
      _mesa_printf( "dco = 0x%02x   rco = 0x%02x\n", dco, rco );
      return 0;
   }
   if ( dca != rca ) {
      _mesa_printf( "\n-----------------------------\n" );
      _mesa_printf( "dca = 0x%02x   rca = 0x%02x\n", dca, rca );
      return 0;
   }
   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      if ( dm[i] != rm[i] ) {
	 _mesa_printf( "\n-----------------------------\n" );
	 _mesa_printf( "(i = %i)\n", i );
	 _mesa_printf( "dm = 0x%02x   rm = 0x%02x\n", dm[i], rm[i] );
	 return 0;
      }
   }

   /* Only verify output on projected points4 case.  FIXME: Do we need
    * to test other cases?
    */
   if ( np || psize < 4 )
      return 1;

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      for ( j = 0 ; j < 4 ; j++ ) {
         if ( significand_match( d[i][j], r[i][j] ) < REQUIRED_PRECISION ) {
            _mesa_printf( "\n-----------------------------\n" );
            _mesa_printf( "(i = %i, j = %i)  dm = 0x%02x   rm = 0x%02x\n",
		    i, j, dm[i], rm[i] );
            _mesa_printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][0], r[i][0], r[i][0]-d[i][0],
		    MAX_PRECISION - significand_match( d[i][0], r[i][0] ) );
            _mesa_printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][1], r[i][1], r[i][1]-d[i][1],
		    MAX_PRECISION - significand_match( d[i][1], r[i][1] ) );
            _mesa_printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][2], r[i][2], r[i][2]-d[i][2],
		    MAX_PRECISION - significand_match( d[i][2], r[i][2] ) );
            _mesa_printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][3], r[i][3], r[i][3]-d[i][3],
		    MAX_PRECISION - significand_match( d[i][3], r[i][3] ) );
            return 0;
         }
      }
   }

   return 1;
}

void _math_test_all_cliptest_functions( char *description )
{
   int np, psize;
   long benchmark_tab[2][4];
   static int first_time = 1;

   if ( first_time ) {
      first_time = 0;
      mesa_profile = _mesa_getenv( "MESA_PROFILE" );
   }

#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      if ( !counter_overhead ) {
	 INIT_COUNTER();
	 _mesa_printf( "counter overhead: %ld cycles\n\n", counter_overhead );
      }
      _mesa_printf( "cliptest results after hooking in %s functions:\n", description );
   }
#endif

#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      _mesa_printf( "\n\t" );
      for ( psize = 2 ; psize <= 4 ; psize++ ) {
	 _mesa_printf( " p%d\t", psize );
      }
      _mesa_printf( "\n--------------------------------------------------------\n\t" );
   }
#endif

   for ( np = 0 ; np < 2 ; np++ ) {
      for ( psize = 2 ; psize <= 4 ; psize++ ) {
	 clip_func func = clip_tab[np][psize];
	 long *cycles = &(benchmark_tab[np][psize-1]);

	 if ( test_cliptest_function( func, np, psize, cycles ) == 0 ) {
	    char buf[100];
	    _mesa_sprintf( buf, "%s[%d] failed test (%s)",
		     cnames[np], psize, description );
	    _mesa_problem( NULL, buf );
	 }
#ifdef RUN_DEBUG_BENCHMARK
	 if ( mesa_profile )
	    _mesa_printf( " %li\t", benchmark_tab[np][psize-1] );
#endif
      }
#ifdef RUN_DEBUG_BENCHMARK
      if ( mesa_profile )
	 _mesa_printf( " | [%s]\n\t", cstrings[np] );
#endif
   }
#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile )
      _mesa_printf( "\n" );
#endif
}


#endif /* DEBUG_MATH */
