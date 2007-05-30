/* longlong.h -- definitions for mixed size 32/64 bit arithmetic.
   Copyright (C) 1991, 1992, 1994, 1995, 1996, 1997, 1998, 1999, 2000
   Free Software Foundation, Inc.

   This definition file is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2, or (at your option) any later version.

   This definition file is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied
   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* You have to define the following before including this file:

   UWtype -- An unsigned type, default type for operations (typically a "word")
   UHWtype -- An unsigned type, at least half the size of UWtype.
   UDWtype -- An unsigned type, at least twice as large a UWtype
   W_TYPE_SIZE -- size in bits of UWtype

   UQItype -- Unsigned 8 bit type.
   SItype, USItype -- Signed and unsigned 32 bit types.
   DItype, UDItype -- Signed and unsigned 64 bit types.

   On a 32 bit machine UWtype should typically be USItype;
   on a 64 bit machine, UWtype should typically be UDItype.
*/

#define __BITS4 (W_TYPE_SIZE / 4)
#define __ll_B ((UWtype) 1 << (W_TYPE_SIZE / 2))
#define __ll_lowpart(t) ((UWtype) (t) & (__ll_B - 1))
#define __ll_highpart(t) ((UWtype) (t) >> (W_TYPE_SIZE / 2))

#ifndef W_TYPE_SIZE
#define W_TYPE_SIZE	32
#define UWtype		USItype
#define UHWtype		USItype
#define UDWtype		UDItype
#endif

/* Define auxiliary asm macros.

   1) umul_ppmm(high_prod, low_prod, multipler, multiplicand) multiplies two
   UWtype integers MULTIPLER and MULTIPLICAND, and generates a two UWtype
   word product in HIGH_PROD and LOW_PROD.

   2) __umulsidi3(a,b) multiplies two UWtype integers A and B, and returns a
   UDWtype product.  This is just a variant of umul_ppmm.

   3) udiv_qrnnd(quotient, remainder, high_numerator, low_numerator,
   denominator) divides a UDWtype, composed by the UWtype integers
   HIGH_NUMERATOR and LOW_NUMERATOR, by DENOMINATOR and places the quotient
   in QUOTIENT and the remainder in REMAINDER.  HIGH_NUMERATOR must be less
   than DENOMINATOR for correct operation.  If, in addition, the most
   significant bit of DENOMINATOR must be 1, then the pre-processor symbol
   UDIV_NEEDS_NORMALIZATION is defined to 1.

   4) sdiv_qrnnd(quotient, remainder, high_numerator, low_numerator,
   denominator).  Like udiv_qrnnd but the numbers are signed.  The quotient
   is rounded towards 0.

   5) count_leading_zeros(count, x) counts the number of zero-bits from the
   msb to the first non-zero bit in the UWtype X.  This is the number of
   steps X needs to be shifted left to set the msb.  Undefined for X == 0,
   unless the symbol COUNT_LEADING_ZEROS_0 is defined to some value.

   6) count_trailing_zeros(count, x) like count_leading_zeros, but counts
   from the least significant end.

   7) add_ssaaaa(high_sum, low_sum, high_addend_1, low_addend_1,
   high_addend_2, low_addend_2) adds two UWtype integers, composed by
   HIGH_ADDEND_1 and LOW_ADDEND_1, and HIGH_ADDEND_2 and LOW_ADDEND_2
   respectively.  The result is placed in HIGH_SUM and LOW_SUM.  Overflow
   (i.e. carry out) is not stored anywhere, and is lost.

   8) sub_ddmmss(high_difference, low_difference, high_minuend, low_minuend,
   high_subtrahend, low_subtrahend) subtracts two two-word UWtype integers,
   composed by HIGH_MINUEND_1 and LOW_MINUEND_1, and HIGH_SUBTRAHEND_2 and
   LOW_SUBTRAHEND_2 respectively.  The result is placed in HIGH_DIFFERENCE
   and LOW_DIFFERENCE.  Overflow (i.e. carry out) is not stored anywhere,
   and is lost.

   If any of these macros are left undefined for a particular CPU,
   C macros are used.  */

/* The CPUs come in alphabetical order below.

   Please add support for more CPUs here, or improve the current support
   for the CPUs below!
   (E.g. WE32100, IBM360.)  */

#if defined (__GNUC__) && !defined (NO_ASM)

/* We sometimes need to clobber "cc" with gcc2, but that would not be
   understood by gcc1.  Use cpp to avoid major code duplication.  */
#if __GNUC__ < 2
#define __CLOBBER_CC
#define __AND_CLOBBER_CC
#else /* __GNUC__ >= 2 */
#define __CLOBBER_CC : "cc"
#define __AND_CLOBBER_CC , "cc"
#endif /* __GNUC__ < 2 */

#if (defined (__a29k__) || defined (_AM29K)) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add %1,%4,%5\n\taddc %0,%2,%3"				\
	   : "=r" ((USItype) (sh)),					\
	    "=&r" ((USItype) (sl))					\
	   : "%r" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "%r" ((USItype) (al)),					\
	     "rI" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub %1,%4,%5\n\tsubc %0,%2,%3"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "r" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "r" ((USItype) (al)),					\
	     "rI" ((USItype) (bl)))
#define umul_ppmm(xh, xl, m0, m1) \
  do {									\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("multiplu %0,%1,%2"					\
	     : "=r" ((USItype) (xl))					\
	     : "r" (__m0),						\
	       "r" (__m1));						\
    __asm__ ("multmu %0,%1,%2"						\
	     : "=r" ((USItype) (xh))					\
	     : "r" (__m0),						\
	       "r" (__m1));						\
  } while (0)
#define udiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("dividu %0,%3,%4"						\
	   : "=r" ((USItype) (q)),					\
	     "=q" ((USItype) (r))					\
	   : "1" ((USItype) (n1)),					\
	     "r" ((USItype) (n0)),					\
	     "r" ((USItype) (d)))
#define count_leading_zeros(count, x) \
    __asm__ ("clz %0,%1"						\
	     : "=r" ((USItype) (count))					\
	     : "r" ((USItype) (x)))
#define COUNT_LEADING_ZEROS_0 32
#endif /* __a29k__ */

#if defined (__alpha) && W_TYPE_SIZE == 64
#define umul_ppmm(ph, pl, m0, m1) \
  do {									\
    UDItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("umulh %r1,%2,%0"						\
	     : "=r" ((UDItype) ph)					\
	     : "%rJ" (__m0),						\
	       "rI" (__m1));						\
    (pl) = __m0 * __m1;							\
  } while (0)
#define UMUL_TIME 46
#ifndef LONGLONG_STANDALONE
#define udiv_qrnnd(q, r, n1, n0, d) \
  do { UDItype __r;							\
    (q) = __udiv_qrnnd (&__r, (n1), (n0), (d));				\
    (r) = __r;								\
  } while (0)
extern UDItype __udiv_qrnnd PARAMS ((UDItype *, UDItype, UDItype, UDItype));
#define UDIV_TIME 220
#endif /* LONGLONG_STANDALONE */
#ifdef __alpha_cix__
#define count_leading_zeros(COUNT,X) \
  __asm__("ctlz %1,%0" : "=r"(COUNT) : "r"(X))
