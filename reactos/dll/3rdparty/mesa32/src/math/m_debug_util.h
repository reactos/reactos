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
 *
 * Authors:
 *    Gareth Hughes
 */

#ifndef __M_DEBUG_UTIL_H__
#define __M_DEBUG_UTIL_H__


#ifdef DEBUG_MATH  /* This code only used for debugging */


/* Comment this out to deactivate the cycle counter.
 * NOTE: it works only on CPUs which know the 'rdtsc' command (586 or higher)
 * (hope, you don't try to debug Mesa on a 386 ;)
 */
#if defined(__GNUC__) && \
    ((defined(__i386__) && defined(USE_X86_ASM)) || \
     (defined(__sparc__) && defined(USE_SPARC_ASM)))
#define  RUN_DEBUG_BENCHMARK
#endif

#define TEST_COUNT		128	/* size of the tested vector array   */

#define REQUIRED_PRECISION	10	/* allow 4 bits to miss              */
#define MAX_PRECISION		24	/* max. precision possible           */


#ifdef  RUN_DEBUG_BENCHMARK
/* Overhead of profiling counter in cycles.  Automatically adjusted to
 * your machine at run time - counter initialization should give very
 * consistent results.
 */
extern long counter_overhead;

/* This is the value of the environment variable MESA_PROFILE, and is
 * used to determine if we should benchmark the functions as well as
 * verify their correctness.
 */
extern char *mesa_profile;

/* Modify the the number of tests if you like.
 * We take the minimum of all results, because every error should be
 * positive (time used by other processes, task switches etc).
 * It is assumed that all calculations are done in the cache.
 */

#if defined(__i386__)

#if 1 /* PPro, PII, PIII version */

/* Profiling on the P6 architecture requires a little more work, due to
 * the internal out-of-order execution.  We must perform a serializing
 * 'cpuid' instruction before and after the 'rdtsc' instructions to make
 * sure no other uops are executed when we sample the timestamp counter.
 */
#define  INIT_COUNTER()							\
   do {									\
      int cycle_i;							\
      counter_overhead = LONG_MAX;					\
      for ( cycle_i = 0 ; cycle_i < 8 ; cycle_i++ ) {			\
	 long cycle_tmp1 = 0, cycle_tmp2 = 0;				\
	 __asm__ __volatile__ ( "push %%ebx       \n"			\
				"xor %%eax, %%eax \n"			\
				"cpuid            \n"			\
				"rdtsc            \n"			\
				"mov %%eax, %0    \n"			\
				"xor %%eax, %%eax \n"			\
				"cpuid            \n"			\
				"pop %%ebx        \n"			\
				"push %%ebx       \n"			\
				"xor %%eax, %%eax \n"			\
				"cpuid            \n"			\
				"rdtsc            \n"			\
				"mov %%eax, %1    \n"			\
				"xor %%eax, %%eax \n"			\
				"cpuid            \n"			\
				"pop %%ebx        \n"			\
				: "=m" (cycle_tmp1), "=m" (cycle_tmp2)	\
				: : "eax", "ecx", "edx" );		\
	 if ( counter_overhead > (cycle_tmp2 - cycle_tmp1) ) {		\
	    counter_overhead = cycle_tmp2 - cycle_tmp1;			\
	 }								\
      }									\
   } while (0)

#define  BEGIN_RACE(x)							\
   x = LONG_MAX;							\
   for ( cycle_i = 0 ; cycle_i < 10 ; cycle_i++ ) {			\
      long cycle_tmp1 = 0, cycle_tmp2 = 0;				\
      __asm__ __volatile__ ( "push %%ebx       \n"			\
			     "xor %%eax, %%eax \n"			\
			     "cpuid            \n"			\
			     "rdtsc            \n"			\
			     "mov %%eax, %0    \n"			\
			     "xor %%eax, %%eax \n"			\
			     "cpuid            \n"			\
			     "pop %%ebx        \n"			\
			     : "=m" (cycle_tmp1)			\
			     : : "eax", "ecx", "edx" );

#define END_RACE(x)							\
      __asm__ __volatile__ ( "push %%ebx       \n"			\
			     "xor %%eax, %%eax \n"			\
			     "cpuid            \n"			\
			     "rdtsc            \n"			\
			     "mov %%eax, %0    \n"			\
			     "xor %%eax, %%eax \n"			\
			     "cpuid            \n"			\
			     "pop %%ebx        \n"			\
			     : "=m" (cycle_tmp2)			\
			     : : "eax", "ecx", "edx" );			\
      if ( x > (cycle_tmp2 - cycle_tmp1) ) {				\
	 x = cycle_tmp2 - cycle_tmp1;					\
      }									\
   }									\
   x -= counter_overhead;

#else /* PPlain, PMMX version */

/* To ensure accurate results, we stall the pipelines with the
 * non-pairable 'cdq' instruction.  This ensures all the code being
 * profiled is complete when the 'rdtsc' instruction executes.
 */
#define  INIT_COUNTER(x)						\
   do {									\
      int cycle_i;							\
      x = LONG_MAX;							\
      for ( cycle_i = 0 ; cycle_i < 32 ; cycle_i++ ) {			\
	 long cycle_tmp1, cycle_tmp2, dummy;				\
	 __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp1) );		\
	 __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp2) );		\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "rdtsc" : "=a" (cycle_tmp1), "=d" (dummy) );		\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "rdtsc" : "=a" (cycle_tmp2), "=d" (dummy) );		\
	 if ( x > (cycle_tmp2 - cycle_tmp1) )				\
	    x = cycle_tmp2 - cycle_tmp1;				\
      }									\
   } while (0)

