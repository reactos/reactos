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
 * The OpenGL SI's __GLimports structure allows per-context specification of
 * replacements for the standard C lib functions.  In practice that's probably
 * never needed; compile-time replacements are far more likely.
 *
 * The _mesa_*() functions defined here don't in general take a context
 * parameter.  I guess we can change that someday, if need be.
 * So for now, the __GLimports stuff really isn't used.
 *
 * \todo Functions still needed:
 * - scanf
 * - qsort
 * - bsearch
 * - rand and RAND_MAX
 *
 * \note When compiled into a XFree86 module these functions wrap around
 * XFree86 own wrappers.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.3
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

/** Wrapper around either malloc() or xf86malloc() */
void *
_mesa_malloc(size_t bytes)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86malloc(bytes);
#else
   return malloc(bytes);
#endif
}

/** Wrapper around either calloc() or xf86calloc() */
void *
_mesa_calloc(size_t bytes)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86calloc(1, bytes);
#else
   return calloc(1, bytes);
#endif
}

/** Wrapper around either free() or xf86free() */
void
_mesa_free(void *ptr)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86free(ptr);
#else
   free(ptr);
#endif
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
}

/**
 * Same as _mesa_align_malloc(), but using _mesa_calloc() instead of
 * _mesa_malloc()
 */
void *
_mesa_align_calloc(size_t bytes, unsigned long alignment)
{
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
#if 0
   _mesa_free( (void *)(*(unsigned long *)((unsigned long)ptr - sizeof(void *))) );
#else
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   _mesa_free(realAddr);
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
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86memcpy(dest, src, n);
#elif defined(SUNOS4)
   return memcpy((char *) dest, (char *) src, (int) n);
#else
   return memcpy(dest, src, n);
#endif
}

/** Wrapper around either memset() or xf86memset() */
void
_mesa_memset( void *dst, int val, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86memset( dst, val, n );
#elif defined(SUNOS4)
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

/** Wrapper around either memcpy() or xf86memcpy() or bzero() */
void
_mesa_bzero( void *dst, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86memset( dst, 0, n );
#elif defined(__FreeBSD__)
   bzero( dst, n );
#else
   memset( dst, 0, n );
#endif
}

/*@}*/


/**********************************************************************/
/** \name Math */
/*@{*/

/** Wrapper around either sin() or xf86sin() */
double
_mesa_sin(double a)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86sin(a);
#else
   return sin(a);
#endif
}

/** Wrapper around either cos() or xf86cos() */
double
_mesa_cos(double a)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86cos(a);
#else
   return cos(a);
#endif
}

/** Wrapper around either sqrt() or xf86sqrt() */
double
_mesa_sqrtd(double x)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86sqrt(x);
#else
   return sqrt(x);
#endif
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

static void init_sqrt_table(void)
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
#elif defined(XFree86LOADER) && defined(IN_MODULE)
        return 1.0F / xf86sqrt(n);
#else
        return (float) (1.0 / sqrt(n));
#endif
}


/**
 * Wrapper around either pow() or xf86pow().
 */
double
_mesa_pow(double x, double y)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86pow(x, y);
#else
   return pow(x, y);
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
/** \name Environment vars */
/*@{*/

/**
 * Wrapper for getenv().
 */
char *
_mesa_getenv( const char *var )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86getenv(var);
#elif defined(_XBOX)
   return NULL;
#else
   return getenv(var);
#endif
}

/*@}*/


/**********************************************************************/
/** \name String */
/*@{*/

/** Wrapper around either strstr() or xf86strstr() */
char *
_mesa_strstr( const char *haystack, const char *needle )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strstr(haystack, needle);
#else
   return strstr(haystack, needle);
#endif
}

/** Wrapper around either strncat() or xf86strncat() */
char *
_mesa_strncat( char *dest, const char *src, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncat(dest, src, n);
#else
   return strncat(dest, src, n);
#endif
}

/** Wrapper around either strcpy() or xf86strcpy() */
char *
_mesa_strcpy( char *dest, const char *src )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strcpy(dest, src);
#else
   return strcpy(dest, src);
#endif
}

/** Wrapper around either strncpy() or xf86strncpy() */
char *
_mesa_strncpy( char *dest, const char *src, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncpy(dest, src, n);
#else
   return strncpy(dest, src, n);
#endif
}

