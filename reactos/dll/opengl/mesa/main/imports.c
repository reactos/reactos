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


#include <precomp.h>

#ifdef _GNU_SOURCE
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

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
   int err = posix_memalign(& mem, alignment, bytes);
   if (err)
      return NULL;
   return mem;
#elif defined(_WIN32) && defined(_MSC_VER)
   return _aligned_malloc(bytes, alignment);
#else
   uintptr_t ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (uintptr_t) malloc(bytes + alignment + sizeof(void *));
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
 * Same as _mesa_align_malloc(), but using calloc(1, ) instead of
 * malloc()
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

   ptr = (uintptr_t) calloc(1, bytes + alignment + sizeof(void *));
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
   free(realAddr);
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
      memcpy(newBuf, oldBuffer, copySize);
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
   void *newBuffer = malloc(newSize);
   if (newBuffer && oldBuffer && copySize > 0)
      memcpy(newBuffer, oldBuffer, copySize);
   if (oldBuffer)
      free(oldBuffer);
   return newBuffer;
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

/*@}*/


/**********************************************************************/
/** \name Math */
/*@{*/

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
        fi_type u;
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

#ifndef __GNUC__
/**
 * Find the first bit set in a word.
 */
int
_mesa_ffs(int32_t i)
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
_mesa_ffsll(int64_t val)
{
   int bit;

   assert(sizeof(val) == 8);

   bit = _mesa_ffs((int32_t)val);
   if (bit != 0)
      return bit;

   bit = _mesa_ffs((int32_t)(val >> 32));
   if (bit != 0)
      return 32 + bit;

   return 0;
}
#endif

#if !defined(__GNUC__) ||\
   ((__GNUC__ * 100 + __GNUC_MINOR__) < 304) /* Not gcc 3.4 or later */
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
 * Return number of bits set in given 64-bit uint.
 */
unsigned int
_mesa_bitcount_64(uint64_t n)
{
   unsigned int bits;
   for (bits = 0; n > 0; n = n >> 1) {
      bits += (n & 1);
   }
   return bits;
}
#endif

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
#if defined(_WIN32_WCE)
   void *mid;
   int cmp;
   while (nmemb) {
      nmemb >>= 1;
      mid = (char *)base + nmemb * size;
      cmp = (*compar)(key, mid);
      if (cmp == 0)
	 return mid;
      if (cmp > 0) {
	 base = (char *)mid + size;
	 --nmemb;
      }
   }
   return NULL;
#else
   return bsearch(key, base, nmemb, size, compar);
#endif
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
#if defined(_XBOX) || defined(_WIN32_WCE)
   return NULL;
#else
   return getenv(var);
#endif
}

/*@}*/


/**********************************************************************/
/** \name String */
/*@{*/

/**
 * Implemented using malloc() and strcpy.
 * Note that NULL is handled accordingly.
 */
char *
_mesa_strdup( const char *s )
{
   if (s) {
      size_t l = strlen(s);
      char *s2 = (char *) malloc(l + 1);
      if (s2)
         strcpy(s2, s);
      return s2;
   }
   else {
      return NULL;
   }
}

/** Wrapper around strtof() */
float
_mesa_strtof( const char *s, char **end )
{
#if defined(_GNU_SOURCE) && !defined(__CYGWIN__) && !defined(__FreeBSD__) && \
   !defined(ANDROID) && !defined(__HAIKU__)
   static locale_t loc = NULL;
   if (!loc) {
      loc = newlocale(LC_CTYPE_MASK, "C", NULL);
   }
   return strtof_l(s, end, loc);
#elif defined(_ISOC99_SOURCE) || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
   return strtof(s, end);
#else
   return (float)strtod(s, end);
#endif
}

/** Compute simple checksum/hash for a string */
unsigned int
_mesa_str_checksum(const char *str)
{
   /* This could probably be much better */
   unsigned int sum, i;
   const char *c;
   sum = i = 1;
   for (c = str; *c; c++, i++)
      sum += *c * (i % 100);
   return sum + i;
}


/*@}*/


/** Wrapper around vsnprintf() */
int
_mesa_snprintf( char *str, size_t size, const char *fmt, ... )
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vsnprintf( str, size, fmt, args );
   va_end( args );
   return r;
}


/**********************************************************************/
/** \name Diagnostics */
/*@{*/

