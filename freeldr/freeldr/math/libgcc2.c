/* More subroutines needed by GCC output code on some machines.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001  Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combine
executable.)

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* It is incorrect to include config.h here, because this file is being
   compiled for the target, and hence definitions concerning only the host
   do not apply.  */

/*
 * This file was taken from the GCC v3.1 source - Brian
 */
#ifdef __i386__
#include "i386.h"
#endif
#define L_clz
#define L_udivdi3
#define L_umoddi3
#include <freeldr.h>

//#include "tconfig.h"
//#include "tsystem.h"

//#include "machmode.h"

/* Don't use `fancy_abort' here even if config.h says to use it.  */
#ifdef abort
#undef abort
#endif

#include "libgcc2.h"

#if defined (L_negdi2) || defined (L_divdi3) || defined (L_moddi3)
#if defined (L_divdi3) || defined (L_moddi3)
static inline
#endif
DWtype
__negdi2 (DWtype u)
{
  DWunion w;
  DWunion uu;

  uu.ll = u;

  w.s.low = -uu.s.low;
  w.s.high = -uu.s.high - ((UWtype) w.s.low > 0);

  return w.ll;
}
#endif

#ifdef L_addvsi3
Wtype
__addvsi3 (Wtype a, Wtype b)
{
  Wtype w;

  w = a + b;

  if (b >= 0 ? w < a : w > a)
    abort ();

  return w;
}
#endif

#ifdef L_addvdi3
DWtype
__addvdi3 (DWtype a, DWtype b)
{
  DWtype w;

  w = a + b;

  if (b >= 0 ? w < a : w > a)
    abort ();

  return w;
}
#endif

#ifdef L_subvsi3
Wtype
__subvsi3 (Wtype a, Wtype b)
{
#ifdef L_addvsi3
  return __addvsi3 (a, (-b));
#else
  DWtype w;

  w = a - b;

  if (b >= 0 ? w > a : w < a)
    abort ();

  return w;
#endif
}
#endif

#ifdef L_subvdi3
DWtype
__subvdi3 (DWtype a, DWtype b)
{
#ifdef L_addvdi3
  return (a, (-b));
#else
  DWtype w;

  w = a - b;

  if (b >= 0 ? w > a : w < a)
    abort ();

  return w;
#endif
}
#endif

#ifdef L_mulvsi3
Wtype
__mulvsi3 (Wtype a, Wtype b)
{
  DWtype w;

  w = a * b;

  if (((a >= 0) == (b >= 0)) ? w < 0 : w > 0)
    abort ();

  return w;
}
#endif

#ifdef L_negvsi2
Wtype
__negvsi2 (Wtype a)
{
   Wtype w;

   w  = -a;

  if (a >= 0 ? w > 0 : w < 0)
    abort ();

   return w;
}
#endif

#ifdef L_negvdi2
DWtype
__negvdi2 (DWtype a)
{
   DWtype w;

   w  = -a;

  if (a >= 0 ? w > 0 : w < 0)
    abort ();

   return w;
}
#endif

#ifdef L_absvsi2
Wtype
__absvsi2 (Wtype a)
{
   Wtype w = a;

   if (a < 0)
#ifdef L_negvsi2
     w = __negvsi2 (a);
#else
     w = -a;

   if (w < 0)
     abort ();
#endif

   return w;
}
#endif

#ifdef L_absvdi2
DWtype
__absvdi2 (DWtype a)
{
   DWtype w = a;

   if (a < 0)
#ifdef L_negvsi2
     w = __negvsi2 (a);
#else
     w = -a;

   if (w < 0)
     abort ();
#endif

   return w;
}
#endif

#ifdef L_mulvdi3
DWtype
__mulvdi3 (DWtype u, DWtype v)
{
   DWtype w;

  w = u * v;

  if (((u >= 0) == (v >= 0)) ? w < 0 : w > 0)
    abort ();

  return w;
}
#endif


/* Unless shift functions are defined whith full ANSI prototypes,
   parameter b will be promoted to int if word_type is smaller than an int.  */