#define  BEGIN_RACE(x)							\
   x = LONG_MAX;							\
   for ( cycle_i = 0 ; cycle_i < 16 ; cycle_i++ ) {			\
      long cycle_tmp1, cycle_tmp2, dummy;				\
      __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp1) );			\
      __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp2) );			\
      __asm__ ( "cdq" );						\
      __asm__ ( "cdq" );						\
      __asm__ ( "rdtsc" : "=a" (cycle_tmp1), "=d" (dummy) );


#define END_RACE(x)							\
      __asm__ ( "cdq" );						\
      __asm__ ( "cdq" );						\
      __asm__ ( "rdtsc" : "=a" (cycle_tmp2), "=d" (dummy) );		\
      if ( x > (cycle_tmp2 - cycle_tmp1) )				\
	 x = cycle_tmp2 - cycle_tmp1;					\
   }									\
   x -= counter_overhead;

#endif

#elif defined(__amd64__)

#define rdtscll(val) do { \
     unsigned int a,d; \
     __asm__ volatile("rdtsc" : "=a" (a), "=d" (d)); \
     (val) = ((unsigned long)a) | (((unsigned long)d)<<32); \
} while(0) 

/* Copied from i386 PIII version */
#define  INIT_COUNTER()							\
   do {									\
      int cycle_i;							\
      counter_overhead = LONG_MAX;					\
      for ( cycle_i = 0 ; cycle_i < 16 ; cycle_i++ ) {			\
	 unsigned long cycle_tmp1, cycle_tmp2;        			\
	 rdtscll(cycle_tmp1);						\
	 rdtscll(cycle_tmp2);						\
	 if ( counter_overhead > (cycle_tmp2 - cycle_tmp1) ) {		\
	    counter_overhead = cycle_tmp2 - cycle_tmp1;			\
	 }								\
      }									\
   } while (0)


#define  BEGIN_RACE(x)							\
   x = LONG_MAX;							\
   for ( cycle_i = 0 ; cycle_i < 10 ; cycle_i++ ) {			\
      unsigned long cycle_tmp1, cycle_tmp2;				\
      rdtscll(cycle_tmp1);						\

#define END_RACE(x)							\
      rdtscll(cycle_tmp2);						\
      if ( x > (cycle_tmp2 - cycle_tmp1) ) {				\
	 x = cycle_tmp2 - cycle_tmp1;					\
      }									\
   }									\
   x -= counter_overhead;

#elif defined(__sparc__)

#define  INIT_COUNTER()	\
	 do { counter_overhead = 5; } while(0)

#define  BEGIN_RACE(x)                                                        \
x = LONG_MAX;                                                                 \
for (cycle_i = 0; cycle_i <10; cycle_i++) {                                   \
   register long cycle_tmp1 asm("l0");					      \
   register long cycle_tmp2 asm("l1");					      \
   /* rd %tick, %l0 */							      \
   __asm__ __volatile__ (".word 0xa1410000" : "=r" (cycle_tmp1));  /*  save timestamp   */

#define END_RACE(x)                                                           \
   /* rd %tick, %l1 */							      \
   __asm__ __volatile__ (".word 0xa3410000" : "=r" (cycle_tmp2));	      \
   if (x > (cycle_tmp2-cycle_tmp1)) x = cycle_tmp2 - cycle_tmp1;              \
}                                                                             \
x -= counter_overhead;

#else
#error Your processor is not supported for RUN_XFORM_BENCHMARK
#endif

#else

#define BEGIN_RACE(x)
#define END_RACE(x)

#endif


/* =============================================================
 * Helper functions
 */

static GLfloat rnd( void )
{
   GLfloat f = (GLfloat)rand() / (GLfloat)RAND_MAX;
   GLfloat gran = (GLfloat)(1 << 13);

   f = (GLfloat)(GLint)(f * gran) / gran;

   return f * 2.0 - 1.0;
}

static int significand_match( GLfloat a, GLfloat b )
{
   GLfloat d = a - b;
   int a_ex, b_ex, d_ex;

   if ( d == 0.0F ) {
      return MAX_PRECISION;   /* Exact match */
   }

   if ( a == 0.0F || b == 0.0F ) {
      /* It would probably be better to check if the
       * non-zero number is denormalized and return
       * the index of the highest set bit here.
       */
      return 0;
   }

   FREXPF( a, &a_ex );
   FREXPF( b, &b_ex );
   FREXPF( d, &d_ex );

   if ( a_ex < b_ex ) {
      return a_ex - d_ex;
   } else {
      return b_ex - d_ex;
   }
}

enum { NIL = 0, ONE = 1, NEG = -1, VAR = 2 };

/* Ensure our arrays are correctly aligned.
 */
#if defined(__GNUC__)
#  define ALIGN16(type, array)	type array __attribute__ ((aligned (16)))
#elif defined(__MSC__)
#  define ALIGN16(type, array)	type array __declspec(align(16)) /* GH: Does this work? */
#elif defined(__WATCOMC__)
#  define ALIGN16(type, array)	                    /* Watcom does not support this */ 
#elif defined(__xlC__)
#  define ALIGN16(type, array)       type __align (16) array 
#else
#  warning "ALIGN16 will not 16-byte align!\n"
#  define ALIGN16
#endif


#endif /* DEBUG_MATH */

#endif /* __M_DEBUG_UTIL_H__ */
