/**
 * \file imports.c
 * Standard C library function wrappers.
 * 
 * Imports are services which the device driver or window system or
 * operating system provides to the core renderer.  The core renderer (Mesa)
 * will call these functions in order to do memory allocation, simple I/O,
 * etc.
 *
 * Some drivers will want to override/replace this file with something
 * specialized, but that'll be rare.
 *
 * Eventually, I want to move roll the glheader.h file into this.
 *
 * \todo Functions still needed:
 * - scanf
 * - qsort
 * - rand and RAND_MAX
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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



#include "imports.h"
#include "context.h"
#include "version.h"


#define MAXSTRING 4000  /* for vsnprintf() */

#ifdef WIN32
#define vsnprintf _vsnprintf
#elif defined(__IBMC__) || defined(__IBMCPP__) || ( defined(__VMS) && __CRTL_VER < 70312000 )
extern int vsnprintf(char *str, size_t count, const char *fmt, va_list arg);
#ifdef __VMS
#include "vsnprintf.c"
#endif
#endif

/**********************************************************************/
/** \name Memory */
/*@{*/

/** Wrapper around malloc() */
void *
_mesa_malloc(size_t bytes)
{
   return malloc(bytes);
}

/** Wrapper around calloc() */
void *
_mesa_calloc(size_t bytes)
{
   return calloc(1, bytes);
}

/** Wrapper around free() */
void
_mesa_free(void *ptr)
{
   free(ptr);
}

/**
 * Allocate aligned memory.
 *
 * \param bytes number of bytes to allocate.
 * \param alignment alignment (must be greater than zero).
 * 
 * Allocates extra memory to accommodate rounding up the address for
 * alignment and to record the real malloc address.
 *
 * \sa _mesa_align_free().
 */