#ifdef L_lshrdi3
DWtype
__lshrdi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.high = 0;
      w.s.low = (UWtype) uu.s.high >> -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.high << bm;

      w.s.high = (UWtype) uu.s.high >> b;
      w.s.low = ((UWtype) uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashldi3
DWtype
__ashldi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.low = 0;
      w.s.high = (UWtype) uu.s.low << -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.low >> bm;

      w.s.low = (UWtype) uu.s.low << b;
      w.s.high = ((UWtype) uu.s.high << b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashrdi3
DWtype
__ashrdi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      /* w.s.high = 1..1 or 0..0 */
      w.s.high = uu.s.high >> (sizeof (Wtype) * BITS_PER_UNIT - 1);
      w.s.low = uu.s.high >> -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.high << bm;

      w.s.high = uu.s.high >> b;
      w.s.low = ((UWtype) uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ffsdi2
DWtype
__ffsdi2 (DWtype u)
{
  DWunion uu;
  UWtype word, count, add;

  uu.ll = u;
  if (uu.s.low != 0)
    word = uu.s.low, add = 0;
  else if (uu.s.high != 0)
    word = uu.s.high, add = BITS_PER_UNIT * sizeof (Wtype);
  else
    return 0;

  count_trailing_zeros (count, word);
  return count + add + 1;
}
#endif

#ifdef L_muldi3
DWtype
__muldi3 (DWtype u, DWtype v)
{
  DWunion w;
  DWunion uu, vv;

  uu.ll = u,
  vv.ll = v;

  w.ll = __umulsidi3 (uu.s.low, vv.s.low);
  w.s.high += ((UWtype) uu.s.low * (UWtype) vv.s.high
	       + (UWtype) uu.s.high * (UWtype) vv.s.low);

  return w.ll;
}
#endif

#ifdef L_udiv_w_sdiv
#if defined (sdiv_qrnnd)
UWtype
__udiv_w_sdiv (UWtype *rp, UWtype a1, UWtype a0, UWtype d)
{
  UWtype q, r;
  UWtype c0, c1, b1;

  if ((Wtype) d >= 0)
    {
      if (a1 < d - a1 - (a0 >> (W_TYPE_SIZE - 1)))
	{
	  /* dividend, divisor, and quotient are nonnegative */
	  sdiv_qrnnd (q, r, a1, a0, d);
	}
      else
	{
	  /* Compute c1*2^32 + c0 = a1*2^32 + a0 - 2^31*d */
	  sub_ddmmss (c1, c0, a1, a0, d >> 1, d << (W_TYPE_SIZE - 1));
	  /* Divide (c1*2^32 + c0) by d */
	  sdiv_qrnnd (q, r, c1, c0, d);
	  /* Add 2^31 to quotient */
	  q += (UWtype) 1 << (W_TYPE_SIZE - 1);
	}
    }
  else
    {
      b1 = d >> 1;			/* d/2, between 2^30 and 2^31 - 1 */
      c1 = a1 >> 1;			/* A/2 */
      c0 = (a1 << (W_TYPE_SIZE - 1)) + (a0 >> 1);

      if (a1 < b1)			/* A < 2^32*b1, so A/2 < 2^31*b1 */
	{
	  sdiv_qrnnd (q, r, c1, c0, b1); /* (A/2) / (d/2) */

	  r = 2*r + (a0 & 1);		/* Remainder from A/(2*b1) */
	  if ((d & 1) != 0)
	    {
	      if (r >= q)
		r = r - q;
	      else if (q - r <= d)
		{
		  r = r - q + d;
		  q--;
		}
	      else
		{
		  r = r - q + 2*d;
		  q -= 2;
		}
	    }
	}
      else if (c1 < b1)			/* So 2^31 <= (A/2)/b1 < 2^32 */
	{
	  c1 = (b1 - 1) - c1;
	  c0 = ~c0;			/* logical NOT */

	  sdiv_qrnnd (q, r, c1, c0, b1); /* (A/2) / (d/2) */

	  q = ~q;			/* (A/2)/b1 */
	  r = (b1 - 1) - r;

	  r = 2*r + (a0 & 1);		/* A/(2*b1) */

	  if ((d & 1) != 0)
	    {
	      if (r >= q)
		r = r - q;
	      else if (q - r <= d)
		{
		  r = r - q + d;
		  q--;
		}
	      else
		{
		  r = r - q + 2*d;
		  q -= 2;
		}
	    }
	}
      else				/* Implies c1 = b1 */
	{				/* Hence a1 = d - 1 = 2*b1 - 1 */
	  if (a0 >= -d)
	    {
	      q = -1;
	      r = a0 + d;
	    }
	  else
	    {
	      q = -2;
	      r = a0 + 2*d;
	    }
	}
    }

  *rp = r;
  return q;
}
#else
/* If sdiv_qrnnd doesn't exist, define dummy __udiv_w_sdiv.  */
UWtype
__udiv_w_sdiv (UWtype *rp __attribute__ ((__unused__)),
	       UWtype a1 __attribute__ ((__unused__)),
	       UWtype a0 __attribute__ ((__unused__)),
	       UWtype d __attribute__ ((__unused__)))
{
  return 0;
}
#endif
#endif

#if (defined (L_udivdi3) || defined (L_divdi3) || \
     defined (L_umoddi3) || defined (L_moddi3))
#define L_udivmoddi4
#endif

#ifdef L_clz
const UQItype __clz_tab[] =
{
  0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};
#endif

#ifdef L_udivmoddi4

#if (defined (L_udivdi3) || defined (L_divdi3) || \
     defined (L_umoddi3) || defined (L_moddi3))
static inline
#endif
UDWtype
__udivmoddi4 (UDWtype n, UDWtype d, UDWtype *rp)
{
  DWunion ww;
  DWunion nn, dd;
  DWunion rr;
  UWtype d0, d1, n0, n1, n2;
  UWtype q0, q1;
  UWtype b, bm;

  nn.ll = n;
  dd.ll = d;

  d0 = dd.s.low;
  d1 = dd.s.high;
  n0 = nn.s.low;
  n1 = nn.s.high;

#if !UDIV_NEEDS_NORMALIZATION
  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  udiv_qrnnd (q1, n1, 0, n1, d0);
	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }

#else /* UDIV_NEEDS_NORMALIZATION */

  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  count_leading_zeros (bm, d0);

	  if (bm != 0)
	    {
	      /* Normalize, i.e. make the most significant bit of the
		 denominator set.  */

	      d0 = d0 << bm;
	      n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
	      n0 = n0 << bm;
	    }

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0 >> bm.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  count_leading_zeros (bm, d0);

	  if (bm == 0)
	    {
	      /* From (n1 >= d0) /\ (the most significant bit of d0 is set),
		 conclude (the most significant bit of n1 is set) /\ (the
		 leading quotient digit q1 = 1).

		 This special case is necessary, not an optimization.
		 (Shifts counts of W_TYPE_SIZE are undefined.)  */

	      n1 -= d0;
	      q1 = 1;
	    }
	  else
	    {
	      /* Normalize.  */

	      b = W_TYPE_SIZE - bm;

	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q1, n1, n2, n1, d0);
	    }

	  /* n1 != d0...  */

	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0 >> bm.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0 >> bm;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }
#endif /* UDIV_NEEDS_NORMALIZATION */

  else
    {
      if (d1 > n1)
	{
	  /* 00 = nn / DD */

	  q0 = 0;
	  q1 = 0;

	  /* Remainder in n1n0.  */
	  if (rp != 0)
	    {
	      rr.s.low = n0;
	      rr.s.high = n1;
	      *rp = rr.ll;
	    }
	}
      else
	{
	  /* 0q = NN / dd */

	  count_leading_zeros (bm, d1);
	  if (bm == 0)
	    {
	      /* From (n1 >= d1) /\ (the most significant bit of d1 is set),
		 conclude (the most significant bit of n1 is set) /\ (the
		 quotient digit q0 = 0 or 1).

		 This special case is necessary, not an optimization.  */

	      /* The condition on the next line takes advantage of that
		 n1 >= d1 (true due to program flow).  */
	      if (n1 > d1 || n0 >= d0)
		{
		  q0 = 1;
		  sub_ddmmss (n1, n0, n1, n0, d1, d0);
		}
	      else
		q0 = 0;

	      q1 = 0;

	      if (rp != 0)
		{
		  rr.s.low = n0;
		  rr.s.high = n1;
		  *rp = rr.ll;
		}
	    }
	  else
	    {
	      UWtype m1, m0;
	      /* Normalize.  */

	      b = W_TYPE_SIZE - bm;

	      d1 = (d1 << bm) | (d0 >> b);
	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q0, n1, n2, n1, d1);
	      umul_ppmm (m1, m0, q0, d0);

	      if (m1 > n1 || (m1 == n1 && m0 > n0))
		{
		  q0--;
		  sub_ddmmss (m1, m0, m1, m0, d1, d0);
		}

	      q1 = 0;

	      /* Remainder in (n1n0 - m1m0) >> bm.  */
	      if (rp != 0)
		{
		  sub_ddmmss (n1, n0, n1, n0, m1, m0);
		  rr.s.low = (n1 << b) | (n0 >> bm);
		  rr.s.high = n1 >> bm;
		  *rp = rr.ll;
		}
	    }
	}
    }

  ww.s.low = q0;
  ww.s.high = q1;
  return ww.ll;
}
#endif

#ifdef L_divdi3
DWtype
__divdi3 (DWtype u, DWtype v)
{
  word_type c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c,
    uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    c = ~c,
    vv.ll = __negdi2 (vv.ll);

  w = __udivmoddi4 (uu.ll, vv.ll, (UDWtype *) 0);
  if (c)
    w = __negdi2 (w);

  return w;
}
#endif

#ifdef L_moddi3
DWtype
__moddi3 (DWtype u, DWtype v)
{
  word_type c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c,
    uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    vv.ll = __negdi2 (vv.ll);

  (void) __udivmoddi4 (uu.ll, vv.ll, &w);
  if (c)
    w = __negdi2 (w);

  return w;
}
#endif

#ifdef L_umoddi3
UDWtype
__umoddi3 (UDWtype u, UDWtype v)
{
  UDWtype w;

  (void) __udivmoddi4 (u, v, &w);

  return w;
}
#endif

#ifdef L_udivdi3
UDWtype
__udivdi3 (UDWtype n, UDWtype d)
{
  return __udivmoddi4 (n, d, (UDWtype *) 0);
}
#endif

#ifdef L_cmpdi2
word_type
__cmpdi2 (DWtype a, DWtype b)
{
  DWunion au, bu;

  au.ll = a, bu.ll = b;

  if (au.s.high < bu.s.high)
    return 0;
  else if (au.s.high > bu.s.high)
    return 2;
  if ((UWtype) au.s.low < (UWtype) bu.s.low)
    return 0;
  else if ((UWtype) au.s.low > (UWtype) bu.s.low)
    return 2;
  return 1;
}
#endif

#ifdef L_ucmpdi2
word_type
__ucmpdi2 (DWtype a, DWtype b)
{
  DWunion au, bu;

  au.ll = a, bu.ll = b;

  if ((UWtype) au.s.high < (UWtype) bu.s.high)
    return 0;
  else if ((UWtype) au.s.high > (UWtype) bu.s.high)
    return 2;
  if ((UWtype) au.s.low < (UWtype) bu.s.low)
    return 0;
  else if ((UWtype) au.s.low > (UWtype) bu.s.low)
    return 2;
  return 1;
}
#endif