/** Wrapper around either strlen() or xf86strlen() */
size_t
_mesa_strlen( const char *s )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strlen(s);
#else
   return strlen(s);
#endif
}

/** Wrapper around either strcmp() or xf86strcmp() */
int
_mesa_strcmp( const char *s1, const char *s2 )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strcmp(s1, s2);
#else
   return strcmp(s1, s2);
#endif
}

/** Wrapper around either strncmp() or xf86strncmp() */
int
_mesa_strncmp( const char *s1, const char *s2, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncmp(s1, s2, n);
#else
   return strncmp(s1, s2, n);
#endif
}

/** Implemented using _mesa_malloc() and _mesa_strcpy */
char *
_mesa_strdup( const char *s )
{
   size_t l = _mesa_strlen(s);
   char *s2 = (char *) _mesa_malloc(l + 1);
   if (s2)
      _mesa_strcpy(s2, s);
   return s2;
}

/** Wrapper around either atoi() or xf86atoi() */
int
_mesa_atoi(const char *s)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86atoi(s);
#else
   return atoi(s);
#endif
}

/** Wrapper around either strtod() or xf86strtod() */
double
_mesa_strtod( const char *s, char **end )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strtod(s, end);
#else
   return strtod(s, end);
#endif
}

/*@}*/


/**********************************************************************/
/** \name I/O */
/*@{*/

/** Wrapper around either vsprintf() or xf86vsprintf() */
int
_mesa_sprintf( char *str, const char *fmt, ... )
{
   int r;
   va_list args;
   va_start( args, fmt );  
   va_end( args );
#if defined(XFree86LOADER) && defined(IN_MODULE)
   r = xf86vsprintf( str, fmt, args );
#else
   r = vsprintf( str, fmt, args );
#endif
   return r;
}

/** Wrapper around either printf() or xf86printf(), using vsprintf() for
 * the formatting. */
void
_mesa_printf( const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end( args );
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86printf("%s", s);
#else
   fprintf(stderr,"%s", s);
#endif
}

/*@}*/


/**********************************************************************/
/** \name Diagnostics */
/*@{*/

/**
 * Display a warning.
 *
 * \param ctx GL context.
 * \param fmtString printf() alike format string.
 * 
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), prints the warning to
 * stderr, either via fprintf() or xf86printf().
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
#if defined(XFree86LOADER) && defined(IN_MODULE)
      xf86fprintf(stderr, "Mesa warning: %s", str);
#else
      fprintf(stderr, "Mesa warning: %s", str);
#endif
   }
}

/**
 * This function is called when the Mesa user has stumbled into a code
 * path which may not be implemented fully or correctly.
 *
 * \param ctx GL context.
 * \param s problem description string.
 *
 * Prints the message to stderr, either via fprintf() or xf86fprintf().
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

#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86fprintf(stderr, "Mesa %s implementation error: %s\n", MESA_VERSION_STRING, str);
   xf86fprintf(stderr, "Please report at bugzilla.freedesktop.org\n");
#else
   fprintf(stderr, "Mesa %s implementation error: %s\n", MESA_VERSION_STRING, str);
   fprintf(stderr, "Please report at bugzilla.freedesktop.org\n");
#endif
}

/**
 * Display an error message.
 *
 * If in debug mode, print error message.
 * Also, record the error code by calling _mesa_record_error().
 * 
 * \param ctx the GL context.
 * \param error the error value.
 * \param fmtString printf() style format string, followed by optional args
 *         
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), interperts the error code and 
 * prints the error message via _mesa_debug().
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
	 default:
	    errstr = "unknown";
	    break;
      }
      _mesa_debug(ctx, "User error: %s in %s\n", errstr, where);
   }

   _mesa_record_error(ctx, error);
}  

/**
 * Report debug information.
 * 
 * \param ctx GL context.
 * \param fmtString printf() alike format string.
 * 
 * Prints the message to stderr, either via fprintf() or xf86printf().
 */
void
_mesa_debug( const GLcontext *ctx, const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   (void) ctx;
   va_start(args, fmtString);
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end(args);
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86fprintf(stderr, "Mesa: %s", s);
#else
   fprintf(stderr, "Mesa: %s", s);
#endif
}

/*@}*/


/**********************************************************************/
/** \name Default Imports Wrapper */
/*@{*/

/** Wrapper around _mesa_malloc() */
static void *
default_malloc(__GLcontext *gc, size_t size)
{
   (void) gc;
   return _mesa_malloc(size);
}

