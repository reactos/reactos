/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/*
 * Updated for P6 architecture by Gareth Hughes.
 */

#include <precomp.h>

#include "m_debug.h"
#include "m_debug_util.h"

#ifdef __UNIXOS2__
/* The linker doesn't like empty files */
static char dummy;
#endif

#ifdef DEBUG_MATH  /* This code only used for debugging */


/* Overhead of profiling counter in cycles.  Automatically adjusted to
 * your machine at run time - counter initialization should give very
 * consistent results.
 */
long counter_overhead = 0;

/* This is the value of the environment variable MESA_PROFILE, and is
 * used to determine if we should benchmark the functions as well as
 * verify their correctness.
 */
char *mesa_profile = NULL;


static int m_general[16] = {
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR
};
static int m_identity[16] = {
   ONE, NIL, NIL, NIL,
   NIL, ONE, NIL, NIL,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int  m_2d[16]  = {
   VAR, VAR, NIL, VAR,
   VAR, VAR, NIL, VAR,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int m_2d_no_rot[16] = {
   VAR, NIL, NIL, VAR,
   NIL, VAR, NIL, VAR,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int m_3d[16] = {
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   NIL, NIL, NIL, ONE
};
static int m_3d_no_rot[16] = {
   VAR, NIL, NIL, VAR,
   NIL, VAR, NIL, VAR,
   NIL, NIL, VAR, VAR,
   NIL, NIL, NIL, ONE
};
static int m_perspective[16] = {
   VAR, NIL, VAR, NIL,
   NIL, VAR, VAR, NIL,
   NIL, NIL, VAR, VAR,
   NIL, NIL, NEG, NIL
};
static int *templates[7] = {
   m_general,
   m_identity,
   m_3d_no_rot,
   m_perspective,
   m_2d,
   m_2d_no_rot,
   m_3d
};
static enum GLmatrixtype mtypes[7] = {
   MATRIX_GENERAL,
   MATRIX_IDENTITY,
   MATRIX_3D_NO_ROT,
   MATRIX_PERSPECTIVE,
   MATRIX_2D,
   MATRIX_2D_NO_ROT,
   MATRIX_3D
};
static char *mstrings[7] = {
   "MATRIX_GENERAL",
   "MATRIX_IDENTITY",
   "MATRIX_3D_NO_ROT",
   "MATRIX_PERSPECTIVE",
   "MATRIX_2D",
   "MATRIX_2D_NO_ROT",
   "MATRIX_3D"
};


/* =============================================================
 * Reference transformations
 */

static void ref_transform( GLvector4f *dst,
                           const GLmatrix *mat,
                           const GLvector4f *src )
{
   GLuint i;
   GLfloat *s = (GLfloat *)src->start;
   GLfloat (*d)[4] = (GLfloat (*)[4])dst->start;
   const GLfloat *m = mat->m;

   for ( i = 0 ; i < src->count ; i++ ) {
      TRANSFORM_POINT( d[i], m, s );
      s = (GLfloat *)((char *)s + src->stride);
   }
}


/* =============================================================
 * Vertex transformation tests
 */

static void init_matrix( GLfloat *m )
{
   m[0] = 63.0; m[4] = 43.0; m[ 8] = 29.0; m[12] = 43.0;
   m[1] = 55.0; m[5] = 17.0; m[ 9] = 31.0; m[13] =  7.0;
   m[2] = 44.0; m[6] =  9.0; m[10] =  7.0; m[14] =  3.0;
   m[3] = 11.0; m[7] = 23.0; m[11] = 91.0; m[15] =  9.0;
}

ALIGN16(static GLfloat, s[TEST_COUNT][4]);
ALIGN16(static GLfloat, d[TEST_COUNT][4]);
ALIGN16(static GLfloat, r[TEST_COUNT][4]);

static int test_transform_function( transform_func func, int psize,
				    int mtype, unsigned long *cycles )
{
   GLvector4f source[1], dest[1], ref[1];
   GLmatrix mat[1];
   GLfloat *m;
   int i, j;
#ifdef  RUN_DEBUG_BENCHMARK
   int cycle_i;                /* the counter for the benchmarks we run */
#endif

   (void) cycles;

   if ( psize > 4 ) {
      _mesa_problem( NULL, "test_transform_function called with psize > 4\n" );
      return 0;
   }

   mat->m = (GLfloat *) _mesa_align_malloc( 16 * sizeof(GLfloat), 16 );
   mat->type = mtypes[mtype];

   m = mat->m;
   ASSERT( ((long)m & 15) == 0 );

   init_matrix( m );

   for ( i = 0 ; i < 4 ; i++ ) {
      for ( j = 0 ; j < 4 ; j++ ) {
         switch ( templates[mtype][i * 4 + j] ) {
         case NIL:
            m[j * 4 + i] = 0.0;
            break;
         case ONE:
            m[j * 4 + i] = 1.0;
            break;
         case NEG:
            m[j * 4 + i] = -1.0;
            break;
         case VAR:
            break;
         default:
            ASSERT(0);
            return 0;
         }
      }
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

   ref_transform( ref, mat, source );

   if ( mesa_profile ) {
      BEGIN_RACE( *cycles );
      func( dest, mat->m, source );
      END_RACE( *cycles );
   }
   else {
      func( dest, mat->m, source );
   }

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      for ( j = 0 ; j < 4 ; j++ ) {
         if ( significand_match( d[i][j], r[i][j] ) < REQUIRED_PRECISION ) {
            printf("-----------------------------\n" );
            printf("(i = %i, j = %i)\n", i, j );
            printf("%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][0], r[i][0], r[i][0]-d[i][0],
		    MAX_PRECISION - significand_match( d[i][0], r[i][0] ) );
            printf("%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][1], r[i][1], r[i][1]-d[i][1],
		    MAX_PRECISION - significand_match( d[i][1], r[i][1] ) );
            printf("%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][2], r[i][2], r[i][2]-d[i][2],
		    MAX_PRECISION - significand_match( d[i][2], r[i][2] ) );
            printf("%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][3], r[i][3], r[i][3]-d[i][3],
		    MAX_PRECISION - significand_match( d[i][3], r[i][3] ) );
            return 0;
         }
      }
   }

   _mesa_align_free( mat->m );
   return 1;
}

void _math_test_all_transform_functions( char *description )
{
   int psize, mtype;
   unsigned long benchmark_tab[4][7];
   static int first_time = 1;

   if ( first_time ) {
      first_time = 0;
      mesa_profile = _mesa_getenv( "MESA_PROFILE" );
   }

#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      if ( !counter_overhead ) {
	 INIT_COUNTER();
	 printf("counter overhead: %lu cycles\n\n", counter_overhead );
      }
      printf("transform results after hooking in %s functions:\n", description );
   }
#endif

#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      printf("\n" );
      for ( psize = 1 ; psize <= 4 ; psize++ ) {
	 printf(" p%d\t", psize );
      }
      printf("\n--------------------------------------------------------\n" );
   }
#endif

   for ( mtype = 0 ; mtype < 7 ; mtype++ ) {
      for ( psize = 1 ; psize <= 4 ; psize++ ) {
	 transform_func func = _mesa_transform_tab[psize][mtypes[mtype]];
	 unsigned long *cycles = &(benchmark_tab[psize-1][mtype]);

	 if ( test_transform_function( func, psize, mtype, cycles ) == 0 ) {
	    char buf[100];
	    sprintf(buf, "_mesa_transform_tab[0][%d][%s] failed test (%s)",
		    psize, mstrings[mtype], description );
	    _mesa_problem( NULL, "%s", buf );
	 }
#ifdef RUN_DEBUG_BENCHMARK
	 if ( mesa_profile )
	    printf(" %li\t", benchmark_tab[psize-1][mtype] );
#endif
      }
#ifdef RUN_DEBUG_BENCHMARK
      if ( mesa_profile )
	 printf(" | [%s]\n", mstrings[mtype] );
#endif
   }
#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile )
      printf( "\n" );
#endif
}


#endif /* DEBUG_MATH */