#if defined(L_fixunstfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunstfDI (TFtype a)
{
  TFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the TFtype, leaving the low part as flonum.  */
  a -= (TFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#if defined(L_fixtfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
DWtype
__fixtfdi (TFtype a)
{
  if (a < 0)
    return - __fixunstfDI (-a);
  return __fixunstfDI (a);
}
#endif

#if defined(L_fixunsxfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunsxfDI (XFtype a)
{
  XFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the XFtype, leaving the low part as flonum.  */
  a -= (XFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#if defined(L_fixxfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
DWtype
__fixxfdi (XFtype a)
{
  if (a < 0)
    return - __fixunsxfDI (-a);
  return __fixunsxfDI (a);
}
#endif

#ifdef L_fixunsdfdi
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunsdfDI (DFtype a)
{
  DFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the DFtype, leaving the low part as flonum.  */
  a -= (DFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#ifdef L_fixdfdi
DWtype
__fixdfdi (DFtype a)
{
  if (a < 0)
    return - __fixunsdfDI (-a);
  return __fixunsdfDI (a);
}
#endif

#ifdef L_fixunssfdi
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunssfDI (SFtype original_a)
{
  /* Convert the SFtype to a DFtype, because that is surely not going
     to lose any bits.  Some day someone else can write a faster version
     that avoids converting to DFtype, and verify it really works right.  */
  DFtype a = original_a;
  DFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the DFtype, leaving the low part as flonum.  */
  a -= (DFtype) v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#ifdef L_fixsfdi
DWtype
__fixsfdi (SFtype a)
{
  if (a < 0)
    return - __fixunssfDI (-a);
  return __fixunssfDI (a);
}
#endif

#if defined(L_floatdixf) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

XFtype
__floatdixf (DWtype u)
{
  XFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#if defined(L_floatditf) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

TFtype
__floatditf (DWtype u)
{
  TFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#ifdef L_floatdidf
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DFtype
__floatdidf (DWtype u)
{
  DFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#ifdef L_floatdisf
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)
#define DI_SIZE (sizeof (DWtype) * BITS_PER_UNIT)

/* Define codes for all the float formats that we know of.  Note
   that this is copied from real.h.  */

#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3

/* Default to IEEE float if not specified.  Nearly all machines use it.  */
#ifndef HOST_FLOAT_FORMAT
#define	HOST_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#if HOST_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define DF_SIZE 53
#define SF_SIZE 24
#endif

#if HOST_FLOAT_FORMAT == IBM_FLOAT_FORMAT
#define DF_SIZE 56
#define SF_SIZE 24
#endif

#if HOST_FLOAT_FORMAT == VAX_FLOAT_FORMAT
#define DF_SIZE 56
#define SF_SIZE 24
#endif

SFtype
__floatdisf (DWtype u)
{
  /* Do the calculation in DFmode
     so that we don't lose any of the precision of the high word
     while multiplying it.  */
  DFtype f;

  /* Protect against double-rounding error.
     Represent any low-order bits, that might be truncated in DFmode,
     by a bit that won't be lost.  The bit can go in anywhere below the
     rounding position of the SFmode.  A fixed mask and bit position
     handles all usual configurations.  It doesn't handle the case
     of 128-bit DImode, however.  */
  if (DF_SIZE < DI_SIZE
      && DF_SIZE > (DI_SIZE - DF_SIZE + SF_SIZE))
    {
#define REP_BIT ((UDWtype) 1 << (DI_SIZE - DF_SIZE))
      if (! (- ((DWtype) 1 << DF_SIZE) < u
	     && u < ((DWtype) 1 << DF_SIZE)))
	{
	  if ((UDWtype) u & (REP_BIT - 1))
	    u |= REP_BIT;
	}
    }
  f = (Wtype) (u >> WORD_SIZE);
  f *= HIGH_HALFWORD_COEFF;
  f *= HIGH_HALFWORD_COEFF;
  f += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return (SFtype) f;
}
#endif

#if defined(L_fixunsxfsi) && LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunsxfSI (XFtype a)
{
  if (a >= - (DFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif

#ifdef L_fixunsdfsi
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunsdfSI (DFtype a)
{
  if (a >= - (DFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif

#ifdef L_fixunssfsi
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunssfSI (SFtype a)
{
  if (a >= - (SFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif

/* From here on down, the routines use normal data types.  */

#define SItype bogus_type
#define USItype bogus_type
#define DItype bogus_type
#define UDItype bogus_type
#define SFtype bogus_type
#define DFtype bogus_type
#undef Wtype
#undef UWtype
#undef HWtype
#undef UHWtype
#undef DWtype
#undef UDWtype

#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double

#ifdef L__gcc_bcmp

/* Like bcmp except the sign is meaningful.
   Result is negative if S1 is less than S2,
   positive if S1 is greater, 0 if S1 and S2 are equal.  */

int
__gcc_bcmp (const unsigned char *s1, const unsigned char *s2, size_t size)
{
  while (size > 0)
    {
      unsigned char c1 = *s1++, c2 = *s2++;
      if (c1 != c2)
	return c1 - c2;
      size--;
    }
  return 0;
}

#endif

/* __eprintf used to be used by GCC's private version of <assert.h>.
   We no longer provide that header, but this routine remains in libgcc.a
   for binary backward compatibility.  Note that it is not included in
   the shared version of libgcc.  */
#ifdef L_eprintf
#ifndef inhibit_libc

#undef NULL /* Avoid errors if stdio.h and our stddef.h mismatch.  */
#include <stdio.h>

void
__eprintf (const char *string, const char *expression,
	   unsigned int line, const char *filename)
{
  fprintf (stderr, string, expression, line, filename);
  fflush (stderr);
  abort ();
}

#endif
#endif

#ifdef L_bb

#if LONG_TYPE_SIZE == GCOV_TYPE_SIZE
typedef long gcov_type;
#else
typedef long long gcov_type;
#endif


/* Structure emitted by -a  */
struct bb
{
  long zero_word;
  const char *filename;
  gcov_type *counts;
  long ncounts;
  struct bb *next;
  const unsigned long *addresses;

  /* Older GCC's did not emit these fields.  */
  long nwords;
  const char **functions;
  const long *line_nums;
  const char **filenames;
  char *flags;
};

#ifdef BLOCK_PROFILER_CODE
BLOCK_PROFILER_CODE
#else
#ifndef inhibit_libc

/* Simple minded basic block profiling output dumper for
   systems that don't provide tcov support.  At present,
   it requires atexit and stdio.  */

#undef NULL /* Avoid errors if stdio.h and our stddef.h mismatch.  */
#include <stdio.h>

#include "gbl-ctors.h"
#include "gcov-io.h"
#include <string.h>
#ifdef TARGET_HAS_F_SETLKW
#include <fcntl.h>
#include <errno.h>
#endif

static struct bb *bb_head;

void
__bb_exit_func (void)
{
  FILE *da_file;
  int i;
  struct bb *ptr;

  if (bb_head == 0)
    return;

  i = strlen (bb_head->filename) - 3;


  for (ptr = bb_head; ptr != (struct bb *) 0; ptr = ptr->next)
    {
      int firstchar;

      /* Make sure the output file exists -
         but don't clobber exiting data.  */
      if ((da_file = fopen (ptr->filename, "a")) != 0)
	fclose (da_file);

      /* Need to re-open in order to be able to write from the start.  */
      da_file = fopen (ptr->filename, "r+b");
      /* Some old systems might not allow the 'b' mode modifier.
         Therefore, try to open without it.  This can lead to a race
         condition so that when you delete and re-create the file, the
         file might be opened in text mode, but then, you shouldn't
         delete the file in the first place.  */
      if (da_file == 0)
	da_file = fopen (ptr->filename, "r+");
      if (da_file == 0)
	{
	  fprintf (stderr, "arc profiling: Can't open output file %s.\n",
		   ptr->filename);
	  continue;
	}

      /* After a fork, another process might try to read and/or write
         the same file simultanously.  So if we can, lock the file to
         avoid race conditions.  */
#if defined (TARGET_HAS_F_SETLKW)
      {
	struct flock s_flock;

	s_flock.l_type = F_WRLCK;
	s_flock.l_whence = SEEK_SET;
	s_flock.l_start = 0;
	s_flock.l_len = 1;
	s_flock.l_pid = getpid ();

	while (fcntl (fileno (da_file), F_SETLKW, &s_flock)
	       && errno == EINTR);
      }
#endif

      /* If the file is not empty, and the number of counts in it is the
         same, then merge them in.  */
      firstchar = fgetc (da_file);
      if (firstchar == EOF)
	{
	  if (ferror (da_file))
	    {
	      fprintf (stderr, "arc profiling: Can't read output file ");
	      perror (ptr->filename);
	    }
	}
      else
	{
	  long n_counts = 0;

	  if (ungetc (firstchar, da_file) == EOF)
	    rewind (da_file);
	  if (__read_long (&n_counts, da_file, 8) != 0)
	    {
	      fprintf (stderr, "arc profiling: Can't read output file %s.\n",
		       ptr->filename);
	      continue;
	    }

	  if (n_counts == ptr->ncounts)
	    {
	      int i;

	      for (i = 0; i < n_counts; i++)
		{
		  gcov_type v = 0;

		  if (__read_gcov_type (&v, da_file, 8) != 0)
		    {
		      fprintf (stderr,
			       "arc profiling: Can't read output file %s.\n",
			       ptr->filename);
		      break;
		    }
		  ptr->counts[i] += v;
		}
	    }

	}

      rewind (da_file);

      /* ??? Should first write a header to the file.  Preferably, a 4 byte
         magic number, 4 bytes containing the time the program was
         compiled, 4 bytes containing the last modification time of the
         source file, and 4 bytes indicating the compiler options used.

         That way we can easily verify that the proper source/executable/
         data file combination is being used from gcov.  */

      if (__write_gcov_type (ptr->ncounts, da_file, 8) != 0)
	{

	  fprintf (stderr, "arc profiling: Error writing output file %s.\n",
		   ptr->filename);
	}
      else
	{
	  int j;
	  gcov_type *count_ptr = ptr->counts;
	  int ret = 0;
	  for (j = ptr->ncounts; j > 0; j--)
	    {
	      if (__write_gcov_type (*count_ptr, da_file, 8) != 0)
		{
		  ret = 1;
		  break;
		}
	      count_ptr++;
	    }
	  if (ret)
	    fprintf (stderr, "arc profiling: Error writing output file %s.\n",
		     ptr->filename);
	}

      if (fclose (da_file) == EOF)
	fprintf (stderr, "arc profiling: Error closing output file %s.\n",
		 ptr->filename);
    }

  return;
}

void
__bb_init_func (struct bb *blocks)
{
  /* User is supposed to check whether the first word is non-0,
     but just in case....  */

  if (blocks->zero_word)
    return;

  /* Initialize destructor.  */
  if (!bb_head)
    atexit (__bb_exit_func);

  /* Set up linked list.  */
  blocks->zero_word = 1;
  blocks->next = bb_head;
  bb_head = blocks;
}

/* Called before fork or exec - write out profile information gathered so
   far and reset it to zero.  This avoids duplication or loss of the
   profile information gathered so far.  */
void
__bb_fork_func (void)
{
  struct bb *ptr;

  __bb_exit_func ();
  for (ptr = bb_head; ptr != (struct bb *) 0; ptr = ptr->next)
    {
      long i;
      for (i = ptr->ncounts - 1; i >= 0; i--)
	ptr->counts[i] = 0;
    }
}

#endif /* not inhibit_libc */
#endif /* not BLOCK_PROFILER_CODE */
#endif /* L_bb */

#ifdef L_clear_cache
/* Clear part of an instruction cache.  */

#define INSN_CACHE_PLANE_SIZE (INSN_CACHE_SIZE / INSN_CACHE_DEPTH)

void
__clear_cache (char *beg __attribute__((__unused__)),
	       char *end __attribute__((__unused__)))
{
#ifdef CLEAR_INSN_CACHE
  CLEAR_INSN_CACHE (beg, end);
#else
#ifdef INSN_CACHE_SIZE
  static char array[INSN_CACHE_SIZE + INSN_CACHE_PLANE_SIZE + INSN_CACHE_LINE_WIDTH];
  static int initialized;
  int offset;
  void *start_addr
  void *end_addr;
  typedef (*function_ptr) (void);

#if (INSN_CACHE_SIZE / INSN_CACHE_LINE_WIDTH) < 16
  /* It's cheaper to clear the whole cache.
     Put in a series of jump instructions so that calling the beginning
     of the cache will clear the whole thing.  */

  if (! initialized)
    {
      int ptr = (((int) array + INSN_CACHE_LINE_WIDTH - 1)
		 & -INSN_CACHE_LINE_WIDTH);
      int end_ptr = ptr + INSN_CACHE_SIZE;

      while (ptr < end_ptr)
	{
	  *(INSTRUCTION_TYPE *)ptr
	    = JUMP_AHEAD_INSTRUCTION + INSN_CACHE_LINE_WIDTH;
	  ptr += INSN_CACHE_LINE_WIDTH;
	}
      *(INSTRUCTION_TYPE *) (ptr - INSN_CACHE_LINE_WIDTH) = RETURN_INSTRUCTION;

      initialized = 1;
    }

  /* Call the beginning of the sequence.  */
  (((function_ptr) (((int) array + INSN_CACHE_LINE_WIDTH - 1)
		    & -INSN_CACHE_LINE_WIDTH))
   ());

#else /* Cache is large.  */

  if (! initialized)
    {
      int ptr = (((int) array + INSN_CACHE_LINE_WIDTH - 1)
		 & -INSN_CACHE_LINE_WIDTH);

      while (ptr < (int) array + sizeof array)
	{
	  *(INSTRUCTION_TYPE *)ptr = RETURN_INSTRUCTION;
	  ptr += INSN_CACHE_LINE_WIDTH;
	}

      initialized = 1;
    }

  /* Find the location in array that occupies the same cache line as BEG.  */

  offset = ((int) beg & -INSN_CACHE_LINE_WIDTH) & (INSN_CACHE_PLANE_SIZE - 1);
  start_addr = (((int) (array + INSN_CACHE_PLANE_SIZE - 1)
		 & -INSN_CACHE_PLANE_SIZE)
		+ offset);

  /* Compute the cache alignment of the place to stop clearing.  */
#if 0  /* This is not needed for gcc's purposes.  */
  /* If the block to clear is bigger than a cache plane,
     we clear the entire cache, and OFFSET is already correct.  */
  if (end < beg + INSN_CACHE_PLANE_SIZE)
#endif
    offset = (((int) (end + INSN_CACHE_LINE_WIDTH - 1)
	       & -INSN_CACHE_LINE_WIDTH)
	      & (INSN_CACHE_PLANE_SIZE - 1));

#if INSN_CACHE_DEPTH > 1
  end_addr = (start_addr & -INSN_CACHE_PLANE_SIZE) + offset;
  if (end_addr <= start_addr)
    end_addr += INSN_CACHE_PLANE_SIZE;

  for (plane = 0; plane < INSN_CACHE_DEPTH; plane++)
    {
      int addr = start_addr + plane * INSN_CACHE_PLANE_SIZE;
      int stop = end_addr + plane * INSN_CACHE_PLANE_SIZE;

      while (addr != stop)
	{
	  /* Call the return instruction at ADDR.  */
	  ((function_ptr) addr) ();

	  addr += INSN_CACHE_LINE_WIDTH;
	}
    }
#else /* just one plane */
  do
    {
      /* Call the return instruction at START_ADDR.  */
      ((function_ptr) start_addr) ();

      start_addr += INSN_CACHE_LINE_WIDTH;
    }
  while ((start_addr % INSN_CACHE_SIZE) != offset);
#endif /* just one plane */
#endif /* Cache is large */
#endif /* Cache exists */
#endif /* CLEAR_INSN_CACHE */
}

#endif /* L_clear_cache */

#ifdef L_trampoline

/* Jump to a trampoline, loading the static chain address.  */

#if defined(WINNT) && ! defined(__CYGWIN__) && ! defined (_UWIN)

long
getpagesize (void)
{
#ifdef _ALPHA_
  return 8192;
#else
  return 4096;
#endif
}

#ifdef __i386__
extern int VirtualProtect (char *, int, int, int *) __attribute__((stdcall));
#endif

int
mprotect (char *addr, int len, int prot)
{
  int np, op;

  if (prot == 7)
    np = 0x40;
  else if (prot == 5)
    np = 0x20;
  else if (prot == 4)
    np = 0x10;
  else if (prot == 3)
    np = 0x04;
  else if (prot == 1)
    np = 0x02;
  else if (prot == 0)
    np = 0x01;

  if (VirtualProtect (addr, len, np, &op))
    return 0;
  else
    return -1;
}

#endif /* WINNT && ! __CYGWIN__ && ! _UWIN */

#ifdef TRANSFER_FROM_TRAMPOLINE
TRANSFER_FROM_TRAMPOLINE
#endif

#if defined (NeXT) && defined (__MACH__)

/* Make stack executable so we can call trampolines on stack.
   This is called from INITIALIZE_TRAMPOLINE in next.h.  */
#ifdef NeXTStep21
 #include <mach.h>
#else
 #include <mach/mach.h>
#endif

void
__enable_execute_stack (char *addr)
{
  kern_return_t r;
  char *eaddr = addr + TRAMPOLINE_SIZE;
  vm_address_t a = (vm_address_t) addr;

  /* turn on execute access on stack */
  r = vm_protect (task_self (), a, TRAMPOLINE_SIZE, FALSE, VM_PROT_ALL);
  if (r != KERN_SUCCESS)
    {
      mach_error("vm_protect VM_PROT_ALL", r);
      exit(1);
    }

  /* We inline the i-cache invalidation for speed */

#ifdef CLEAR_INSN_CACHE
  CLEAR_INSN_CACHE (addr, eaddr);
#else
  __clear_cache ((int) addr, (int) eaddr);
#endif
}

#endif /* defined (NeXT) && defined (__MACH__) */

#ifdef __convex__

/* Make stack executable so we can call trampolines on stack.
   This is called from INITIALIZE_TRAMPOLINE in convex.h.  */

#include <sys/mman.h>
#include <sys/vmparam.h>
#include <machine/machparam.h>

void
__enable_execute_stack (void)
{
  int fp;
  static unsigned lowest = USRSTACK;
  unsigned current = (unsigned) &fp & -NBPG;

  if (lowest > current)
    {
      unsigned len = lowest - current;
      mremap (current, &len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE);
      lowest = current;
    }

  /* Clear instruction cache in case an old trampoline is in it.  */
  asm ("pich");
}
#endif /* __convex__ */

#ifdef __sysV88__

/* Modified from the convex -code above.  */

#include <sys/param.h>
#include <errno.h>
#include <sys/m88kbcs.h>

void
__enable_execute_stack (void)
{
  int save_errno;
  static unsigned long lowest = USRSTACK;
  unsigned long current = (unsigned long) &save_errno & -NBPC;

  /* Ignore errno being set. memctl sets errno to EINVAL whenever the
     address is seen as 'negative'. That is the case with the stack.  */

  save_errno=errno;
  if (lowest > current)
    {
      unsigned len=lowest-current;
      memctl(current,len,MCT_TEXT);
      lowest = current;
    }
  else
    memctl(current,NBPC,MCT_TEXT);
  errno=save_errno;
}

#endif /* __sysV88__ */

#ifdef __sysV68__

#include <sys/signal.h>
#include <errno.h>

/* Motorola forgot to put memctl.o in the libp version of libc881.a,
   so define it here, because we need it in __clear_insn_cache below */
/* On older versions of this OS, no memctl or MCT_TEXT are defined;
   hence we enable this stuff only if MCT_TEXT is #define'd.  */

#ifdef MCT_TEXT
asm("\n\
	global memctl\n\
memctl:\n\
	movq &75,%d0\n\
	trap &0\n\
	bcc.b noerror\n\
	jmp cerror%\n\
noerror:\n\
	movq &0,%d0\n\
	rts");
#endif

/* Clear instruction cache so we can call trampolines on stack.
   This is called from FINALIZE_TRAMPOLINE in mot3300.h.  */

void
__clear_insn_cache (void)
{
#ifdef MCT_TEXT
  int save_errno;

  /* Preserve errno, because users would be surprised to have
  errno changing without explicitly calling any system-call.  */
  save_errno = errno;

  /* Keep it simple : memctl (MCT_TEXT) always fully clears the insn cache.
     No need to use an address derived from _start or %sp, as 0 works also.  */
  memctl(0, 4096, MCT_TEXT);
  errno = save_errno;
#endif
}

#endif /* __sysV68__ */

#ifdef __pyr__

#undef NULL /* Avoid errors if stdio.h and our stddef.h mismatch.  */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vmmac.h>

/* Modified from the convex -code above.
   mremap promises to clear the i-cache.  */

void
__enable_execute_stack (void)
{
  int fp;
  if (mprotect (((unsigned int)&fp/PAGSIZ)*PAGSIZ, PAGSIZ,
		PROT_READ|PROT_WRITE|PROT_EXEC))
    {
      perror ("mprotect in __enable_execute_stack");
      fflush (stderr);
      abort ();
    }
}
#endif /* __pyr__ */

#if defined (sony_news) && defined (SYSTYPE_BSD)

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <syscall.h>
#include <machine/sysnews.h>

/* cacheflush function for NEWS-OS 4.2.
   This function is called from trampoline-initialize code
   defined in config/mips/mips.h.  */

void
cacheflush (char *beg, int size, int flag)
{
  if (syscall (SYS_sysnews, NEWS_CACHEFLUSH, beg, size, FLUSH_BCACHE))
    {
      perror ("cache_flush");
      fflush (stderr);
      abort ();
    }
}

#endif /* sony_news */
#endif /* L_trampoline */

#ifndef __CYGWIN__
#ifdef L__main

#include "gbl-ctors.h"
/* Some systems use __main in a way incompatible with its use in gcc, in these
   cases use the macros NAME__MAIN to give a quoted symbol and SYMBOL__MAIN to
   give the same symbol without quotes for an alternative entry point.  You
   must define both, or neither.  */
#ifndef NAME__MAIN
#define NAME__MAIN "__main"
#define SYMBOL__MAIN __main
#endif

#ifdef INIT_SECTION_ASM_OP
#undef HAS_INIT_SECTION
#define HAS_INIT_SECTION
#endif

#if !defined (HAS_INIT_SECTION) || !defined (OBJECT_FORMAT_ELF)

/* Some ELF crosses use crtstuff.c to provide __CTOR_LIST__, but use this
   code to run constructors.  In that case, we need to handle EH here, too.  */

#ifdef EH_FRAME_SECTION_NAME
#include "unwind-dw2-fde.h"
extern unsigned char __EH_FRAME_BEGIN__[];
#endif

/* Run all the global destructors on exit from the program.  */

void
__do_global_dtors (void)
{
#ifdef DO_GLOBAL_DTORS_BODY
  DO_GLOBAL_DTORS_BODY;
#else
  static func_ptr *p = __DTOR_LIST__ + 1;
  while (*p)
    {
      p++;
      (*(p-1)) ();
    }
#endif
#if defined (EH_FRAME_SECTION_NAME) && !defined (HAS_INIT_SECTION)
  {
    static int completed = 0;
    if (! completed)
      {
	completed = 1;
	__deregister_frame_info (__EH_FRAME_BEGIN__);
      }
  }
#endif
}
#endif

#ifndef HAS_INIT_SECTION
/* Run all the global constructors on entry to the program.  */

void
__do_global_ctors (void)
{
#ifdef EH_FRAME_SECTION_NAME
  {
    static struct object object;
    __register_frame_info (__EH_FRAME_BEGIN__, &object);
  }
#endif
  DO_GLOBAL_CTORS_BODY;
  atexit (__do_global_dtors);
}
#endif /* no HAS_INIT_SECTION */

#if !defined (HAS_INIT_SECTION) || defined (INVOKE__main)
/* Subroutine called automatically by `main'.
   Compiling a global function named `main'
   produces an automatic call to this function at the beginning.

   For many systems, this routine calls __do_global_ctors.
   For systems which support a .init section we use the .init section
   to run __do_global_ctors, so we need not do anything here.  */

void
SYMBOL__MAIN ()
{
  /* Support recursive calls to `main': run initializers just once.  */
  static int initialized;
  if (! initialized)
    {
      initialized = 1;
      __do_global_ctors ();
    }
}
#endif /* no HAS_INIT_SECTION or INVOKE__main */

#endif /* L__main */
#endif /* __CYGWIN__ */

#ifdef L_ctors

#include "gbl-ctors.h"

/* Provide default definitions for the lists of constructors and
   destructors, so that we don't get linker errors.  These symbols are
   intentionally bss symbols, so that gld and/or collect will provide
   the right values.  */

/* We declare the lists here with two elements each,
   so that they are valid empty lists if no other definition is loaded.

   If we are using the old "set" extensions to have the gnu linker
   collect ctors and dtors, then we __CTOR_LIST__ and __DTOR_LIST__
   must be in the bss/common section.

   Long term no port should use those extensions.  But many still do.  */
#if !defined(INIT_SECTION_ASM_OP) && !defined(CTOR_LISTS_DEFINED_EXTERNALLY)
#if defined (TARGET_ASM_CONSTRUCTOR) || defined (USE_COLLECT2)
func_ptr __CTOR_LIST__[2] = {0, 0};
func_ptr __DTOR_LIST__[2] = {0, 0};
#else
func_ptr __CTOR_LIST__[2];
func_ptr __DTOR_LIST__[2];
#endif
#endif /* no INIT_SECTION_ASM_OP and not CTOR_LISTS_DEFINED_EXTERNALLY */
#endif /* L_ctors */

#ifdef L_exit

#include "gbl-ctors.h"

#ifdef NEED_ATEXIT

#ifndef ON_EXIT

# include <errno.h>

static func_ptr *atexit_chain = 0;
static long atexit_chain_length = 0;
static volatile long last_atexit_chain_slot = -1;

int
atexit (func_ptr func)
{
  if (++last_atexit_chain_slot == atexit_chain_length)
    {
      atexit_chain_length += 32;
      if (atexit_chain)
	atexit_chain = (func_ptr *) realloc (atexit_chain, atexit_chain_length
					     * sizeof (func_ptr));
      else
	atexit_chain = (func_ptr *) malloc (atexit_chain_length
					    * sizeof (func_ptr));
      if (! atexit_chain)
	{
	  atexit_chain_length = 0;
	  last_atexit_chain_slot = -1;
	  errno = ENOMEM;
	  return (-1);
	}
    }
  atexit_chain[last_atexit_chain_slot] = func;
  return (0);
}

extern void _cleanup (void);
extern void _exit (int) __attribute__ ((__noreturn__));

void
exit (int status)
{
  if (atexit_chain)
    {
      for ( ; last_atexit_chain_slot-- >= 0; )
	{
	  (*atexit_chain[last_atexit_chain_slot + 1]) ();
	  atexit_chain[last_atexit_chain_slot + 1] = 0;
	}
      free (atexit_chain);
      atexit_chain = 0;
    }
#ifdef EXIT_BODY
  EXIT_BODY;
#else
  _cleanup ();
#endif
  _exit (status);
}

#else /* ON_EXIT */

/* Simple; we just need a wrapper for ON_EXIT.  */
int
atexit (func_ptr func)
{
  return ON_EXIT (func);
}

#endif /* ON_EXIT */
#endif /* NEED_ATEXIT */

#endif /* L_exit */