#define count_trailing_zeros(COUNT,X) \
  __asm__("cttz %1,%0" : "=r"(COUNT) : "r"(X))
#define COUNT_LEADING_ZEROS_0 64
#else
extern const UQItype __clz_tab[];
#define count_leading_zeros(COUNT,X) \
  do {									\
    UDItype __xr = (X), __t, __a;					\
    __asm__("cmpbge $31,%1,%0" : "=r"(__t) : "r"(__xr));		\
    __a = __clz_tab[__t ^ 0xff] - 1;					\
    __asm__("extbl %1,%2,%0" : "=r"(__t) : "r"(__xr), "r"(__a));	\
    (COUNT) = 64 - (__clz_tab[__t] + __a*8);				\
  } while (0)
#define count_trailing_zeros(COUNT,X) \
  do {									\
    UDItype __xr = (X), __t, __a;					\
    __asm__("cmpbge $31,%1,%0" : "=r"(__t) : "r"(__xr));		\
    __t = ~__t & -~__t;							\
    __a = ((__t & 0xCC) != 0) * 2;					\
    __a += ((__t & 0xF0) != 0) * 4;					\
    __a += ((__t & 0xAA) != 0);						\
    __asm__("extbl %1,%2,%0" : "=r"(__t) : "r"(__xr), "r"(__a));	\
    __a <<= 3;								\
    __t &= -__t;							\
    __a += ((__t & 0xCC) != 0) * 2;					\
    __a += ((__t & 0xF0) != 0) * 4;					\
    __a += ((__t & 0xAA) != 0);						\
    (COUNT) = __a;							\
  } while (0)
#endif /* __alpha_cix__ */
#endif /* __alpha */

#if defined (__arc__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add.f	%1, %4, %5\n\tadc	%0, %2, %3"		\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%r" ((USItype) (ah)),					\
	     "rIJ" ((USItype) (bh)),					\
	     "%r" ((USItype) (al)),					\
	     "rIJ" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub.f	%1, %4, %5\n\tsbc	%0, %2, %3"		\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "r" ((USItype) (ah)),					\
	     "rIJ" ((USItype) (bh)),					\
	     "r" ((USItype) (al)),					\
	     "rIJ" ((USItype) (bl)))
/* Call libgcc routine.  */
#define umul_ppmm(w1, w0, u, v) \
do {									\
  DWunion __w;								\
  __w.ll = __umulsidi3 (u, v);						\
  w1 = __w.s.high;							\
  w0 = __w.s.low;							\
} while (0)
#define __umulsidi3 __umulsidi3
UDItype __umulsidi3 (USItype, USItype);
#endif

#if defined (__arm__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("adds	%1, %4, %5\n\tadc	%0, %2, %3"		\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%r" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "%r" ((USItype) (al)),					\
	     "rI" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subs	%1, %4, %5\n\tsbc	%0, %2, %3"		\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "r" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "r" ((USItype) (al)),					\
	     "rI" ((USItype) (bl)))
#define umul_ppmm(xh, xl, a, b) \
{register USItype __t0, __t1, __t2;					\
  __asm__ ("%@ Inlined umul_ppmm\n"					\
	   "	mov	%2, %5, lsr #16\n"				\
	   "	mov	%0, %6, lsr #16\n"				\
	   "	bic	%3, %5, %2, lsl #16\n"				\
	   "	bic	%4, %6, %0, lsl #16\n"				\
	   "	mul	%1, %3, %4\n"					\
	   "	mul	%4, %2, %4\n"					\
	   "	mul	%3, %0, %3\n"					\
	   "	mul	%0, %2, %0\n"					\
	   "	adds	%3, %4, %3\n"					\
	   "	addcs	%0, %0, #65536\n"				\
	   "	adds	%1, %1, %3, lsl #16\n"				\
	   "	adc	%0, %0, %3, lsr #16"				\
	   : "=&r" ((USItype) (xh)),					\
	     "=r" ((USItype) (xl)),					\
	     "=&r" (__t0), "=&r" (__t1), "=r" (__t2)			\
	   : "r" ((USItype) (a)),					\
	     "r" ((USItype) (b)));}
#define UMUL_TIME 20
#define UDIV_TIME 100
#endif /* __arm__ */

#if defined (__clipper__) && W_TYPE_SIZE == 32
#define umul_ppmm(w1, w0, u, v) \
  ({union {UDItype __ll;						\
	   struct {USItype __l, __h;} __i;				\
	  } __xx;							\
  __asm__ ("mulwux %2,%0"						\
	   : "=r" (__xx.__ll)						\
	   : "%0" ((USItype) (u)),					\
	     "r" ((USItype) (v)));					\
  (w1) = __xx.__i.__h; (w0) = __xx.__i.__l;})
#define smul_ppmm(w1, w0, u, v) \
  ({union {DItype __ll;							\
	   struct {SItype __l, __h;} __i;				\
	  } __xx;							\
  __asm__ ("mulwx %2,%0"						\
	   : "=r" (__xx.__ll)						\
	   : "%0" ((SItype) (u)),					\
	     "r" ((SItype) (v)));					\
  (w1) = __xx.__i.__h; (w0) = __xx.__i.__l;})
#define __umulsidi3(u, v) \
  ({UDItype __w;							\
    __asm__ ("mulwux %2,%0"						\
	     : "=r" (__w)						\
	     : "%0" ((USItype) (u)),					\
	       "r" ((USItype) (v)));					\
    __w; })
#endif /* __clipper__ */

#if defined (__gmicro__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add.w %5,%1\n\taddx %3,%0"					\
	   : "=g" ((USItype) (sh)),					\
	     "=&g" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub.w %5,%1\n\tsubx %3,%0"					\
	   : "=g" ((USItype) (sh)),					\
	     "=&g" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define umul_ppmm(ph, pl, m0, m1) \
  __asm__ ("mulx %3,%0,%1"						\
	   : "=g" ((USItype) (ph)),					\
	     "=r" ((USItype) (pl))					\
	   : "%0" ((USItype) (m0)),					\
	     "g" ((USItype) (m1)))
#define udiv_qrnnd(q, r, nh, nl, d) \
  __asm__ ("divx %4,%0,%1"						\
	   : "=g" ((USItype) (q)),					\
	     "=r" ((USItype) (r))					\
	   : "1" ((USItype) (nh)),					\
	     "0" ((USItype) (nl)),					\
	     "g" ((USItype) (d)))
#define count_leading_zeros(count, x) \
  __asm__ ("bsch/1 %1,%0"						\
	   : "=g" (count)						\
	   : "g" ((USItype) (x)),					\
	     "0" ((USItype) 0))
#endif

#if defined (__hppa) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add %4,%5,%1\n\taddc %2,%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%rM" ((USItype) (ah)),					\
	     "rM" ((USItype) (bh)),					\
	     "%rM" ((USItype) (al)),					\
	     "rM" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub %4,%5,%1\n\tsubb %2,%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "rM" ((USItype) (ah)),					\
	     "rM" ((USItype) (bh)),					\
	     "rM" ((USItype) (al)),					\
	     "rM" ((USItype) (bl)))