void *
_mesa_align_malloc(size_t bytes, unsigned long alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;

   (void) posix_memalign(& mem, alignment, bytes);
   return mem;
#elif defined(_WIN32) && defined(_MSC_VER)
   return _aligned_malloc(bytes, alignment);
#else
   uintptr_t ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (uintptr_t) _mesa_malloc(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(uintptr_t)(alignment - 1);
   *(uintptr_t *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *) buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Same as _mesa_align_malloc(), but using _mesa_calloc() instead of
 * _mesa_malloc()
 */
void *
_mesa_align_calloc(size_t bytes, unsigned long alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;
   
   mem = _mesa_align_malloc(bytes, alignment);
   if (mem != NULL) {
      (void) memset(mem, 0, bytes);
   }

   return mem;
#elif defined(_WIN32) && defined(_MSC_VER)
   void *mem;

   mem = _aligned_malloc(bytes, alignment);
   if (mem != NULL) {
      (void) memset(mem, 0, bytes);
   }

   return mem;
#else
   uintptr_t ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (uintptr_t) _mesa_calloc(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(uintptr_t)(alignment - 1);
   *(uintptr_t *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *)buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Free memory which was allocated with either _mesa_align_malloc()
 * or _mesa_align_calloc().
 * \param ptr pointer to the memory to be freed.
 * The actual address to free is stored in the word immediately before the
 * address the client sees.
 */
void
_mesa_align_free(void *ptr)
{
#if defined(HAVE_POSIX_MEMALIGN)
   free(ptr);
#elif defined(_WIN32) && defined(_MSC_VER)
   _aligned_free(ptr);
#else
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   _mesa_free(realAddr);
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Reallocate memory, with alignment.
 */
void *
_mesa_align_realloc(void *oldBuffer, size_t oldSize, size_t newSize,
                    unsigned long alignment)
{
#if defined(_WIN32) && defined(_MSC_VER)
   (void) oldSize;
   return _aligned_realloc(oldBuffer, newSize, alignment);
#else
   const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
   void *newBuf = _mesa_align_malloc(newSize, alignment);
   if (newBuf && oldBuffer && copySize > 0) {
      _mesa_memcpy(newBuf, oldBuffer, copySize);
   }
   if (oldBuffer)
      _mesa_align_free(oldBuffer);
   return newBuf;
#endif
}



/** Reallocate memory */
void *
_mesa_realloc(void *oldBuffer, size_t oldSize, size_t newSize)
{
   const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
   void *newBuffer = _mesa_malloc(newSize);
   if (newBuffer && oldBuffer && copySize > 0)
      _mesa_memcpy(newBuffer, oldBuffer, copySize);
   if (oldBuffer)
      _mesa_free(oldBuffer);
   return newBuffer;
}

/** memcpy wrapper */
void *
_mesa_memcpy(void *dest, const void *src, size_t n)
{
#if defined(SUNOS4)
   return memcpy((char *) dest, (char *) src, (int) n);
#else
   return memcpy(dest, src, n);
#endif
}

/** Wrapper around memset() */
void
_mesa_memset( void *dst, int val, size_t n )
{
#if defined(SUNOS4)
   memset( (char *) dst, (int) val, (int) n );
#else
   memset(dst, val, n);
#endif
}

/**
 * Fill memory with a constant 16bit word.
 * \param dst destination pointer.
 * \param val value.
 * \param n number of words.
 */
void
_mesa_memset16( unsigned short *dst, unsigned short val, size_t n )
{
   while (n-- > 0)
      *dst++ = val;
}

/** Wrapper around either memset() or bzero() */
void
_mesa_bzero( void *dst, size_t n )
{
#if defined(__FreeBSD__)
   bzero( dst, n );
#else
   memset( dst, 0, n );
#endif
}

/** Wrapper around memcmp() */
int
_mesa_memcmp( const void *s1, const void *s2, size_t n )
{
#if defined(SUNOS4)
   return memcmp( (char *) s1, (char *) s2, (int) n );
#else
   return memcmp(s1, s2, n);
#endif
}

/*@}*/


/**********************************************************************/
/** \name Math */
/*@{*/

/** Wrapper around sin() */
double
_mesa_sin(double a)
{
   return sin(a);
}

/** Single precision wrapper around sin() */
float
_mesa_sinf(float a)
{
   return (float) sin((double) a);
}

/** Wrapper around cos() */
double
_mesa_cos(double a)
{
   return cos(a);
}

/** Single precision wrapper around asin() */
float
_mesa_asinf(float x)
{
   return (float) asin((double) x);
}

/** Single precision wrapper around atan() */
float
_mesa_atanf(float x)
{
   return (float) atan((double) x);
}

/** Wrapper around sqrt() */
double
_mesa_sqrtd(double x)
{
   return sqrt(x);
}


/*
 * A High Speed, Low Precision Square Root
 * by Paul Lalonde and Robert Dawson
 * from "Graphics Gems", Academic Press, 1990
 *
 * SPARC implementation of a fast square root by table
 * lookup.
 * SPARC floating point format is as follows:
 *
 * BIT 31 	30 	23 	22 	0
 *     sign	exponent	mantissa
 */
static short sqrttab[0x100];    /* declare table of square roots */

void
_mesa_init_sqrt_table(void)
{
#if defined(USE_IEEE) && !defined(DEBUG)
   unsigned short i;
   fi_type fi;     /* to access the bits of a float in  C quickly  */
                   /* we use a union defined in glheader.h         */

   for(i=0; i<= 0x7f; i++) {
      fi.i = 0;

      /*
       * Build a float with the bit pattern i as mantissa
       * and an exponent of 0, stored as 127
       */

      fi.i = (i << 16) | (127 << 23);
      fi.f = _mesa_sqrtd(fi.f);

      /*
       * Take the square root then strip the first 7 bits of
       * the mantissa into the table
       */

      sqrttab[i] = (fi.i & 0x7fffff) >> 16;

      /*
       * Repeat the process, this time with an exponent of
       * 1, stored as 128
       */

      fi.i = 0;
      fi.i = (i << 16) | (128 << 23);
      fi.f = sqrt(fi.f);
      sqrttab[i+0x80] = (fi.i & 0x7fffff) >> 16;
   }
#else
   (void) sqrttab;  /* silence compiler warnings */
#endif /*HAVE_FAST_MATH*/
}


/**
 * Single precision square root.
 */
float
_mesa_sqrtf( float x )
{
#if defined(USE_IEEE) && !defined(DEBUG)
   fi_type num;
                                /* to access the bits of a float in C
                                 * we use a union from glheader.h     */

   short e;                     /* the exponent */
   if (x == 0.0F) return 0.0F;  /* check for square root of 0 */
   num.f = x;
   e = (num.i >> 23) - 127;     /* get the exponent - on a SPARC the */
                                /* exponent is stored with 127 added */
   num.i &= 0x7fffff;           /* leave only the mantissa */
   if (e & 0x01) num.i |= 0x800000;
                                /* the exponent is odd so we have to */
                                /* look it up in the second half of  */
                                /* the lookup table, so we set the   */
                                /* high bit                                */
   e >>= 1;                     /* divide the exponent by two */
                                /* note that in C the shift */
                                /* operators are sign preserving */
                                /* for signed operands */
   /* Do the table lookup, based on the quaternary mantissa,
    * then reconstruct the result back into a float
    */
   num.i = ((sqrttab[num.i >> 16]) << 16) | ((e + 127) << 23);

   return num.f;
#else
   return (float) _mesa_sqrtd((double) x);
#endif
}


/**
 inv_sqrt - A single precision 1/sqrt routine for IEEE format floats.
 written by Josh Vanderhoof, based on newsgroup posts by James Van Buskirk
 and Vesa Karvonen.
*/
float
_mesa_inv_sqrtf(float n)
{
#if defined(USE_IEEE) && !defined(DEBUG)
        float r0, x0, y0;
        float r1, x1, y1;
        float r2, x2, y2;
#if 0 /* not used, see below -BP */
        float r3, x3, y3;
#endif
        union { float f; unsigned int i; } u;
        unsigned int magic;

        /*
         Exponent part of the magic number -

         We want to:
         1. subtract the bias from the exponent,
         2. negate it
         3. divide by two (rounding towards -inf)
         4. add the bias back

         Which is the same as subtracting the exponent from 381 and dividing
         by 2.

         floor(-(x - 127) / 2) + 127 = floor((381 - x) / 2)
        */

        magic = 381 << 23;

        /*
         Significand part of magic number -

         With the current magic number, "(magic - u.i) >> 1" will give you:

         for 1 <= u.f <= 2: 1.25 - u.f / 4
         for 2 <= u.f <= 4: 1.00 - u.f / 8

         This isn't a bad approximation of 1/sqrt.  The maximum difference from
         1/sqrt will be around .06.  After three Newton-Raphson iterations, the
         maximum difference is less than 4.5e-8.  (Which is actually close
         enough to make the following bias academic...)

         To get a better approximation you can add a bias to the magic
         number.  For example, if you subtract 1/2 of the maximum difference in
         the first approximation (.03), you will get the following function:

         for 1 <= u.f <= 2:    1.22 - u.f / 4
         for 2 <= u.f <= 3.76: 0.97 - u.f / 8
         for 3.76 <= u.f <= 4: 0.72 - u.f / 16
         (The 3.76 to 4 range is where the result is < .5.)

         This is the closest possible initial approximation, but with a maximum
         error of 8e-11 after three NR iterations, it is still not perfect.  If
         you subtract 0.0332281 instead of .03, the maximum error will be
         2.5e-11 after three NR iterations, which should be about as close as
         is possible.

         for 1 <= u.f <= 2:    1.2167719 - u.f / 4
         for 2 <= u.f <= 3.73: 0.9667719 - u.f / 8
         for 3.73 <= u.f <= 4: 0.7167719 - u.f / 16

        */

        magic -= (int)(0.0332281 * (1 << 25));

        u.f = n;
        u.i = (magic - u.i) >> 1;

        /*
         Instead of Newton-Raphson, we use Goldschmidt's algorithm, which
         allows more parallelism.  From what I understand, the parallelism
         comes at the cost of less precision, because it lets error
         accumulate across iterations.
        */
        x0 = 1.0f;
        y0 = 0.5f * n;
        r0 = u.f;

        x1 = x0 * r0;
        y1 = y0 * r0 * r0;
        r1 = 1.5f - y1;

        x2 = x1 * r1;
        y2 = y1 * r1 * r1;
        r2 = 1.5f - y2;

#if 1
        return x2 * r2;  /* we can stop here, and be conformant -BP */
#else
        x3 = x2 * r2;
        y3 = y2 * r2 * r2;
        r3 = 1.5f - y3;

        return x3 * r3;
#endif
#else
        return (float) (1.0 / sqrt(n));
#endif
}


/** Wrapper around pow() */
double
_mesa_pow(double x, double y)
{
   return pow(x, y);
}


/**
 * Find the first bit set in a word.
 */
int
_mesa_ffs(int i)
{
#if (defined(_WIN32) ) || defined(__IBMC__) || defined(__IBMCPP__)
   register int bit = 0;
   if (i != 0) {
      if ((i & 0xffff) == 0) {
         bit += 16;
         i >>= 16;
      }
      if ((i & 0xff) == 0) {
         bit += 8;
         i >>= 8;
      }
      if ((i & 0xf) == 0) {
         bit += 4;
         i >>= 4;
      }
      while ((i & 1) == 0) {
         bit++;
         i >>= 1;
      }
      bit++;
   }
   return bit;
#else
   return ffs(i);
#endif
}


/**
 * Find position of first bit set in given value.
 * XXX Warning: this function can only be used on 64-bit systems!
 * \return  position of least-significant bit set, starting at 1, return zero
 *          if no bits set.
 */
int
#ifdef __MINGW32__
_mesa_ffsll(long val)
#else
_mesa_ffsll(long long val)
#endif
{
#ifdef ffsll
   return ffsll(val);
#else
   int bit;

   assert(sizeof(val) == 8);

   bit = _mesa_ffs(val);
   if (bit != 0)
      return bit;

   bit = _mesa_ffs(val >> 32);
   if (bit != 0)
      return 32 + bit;

   return 0;
#endif
}


/**
 * Return number of bits set in given GLuint.
 */
unsigned int
_mesa_bitcount(unsigned int n)
{
   unsigned int bits;
   for (bits = 0; n > 0; n = n >> 1) {
      bits += (n & 1);
   }
   return bits;
}


/**
 * Convert a 4-byte float to a 2-byte half float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
GLhalfARB
_mesa_float_to_half(float val)
{
   const int flt = *((int *) (void *) &val);
   const int flt_m = flt & 0x7fffff;
   const int flt_e = (flt >> 23) & 0xff;
   const int flt_s = (flt >> 31) & 0x1;
   int s, e, m = 0;
   GLhalfARB result;
   
   /* sign bit */
   s = flt_s;

   /* handle special cases */
   if ((flt_e == 0) && (flt_m == 0)) {
      /* zero */
      /* m = 0; - already set */
      e = 0;
   }
   else if ((flt_e == 0) && (flt_m != 0)) {
      /* denorm -- denorm float maps to 0 half */
      /* m = 0; - already set */
      e = 0;
   }
   else if ((flt_e == 0xff) && (flt_m == 0)) {
      /* infinity */
      /* m = 0; - already set */
      e = 31;
   }
   else if ((flt_e == 0xff) && (flt_m != 0)) {
      /* NaN */
      m = 1;
      e = 31;
   }
   else {
      /* regular number */
      const int new_exp = flt_e - 127;
      if (new_exp < -24) {
         /* this maps to 0 */
         /* m = 0; - already set */
         e = 0;
      }
      else if (new_exp < -14) {
         /* this maps to a denorm */
         unsigned int exp_val = (unsigned int) (-14 - new_exp); /* 2^-exp_val*/
         e = 0;
         switch (exp_val) {
            case 0:
               _mesa_warning(NULL,
                   "float_to_half: logical error in denorm creation!\n");
               /* m = 0; - already set */
               break;
            case 1: m = 512 + (flt_m >> 14); break;
            case 2: m = 256 + (flt_m >> 15); break;
            case 3: m = 128 + (flt_m >> 16); break;
            case 4: m = 64 + (flt_m >> 17); break;
            case 5: m = 32 + (flt_m >> 18); break;
            case 6: m = 16 + (flt_m >> 19); break;
            case 7: m = 8 + (flt_m >> 20); break;
            case 8: m = 4 + (flt_m >> 21); break;
            case 9: m = 2 + (flt_m >> 22); break;
            case 10: m = 1; break;
         }
      }
      else if (new_exp > 15) {
         /* map this value to infinity */
         /* m = 0; - already set */
         e = 31;
      }
      else {
         /* regular */
         e = new_exp + 15;
         m = flt_m >> 13;
      }
   }

   result = (s << 15) | (e << 10) | m;
   return result;
}


/**
 * Convert a 2-byte half float to a 4-byte float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
float
_mesa_half_to_float(GLhalfARB val)
{
   /* XXX could also use a 64K-entry lookup table */
   const int m = val & 0x3ff;
   const int e = (val >> 10) & 0x1f;
   const int s = (val >> 15) & 0x1;
   int flt_m, flt_e, flt_s, flt;
   float result;

   /* sign bit */
   flt_s = s;

   /* handle special cases */
   if ((e == 0) && (m == 0)) {
      /* zero */
      flt_m = 0;
      flt_e = 0;
   }
   else if ((e == 0) && (m != 0)) {
      /* denorm -- denorm half will fit in non-denorm single */
      const float half_denorm = 1.0f / 16384.0f; /* 2^-14 */
      float mantissa = ((float) (m)) / 1024.0f;
      float sign = s ? -1.0f : 1.0f;
      return sign * mantissa * half_denorm;
   }
   else if ((e == 31) && (m == 0)) {
      /* infinity */
      flt_e = 0xff;
      flt_m = 0;
   }
   else if ((e == 31) && (m != 0)) {
      /* NaN */
      flt_e = 0xff;
      flt_m = 1;
   }
   else {
      /* regular */
      flt_e = e + 112;
      flt_m = m << 13;
   }

   flt = (flt_s << 31) | (flt_e << 23) | flt_m;
   result = *((float *) (void *) &flt);
   return result;
}

/*@}*/


/**********************************************************************/
/** \name Sort & Search */
/*@{*/

/**
 * Wrapper for bsearch().
 */
void *
_mesa_bsearch( const void *key, const void *base, size_t nmemb, size_t size, 
               int (*compar)(const void *, const void *) )
{
   return bsearch(key, base, nmemb, size, compar);
}

/*@}*/


/**********************************************************************/
/** \name Environment vars */
/*@{*/

/**
 * Wrapper for getenv().
 */
char *
_mesa_getenv( const char *var )
{
#if defined(_XBOX)
   return NULL;
#else
   return getenv(var);
#endif
}

/*@}*/


/**********************************************************************/
/** \name String */
/*@{*/

/** Wrapper around strstr() */
char *
_mesa_strstr( const char *haystack, const char *needle )
{
   return strstr(haystack, needle);
}

/** Wrapper around strncat() */
char *
_mesa_strncat( char *dest, const char *src, size_t n )
{
   return strncat(dest, src, n);
}

/** Wrapper around strcpy() */
char *
_mesa_strcpy( char *dest, const char *src )
{
   return strcpy(dest, src);
}

/** Wrapper around strncpy() */
char *
_mesa_strncpy( char *dest, const char *src, size_t n )
{
   return strncpy(dest, src, n);
}

/** Wrapper around strlen() */
size_t
_mesa_strlen( const char *s )
{
   return strlen(s);
}

/** Wrapper around strcmp() */
int
_mesa_strcmp( const char *s1, const char *s2 )
{
   return strcmp(s1, s2);
}

/** Wrapper around strncmp() */
int
_mesa_strncmp( const char *s1, const char *s2, size_t n )
{
   return strncmp(s1, s2, n);
}

/**
 * Implemented using _mesa_malloc() and _mesa_strcpy.
 * Note that NULL is handled accordingly.
 */
char *
_mesa_strdup( const char *s )
{
   if (s) {
      size_t l = _mesa_strlen(s);
      char *s2 = (char *) _mesa_malloc(l + 1);
      if (s2)
         _mesa_strcpy(s2, s);
      return s2;
   }
   else {
      return NULL;
   }
}

/** Wrapper around atoi() */
int
_mesa_atoi(const char *s)
{
   return atoi(s);
}

/** Wrapper around strtod() */
double
_mesa_strtod( const char *s, char **end )
{
   return strtod(s, end);
}

/*@}*/


/**********************************************************************/
/** \name I/O */
/*@{*/

/** Wrapper around vsprintf() */
int
_mesa_sprintf( char *str, const char *fmt, ... )
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vsprintf( str, fmt, args );
   va_end( args );
   return r;
}

/** Wrapper around printf(), using vsprintf() for the formatting. */
void
_mesa_printf( const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end( args );
   fprintf(stderr,"%s", s);
}

/** Wrapper around vsprintf() */
int
_mesa_vsprintf( char *str, const char *fmt, va_list args )
{
   return vsprintf( str, fmt, args );
}

/*@}*/


/**********************************************************************/
/** \name Diagnostics */
/*@{*/

/**
 * Report a warning (a recoverable error condition) to stderr if
 * either DEBUG is defined or the MESA_DEBUG env var is set.
 *
 * \param ctx GL context.
 * \param fmtString printf() alike format string.
 */
void
_mesa_warning( GLcontext *ctx, const char *fmtString, ... )
{
   GLboolean debug;
   char str[MAXSTRING];
   va_list args;
   (void) ctx;
   va_start( args, fmtString );  
   (void) vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
#ifdef DEBUG
   debug = GL_TRUE; /* always print warning */
#else
   debug = _mesa_getenv("MESA_DEBUG") ? GL_TRUE : GL_FALSE;
#endif
   if (debug) {
      fprintf(stderr, "Mesa warning: %s\n", str);
   }
}

/**
 * Report an internla implementation problem.
 * Prints the message to stderr via fprintf().
 *
 * \param ctx GL context.
 * \param s problem description string.
 */
void
_mesa_problem( const GLcontext *ctx, const char *fmtString, ... )
{
   va_list args;
   char str[MAXSTRING];
   (void) ctx;

   va_start( args, fmtString );  
   vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );

   fprintf(stderr, "Mesa %s implementation error: %s\n", MESA_VERSION_STRING, str);
   fprintf(stderr, "Please report at bugzilla.freedesktop.org\n");
}

/**
 * Record an OpenGL state error.  These usually occur when the users
 * passes invalid parameters to a GL function.
 *
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), report the error with
 * _mesa_debug().
 * 
 * \param ctx the GL context.
 * \param error the error value.
 * \param fmtString printf() style format string, followed by optional args
 */
void
_mesa_error( GLcontext *ctx, GLenum error, const char *fmtString, ... )
{
   const char *debugEnv;
   GLboolean debug;

   debugEnv = _mesa_getenv("MESA_DEBUG");

#ifdef DEBUG
   if (debugEnv && _mesa_strstr(debugEnv, "silent"))
      debug = GL_FALSE;
   else
      debug = GL_TRUE;
#else
   if (debugEnv)
      debug = GL_TRUE;
   else
      debug = GL_FALSE;
#endif

   if (debug) {
      va_list args;
      char where[MAXSTRING];
      const char *errstr;

      va_start( args, fmtString );  
      vsnprintf( where, MAXSTRING, fmtString, args );
      va_end( args );

      switch (error) {
	 case GL_NO_ERROR:
	    errstr = "GL_NO_ERROR";
	    break;
	 case GL_INVALID_VALUE:
	    errstr = "GL_INVALID_VALUE";
	    break;
	 case GL_INVALID_ENUM:
	    errstr = "GL_INVALID_ENUM";
	    break;
	 case GL_INVALID_OPERATION:
	    errstr = "GL_INVALID_OPERATION";
	    break;
	 case GL_STACK_OVERFLOW:
	    errstr = "GL_STACK_OVERFLOW";
	    break;
	 case GL_STACK_UNDERFLOW:
	    errstr = "GL_STACK_UNDERFLOW";
	    break;
	 case GL_OUT_OF_MEMORY:
	    errstr = "GL_OUT_OF_MEMORY";
	    break;
         case GL_TABLE_TOO_LARGE:
            errstr = "GL_TABLE_TOO_LARGE";
            break;
         case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
            errstr = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
	 default:
	    errstr = "unknown";
	    break;
      }
      _mesa_debug(ctx, "User error: %s in %s\n", errstr, where);
   }

   _mesa_record_error(ctx, error);
}  

/**
 * Report debug information.  Print error message to stderr via fprintf().
 * No-op if DEBUG mode not enabled.
 * 
 * \param ctx GL context.
 * \param fmtString printf()-style format string, followed by optional args.
 */
void
_mesa_debug( const GLcontext *ctx, const char *fmtString, ... )
{
#ifdef DEBUG
   char s[MAXSTRING];
   va_list args;
   va_start(args, fmtString);
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end(args);
   fprintf(stderr, "Mesa: %s", s);
#endif /* DEBUG */
   (void) ctx;
   (void) fmtString;
}

/*@}*/


/**
 * Wrapper for exit().
 */
void
_mesa_exit( int status )
{
   exit(status);
}
