/* Copyright (C) 1991, 1992, 1995 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <msvcrt/math.h>
#include <msvcrt/float.h>
#include <msvcrt/internal/ieee.h>


int _isnan(double __x)
{
	double_t * x = (double_t *)&__x;
	return ( x->exponent == 0x7ff  && ( x->mantissah != 0 || x->mantissal != 0 ));	
}

int _isnanl(long double __x)
{
	/* Intel's extended format has the normally implicit 1 explicit
	   present.  Sigh!  */
	
	long_double_t * x = (long_double_t *)&__x;
	
	
	 /* IEEE 854 NaN's have the maximum possible
     exponent and a nonzero mantissa.  */
          
	return (( x->exponent == 0x7fff)  
	  && ( (x->mantissah & 0x80000000) != 0) 
	  && ( (x->mantissah & (unsigned int)0x7fffffff) != 0  || x->mantissal != 0 ));	
}

int _isinf(double __x)
{
	double_t * x = (double_t *)&__x;
	return ( x->exponent == 0x7ff  && ( x->mantissah == 0 && x->mantissal == 0 ));	
}

int _finite( double x )
{
	return !_isinf(x);
}

int _isinfl(long double __x)
{
	/* Intel's extended format has the normally implicit 1 explicit
	   present.  Sigh!  */
	   
	long_double_t * x = (long_double_t *)&__x;
	
	
	 /* An IEEE 854 infinity has an exponent with the
     maximum possible value and a zero mantissa.  */
 
		
	if ( x->exponent == 0x7fff  && ( (x->mantissah == 0x80000000 )   && x->mantissal == 0 ))
		return x->sign ? -1 : 1;
	return 0;
}