static void
output_if_debug(const char *prefixString, const char *outputString,
                GLboolean newline)
{
   static int debug = -1;

   /* Check the MESA_DEBUG environment variable if it hasn't
    * been checked yet.  We only have to check it once...
    */
   if (debug == -1) {
      char *env = _mesa_getenv("MESA_DEBUG");

      /* In a debug build, we print warning messages *unless*
       * MESA_DEBUG is 0.  In a non-debug build, we don't
       * print warning messages *unless* MESA_DEBUG is
       * set *to any value*.
       */
#ifdef DEBUG
      debug = (env != NULL && atoi(env) == 0) ? 0 : 1;
#else
      debug = (env != NULL) ? 1 : 0;
#endif
   }

   /* Now only print the string if we're required to do so. */
   if (debug) {
      fprintf(stderr, "%s: %s", prefixString, outputString);
      if (newline)
         fprintf(stderr, "\n");

#if defined(_WIN32) && !defined(_WIN32_WCE)
      /* stderr from windows applications without console is not usually 
       * visible, so communicate with the debugger instead */ 
      {
         char buf[4096];
         _mesa_snprintf(buf, sizeof(buf), "%s: %s%s", prefixString, outputString, newline ? "\n" : "");
         OutputDebugStringA(buf);
      }
#endif
   }
}


/**
 * Return string version of GL error code.
 */
static const char *
error_string( GLenum error )
{
   switch (error) {
   case GL_NO_ERROR:
      return "GL_NO_ERROR";
   case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
   case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
   case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
   case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
   case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
   case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
   case GL_TABLE_TOO_LARGE:
      return "GL_TABLE_TOO_LARGE";
   case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
   default:
      return "unknown";
   }
}


/**
 * When a new type of error is recorded, print a message describing
 * previous errors which were accumulated.
 */
static void
flush_delayed_errors( struct gl_context *ctx )
{
   char s[MAXSTRING];

   if (ctx->ErrorDebugCount) {
      _mesa_snprintf(s, MAXSTRING, "%d similar %s errors", 
                     ctx->ErrorDebugCount,
                     error_string(ctx->ErrorValue));

      output_if_debug("Mesa", s, GL_TRUE);

      ctx->ErrorDebugCount = 0;
   }
}


/**
 * Report a warning (a recoverable error condition) to stderr if
 * either DEBUG is defined or the MESA_DEBUG env var is set.
 *
 * \param ctx GL context.
 * \param fmtString printf()-like format string.
 */
void
_mesa_warning( struct gl_context *ctx, const char *fmtString, ... )
{
   char str[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   (void) vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
   
   if (ctx)
      flush_delayed_errors( ctx );

   output_if_debug("Mesa warning", str, GL_TRUE);
}


/**
 * Report an internal implementation problem.
 * Prints the message to stderr via fprintf().
 *
 * \param ctx GL context.
 * \param fmtString problem description string.
 */
void
_mesa_problem( const struct gl_context *ctx, const char *fmtString, ... )
{
   va_list args;
   char str[MAXSTRING];
   static int numCalls = 0;

   (void) ctx;

   if (numCalls < 50) {
      numCalls++;

      va_start( args, fmtString );  
      vsnprintf( str, MAXSTRING, fmtString, args );
      va_end( args );
      fprintf(stderr, "Mesa %s implementation error: %s\n",
              MESA_VERSION_STRING, str);
      fprintf(stderr, "Please report at bugs.freedesktop.org\n");
   }
}


/**
 * Record an OpenGL state error.  These usually occur when the user
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
_mesa_error( struct gl_context *ctx, GLenum error, const char *fmtString, ... )
{
   static GLint debug = -1;

   /* Check debug environment variable only once:
    */
   if (debug == -1) {
      const char *debugEnv = _mesa_getenv("MESA_DEBUG");

#ifdef DEBUG
      if (debugEnv && strstr(debugEnv, "silent"))
         debug = GL_FALSE;
      else
         debug = GL_TRUE;
#else
      if (debugEnv)
         debug = GL_TRUE;
      else
         debug = GL_FALSE;
#endif
   }

   if (debug) {      
      if (ctx->ErrorValue == error &&
          ctx->ErrorDebugFmtString == fmtString) {
         ctx->ErrorDebugCount++;
      }
      else {
         char s[MAXSTRING], s2[MAXSTRING];
         va_list args;

         flush_delayed_errors( ctx );
         
         va_start(args, fmtString);
         vsnprintf(s, MAXSTRING, fmtString, args);
         va_end(args);

         _mesa_snprintf(s2, MAXSTRING, "%s in %s", error_string(error), s);
         output_if_debug("Mesa: User error", s2, GL_TRUE);
         
         ctx->ErrorDebugFmtString = fmtString;
         ctx->ErrorDebugCount = 0;
      }
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
_mesa_debug( const struct gl_context *ctx, const char *fmtString, ... )
{
//#ifdef DEBUG
   char s[MAXSTRING];
   va_list args;
   va_start(args, fmtString);
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end(args);
   output_if_debug("Mesa", s, GL_FALSE);
//#endif /* DEBUG */
   (void) ctx;
   (void) fmtString;
}

/*@}*/