#if defined (_PA_RISC1_1)
#define umul_ppmm(w1, w0, u, v) \
  do {									\
    union								\
      {									\
	UDItype __f;							\
	struct {USItype __w1, __w0;} __w1w0;				\
      } __t;								\
    __asm__ ("xmpyu %1,%2,%0"						\
	     : "=x" (__t.__f)						\
	     : "x" ((USItype) (u)),					\
	       "x" ((USItype) (v)));					\
    (w1) = __t.__w1w0.__w1;						\
    (w0) = __t.__w1w0.__w0;						\
     } while (0)
#define UMUL_TIME 8
#else
#define UMUL_TIME 30
#endif
#define UDIV_TIME 40
#define count_leading_zeros(count, x) \
  do {									\
    USItype __tmp;							\
    __asm__ (								\
       "ldi		1,%0\n"						\
"	extru,=		%1,15,16,%%r0		; Bits 31..16 zero?\n"	\
"	extru,tr	%1,15,16,%1		; No.  Shift down, skip add.\n"\
"	ldo		16(%0),%0		; Yes.  Perform add.\n"	\
"	extru,=		%1,23,8,%%r0		; Bits 15..8 zero?\n"	\
"	extru,tr	%1,23,8,%1		; No.  Shift down, skip add.\n"\
"	ldo		8(%0),%0		; Yes.  Perform add.\n"	\
"	extru,=		%1,27,4,%%r0		; Bits 7..4 zero?\n"	\
"	extru,tr	%1,27,4,%1		; No.  Shift down, skip add.\n"\
"	ldo		4(%0),%0		; Yes.  Perform add.\n"	\
"	extru,=		%1,29,2,%%r0		; Bits 3..2 zero?\n"	\
"	extru,tr	%1,29,2,%1		; No.  Shift down, skip add.\n"\
"	ldo		2(%0),%0		; Yes.  Perform add.\n"	\
"	extru		%1,30,1,%1		; Extract bit 1.\n"	\
"	sub		%0,%1,%0		; Subtract it.\n"	\
	: "=r" (count), "=r" (__tmp) : "1" (x));			\
  } while (0)
#endif

#if (defined (__i370__) || defined (__mvs__)) && W_TYPE_SIZE == 32
#define umul_ppmm(xh, xl, m0, m1) \
  do {									\
    union {UDItype __ll;						\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("mr %0,%3"							\
	     : "=r" (__xx.__i.__h),					\
	       "=r" (__xx.__i.__l)					\
	     : "%1" (__m0),						\
	       "r" (__m1));						\
    (xh) = __xx.__i.__h; (xl) = __xx.__i.__l;				\
    (xh) += ((((SItype) __m0 >> 31) & __m1)				\
	     + (((SItype) __m1 >> 31) & __m0));				\
  } while (0)
#define smul_ppmm(xh, xl, m0, m1) \
  do {									\
    union {DItype __ll;							\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
    __asm__ ("mr %0,%3"							\
	     : "=r" (__xx.__i.__h),					\
	       "=r" (__xx.__i.__l)					\
	     : "%1" (m0),						\
	       "r" (m1));						\
    (xh) = __xx.__i.__h; (xl) = __xx.__i.__l;				\
  } while (0)
#define sdiv_qrnnd(q, r, n1, n0, d) \
  do {									\
    union {DItype __ll;							\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
    __xx.__i.__h = n1; __xx.__i.__l = n0;				\
    __asm__ ("dr %0,%2"							\
	     : "=r" (__xx.__ll)						\
	     : "0" (__xx.__ll), "r" (d));				\
    (q) = __xx.__i.__l; (r) = __xx.__i.__h;				\
  } while (0)
#endif

#if (defined (__i386__) || defined (__i486__)) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addl %5,%1\n\tadcl %3,%0"					\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subl %5,%1\n\tsbbl %3,%0"					\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("mull %3"							\
	   : "=a" ((USItype) (w0)),					\
	     "=d" ((USItype) (w1))					\
	   : "%0" ((USItype) (u)),					\
	     "rm" ((USItype) (v)))
#define udiv_qrnnd(q, r, n1, n0, dv) \
  __asm__ ("divl %4"							\
	   : "=a" ((USItype) (q)),					\
	     "=d" ((USItype) (r))					\
	   : "0" ((USItype) (n0)),					\
	     "1" ((USItype) (n1)),					\
	     "rm" ((USItype) (dv)))
#define count_leading_zeros(count, x) \
  do {									\
    USItype __cbtmp;							\
    __asm__ ("bsrl %1,%0"						\
	     : "=r" (__cbtmp) : "rm" ((USItype) (x)));			\
    (count) = __cbtmp ^ 31;						\
  } while (0)
#define count_trailing_zeros(count, x) \
  __asm__ ("bsfl %1,%0" : "=r" (count) : "rm" ((USItype)(x)))
#define UMUL_TIME 40
#define UDIV_TIME 40
#endif /* 80x86 */

#if defined (__i860__) && W_TYPE_SIZE == 32
#if 0
/* Make sure these patterns really improve the code before
   switching them on.  */
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  do {									\
    union								\
      {									\
	DItype __ll;							\
	struct {USItype __l, __h;} __i;					\
      }  __a, __b, __s;							\
    __a.__i.__l = (al);							\
    __a.__i.__h = (ah);							\
    __b.__i.__l = (bl);							\
    __b.__i.__h = (bh);							\
    __asm__ ("fiadd.dd %1,%2,%0"					\
	     : "=f" (__s.__ll)						\
	     : "%f" (__a.__ll), "f" (__b.__ll));			\
    (sh) = __s.__i.__h;							\
    (sl) = __s.__i.__l;							\
    } while (0)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  do {									\
    union								\
      {									\
	DItype __ll;							\
	struct {USItype __l, __h;} __i;					\
      }  __a, __b, __s;							\
    __a.__i.__l = (al);							\
    __a.__i.__h = (ah);							\
    __b.__i.__l = (bl);							\
    __b.__i.__h = (bh);							\
    __asm__ ("fisub.dd %1,%2,%0"					\
	     : "=f" (__s.__ll)						\
	     : "%f" (__a.__ll), "f" (__b.__ll));			\
    (sh) = __s.__i.__h;							\
    (sl) = __s.__i.__l;							\
    } while (0)
#endif
#endif /* __i860__ */

#if defined (__i960__) && W_TYPE_SIZE == 32
#define umul_ppmm(w1, w0, u, v) \
  ({union {UDItype __ll;						\
	   struct {USItype __l, __h;} __i;				\
	  } __xx;							\
  __asm__ ("emul	%2,%1,%0"					\
	   : "=d" (__xx.__ll)						\
	   : "%dI" ((USItype) (u)),					\
	     "dI" ((USItype) (v)));					\
  (w1) = __xx.__i.__h; (w0) = __xx.__i.__l;})
#define __umulsidi3(u, v) \
  ({UDItype __w;							\
    __asm__ ("emul	%2,%1,%0"					\
	     : "=d" (__w)						\
	     : "%dI" ((USItype) (u)),					\
	       "dI" ((USItype) (v)));					\
    __w; })
#endif /* __i960__ */

#if defined (__M32R__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  /* The cmp clears the condition bit.  */ \
  __asm__ ("cmp %0,%0\n\taddx %%5,%1\n\taddx %%3,%0"			\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "r" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "r" ((USItype) (bl))					\
	   : "cbit")
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  /* The cmp clears the condition bit.  */ \
  __asm__ ("cmp %0,%0\n\tsubx %5,%1\n\tsubx %3,%0"			\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "r" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "r" ((USItype) (bl))					\
	   : "cbit")
#endif /* __M32R__ */

#if defined (__mc68000__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add%.l %5,%1\n\taddx%.l %3,%0"				\
	   : "=d" ((USItype) (sh)),					\
	     "=&d" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "d" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub%.l %5,%1\n\tsubx%.l %3,%0"				\
	   : "=d" ((USItype) (sh)),					\
	     "=&d" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "d" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))

/* The '020, '030, '040 and CPU32 have 32x32->64 and 64/32->32q-32r.  */
#if defined (__mc68020__) || defined(mc68020) \
	|| defined(__mc68030__) || defined(mc68030) \
	|| defined(__mc68040__) || defined(mc68040) \
	|| defined(__mcpu32__) || defined(mcpu32) \
	|| defined(__NeXT__)
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("mulu%.l %3,%1:%0"						\
	   : "=d" ((USItype) (w0)),					\
	     "=d" ((USItype) (w1))					\
	   : "%0" ((USItype) (u)),					\
	     "dmi" ((USItype) (v)))
#define UMUL_TIME 45
#define udiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("divu%.l %4,%1:%0"						\
	   : "=d" ((USItype) (q)),					\
	     "=d" ((USItype) (r))					\
	   : "0" ((USItype) (n0)),					\
	     "1" ((USItype) (n1)),					\
	     "dmi" ((USItype) (d)))
#define UDIV_TIME 90
#define sdiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("divs%.l %4,%1:%0"						\
	   : "=d" ((USItype) (q)),					\
	     "=d" ((USItype) (r))					\
	   : "0" ((USItype) (n0)),					\
	     "1" ((USItype) (n1)),					\
	     "dmi" ((USItype) (d)))

