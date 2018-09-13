/*
        File: dblint.h.  Supplement to bignum.h

        This file has declarations related to 
        double-precision integers,
        such as typedefs, constants, and primitive operations.

        Before #including this, one should #define

                digit_t -- typedef for single-precision integers.
                RADIX_BITS -- Number of bits per digit_t.

        and identify which compiler one is using.

        Constants defined herein include

                DBLINT_BUILTIN -- 1 if compiler directly
                                  supports double integers, 0 if not.

                DBLINT_HIGH_INDEX (optional)  -- When DBLINT_BUILTIN == 1,
                                  this is 0 if compiler stores
                                  the most significant half of a
                                  dblint_t datum first, and 1
                                  if compiler stores the least
                                  significant half first.  See
                                  HIGH_DIGIT and MAKE_DBLINT below.

                                  If this is not defined, then HIGH_DIGIT
                                  and MAKE_DBLINT are defined using
                                  shifts by RADIX_BITS.  If the compiler
                                  optimizes such shifts, then
                                  leave DBLINT_HIGH_INDEX undefined.


        The dblint_t type is unsigned and holds 
        twice as many bits as a digit_t datum.
        If (DBLINT_BUILTIN = 1),
        then use the type already in the language.
        Otherwise (DBLINT_BUILTIN = 0) 
        construct one of our own, 
        using a struct with two digit_t fields.

        Let u, u1, u2 have type digit_t and
        d, d1, d2 have type dblint_t.  
        The following primitives are defined, 
        whether we use the built-in type or our own type:

                DBLINT(u) -- Convert u from type digit_t to type dblint_t.             
                DBLINT_ADD(d1, d2) -- Sum d1 + d2.
                DBLINT_EQ(d1, d2)  -- Test whether d1 == d2.
                DBLINT_GE(d1, d2)  -- Test whether d1 >= d2.
                DBLINT_GT(d1, d2)  -- Test whether d1 > d2.
                DBLINT_LE(d1, d2)  -- Test whether d1 >= d2.
                DBLINT_LT(d1, d2)  -- Test whether d1 > d2.
                DBLINT_NE(d1, d2)  -- Test whether d1 <> d2.
                DBLINT_SUB(d1, d2) -- Difference d1 - d2.
                DPRODUU(u1, u2) -- Product of u1 and u2, as a dblint_t.
                HPRODUU(u1, u2) -- Most significant half of product
                                   of u1 and u2, as a digit_t.
                HIGH_DIGIT(d) -- Most significant half of d.
                LOW_DIGIT(d) -- Least significant half of d.
                MAKE_DBLINT(u1, u2) -- Construct a dblint_t
                        whose most significant half is u1 and
                        whose least significant half is u2.
*/

#if COMPILER == COMPILER_GCC

    #define DBLINT_BUILTIN 1
    typedef unsigned long long dblint_t;
    #define DBLINT_HIGH_INDEX 0  
                /* GCC on SPARC stores high half of dblint_t first */
#endif

#if COMPILER == COMPILER_VC && RADIX_BITS == 32  
    #define DBLINT_BUILTIN 1
    typedef unsigned __int64 dblint_t;
#if TARGET == TARGET_ALPHA
				/* If the Alpha is using RADIX_BITS == 32,
				   then use the shift instruction 
	               for HIGH_DIGIT and MAKE_DBLINT */ 
#else
    #define DBLINT_HIGH_INDEX 1
                /* Visual C++ on ix86 stores low half of dblint_t first */
#endif
#endif

#ifndef DBLINT_BUILTIN
                        /* No language support -- simulate using structs */
    #define DBLINT_BUILTIN 0
    typedef struct {
                     digit_t high;
                     digit_t low;
                   } dblint_t;
#endif

typedef const dblint_t dblint_tc;


#if DBLINT_BUILTIN
/*
        If language has support for double-length integers, use it.
        Good compilers will inline these simple operations.
*/

#define DBLINT(u) ((dblint_t)(u))


#define DBLINT_ADD(d1, d2) ((d1) + (d2))
#define DBLINT_EQ( d1, d2) ((d1) == (d2))
#define DBLINT_GE( d1, d2) ((d1) >= (d2))
#define DBLINT_GT( d1, d2) ((d1) > (d2))
#define DBLINT_LE( d1, d2) ((d1) <= (d2))
#define DBLINT_LT( d1, d2) ((d1) < (d2))
#define DBLINT_NE( d1, d2) ((d1) != (d2))
#define DBLINT_SUB(d1, d2) ((d1) - (d2))