/** Wrapper around _mesa_malloc() */
static void *
default_calloc(__GLcontext *gc, size_t numElem, size_t elemSize)
{
   (void) gc;
   return _mesa_calloc(numElem * elemSize);
}

/** Wrapper around either realloc() or xf86realloc() */
static void *
default_realloc(__GLcontext *gc, void *oldAddr, size_t newSize)
{
   (void) gc;
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86realloc(oldAddr, newSize);
#else
   return realloc(oldAddr, newSize);
#endif
}

/** Wrapper around _mesa_free() */
static void
default_free(__GLcontext *gc, void *addr)
{
   (void) gc;
   _mesa_free(addr);
}

/** Wrapper around _mesa_getenv() */
static char * CAPI
default_getenv( __GLcontext *gc, const char *var )
{
   (void) gc;
   return _mesa_getenv(var);
}

/** Wrapper around _mesa_warning() */
static void
default_warning(__GLcontext *gc, char *str)
{
   _mesa_warning(gc, str);
}

/** Wrapper around _mesa_problem() */
static void
default_fatal(__GLcontext *gc, char *str)
{
   _mesa_problem(gc, str);
   abort();
}

/** Wrapper around atoi() */
static int CAPI
default_atoi(__GLcontext *gc, const char *str)
{
   (void) gc;
   return atoi(str);
}

/** Wrapper around vsprintf() */
static int CAPI
default_sprintf(__GLcontext *gc, char *str, const char *fmt, ...)
{
   int r;
   va_list args;
   (void) gc;
   va_start( args, fmt );  
   r = vsprintf( str, fmt, args );
   va_end( args );
   return r;
}

/** Wrapper around fopen() */
static void * CAPI
default_fopen(__GLcontext *gc, const char *path, const char *mode)
{
   (void) gc;
   return fopen(path, mode);
}

/** Wrapper around fclose() */
static int CAPI
default_fclose(__GLcontext *gc, void *stream)
{
   (void) gc;
   return fclose((FILE *) stream);
}

/** Wrapper around vfprintf() */
static int CAPI
default_fprintf(__GLcontext *gc, void *stream, const char *fmt, ...)
{
   int r;
   va_list args;
   (void) gc;
   va_start( args, fmt );  
   r = vfprintf( (FILE *) stream, fmt, args );
   va_end( args );
   return r;
}

/**
 * \todo this really is driver-specific and can't be here 
 */
static __GLdrawablePrivate *
default_GetDrawablePrivate(__GLcontext *gc)
{
   (void) gc;
   return NULL;
}

/*@}*/


/**
 * Initialize a __GLimports object to point to the functions in this
 * file.  
 *
 * This is to be called from device drivers.
 * 
 * Also, do some one-time initializations.
 * 
 * \param imports the object to initialize.
 * \param driverCtx pointer to device driver-specific data.
 */
void
_mesa_init_default_imports(__GLimports *imports, void *driverCtx)
{
   /* XXX maybe move this one-time init stuff into context.c */
   static GLboolean initialized = GL_FALSE;
   if (!initialized) {
      init_sqrt_table();

#if defined(_FPU_GETCW) && defined(_FPU_SETCW)
      {
         const char *debug = _mesa_getenv("MESA_DEBUG");
         if (debug && _mesa_strcmp(debug, "FP")==0) {
            /* die on FP exceptions */
            fpu_control_t mask;
            _FPU_GETCW(mask);
            mask &= ~(_FPU_MASK_IM | _FPU_MASK_DM | _FPU_MASK_ZM
                      | _FPU_MASK_OM | _FPU_MASK_UM);
            _FPU_SETCW(mask);
         }
      }
#endif
      initialized = GL_TRUE;
   }

   imports->malloc = default_malloc;
   imports->calloc = default_calloc;
   imports->realloc = default_realloc;
   imports->free = default_free;
   imports->warning = default_warning;
   imports->fatal = default_fatal;
   imports->getenv = default_getenv; /* not used for now */
   imports->atoi = default_atoi;
   imports->sprintf = default_sprintf;
   imports->fopen = default_fopen;
   imports->fclose = default_fclose;
   imports->fprintf = default_fprintf;
   imports->getDrawablePrivate = default_GetDrawablePrivate;
   imports->other = driverCtx;
}