#else /* not mc68020 */
#if !defined(__mcf5200__)
/* %/ inserts REGISTER_PREFIX, %# inserts IMMEDIATE_PREFIX.  */
#define umul_ppmm(xh, xl, a, b) \
  __asm__ ("| Inlined umul_ppmm\n"					\
	   "	move%.l	%2,%/d0\n"					\
	   "	move%.l	%3,%/d1\n"					\
	   "	move%.l	%/d0,%/d2\n"					\
	   "	swap	%/d0\n"						\
	   "	move%.l	%/d1,%/d3\n"					\
	   "	swap	%/d1\n"						\
	   "	move%.w	%/d2,%/d4\n"					\
	   "	mulu	%/d3,%/d4\n"					\
	   "	mulu	%/d1,%/d2\n"					\
	   "	mulu	%/d0,%/d3\n"					\
	   "	mulu	%/d0,%/d1\n"					\
	   "	move%.l	%/d4,%/d0\n"					\
	   "	eor%.w	%/d0,%/d0\n"					\
	   "	swap	%/d0\n"						\
	   "	add%.l	%/d0,%/d2\n"					\
	   "	add%.l	%/d3,%/d2\n"					\
	   "	jcc	1f\n"						\
	   "	add%.l	%#65536,%/d1\n"					\
	   "1:	swap	%/d2\n"						\
	   "	moveq	%#0,%/d0\n"					\
	   "	move%.w	%/d2,%/d0\n"					\
	   "	move%.w	%/d4,%/d2\n"					\
	   "	move%.l	%/d2,%1\n"					\
	   "	add%.l	%/d1,%/d0\n"					\
	   "	move%.l	%/d0,%0"					\
	   : "=g" ((USItype) (xh)),					\
	     "=g" ((USItype) (xl))					\
	   : "g" ((USItype) (a)),					\
	     "g" ((USItype) (b))					\
	   : "d0", "d1", "d2", "d3", "d4")
#define UMUL_TIME 100
#define UDIV_TIME 400
#endif /* not mcf5200 */
#endif /* not mc68020 */

/* The '020, '030, '040 and '060 have bitfield insns.  */
#if defined (__mc68020__) || defined(mc68020) \
	|| defined(__mc68030__) || defined(mc68030) \
	|| defined(__mc68040__) || defined(mc68040) \
	|| defined(__mc68060__) || defined(mc68060) \
	|| defined(__NeXT__)
#define count_leading_zeros(count, x) \
  __asm__ ("bfffo %1{%b2:%b2},%0"					\
	   : "=d" ((USItype) (count))					\
	   : "od" ((USItype) (x)), "n" (0))
#endif
#endif /* mc68000 */

#if defined (__m88000__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addu.co %1,%r4,%r5\n\taddu.ci %0,%r2,%r3"			\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%rJ" ((USItype) (ah)),					\
	     "rJ" ((USItype) (bh)),					\
	     "%rJ" ((USItype) (al)),					\
	     "rJ" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subu.co %1,%r4,%r5\n\tsubu.ci %0,%r2,%r3"			\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "rJ" ((USItype) (ah)),					\
	     "rJ" ((USItype) (bh)),					\
	     "rJ" ((USItype) (al)),					\
	     "rJ" ((USItype) (bl)))
#define count_leading_zeros(count, x) \
  do {									\
    USItype __cbtmp;							\
    __asm__ ("ff1 %0,%1"						\
	     : "=r" (__cbtmp)						\
	     : "r" ((USItype) (x)));					\
    (count) = __cbtmp ^ 31;						\
  } while (0)