#if COMPILER == COMPILER_GCC
#define DPRODUU(u1, u2) (DBLINT(u1) * DBLINT(u2))
#endif

#if COMPILER == COMPILER_VC
/*
        A problem in Visual C/C++ 4.0 (x86 version, 1995)
        prevents proper inlining of the DPRODUU function
        if we code it in a straightforward way.  Specifically,
        if we have two nearby references DPRODUU(x, y)
        and DPRODUU(x, z), where one argument (here x) is
        repeated, then the compiler calls library function
        __allmul rather than emit a MUL instruction.
        The -volatile- keyword inhibits the compiler from
        recognizing the repeated subexpression DBLINT(x),
        and circumvents the problem, alas with extra memory
        references.

		x86 version of VC 4.1 adds an __emulu function
*/
static inline dblint_t DPRODUU(digit_tc u1, digit_tc u2)
{
#if TARGET == TARGET_IX86

    #if _MFC_VER < 0x0410
        volatile digit_tc u1copy = u1, u2copy = u2;
	    return DBLINT(u1copy) * DBLINT(u2copy);
    #else
        #pragma intrinsic(__emulu)
		return __emulu(u1, u2);
    #endif
#elif TARGET == TARGET_MIPS 
        #pragma intrinsic(__emulu)
		return __emulu(u1, u2);
#else
		return DBLINT(u1) * DBLINT(u2);
#endif
}
#endif

#define LOW_DIGIT(d)   ((digit_t)(d))

#ifdef DBLINT_HIGH_INDEX
    #if DBLINT_HIGH_INDEX < 0 || DBLINT_HIGH_INDEX > 1
        #error "Illegal value of DBLINT_HIGH_INDEX"
    #endif

    static inline digit_t HIGH_DIGIT(dblint_tc d)
    {
        dblint_tc dcopy = d;
        return ((digit_tc*)&dcopy)[DBLINT_HIGH_INDEX];
    }

    static inline dblint_t MAKE_DBLINT(digit_tc high, digit_tc low)
    {
        dblint_t build = low;
        ((digit_t*)&build)[DBLINT_HIGH_INDEX] = high;
        return build;
    }
#else /* DBLINT_HIGH_INDEX */
    #define HIGH_DIGIT(d)  ((digit_t)((d) >> RADIX_BITS))

    #define MAKE_DBLINT(high, low) \
       ( (DBLINT(high) << RADIX_BITS) | DBLINT(low) )

#endif /* DBLINT_HIGH_INDEX */

#else  /* DBLINT_BUILTIN */
    

static inline dblint_t DBLINT(digit_tc d)
{
    dblint_t answer;
    answer.low = d;
    answer.high = 0;
    return answer;
}

static inline dblint_t DBLINT_ADD(dblint_tc d1, dblint_tc d2)
{
    dblint_t answer;
    answer.low = d1.low + d2.low;
    answer.high = d1.high + d2.high + (answer.low < d1.low);
    return answer;
}

static inline BOOL DBLINT_EQ(dblint_tc d1, dblint_tc d2)
{
   return (d1.high == d2.high && d1.low == d2.low);
}

static inline BOOL DBLINT_GE(dblint_tc d1, dblint_tc d2)
{
   return (d1.high == d2.high ? d1.low  >= d2.low
                              : d1.high >= d2.high);
}

static inline BOOL DBLINT_GT(dblint_tc d1, dblint_tc d2)
{
   return (d1.high == d2.high ? d1.low  > d2.low
                              : d1.high > d2.high);
}

#define DBLINT_LE(d1, d2) DBLINT_GE(d2, d1)
#define DBLINT_LT(d1, d2) DBLINT_GT(d2, d1)

static inline BOOL DBLINT_NE(dblint_tc d1, dblint_tc d2)
{
   return (d1.high != d2.high || d1.low != d2.low);
}

static inline dblint_t DBLINT_SUB(dblint_tc d1, dblint_tc d2)
{    
    dblint_t answer;
    answer.low = d1.low - d2.low;
    answer.high = d1.high - d2.high - (d1.low < d2.low);
    return answer;
}

#define HIGH_DIGIT(d) ((d).high)
#define LOW_DIGIT(d)  ((d).low)

static inline dblint_t MAKE_DBLINT(digit_tc high, digit_tc low)
{
     dblint_t answer;
     answer.low = low; 
     answer.high = high;
     return answer;
}

#if TARGET == TARGET_ALPHA
    #pragma intrinsic(__UMULH)
    #define HPRODUU(u1, u2) __UMULH(u1, u2)
    static inline dblint_t DPRODUU(digit_tc u1, digit_tc u2)
	{
		dblint_t answer;

		answer.high = HPRODUU(u1, u2);   /* Upper product */
		answer.low = u1*u2;			 	 /* Lower product */
		return answer;
	}
#else
static inline dblint_t DPRODUU(digit_tc u1, digit_tc u2)
/*                            
        Multiply two single-precision operands,
        return double precision product.
        This will normally be replaced by an assembly language routine.
        unless the top half of the product is available in C.
*/
{
    dblint_t answer;
    digit_tc u1bot = u1 & RADIX_HALFMASK_BOTTOM,  u1top = u1 >> HALF_RADIX_BITS;
    digit_tc u2bot = u2 & RADIX_HALFMASK_BOTTOM,  u2top = u2 >> HALF_RADIX_BITS;

    digit_tc low  = u1bot * u2bot;
    digit_t  mid1 = u1bot * u2top;
    digit_tc mid2 = u1top * u2bot;
    digit_tc high = u1top * u2top;
/*
        Each half-word product is bounded by
        (SQRT(RADIX) - 1)^2 = RADIX - 2*SQRT(RADIX) + 1,
        so we can add two half-word operands
        to any product without risking integer overflow.
*/
    mid1 += (mid2 & RADIX_HALFMASK_BOTTOM) + (low >> HALF_RADIX_BITS);

    answer.high = high + (mid1 >> HALF_RADIX_BITS) 
                       + (mid2 >> HALF_RADIX_BITS);
    answer.low = (low & RADIX_HALFMASK_BOTTOM) + (mid1 << HALF_RADIX_BITS);
    return answer;
}
#endif /* multiplication */ 

#endif  /* DBLINT_BUILTIN */

#ifndef HPRODUU
    #define HPRODUU(u1, u2) HIGH_DIGIT(DPRODUU(u1, u2))
#endif

/*
    The DBLINT_SUM, MULTIPLY_ADD1. MULTIPLY_ADD2
    functions take single-length (digit_t) operands and
    return double-length (dblint_t) results.
    Overflow is impossible.
*/

#if TARGET == TARGET_ALPHA && RADIX_BITS == 64 && !DBLINT_BUILT_IN
	static inline dblint_t DBLINT_SUM(digit_tc d1, digit_tc d2)
	{
        dblint_t answer;
		answer.low = d1 + d2;
		answer.high = (answer.low < d1);
		return answer;
	}
    static inline dblint_t MULTIPLY_ADD1(digit_tc d1, digit_tc d2, digit_tc d3)
	{
        dblint_t answer;
		digit_t ah, al;

        al = d1*d2;
		ah = __UMULH(d1, d2);
		al += d3;
		answer.high = ah + (al < d3);
		answer.low = al;
		return answer;
	}
	static inline dblint_t MULTIPLY_ADD2(digit_tc d1, digit_tc d2, 
		                                 digit_tc d3, digit_tc d4)
	{
		dblint_t answer;
		digit_t ah, al, bh, bl;

		al = d1*d2;
		ah = __UMULH(d1, d2);
		bl = d3 + d4;
		bh = (bl < d3);
        answer.low = al + bl;
		answer.high = ah + bh + (answer.low < al);
		return answer;
	}

#else
    #define DBLINT_SUM(d1, d2) DBLINT_ADD(DBLINT(d1), DBLINT(d2))
            /* d1 + d2 */

    #define MULTIPLY_ADD1(d1, d2, d3) \
        DBLINT_ADD(DPRODUU(d1, d2), DBLINT(d3));
           /* d1*d2 + d3 */

    #define MULTIPLY_ADD2(d1, d2, d3, d4) \
        DBLINT_ADD(DBLINT_ADD(DPRODUU(d1, d2), DBLINT(d3)), \
                   DBLINT(d4))
          /* d1*d2 + d3 + d4 */

#endif