#define COUNT_LEADING_ZEROS_0 63 /* sic */
#if defined (__mc88110__)
#define umul_ppmm(wh, wl, u, v) \
  do {									\
    union {UDItype __ll;						\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
    __asm__ ("mulu.d	%0,%1,%2"					\
	     : "=r" (__xx.__ll)						\
	     : "r" ((USItype) (u)),					\
	       "r" ((USItype) (v)));					\
    (wh) = __xx.__i.__h;						\
    (wl) = __xx.__i.__l;						\
  } while (0)
#define udiv_qrnnd(q, r, n1, n0, d) \
  ({union {UDItype __ll;						\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
  USItype __q;								\
  __xx.__i.__h = (n1); __xx.__i.__l = (n0);				\
  __asm__ ("divu.d %0,%1,%2"						\
	   : "=r" (__q)							\
	   : "r" (__xx.__ll),						\
	     "r" ((USItype) (d)));					\
  (r) = (n0) - __q * (d); (q) = __q; })
#define UMUL_TIME 5
#define UDIV_TIME 25
#else
#define UMUL_TIME 17
#define UDIV_TIME 150
#endif /* __mc88110__ */
#endif /* __m88000__ */

#if defined (__mips__) && W_TYPE_SIZE == 32
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("multu %2,%3"						\
	   : "=l" ((USItype) (w0)),					\
	     "=h" ((USItype) (w1))					\
	   : "d" ((USItype) (u)),					\
	     "d" ((USItype) (v)))
#define UMUL_TIME 10
#define UDIV_TIME 100
#endif /* __mips__ */

#if defined (__ns32000__) && W_TYPE_SIZE == 32
#define umul_ppmm(w1, w0, u, v) \
  ({union {UDItype __ll;						\
	   struct {USItype __l, __h;} __i;				\
	  } __xx;							\
  __asm__ ("meid %2,%0"							\
	   : "=g" (__xx.__ll)						\
	   : "%0" ((USItype) (u)),					\
	     "g" ((USItype) (v)));					\
  (w1) = __xx.__i.__h; (w0) = __xx.__i.__l;})
#define __umulsidi3(u, v) \
  ({UDItype __w;							\
    __asm__ ("meid %2,%0"						\
	     : "=g" (__w)						\
	     : "%0" ((USItype) (u)),					\
	       "g" ((USItype) (v)));					\
    __w; })
#define udiv_qrnnd(q, r, n1, n0, d) \
  ({union {UDItype __ll;						\
	   struct {USItype __l, __h;} __i;				\
	  } __xx;							\
  __xx.__i.__h = (n1); __xx.__i.__l = (n0);				\
  __asm__ ("deid %2,%0"							\
	   : "=g" (__xx.__ll)						\
	   : "0" (__xx.__ll),						\
	     "g" ((USItype) (d)));					\
  (r) = __xx.__i.__l; (q) = __xx.__i.__h; })
#define count_trailing_zeros(count,x) \
  do {
    __asm__ ("ffsd     %2,%0"                                          \
            : "=r" ((USItype) (count))                                 \
            : "0" ((USItype) 0),                                       \
              "r" ((USItype) (x)));                                    \
  } while (0)
#endif /* __ns32000__ */

#if (defined (_ARCH_PPC) || defined (_IBMR2))
#if W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  do {									\
    if (__builtin_constant_p (bh) && (bh) == 0)				\
      __asm__ ("{a%I4|add%I4c} %1,%3,%4\n\t{aze|addze} %0,%2"		\
	     : "=r" ((USItype) (sh)),					\
	       "=&r" ((USItype) (sl))					\
	     : "%r" ((USItype) (ah)),					\
	       "%r" ((USItype) (al)),					\
	       "rI" ((USItype) (bl)));					\
    else if (__builtin_constant_p (bh) && (bh) ==~(USItype) 0)		\
      __asm__ ("{a%I4|add%I4c} %1,%3,%4\n\t{ame|addme} %0,%2"		\
	     : "=r" ((USItype) (sh)),					\
	       "=&r" ((USItype) (sl))					\
	     : "%r" ((USItype) (ah)),					\
	       "%r" ((USItype) (al)),					\
	       "rI" ((USItype) (bl)));					\
    else								\
      __asm__ ("{a%I5|add%I5c} %1,%4,%5\n\t{ae|adde} %0,%2,%3"		\
	     : "=r" ((USItype) (sh)),					\
	       "=&r" ((USItype) (sl))					\
	     : "%r" ((USItype) (ah)),					\
	       "r" ((USItype) (bh)),					\
	       "%r" ((USItype) (al)),					\
	       "rI" ((USItype) (bl)));					\
  } while (0)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  do {									\
    if (__builtin_constant_p (ah) && (ah) == 0)				\
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{sfze|subfze} %0,%2"	\
	       : "=r" ((USItype) (sh)),					\
		 "=&r" ((USItype) (sl))					\
	       : "r" ((USItype) (bh)),					\
		 "rI" ((USItype) (al)),					\
		 "r" ((USItype) (bl)));					\
    else if (__builtin_constant_p (ah) && (ah) ==~(USItype) 0)		\
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{sfme|subfme} %0,%2"	\
	       : "=r" ((USItype) (sh)),					\
		 "=&r" ((USItype) (sl))					\
	       : "r" ((USItype) (bh)),					\
		 "rI" ((USItype) (al)),					\
		 "r" ((USItype) (bl)));					\
    else if (__builtin_constant_p (bh) && (bh) == 0)			\
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{ame|addme} %0,%2"		\
	       : "=r" ((USItype) (sh)),					\
		 "=&r" ((USItype) (sl))					\
	       : "r" ((USItype) (ah)),					\
		 "rI" ((USItype) (al)),					\
		 "r" ((USItype) (bl)));					\
    else if (__builtin_constant_p (bh) && (bh) ==~(USItype) 0)		\
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{aze|addze} %0,%2"		\
	       : "=r" ((USItype) (sh)),					\
		 "=&r" ((USItype) (sl))					\
	       : "r" ((USItype) (ah)),					\
		 "rI" ((USItype) (al)),					\
		 "r" ((USItype) (bl)));					\
    else								\
      __asm__ ("{sf%I4|subf%I4c} %1,%5,%4\n\t{sfe|subfe} %0,%3,%2"	\
	       : "=r" ((USItype) (sh)),					\
		 "=&r" ((USItype) (sl))					\
	       : "r" ((USItype) (ah)),					\
		 "r" ((USItype) (bh)),					\
		 "rI" ((USItype) (al)),					\
		 "r" ((USItype) (bl)));					\
  } while (0)
#endif /* W_TYPE_SIZE */
#define count_leading_zeros(count, x) \
  __asm__ ("{cntlz|cntlzw} %0,%1"					\
	   : "=r" ((USItype) (count))					\
	   : "r" ((USItype) (x)))
#define COUNT_LEADING_ZEROS_0 32
#if defined (_ARCH_PPC)
#define umul_ppmm(ph, pl, m0, m1) \
  do {									\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("mulhwu %0,%1,%2"						\
	     : "=r" ((USItype) ph)					\
	     : "%r" (__m0),						\
	       "r" (__m1));						\
    (pl) = __m0 * __m1;							\
  } while (0)
#define UMUL_TIME 15
#define smul_ppmm(ph, pl, m0, m1) \
  do {									\
    SItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("mulhw %0,%1,%2"						\
	     : "=r" ((SItype) ph)					\
	     : "%r" (__m0),						\
	       "r" (__m1));						\
    (pl) = __m0 * __m1;							\
  } while (0)
#define SMUL_TIME 14
#define UDIV_TIME 120
#elif defined (_ARCH_PWR)
#define umul_ppmm(xh, xl, m0, m1) \
  do {									\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("mul %0,%2,%3"						\
	     : "=r" ((USItype) (xh)),					\
	       "=q" ((USItype) (xl))					\
	     : "r" (__m0),						\
	       "r" (__m1));						\
    (xh) += ((((SItype) __m0 >> 31) & __m1)				\
	     + (((SItype) __m1 >> 31) & __m0));				\
  } while (0)
#define UMUL_TIME 8
#define smul_ppmm(xh, xl, m0, m1) \
  __asm__ ("mul %0,%2,%3"						\
	   : "=r" ((SItype) (xh)),					\
	     "=q" ((SItype) (xl))					\
	   : "r" (m0),							\
	     "r" (m1))
#define SMUL_TIME 4
#define sdiv_qrnnd(q, r, nh, nl, d) \
  __asm__ ("div %0,%2,%4"						\
	   : "=r" ((SItype) (q)), "=q" ((SItype) (r))			\
	   : "r" ((SItype) (nh)), "1" ((SItype) (nl)), "r" ((SItype) (d)))
#define UDIV_TIME 100
#endif
#endif /* Power architecture variants.  */

#if defined (__pyr__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addw	%5,%1\n\taddwc	%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subw	%5,%1\n\tsubwb	%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
/* This insn works on Pyramids with AP, XP, or MI CPUs, but not with SP.  */
#define umul_ppmm(w1, w0, u, v) \
  ({union {UDItype __ll;						\
	   struct {USItype __h, __l;} __i;				\
	  } __xx;							\
  __asm__ ("movw %1,%R0\n\tuemul %2,%0"					\
	   : "=&r" (__xx.__ll)						\
	   : "g" ((USItype) (u)),					\
	     "g" ((USItype) (v)));					\
  (w1) = __xx.__i.__h; (w0) = __xx.__i.__l;})
#endif /* __pyr__ */

#if defined (__ibm032__) /* RT/ROMP */ && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("a %1,%5\n\tae %0,%3"					\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "r" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "r" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("s %1,%5\n\tse %0,%3"					\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "r" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "r" ((USItype) (bl)))
#define umul_ppmm(ph, pl, m0, m1) \
  do {									\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ (								\
       "s	r2,r2\n"						\
"	mts	r10,%2\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	m	r2,%3\n"						\
"	cas	%0,r2,r0\n"						\
"	mfs	r10,%1"							\
	     : "=r" ((USItype) (ph)),					\
	       "=r" ((USItype) (pl))					\
	     : "%r" (__m0),						\
		"r" (__m1)						\
	     : "r2");							\
    (ph) += ((((SItype) __m0 >> 31) & __m1)				\
	     + (((SItype) __m1 >> 31) & __m0));				\
  } while (0)
#define UMUL_TIME 20
#define UDIV_TIME 200
#define count_leading_zeros(count, x) \
  do {									\
    if ((x) >= 0x10000)							\
      __asm__ ("clz	%0,%1"						\
	       : "=r" ((USItype) (count))				\
	       : "r" ((USItype) (x) >> 16));				\
    else								\
      {									\
	__asm__ ("clz	%0,%1"						\
		 : "=r" ((USItype) (count))				\
		 : "r" ((USItype) (x)));					\
	(count) += 16;							\
      }									\
  } while (0)
#endif

#if defined (__sh2__) && W_TYPE_SIZE == 32
#define umul_ppmm(w1, w0, u, v) \
  __asm__ (								\
       "dmulu.l	%2,%3\n\tsts	macl,%1\n\tsts	mach,%0"		\
	   : "=r" ((USItype)(w1)),					\
	     "=r" ((USItype)(w0))					\
	   : "r" ((USItype)(u)),					\
	     "r" ((USItype)(v))						\
	   : "macl", "mach")
#define UMUL_TIME 5
#endif

#if defined (__SH5__) && __SHMEDIA__ && W_TYPE_SIZE == 32
#define __umulsidi3(u,v) ((UDItype)(USItype)u*(USItype)v)
#define count_leading_zeros(count, x) \
  do									\
    {									\
      UDItype x_ = (USItype)(x);					\
      SItype c_;							\
									\
      __asm__ ("nsb %1, %0" : "=r" (c_) : "r" (x_));			\
      (count) = c_ - 31;						\
    }									\
  while (0)
#define COUNT_LEADING_ZEROS_0 32
#endif

#if defined (__sparc__) && !defined (__arch64__) && !defined (__sparcv9) \
    && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addcc %r4,%5,%1\n\taddx %r2,%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "%rJ" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "%rJ" ((USItype) (al)),					\
	     "rI" ((USItype) (bl))					\
	   __CLOBBER_CC)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subcc %r4,%5,%1\n\tsubx %r2,%3,%0"				\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "rJ" ((USItype) (ah)),					\
	     "rI" ((USItype) (bh)),					\
	     "rJ" ((USItype) (al)),					\
	     "rI" ((USItype) (bl))					\
	   __CLOBBER_CC)
#if defined (__sparc_v8__)
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("umul %2,%3,%1;rd %%y,%0"					\
	   : "=r" ((USItype) (w1)),					\
	     "=r" ((USItype) (w0))					\
	   : "r" ((USItype) (u)),					\
	     "r" ((USItype) (v)))
#define udiv_qrnnd(__q, __r, __n1, __n0, __d) \
  __asm__ ("mov %2,%%y;nop;nop;nop;udiv %3,%4,%0;umul %0,%4,%1;sub %3,%1,%1"\
	   : "=&r" ((USItype) (__q)),					\
	     "=&r" ((USItype) (__r))					\
	   : "r" ((USItype) (__n1)),					\
	     "r" ((USItype) (__n0)),					\
	     "r" ((USItype) (__d)))
#else
#if defined (__sparclite__)
/* This has hardware multiply but not divide.  It also has two additional
   instructions scan (ffs from high bit) and divscc.  */
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("umul %2,%3,%1;rd %%y,%0"					\
	   : "=r" ((USItype) (w1)),					\
	     "=r" ((USItype) (w0))					\
	   : "r" ((USItype) (u)),					\
	     "r" ((USItype) (v)))
#define udiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("! Inlined udiv_qrnnd\n"					\
"	wr	%%g0,%2,%%y	! Not a delayed write for sparclite\n"	\
"	tst	%%g0\n"							\
"	divscc	%3,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%%g1\n"						\
"	divscc	%%g1,%4,%0\n"						\
"	rd	%%y,%1\n"						\
"	bl,a 1f\n"							\
"	add	%1,%4,%1\n"						\
"1:	! End of inline udiv_qrnnd"					\
	   : "=r" ((USItype) (q)),					\
	     "=r" ((USItype) (r))					\
	   : "r" ((USItype) (n1)),					\
	     "r" ((USItype) (n0)),					\
	     "rI" ((USItype) (d))					\
	   : "g1" __AND_CLOBBER_CC)
#define UDIV_TIME 37
#define count_leading_zeros(count, x) \
  do {                                                                  \
  __asm__ ("scan %1,1,%0"                                               \
           : "=r" ((USItype) (count))                                   \
           : "r" ((USItype) (x)));					\
  } while (0)
/* Early sparclites return 63 for an argument of 0, but they warn that future
   implementations might change this.  Therefore, leave COUNT_LEADING_ZEROS_0
   undefined.  */
#else
/* SPARC without integer multiplication and divide instructions.
   (i.e. at least Sun4/20,40,60,65,75,110,260,280,330,360,380,470,490) */
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("! Inlined umul_ppmm\n"					\
"	wr	%%g0,%2,%%y	! SPARC has 0-3 delay insn after a wr\n"\
"	sra	%3,31,%%o5	! Don't move this insn\n"		\
"	and	%2,%%o5,%%o5	! Don't move this insn\n"		\
"	andcc	%%g0,0,%%g1	! Don't move this insn\n"		\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,%3,%%g1\n"						\
"	mulscc	%%g1,0,%%g1\n"						\
"	add	%%g1,%%o5,%0\n"						\
"	rd	%%y,%1"							\
	   : "=r" ((USItype) (w1)),					\
	     "=r" ((USItype) (w0))					\
	   : "%rI" ((USItype) (u)),					\
	     "r" ((USItype) (v))						\
	   : "g1", "o5" __AND_CLOBBER_CC)
#define UMUL_TIME 39		/* 39 instructions */
/* It's quite necessary to add this much assembler for the sparc.
   The default udiv_qrnnd (in C) is more than 10 times slower!  */
#define udiv_qrnnd(__q, __r, __n1, __n0, __d) \
  __asm__ ("! Inlined udiv_qrnnd\n"					\
"	mov	32,%%g1\n"						\
"	subcc	%1,%2,%%g0\n"						\
"1:	bcs	5f\n"							\
"	 addxcc %0,%0,%0	! shift n1n0 and a q-bit in lsb\n"	\
"	sub	%1,%2,%1	! this kills msb of n\n"		\
"	addx	%1,%1,%1	! so this can't give carry\n"		\
"	subcc	%%g1,1,%%g1\n"						\
"2:	bne	1b\n"							\
"	 subcc	%1,%2,%%g0\n"						\
"	bcs	3f\n"							\
"	 addxcc %0,%0,%0	! shift n1n0 and a q-bit in lsb\n"	\
"	b	3f\n"							\
"	 sub	%1,%2,%1	! this kills msb of n\n"		\
"4:	sub	%1,%2,%1\n"						\
"5:	addxcc	%1,%1,%1\n"						\
"	bcc	2b\n"							\
"	 subcc	%%g1,1,%%g1\n"						\
"! Got carry from n.  Subtract next step to cancel this carry.\n"	\
"	bne	4b\n"							\
"	 addcc	%0,%0,%0	! shift n1n0 and a 0-bit in lsb\n"	\
"	sub	%1,%2,%1\n"						\
"3:	xnor	%0,0,%0\n"						\
"	! End of inline udiv_qrnnd"					\
	   : "=&r" ((USItype) (__q)),					\
	     "=&r" ((USItype) (__r))					\
	   : "r" ((USItype) (__d)),					\
	     "1" ((USItype) (__n1)),					\
	     "0" ((USItype) (__n0)) : "g1" __AND_CLOBBER_CC)
#define UDIV_TIME (3+7*32)	/* 7 instructions/iteration. 32 iterations.  */
#endif /* __sparclite__ */
#endif /* __sparc_v8__ */
#endif /* sparc32 */

#if ((defined (__sparc__) && defined (__arch64__)) || defined (__sparcv9)) \
    && W_TYPE_SIZE == 64
#define add_ssaaaa(sh, sl, ah, al, bh, bl)				\
  __asm__ ("addcc %r4,%5,%1\n\t"					\
   	   "add %r2,%3,%0\n\t"						\
   	   "bcs,a,pn %%xcc, 1f\n\t"					\
   	   "add %0, 1, %0\n"						\
	   "1:"								\
	   : "=r" ((UDItype)(sh)),				      	\
	     "=&r" ((UDItype)(sl))				      	\
	   : "%rJ" ((UDItype)(ah)),				     	\
	     "rI" ((UDItype)(bh)),				      	\
	     "%rJ" ((UDItype)(al)),				     	\
	     "rI" ((UDItype)(bl))				       	\
	   __CLOBBER_CC)

#define sub_ddmmss(sh, sl, ah, al, bh, bl) 				\
  __asm__ ("subcc %r4,%5,%1\n\t"					\
   	   "sub %r2,%3,%0\n\t"						\
   	   "bcs,a,pn %%xcc, 1f\n\t"					\
   	   "sub %0, 1, %0\n\t"						\
	   "1:"								\
	   : "=r" ((UDItype)(sh)),				      	\
	     "=&r" ((UDItype)(sl))				      	\
	   : "rJ" ((UDItype)(ah)),				     	\
	     "rI" ((UDItype)(bh)),				      	\
	     "rJ" ((UDItype)(al)),				     	\
	     "rI" ((UDItype)(bl))				       	\
	   __CLOBBER_CC)

#define umul_ppmm(wh, wl, u, v)						\
  do {									\
	  UDItype tmp1, tmp2, tmp3, tmp4;				\
	  __asm__ __volatile__ (					\
		   "srl %7,0,%3\n\t"					\
		   "mulx %3,%6,%1\n\t"					\
		   "srlx %6,32,%2\n\t"					\
		   "mulx %2,%3,%4\n\t"					\
		   "sllx %4,32,%5\n\t"					\
		   "srl %6,0,%3\n\t"					\
		   "sub %1,%5,%5\n\t"					\
		   "srlx %5,32,%5\n\t"					\
		   "addcc %4,%5,%4\n\t"					\
		   "srlx %7,32,%5\n\t"					\
		   "mulx %3,%5,%3\n\t"					\
		   "mulx %2,%5,%5\n\t"					\
		   "sethi %%hi(0x80000000),%2\n\t"			\
		   "addcc %4,%3,%4\n\t"					\
		   "srlx %4,32,%4\n\t"					\
		   "add %2,%2,%2\n\t"					\
		   "movcc %%xcc,%%g0,%2\n\t"				\
		   "addcc %5,%4,%5\n\t"					\
		   "sllx %3,32,%3\n\t"					\
		   "add %1,%3,%1\n\t"					\
		   "add %5,%2,%0"					\
	   : "=r" ((UDItype)(wh)),					\
	     "=&r" ((UDItype)(wl)),					\
	     "=&r" (tmp1), "=&r" (tmp2), "=&r" (tmp3), "=&r" (tmp4)	\
	   : "r" ((UDItype)(u)),					\
	     "r" ((UDItype)(v))						\
	   __CLOBBER_CC);						\
  } while (0)
#define UMUL_TIME 96
#define UDIV_TIME 230
#endif /* sparc64 */

#if defined (__vax__) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addl2 %5,%1\n\tadwc %3,%0"					\
	   : "=g" ((USItype) (sh)),					\
	     "=&g" ((USItype) (sl))					\
	   : "%0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "%1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subl2 %5,%1\n\tsbwc %3,%0"					\
	   : "=g" ((USItype) (sh)),					\
	     "=&g" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define umul_ppmm(xh, xl, m0, m1) \
  do {									\
    union {								\
	UDItype __ll;							\
	struct {USItype __l, __h;} __i;					\
      } __xx;								\
    USItype __m0 = (m0), __m1 = (m1);					\
    __asm__ ("emul %1,%2,$0,%0"						\
	     : "=r" (__xx.__ll)						\
	     : "g" (__m0),						\
	       "g" (__m1));						\
    (xh) = __xx.__i.__h;						\
    (xl) = __xx.__i.__l;						\
    (xh) += ((((SItype) __m0 >> 31) & __m1)				\
	     + (((SItype) __m1 >> 31) & __m0));				\
  } while (0)
#define sdiv_qrnnd(q, r, n1, n0, d) \
  do {									\
    union {DItype __ll;							\
	   struct {SItype __l, __h;} __i;				\
	  } __xx;							\
    __xx.__i.__h = n1; __xx.__i.__l = n0;				\
    __asm__ ("ediv %3,%2,%0,%1"						\
	     : "=g" (q), "=g" (r)					\
	     : "g" (__xx.__ll), "g" (d));				\
  } while (0)
#endif /* __vax__ */

#if defined (__z8000__) && W_TYPE_SIZE == 16
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("add	%H1,%H5\n\tadc	%H0,%H3"				\
	   : "=r" ((unsigned int)(sh)),					\
	     "=&r" ((unsigned int)(sl))					\
	   : "%0" ((unsigned int)(ah)),					\
	     "r" ((unsigned int)(bh)),					\
	     "%1" ((unsigned int)(al)),					\
	     "rQR" ((unsigned int)(bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("sub	%H1,%H5\n\tsbc	%H0,%H3"				\
	   : "=r" ((unsigned int)(sh)),					\
	     "=&r" ((unsigned int)(sl))					\
	   : "0" ((unsigned int)(ah)),					\
	     "r" ((unsigned int)(bh)),					\
	     "1" ((unsigned int)(al)),					\
	     "rQR" ((unsigned int)(bl)))
#define umul_ppmm(xh, xl, m0, m1) \
  do {									\
    union {long int __ll;						\
	   struct {unsigned int __h, __l;} __i;				\
	  } __xx;							\
    unsigned int __m0 = (m0), __m1 = (m1);				\
    __asm__ ("mult	%S0,%H3"					\
	     : "=r" (__xx.__i.__h),					\
	       "=r" (__xx.__i.__l)					\
	     : "%1" (__m0),						\
	       "rQR" (__m1));						\
    (xh) = __xx.__i.__h; (xl) = __xx.__i.__l;				\
    (xh) += ((((signed int) __m0 >> 15) & __m1)				\
	     + (((signed int) __m1 >> 15) & __m0));			\
  } while (0)
#endif /* __z8000__ */

#endif /* __GNUC__ */

/* If this machine has no inline assembler, use C macros.  */

#if !defined (add_ssaaaa)
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  do {									\
    UWtype __x;								\
    __x = (al) + (bl);							\
    (sh) = (ah) + (bh) + (__x < (al));					\
    (sl) = __x;								\
  } while (0)
#endif

#if !defined (sub_ddmmss)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  do {									\
    UWtype __x;								\
    __x = (al) - (bl);							\
    (sh) = (ah) - (bh) - (__x > (al));					\
    (sl) = __x;								\
  } while (0)
#endif

#if !defined (umul_ppmm)
#define umul_ppmm(w1, w0, u, v)						\
  do {									\
    UWtype __x0, __x1, __x2, __x3;					\
    UHWtype __ul, __vl, __uh, __vh;					\
									\
    __ul = __ll_lowpart (u);						\
    __uh = __ll_highpart (u);						\
    __vl = __ll_lowpart (v);						\
    __vh = __ll_highpart (v);						\
									\
    __x0 = (UWtype) __ul * __vl;					\
    __x1 = (UWtype) __ul * __vh;					\
    __x2 = (UWtype) __uh * __vl;					\
    __x3 = (UWtype) __uh * __vh;					\
									\
    __x1 += __ll_highpart (__x0);/* this can't give carry */		\
    __x1 += __x2;		/* but this indeed can */		\
    if (__x1 < __x2)		/* did we get it? */			\
      __x3 += __ll_B;		/* yes, add it in the proper pos.  */	\
									\
    (w1) = __x3 + __ll_highpart (__x1);					\
    (w0) = __ll_lowpart (__x1) * __ll_B + __ll_lowpart (__x0);		\
  } while (0)
#endif

#if !defined (__umulsidi3)
#define __umulsidi3(u, v) \
  ({DWunion __w;							\
    umul_ppmm (__w.s.high, __w.s.low, u, v);				\
    __w.ll; })
#endif

/* Define this unconditionally, so it can be used for debugging.  */
#define __udiv_qrnnd_c(q, r, n1, n0, d) \
  do {									\
    UWtype __d1, __d0, __q1, __q0;					\
    UWtype __r1, __r0, __m;						\
    __d1 = __ll_highpart (d);						\
    __d0 = __ll_lowpart (d);						\
									\
    __r1 = (n1) % __d1;							\
    __q1 = (n1) / __d1;							\
    __m = (UWtype) __q1 * __d0;						\
    __r1 = __r1 * __ll_B | __ll_highpart (n0);				\
    if (__r1 < __m)							\
      {									\
	__q1--, __r1 += (d);						\
	if (__r1 >= (d)) /* i.e. we didn't get carry when adding to __r1 */\
	  if (__r1 < __m)						\
	    __q1--, __r1 += (d);					\
      }									\
    __r1 -= __m;							\
									\
    __r0 = __r1 % __d1;							\
    __q0 = __r1 / __d1;							\
    __m = (UWtype) __q0 * __d0;						\
    __r0 = __r0 * __ll_B | __ll_lowpart (n0);				\
    if (__r0 < __m)							\
      {									\
	__q0--, __r0 += (d);						\
	if (__r0 >= (d))						\
	  if (__r0 < __m)						\
	    __q0--, __r0 += (d);					\
      }									\
    __r0 -= __m;							\
									\
    (q) = (UWtype) __q1 * __ll_B | __q0;				\
    (r) = __r0;								\
  } while (0)

/* If the processor has no udiv_qrnnd but sdiv_qrnnd, go through
   __udiv_w_sdiv (defined in libgcc or elsewhere).  */
#if !defined (udiv_qrnnd) && defined (sdiv_qrnnd)
#define udiv_qrnnd(q, r, nh, nl, d) \
  do {									\
    USItype __r;							\
    (q) = __udiv_w_sdiv (&__r, nh, nl, d);				\
    (r) = __r;								\
  } while (0)
#endif

/* If udiv_qrnnd was not defined for this processor, use __udiv_qrnnd_c.  */
#if !defined (udiv_qrnnd)
#define UDIV_NEEDS_NORMALIZATION 1
#define udiv_qrnnd __udiv_qrnnd_c
#endif

#if !defined (count_leading_zeros)
extern const UQItype __clz_tab[];
#define count_leading_zeros(count, x) \
  do {									\
    UWtype __xr = (x);							\
    UWtype __a;								\
									\
    if (W_TYPE_SIZE <= 32)						\
      {									\
	__a = __xr < ((UWtype)1<<2*__BITS4)				\
	  ? (__xr < ((UWtype)1<<__BITS4) ? 0 : __BITS4)			\
	  : (__xr < ((UWtype)1<<3*__BITS4) ?  2*__BITS4 : 3*__BITS4);	\
      }									\
    else								\
      {									\
	for (__a = W_TYPE_SIZE - 8; __a > 0; __a -= 8)			\
	  if (((__xr >> __a) & 0xff) != 0)				\
	    break;							\
      }									\
									\
    (count) = W_TYPE_SIZE - (__clz_tab[__xr >> __a] + __a);		\
  } while (0)
#define COUNT_LEADING_ZEROS_0 W_TYPE_SIZE
#endif

#if !defined (count_trailing_zeros)
/* Define count_trailing_zeros using count_leading_zeros.  The latter might be
   defined in asm, but if it is not, the C version above is good enough.  */
#define count_trailing_zeros(count, x) \
  do {									\
    UWtype __ctz_x = (x);						\
    UWtype __ctz_c;							\
    count_leading_zeros (__ctz_c, __ctz_x & -__ctz_x);			\
    (count) = W_TYPE_SIZE - 1 - __ctz_c;				\
  } while (0)
#endif

#ifndef UDIV_NEEDS_NORMALIZATION
#define UDIV_NEEDS_NORMALIZATION 0
#endif
